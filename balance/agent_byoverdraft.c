/* This file is part of the balancer
 * Author: Henry C. Schmitt, hcs+@cmu.edu
 */

/*
 *        Copyright 1995 by Carnegie Mellon University
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
#if BYOVERDRAFT
#include <stdio.h>

#include "balance.h"
#include "balutil.h"
#include "hash.h"
#include "mygetopt.h"

#include "agent_byoverdraft.h"

/* the definition of allocation is quota/size */
#define ALLOCATION(ap) (double)((double)((ap)->maxquota)/(double)((ap)->size))

/* allocation of a partition with and without volume v */
#define ALLOCATION_WI_V(ap, v) (double)(((double)((ap)->maxquota)+(double)(v->maxquota))/(double)((ap)->size))
#define ALLOCATION_WO_V(ap, v) (double)(((double)((ap)->maxquota)-(double)(v->maxquota))/(double)((ap)->size))

#if !NEW_MATH
/* we take our precision to mean that our window is defined around
 * the average occupation of servers by allowing us to encroach
 * upwards precision% into the free space avaliable and downwards
 * precision% into the occupied space. This is NOT symmetric.
 *
 * these evaluate to a percentage occupation
 */
#define UPPER_BOUND (avg_alloc + avg_alloc*precision)
#define MOVE_BOUND  (avg_alloc + avg_alloc*(precision/2.0))
#define LOWER_BOUND (avg_alloc * precision) 
#else
#define FUDGE (avg_alloc * (1-avg_alloc) * precision)
#define UPPER_BOUND (avg_alloc + FUDGE)
#define MOVE_BOUND  (avg_alloc + FUDGE/2)
#define LOWER_BOUND (avg_alloc - FUDGE) 
#endif
/* a simple wrapper so we can sort partitions the way we want */
struct byoverdraft_partition {
    struct partition *this;         /* partition here */
    struct byoverdraft_volume *vols,     /* sorted list of volumes on this partition */
                         *nextvol;  /* pointer into vols list, next volume we may
				     * want to care about on partition */
    struct byoverdraft_partition *next, *prev;
};

/* a simple wrapper so we can sort volumes the way we want */
struct byoverdraft_volume {
    struct volume *this;           /* volume here */
    struct byoverdraft_volume *next;
};

static struct agent_procs agent_byoverdraft_procs = {
    byoverdraft_servconf,
    byoverdraft_getrequest,
    byoverdraft_queryrequest,
    byoverdraft_setrequest,
    byoverdraft_discardv,
    byoverdraft_report
};

/****************************
 * Byoverdraft Agent Global Data *
 ****************************/

static struct byoverdraft_partition *head = NULL, 
                               *tail = NULL;     /* sorted chain of partitions */
#if NEW_MATH
static int tot_alloc = 0;
static int tot_space = 0;
#endif
static double avg_alloc = 0;                       /* average % allocation of partitions */
static unsigned long npart = 0;                  /* number of partitions we have */
static double precision = 0.20;                  /* get partitions to within 20% by default */
static struct hashtable *part_ht = NULL;         /* struct partition -> struct byoverdraft_partition map */
static struct hashtable *vol_ht = NULL;          /* struct volume -> struct byoverdraft_volume map */
static struct move_request *byoverdraft_mover = NULL; /* cached move request we currently care about */
static union hashtable_key key;                  /* generic hashtable key */

/*unsigned long part_quota(pp)
struct partition *pp;
{
    unsigned long quota = 0;
    struct volume *vp = pp->vols;
    
    while (vp != NULL)
    {
	quota += vp->maxquota;
	vp = vp->next;
    }
    return quota;
}*/

struct agent_procs *byoverdraft_doargs(args)
char *args;
{
    int argc, error, c;
    char **argv;
    extern char *optarg;

    if (args != NULL) {
#ifdef DEBUG
	printf("%s processing %s\n", BYOVERDRAFT_NAME, args);
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

    return &agent_byoverdraft_procs;
}

/* insert a partition, sorted, into the partition chain */
static void byoverdraft_insertp(forepp, tailpp, addp)
struct byoverdraft_partition **forepp, **tailpp, *addp;
{
    addp->prev = NULL;

    while (*forepp) {
	/* if elt has less allocation than what we are adding 
	 * we've found the slot to drop into
	 */
	if (ALLOCATION((*forepp)->this) <= ALLOCATION(addp->this))
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
static void byoverdraft_delp(forepp, tailpp, addp)
struct byoverdraft_partition **forepp, **tailpp, *addp;
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
static void byoverdraft_insertv(lvp, addv)
struct byoverdraft_volume **lvp, *addv;
{
    while (*lvp) {
	/* if elt has less usage than the one we're
	 * adding, we've found the slot to drop into
	 */
	if ((*lvp)->this->maxquota <= addv->this->maxquota)
	  break;

	lvp = &(*lvp)->next;
    }

    /* thread onto the chain */
    addv->next = *lvp;
    *lvp = addv;
}

/* delete a volume from a volume chain */
static void byoverdraft_delv(lvp, addv)
struct byoverdraft_volume **lvp, *addv;
{
    while (*lvp) {
	if (*lvp == addv) {
	    *lvp = addv->next;
	    addv->next = (struct byoverdraft_volume *) NULL;
	    return;
	}

	lvp = &(*lvp)->next;
    }
}

void byoverdraft_servconf(cell)
struct server *cell;
{
    struct partition *pp;
    struct volume *vp;
    struct byoverdraft_partition *up;
    struct byoverdraft_volume *uv;

    while (cell) {
	pp = cell->parts;

	/* walk along partitions, adding them into sorted local structs */
	while (pp) {
	    up = (struct byoverdraft_partition *) xmalloc(sizeof(struct byoverdraft_partition));

	    up->this = pp;
	    
	    byoverdraft_insertp(&head, &tail, up);

	    if (part_ht == NULL) part_ht = hashtable_create(hashfun_addr, hashcmp_addr,
							    hashdup_addr, hashzap_addr);
	    /* index struct byoverdraft_partition by struct partition */
	    hashtable_add(part_ht, (key.addr = pp, key), up);

	    vp = pp->vols;

	    /* walk along volumes, adding them into sorted local structs */
	    while (vp) {
		uv = (struct byoverdraft_volume *) xmalloc(sizeof(struct byoverdraft_volume));

		uv->this = vp;
		byoverdraft_insertv(&up->vols, uv);

		if (vol_ht == NULL) vol_ht = hashtable_create(hashfun_addr, hashcmp_addr,
							      hashdup_addr, hashzap_addr);
		/* index struct byoverdraft_volume by struct volume */
		hashtable_add(vol_ht, (key.addr = vp, key), uv);

		vp = vp->next;
	    }

	    /* init next volume pointer */
	    up->nextvol = up->vols;

	    /* update allocation counters */
	    npart++;
#if !NEW_MATH
            avg_alloc = ((avg_alloc * (npart - 1)) +
                         ALLOCATION(up->this))/npart;
#else
            tot_alloc += up->this->maxquota;
            tot_space += up->this->size;
#endif

	    pp = pp->next;
	}

	cell = cell->next;
    }
#if NEW_MATH
    avg_alloc  = ((double)tot_alloc) / ((double)tot_space);
#endif
}

/* find a place to move this volume, NULL if we didn't
 * find anything
 *
 * start at "start", so we can keep polling for partitions
 * in our searcher
 */
static struct byoverdraft_partition *byoverdraft_findest(start, v)
struct byoverdraft_partition *start;
struct volume *v;
{
    struct byoverdraft_partition *pp = start;

    while (pp) {
	/* if this partition can take this volume without
	 * pushing it above the upper precision limit where
	 * we may want to move it again ...
	 */
	if (ALLOCATION_WI_V(pp->this, v) < MOVE_BOUND)
	  /* let's do it */
	  return pp;

	pp = pp->prev;
    }

    return (struct byoverdraft_partition *) NULL;
}

struct move_request *byoverdraft_getrequest()
{
    struct byoverdraft_partition *pp = head;
    static struct byoverdraft_partition *destp; /* handle on last partition we
					    * tried to move volume to */
    struct byoverdraft_volume *mv = NULL;

    /* if we are currently trying to move a volume */
    if (byoverdraft_mover != NULL) {
	/* starting at the next partition, try to find a place
	 * we want to move this thing to
	 */
	destp = byoverdraft_findest(destp->prev, byoverdraft_mover->vol);

	/* if we didn't fail ... */
	if (destp != NULL) {
	    /* update our transaction */
	    byoverdraft_mover->sto = destp->this->who;
	    byoverdraft_mover->pto = destp->this;

	    /* and try again */
	    return byoverdraft_mover;
	}

	/* oh well, we have to punt on this volume now */
	/* find the bynumber_partition this volume is on 
	 * to discover our place in the search
	 */
	pp = (struct byoverdraft_partition *)hashtable_find(part_ht, (key.addr = byoverdraft_mover->pfrom, key));

#ifdef DEBUG
	printf("byoverdraft: rejection moving %s from %s/%c\n",
	       byoverdraft_mover->vol->name,
	       byoverdraft_mover->sfrom->who->name,
	       byoverdraft_mover->pfrom->pid + 'a');
#endif

	/* fall off into finding another volume */
    }

    /* for all partitions while we don't have a volume to move */
    while (pp && !mv) {
	/* if we need to balance this partition */
	if (ALLOCATION(pp->this) > MOVE_BOUND) {
	    /* for all volumes on this partition ... */
	    while (pp->nextvol && !mv) {
		/* if moving this volume will make the partition more
		 * happy while not driving the partition under the lower
		 * precision limit (to try to prevent oscillation), and
		 * we can find a place to move it to.
		 */
		if (ALLOCATION_WO_V(pp->this, pp->nextvol->this) > LOWER_BOUND &&
		    (destp = byoverdraft_findest(tail, pp->nextvol->this)))
		  /* got us a volume to move */
		  mv = pp->nextvol;

		/* push along the volume list */
		pp->nextvol = pp->nextvol->next;
	    }
	}

	/* push along partition list */
	pp = pp->next;
    }

    if (mv) {
	if (byoverdraft_mover == NULL)
	  byoverdraft_mover = (struct move_request *) xmalloc(sizeof(struct move_request));

	/* fill in the move structure */
	byoverdraft_mover->vol = mv->this;
	byoverdraft_mover->sfrom = byoverdraft_mover->vol->shome;
	byoverdraft_mover->pfrom = byoverdraft_mover->vol->phome;
	byoverdraft_mover->sto = destp->this->who;
	byoverdraft_mover->pto = destp->this;

	return byoverdraft_mover;
    }

    /* Give Up */
    if (byoverdraft_mover) {
	free(byoverdraft_mover);
	byoverdraft_mover = NULL;
    }
    return (struct move_request *) NULL;
}

/* a tester function, returning boolean true if we are OK with
 * the propsed move
 */
int byoverdraft_queryrequest(mover)
struct move_request *mover;
{
    /* yeah, yeah */
    if (mover == byoverdraft_mover) return 1;

    if (ALLOCATION_WI_V(mover->pto, mover->vol) < UPPER_BOUND)
      return 1;

#ifdef DEBUG
    printf("REJECT: byoverdraft onto a partition %f >= %f\n",ALLOCATION_WI_V(mover->pto, mover->vol),UPPER_BOUND);
#endif

    return 0;
}

/* get notified of transactions that are taking place,
 * update internal data structures
 *
 * by the time we are called, the master structure is updated
 * IN PARTICULAR the partition info is correct
 */
void byoverdraft_setrequest(mover)
struct move_request *mover;
{
    struct byoverdraft_partition *fromp, *top;
    struct byoverdraft_volume *vol;

    /* means our transaction has been accepted, so we will want to
     * get a new transaction to meddle with
     */
    if (mover == byoverdraft_mover) byoverdraft_mover = NULL;

    /* find the byoverdraft_partition equivalents */
    fromp = (struct byoverdraft_partition *)hashtable_find(part_ht, (key.addr = mover->pfrom, key));
    top = (struct byoverdraft_partition *)hashtable_find(part_ht, (key.addr = mover->pto, key));

    /* update the maxquota counters */
    fromp->this->maxquota -= mover->vol->maxquota;
    top->this->maxquota += mover->vol->maxquota;
    
    /* this is kinda sleezy, but is easy to grok */

    /* take 'em out */
    byoverdraft_delp(&head, &tail, fromp);
    byoverdraft_delp(&head, &tail, top);

    /* put 'em back in */
    byoverdraft_insertp(&head, &tail, fromp);
    byoverdraft_insertp(&head, &tail, top);

    /* find the byoverdraft_volume equivalent */
    vol = (struct byoverdraft_volume *)hashtable_find(vol_ht, (key.addr = mover->vol, key));
    
    /* move the actual volume structure now */
    byoverdraft_delv(&fromp->vols, vol);
    byoverdraft_insertv(&top->vols, vol);

    /* if from partition is within our destination
     * candidate pool, we assume that this operation may "break 
     * something free" for us and reset our nextvol pointers
     */
    if (ALLOCATION(fromp->this) < MOVE_BOUND) {
	fromp = head;
	while(fromp) {
	    fromp->nextvol = fromp->vols;
	    fromp = fromp->next;
	}
    }

#if !NEW_MATH
    /* update avg_alloc */
    avg_alloc -= (ALLOCATION_WI_V(mover->pfrom, mover->vol) + ALLOCATION_WO_V(mover->pto, mover->vol))/npart;
    avg_alloc += (ALLOCATION(mover->pfrom) + ALLOCATION(mover->pto))/npart;
#endif
    
    return;
}

void byoverdraft_discardv(v)
struct volume *v;
{
    struct byoverdraft_volume *bv;
    struct byoverdraft_partition *bp;

    /* if our mover transaction is open, take this as
     * a hint to give up
     */
    if (byoverdraft_mover != NULL) {
	free(byoverdraft_mover);
	byoverdraft_mover = NULL;
    }

    bv = (struct byoverdraft_volume *) hashtable_find(vol_ht, (key.addr = v, key));
    bp = (struct byoverdraft_partition *) hashtable_find(part_ht, (key.addr = v->phome, key));

    /* make sure we don't do something stupid, like keep a reference to it */
    if (bp->nextvol == bv)
      bp->nextvol = bv->next;

    byoverdraft_delv(&bp->vols, bv);
}

/* dump a report of our state of the world */
void byoverdraft_report()
{
    struct byoverdraft_partition *up;
    struct byoverdraft_volume *uv;

    puts(">>>> byoverdraft");

    up = head;
    while (up) {
	printf("partition /vicep%c of %s, %luK of %luK allocated (%.2f%% alloc)\n",
	       up->this->pid + 'a',
	       up->this->who->who->name,
	       up->this->maxquota,
	       up->this->size,
	       ALLOCATION(up->this) * 100);
#ifdef DEBUG
	uv = up->vols;
	while (uv) {
	    printf("%s using %luK of %luK quota\n",
		   uv->this->name,
		   uv->this->maxquota,
		   uv->this->size);
	    uv = uv->next;
	}
#endif
	up = up->next;
    }
    printf("TOTAL avg_alloc %.2f%% in %lu partitions\n", avg_alloc * 100, npart);
}
#endif
