/* This file is part of the balancer
 * Author: Dan Lovinger, del+@cmu.edu
 */

/*
 *        Copyright 1993 by Carnegie Mellon University
 * 
 *                    All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of CMU not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
#if BYNUMBER
#include <stdio.h>

#include "balance.h"
#include "balutil.h"
#include "hash.h"
#include "mygetopt.h"

#include "agent_bynumber.h"

/* simple definition of # volumes per unit size */
#define VOLPERSIZE(pp) (double)((double)(pp)->nvols/(double)(pp)->size)

/* ooh, the complexity - add a volume, take a volume away */
#define VOLPERSIZE_WI_V(pp) (double)(((double)(pp)->nvols + 1)/(double)(pp)->size)
#define VOLPERSIZE_WO_V(pp) (double)(((double)(pp)->nvols - 1)/(double)(pp)->size)

/* these evaluate to a percentage occupation, nothing fancy.
 */
#define UPPER_BOUND (avg_vol_per_size * (1.0 + precision))
#define MOVE_BOUND  (avg_vol_per_size * (1.0 + (precision/2.0)))
#define LOWER_BOUND (avg_vol_per_size * (1.0 - precision)) 

/* a simple wrapper so we can sort partitions the way we want */
struct bynumber_partition {
    struct partition *this;         /* partition here */
    struct bynumber_volume *vols,   /* sorted list of volumes on this partition */
                        *nextvol;   /* pointer into vols list, next volume we may
				     * want to care about on partition */
    struct bynumber_partition *next, *prev;
};

/* a simple wrapper so we can sort volumes the way we want */
struct bynumber_volume {
    struct volume *this;           /* volume here */
    struct bynumber_volume *next;
};

static struct agent_procs agent_bynumber_procs = {
    bynumber_servconf,
    bynumber_getrequest,
    bynumber_queryrequest,
    bynumber_setrequest,
    bynumber_discardv,
    bynumber_report
};

/****************************
 * ByNumber Agent Global Data *
 ****************************/

static struct bynumber_partition *head = NULL, 
                               *tail = NULL;       /* sorted chain of partitions */
static double avg_vol_per_size = 0;                /* average # of volumes per size unit on partitions */
static unsigned long npart = 0;                    /* number of partitions we have */
static double precision = 0.40;                    /* get partitions to within 40% by default */
static struct hashtable *part_ht = NULL;           /* struct partition -> struct bynumber_partition map */
static struct hashtable *vol_ht = NULL;            /* struct volume -> struct bynumber_volume map */
static struct move_request *bynumber_mover = NULL; /* cached move request we currently care about */
static union hashtable_key key;                    /* generic hashtable key */

struct agent_procs *bynumber_doargs(args)
char *args;
{
    int argc, error, c;
    char **argv;
    extern char *optarg;

    if (args != NULL) {
#ifdef DEBUG
	printf("%s processing %s\n", BYNUMBER_NAME, args);
#endif

	argv = makeargv(&argc, args, NULL);

	mygetopt(0, NULL, NULL); /* reset getopt */
	while ((c = mygetopt(argc, argv, "p:")) != EOF) {
	    switch (c) {
	    case 'p':
		precision = atoi(optarg)/100.0;
		break;
	    default:
		error++;
	    }
	}
	free(argv);
    }

    return &agent_bynumber_procs;
}

/* insert a partition, sorted, into the partition chain */
static void bynumber_insertp(forepp, tailpp, addp)
struct bynumber_partition **forepp, **tailpp, *addp;
{
    addp->prev = NULL;

    while (*forepp) {
	/* if elt has fewer vols than what we are adding 
	 * we've found the slot to drop into
	 */
	if (VOLPERSIZE((*forepp)->this) < VOLPERSIZE(addp->this))
	  break;

	/* if we're about to break out, set the prev pointer now */
	if ((*forepp)->next == NULL)
	  addp->prev = *forepp;

	forepp = &(*forepp)->next;
    }

    /* thread onto the forward chain */
    addp->next = *forepp;
    *forepp = addp;
    if (addp->next) {
	/* thread onto the backward chain */
	addp->prev = addp->next->prev;
	addp->next->prev = addp;
    } else {
	/* we are at the end of the forward chain,
	 * so update tail pointer
	 */
	*tailpp = addp;
    }
}

/* delete a partition from the partition chain */
static void bynumber_delp(forepp, tailpp, addp)
struct bynumber_partition **forepp, **tailpp, *addp;
{
    if (addp->prev) {
	/* not at top of the list */
	addp->prev->next = addp->next;
    } else {
	/* update top of the list */
	*forepp = addp->next;
    }

    if (addp->next) {
	/* not at end of the list */
	addp->next->prev = addp->prev;
    } else {
	/* update end of the list */
	*tailpp = addp->prev;
    }
}

/* insert a volume, sorted, into a volume chain */
static void bynumber_insertv(lvp, addv)
struct bynumber_volume **lvp, *addv;
{
    while (*lvp) {
	/* if elt has more usage than the one we're
	 * adding, we've found the slot to drop into
	 */
	if ((*lvp)->this->size >= addv->this->size)
	  break;

	lvp = &(*lvp)->next;
    }

    /* thread onto the chain */
    addv->next = *lvp;
    *lvp = addv;
}

/* delete a volume from a volume chain */
static void bynumber_delv(lvp, addv)
struct bynumber_volume **lvp, *addv;
{
    while (*lvp) {
	if (*lvp == addv) {
	    *lvp = addv->next;
	    addv->next = (struct bynumber_volume *) NULL;
	    return;
	}

	lvp = &(*lvp)->next;
    }
}

void bynumber_servconf(cell)
struct server *cell;
{
    struct partition *pp;
    struct volume *vp;
    struct bynumber_partition *up;
    struct bynumber_volume *uv;
#if NEW_MATH
    unsigned long tot_vols = 0, tot_size = 0;
#endif

    while (cell) {
	pp = cell->parts;

	/* walk along partitions, adding them into sorted local structs */
	while (pp) {
	    up = (struct bynumber_partition *) xmalloc(sizeof(struct bynumber_partition));

	    up->this = pp;
	    bynumber_insertp(&head, &tail, up);

	    if (part_ht == NULL) part_ht = hashtable_create(hashfun_addr, hashcmp_addr,
							    hashdup_addr, hashzap_addr);
	    /* index struct bynumber_partition by struct partition */
	    hashtable_add(part_ht, (key.addr = pp, key), up);

	    vp = pp->vols;

	    /* walk along volumes, adding them into sorted local structs */
	    while (vp) {
		uv = (struct bynumber_volume *) xmalloc(sizeof(struct bynumber_volume));

		uv->this = vp;
		bynumber_insertv(&up->vols, uv);

		if (vol_ht == NULL) vol_ht = hashtable_create(hashfun_addr, hashcmp_addr,
							      hashdup_addr, hashzap_addr);
		/* index struct bynumber_volume by struct volume */
		hashtable_add(vol_ht, (key.addr = vp, key), uv);

		vp = vp->next;
	    }

	    /* init next volume pointer */
	    up->nextvol = up->vols;

	    /* update occupancy counters */
	    npart++;
#if !NEW_MATH
            avg_vol_per_size = ((avg_vol_per_size * (npart - 1)) +
                                VOLPERSIZE(up->this))/npart;
#else
	    tot_vols += up->this->nvols;
            tot_size += up->this->size;
#endif
            /* avg_vol_per_size = ((avg_vol_per_size * (npart - 1)) +
               VOLPERSIZE(up->this))/npart; */

	    pp = pp->next;
	}

	cell = cell->next;
    }
#if NEW_MATH
    avg_vol_per_size = ((double)tot_vols)/((double)tot_size);
#endif
}

/* find a place to move this volume, NULL if we didn't
 * find anything
 *
 * start at "start", so we can keep polling for partitions
 * in our searcher
 */
static struct bynumber_partition *bynumber_findest(start)
struct bynumber_partition *start;
{
    struct bynumber_partition *pp = start;

    while (pp) {
	/* if this partition can take this volume without
	 * pushing it above the upper precision limit where
	 * we may want to move it again ...
	 */
	if (VOLPERSIZE_WI_V(pp->this) < MOVE_BOUND)
	  /* let's do it */
	  return pp;

	pp = pp->prev;
    }

    return (struct bynumber_partition *) NULL;
}

struct move_request *bynumber_getrequest()
{
    struct bynumber_partition *pp = head;
    static struct bynumber_partition *destp; /* handle on last partition we
					      * tried to move volume to */
    struct bynumber_volume *mv = NULL;

    /* if we are currently trying to move a volume */
    if (bynumber_mover != NULL) {
	/* starting at the next partition, try to find a place
	 * we want to move this thing to
	 */
	destp = bynumber_findest(destp->prev);

	/* if we didn't fail ... */
	if (destp != NULL) {
	    /* update our transaction */
	    bynumber_mover->sto = destp->this->who;
	    bynumber_mover->pto = destp->this;

	    /* and try again */
	    return bynumber_mover;
	}

	/* oh well, we have to punt on this volume now */

	/* find the bynumber_partition this volume is on 
	 * to discover our place in the search
	 */
	pp = (struct bynumber_partition *)hashtable_find(part_ht, (key.addr = bynumber_mover->pfrom, key));

#ifdef DEBUG
	printf("bynumber: rejection moving %s from %s/%c\n",
	       bynumber_mover->vol->name,
	       bynumber_mover->sfrom->who->name,
	       bynumber_mover->pfrom->pid + 'a');
#endif

	/* fall off into finding another volume */
    }

    /* for all partitions */
    while (pp) {
	/* if we can and need to balance this partition */
	if (pp->nextvol && VOLPERSIZE(pp->this) > MOVE_BOUND && (destp = bynumber_findest(tail))) {
	    /* since every volume has a unitary effect, we don't need to bother
	     * looking at the lower bound */
	    mv = pp->nextvol;
	    /* and push past */
	    pp->nextvol = mv->next;

	    break;
	}

	/* push along partition list */
	pp = pp->next;
    }

    if (mv) {
	if (bynumber_mover == NULL)
	  bynumber_mover = (struct move_request *) xmalloc(sizeof(struct move_request));

	/* fill in the move structure */
	bynumber_mover->vol = mv->this;
	bynumber_mover->sfrom = bynumber_mover->vol->shome;
	bynumber_mover->pfrom = bynumber_mover->vol->phome;
	bynumber_mover->sto = destp->this->who;
	bynumber_mover->pto = destp->this;

	return bynumber_mover;
    }

    /* Give Up */
    if (bynumber_mover) {
	free(bynumber_mover);
	bynumber_mover = NULL;
    }
    return (struct move_request *) NULL;
}

/* a tester function, returning boolean true if we are OK with
 * the propsed move
 */
int bynumber_queryrequest(mover)
struct move_request *mover;
{
    /* yeah, yeah */
    if (mover == bynumber_mover) return 1;

    if (VOLPERSIZE_WI_V(mover->pto) < UPPER_BOUND)
      return 1;

#ifdef DEBUG
    printf("REJECT: bynumber onto a partition %f >= %f\n",VOLPERSIZE_WI_V(mover->pto),UPPER_BOUND);
#endif

    return 0;
}

/* get notified of transactions that are taking place,
 * update internal data structures
 *
 * by the time we are called, the master structure is updated
 * IN PARTICULAR the partition info is correct
 */
void bynumber_setrequest(mover)
struct move_request *mover;
{
    struct bynumber_partition *fromp, *top;
    struct bynumber_volume *vol;

    /* means our transaction has been accepted, so we will want to
     * get a new transaction to meddle with
     */
    if (mover == bynumber_mover) bynumber_mover = NULL;

    /* find the bynumber_partition equivalents */
    fromp = (struct bynumber_partition *)hashtable_find(part_ht, (key.addr = mover->pfrom, key));
    top = (struct bynumber_partition *)hashtable_find(part_ht, (key.addr = mover->pto, key));
    
    /* this is kinda sleezy, but is easy to grok */

    /* take 'em out */
    bynumber_delp(&head, &tail, fromp);
    bynumber_delp(&head, &tail, top);

    /* put 'em back in */
    bynumber_insertp(&head, &tail, fromp);
    bynumber_insertp(&head, &tail, top);

    /* find the bynumber_volume equivalent */
    vol = (struct bynumber_volume *)hashtable_find(vol_ht, (key.addr = mover->vol, key));
    
    /* move the actual volume structure now */
    bynumber_delv(&fromp->vols, vol);
    bynumber_insertv(&top->vols, vol);

    /* if the from partition touched is within our destination
     * candidate pool, we assume that this operation may "break 
     * something free" for us and reset our nextvol pointers
     */
    if (VOLPERSIZE(fromp->this) < MOVE_BOUND) {
	fromp = head;
	while(fromp) {
	    fromp->nextvol = fromp->vols;
	    fromp = fromp->next;
	}
    }
#if !NEW_MATH
    /* update avg_occ */
    avg_vol_per_size = avg_vol_per_size + (1.0/mover->pto->size -
                                           1.0/mover->pfrom->size)/npart;
#endif
    return;
}

void bynumber_discardv(v)
struct volume *v;
{
    struct bynumber_volume *bv;
    struct bynumber_partition *bp;

    /* if our mover transaction is open, take this as
     * a hint to give up
     */
    if (bynumber_mover != NULL) {
	free(bynumber_mover);
	bynumber_mover = NULL;
    }

    bv = (struct bynumber_volume *) hashtable_find(vol_ht, (key.addr = v, key));
    bp = (struct bynumber_partition *) hashtable_find(part_ht, (key.addr = v->phome, key));

    /* make sure we don't do something stupid, like keep a reference to it */
    if (bp->nextvol == bv)
      bp->nextvol = bv->next;

    bynumber_delv(&bp->vols, bv);
}

/* dump a report of our state of the world */
void bynumber_report()
{
    struct bynumber_partition *up;
    struct bynumber_volume *uv;

    puts(">>>> bynumber");

    up = head;
    while (up) {
	printf("partition /vicep%c of %s, %d vols in %dK (%f v/s)\n",
	       up->this->pid + 'a',
	       up->this->who->who->name,
	       up->this->nvols,
	       up->this->size,
	       VOLPERSIZE(up->this));

#ifdef DEBUG
	uv = up->vols;
	while (uv) {
	    printf("%s using %dK of %dK quota\n",
		   uv->this->name,
		   uv->this->size,
		   uv->this->maxquota);
	    uv = uv->next;
	}
#endif
	up = up->next;
    }
    printf("TOTAL avg_vol_per_size %f, npart %d\n", avg_vol_per_size, npart);
#ifdef DEBUG
    puts("part_ht:");
    hashtable_stats(part_ht);
    puts("vol_ht:");
    hashtable_stats(vol_ht);
#endif
}
#endif
