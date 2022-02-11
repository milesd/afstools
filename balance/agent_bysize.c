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
#if BYSIZE
#include <stdio.h>

#include "balance.h"
#include "balutil.h"
#include "hash.h"
#include "mygetopt.h"

#include "agent_bysize.h"

/* the definition of occupancy is used/size */
#define OCCUPANCY(ap) (double)(1 - ((double)(ap)->free/(double)(ap)->size))

/* occupancy of a partition with and without volume v */
#define OCCUPANCY_WI_V(ap, v) (double)(1 - (((double)(ap)->free - (double)(v)->size)/(double)(ap)->size))
#define OCCUPANCY_WO_V(ap, v) (double)(1 - (((double)(ap)->free + (double)(v)->size)/(double)(ap)->size))

#if !NEW_MATH
/* we take our precision to mean that our window is defined around
 * the average occupation of servers by allowing us to encroach
 * upwards precision% into the free space avaliable and downwards
 * precision% into the occupied space. This is NOT symmetric.
 *
 * these evaluate to a percentage occupation
 */
#define UPPER_BOUND (avg_occ + (1 - avg_occ)*precision)
#define MOVE_BOUND  (avg_occ + (1 - avg_occ)*(precision/2.0))
#define LOWER_BOUND (avg_occ * (1 - precision)) 
#else
  /* we take our precision to mean that our window is defined around
   * the average occupation of servers by allowing us to encroach
   * depending how full we are.  The closer the average is to a boundary
   * the smaller our window.  This formula IS symmetric.
   *
   * these evaluate to a percentage occupation
   */
#define FUDGE (avg_occ * (1-avg_occ) * precision)
#define UPPER_BOUND (avg_occ + FUDGE)
#define MOVE_BOUND  (avg_occ + FUDGE/2)
#define LOWER_BOUND (avg_occ - FUDGE)
#endif
/* a simple wrapper so we can sort partitions the way we want */
struct bysize_partition {
    struct partition *this;         /* partition here */
    struct bysize_volume *vols,     /* sorted list of volumes on this partition */
                         *nextvol;  /* pointer into vols list, next volume we may
				     * want to care about on partition */
    struct bysize_partition *next, *prev;
};

/* a simple wrapper so we can sort volumes the way we want */
struct bysize_volume {
    struct volume *this;           /* volume here */
    struct bysize_volume *next;
};

static struct agent_procs agent_bysize_procs = {
    bysize_servconf,
    bysize_getrequest,
    bysize_queryrequest,
    bysize_setrequest,
    bysize_discardv,
    bysize_report
};

/****************************
 * BySize Agent Global Data *
 ****************************/

static struct bysize_partition *head = NULL, 
                               *tail = NULL;     /* sorted chain of partitions
                                                  */
#if NEW_MATH
static int tot_occ = 0;                                /* total occupancy of partitions */
static int tot_space = 0;                      /* total partition space to
                                                  occupy */
#endif
static double avg_occ = 0;                       /* average % occupancy of partitions */
static unsigned long npart = 0;                  /* number of partitions we have */
static double precision = 0.20;                  /* get partitions to within 20% by default */
static struct hashtable *part_ht = NULL;         /* struct partition -> struct bysize_partition map */
static struct hashtable *vol_ht = NULL;          /* struct volume -> struct bysize_volume map */
static struct move_request *bysize_mover = NULL; /* cached move request we currently care about */
static union hashtable_key key;                  /* generic hashtable key */

struct agent_procs *bysize_doargs(args)
char *args;
{
    int argc, error, c;
    char **argv;
    extern char *optarg;

    if (args != NULL) {
#ifdef DEBUG
	printf("%s processing %s\n", BYSIZE_NAME, args);
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

    return &agent_bysize_procs;
}

/* insert a partition, sorted, into the partition chain */
static void bysize_insertp(forepp, tailpp, addp)
struct bysize_partition **forepp, **tailpp, *addp;
{
    addp->prev = NULL;

    while (*forepp) {
	/* if elt has less occupancy than what we are adding 
	 * we've found the slot to drop into
	 */
	if (OCCUPANCY((*forepp)->this) <= OCCUPANCY(addp->this))
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
static void bysize_delp(forepp, tailpp, addp)
struct bysize_partition **forepp, **tailpp, *addp;
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
static void bysize_insertv(lvp, addv)
struct bysize_volume **lvp, *addv;
{
    while (*lvp) {
	/* if elt has less usage than the one we're
	 * adding, we've found the slot to drop into
	 */
	if ((*lvp)->this->size <= addv->this->size)
	  break;

	lvp = &(*lvp)->next;
    }

    /* thread onto the chain */
    addv->next = *lvp;
    *lvp = addv;
}

/* delete a volume from a volume chain */
static void bysize_delv(lvp, addv)
struct bysize_volume **lvp, *addv;
{
    while (*lvp) {
	if (*lvp == addv) {
	    *lvp = addv->next;
	    addv->next = (struct bysize_volume *) NULL;
	    return;
	}

	lvp = &(*lvp)->next;
    }
}

void bysize_servconf(cell)
struct server *cell;
{
    struct partition *pp;
    struct volume *vp;
    struct bysize_partition *up;
    struct bysize_volume *uv;

    while (cell) {
	pp = cell->parts;

	/* walk along partitions, adding them into sorted local structs */
	while (pp) {
	    up = (struct bysize_partition *) xmalloc(sizeof(struct bysize_partition));

	    up->this = pp;
	    bysize_insertp(&head, &tail, up);

	    if (part_ht == NULL) part_ht = hashtable_create(hashfun_addr, hashcmp_addr,
							    hashdup_addr, hashzap_addr);
	    /* index struct bysize_partition by struct partition */
	    hashtable_add(part_ht, (key.addr = pp, key), up);

	    vp = pp->vols;

	    /* walk along volumes, adding them into sorted local structs */
	    while (vp) {
		uv = (struct bysize_volume *) xmalloc(sizeof(struct bysize_volume));

		uv->this = vp;
		bysize_insertv(&up->vols, uv);

		if (vol_ht == NULL) vol_ht = hashtable_create(hashfun_addr, hashcmp_addr,
							      hashdup_addr, hashzap_addr);
		/* index struct bysize_volume by struct volume */
		hashtable_add(vol_ht, (key.addr = vp, key), uv);

		vp = vp->next;
	    }

	    /* init next volume pointer */
	    up->nextvol = up->vols;

	    /* update occupancy counters */
	    npart++;
#if !NEW_MATH
            avg_occ = ((avg_occ * (npart -1)) + OCCUPANCY(up->this))/npart;
#else
            tot_occ += up->this->size - up->this->free;
            tot_space += up->this->size;
#endif
	    pp = pp->next;
	}

	cell = cell->next;
    }
#if NEW_MATH
    avg_occ = ((double)tot_occ) / ((double)tot_space);
#endif

}

/* find a place to move this volume, NULL if we didn't
 * find anything
 *
 * start at "start", so we can keep polling for partitions
 * in our searcher
 */
static struct bysize_partition *bysize_findest(start, v)
struct bysize_partition *start;
struct volume *v;
{
    struct bysize_partition *pp = start;

    while (pp) {
	/* if this partition can take this volume without
	 * pushing it above the upper precision limit where
	 * we may want to move it again ...
	 */
	if (OCCUPANCY_WI_V(pp->this, v) < MOVE_BOUND)
	  /* let's do it */
	  return pp;

	pp = pp->prev;
    }

    return (struct bysize_partition *) NULL;
}

struct move_request *bysize_getrequest()
{
    struct bysize_partition *pp = head;
    static struct bysize_partition *destp; /* handle on last partition we
					    * tried to move volume to */
    struct bysize_volume *mv = NULL;

    /* if we are currently trying to move a volume */
    if (bysize_mover != NULL) {
	/* starting at the next partition, try to find a place
	 * we want to move this thing to
	 */
	destp = bysize_findest(destp->prev, bysize_mover->vol);

	/* if we didn't fail ... */
	if (destp != NULL) {
	    /* update our transaction */
	    bysize_mover->sto = destp->this->who;
	    bysize_mover->pto = destp->this;

	    /* and try again */
	    return bysize_mover;
	}

	/* oh well, we have to punt on this volume now */
	/* find the bynumber_partition this volume is on 
	 * to discover our place in the search
	 */
	pp = (struct bysize_partition *)hashtable_find(part_ht, (key.addr = bysize_mover->pfrom, key));

#ifdef DEBUG
	printf("bysize: rejection moving %s from %s/%c\n",
	       bysize_mover->vol->name,
	       bysize_mover->sfrom->who->name,
	       bysize_mover->pfrom->pid + 'a');
#endif

	/* fall off into finding another volume */
    }

    /* for all partitions while we don't have a volume to move */
    while (pp && !mv) {
	/* if we need to balance this partition */
	if (OCCUPANCY(pp->this) > MOVE_BOUND) {
	    /* for all volumes on this partition ... */
	    while (pp->nextvol && !mv) {
		/* if moving this volume will make the partition more
		 * happy while not driving the partition under the lower
		 * precision limit (to try to prevent oscillation), and
		 * we can find a place to move it to.
		 */
		if (OCCUPANCY_WO_V(pp->this, pp->nextvol->this) > LOWER_BOUND &&
		    (destp = bysize_findest(tail, pp->nextvol->this)))
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
	if (bysize_mover == NULL)
	  bysize_mover = (struct move_request *) xmalloc(sizeof(struct move_request));

	/* fill in the move structure */
	bysize_mover->vol = mv->this;
	bysize_mover->sfrom = bysize_mover->vol->shome;
	bysize_mover->pfrom = bysize_mover->vol->phome;
	bysize_mover->sto = destp->this->who;
	bysize_mover->pto = destp->this;

	return bysize_mover;
    }

    /* Give Up */
    if (bysize_mover) {
	free(bysize_mover);
	bysize_mover = NULL;
    }
    return (struct move_request *) NULL;
}

/* a tester function, returning boolean true if we are OK with
 * the propsed move
 */
int bysize_queryrequest(mover)
struct move_request *mover;
{
    /* yeah, yeah */
    if (mover == bysize_mover) return 1;

    if (OCCUPANCY_WI_V(mover->pto, mover->vol) < UPPER_BOUND)
      return 1;

#ifdef DEBUG
    printf("REJECT: bysize onto a partition %f >= %f\n",OCCUPANCY_WI_V(mover->pto, mover->vol),UPPER_BOUND);
#endif

    return 0;
}

/* get notified of transactions that are taking place,
 * update internal data structures
 *
 * by the time we are called, the master structure is updated
 * IN PARTICULAR the partition info is correct
 */
void bysize_setrequest(mover)
struct move_request *mover;
{
    struct bysize_partition *fromp, *top;
    struct bysize_volume *vol;

    /* means our transaction has been accepted, so we will want to
     * get a new transaction to meddle with
     */
    if (mover == bysize_mover) bysize_mover = NULL;

    /* find the bysize_partition equivalents */
    fromp = (struct bysize_partition *)hashtable_find(part_ht, (key.addr = mover->pfrom, key));
    top = (struct bysize_partition *)hashtable_find(part_ht, (key.addr = mover->pto, key));
    
    /* this is kinda sleezy, but is easy to grok */

    /* take 'em out */
    bysize_delp(&head, &tail, fromp);
    bysize_delp(&head, &tail, top);

    /* put 'em back in */
    bysize_insertp(&head, &tail, fromp);
    bysize_insertp(&head, &tail, top);

    /* find the bysize_volume equivalent */
    vol = (struct bysize_volume *)hashtable_find(vol_ht, (key.addr = mover->vol, key));
    
    /* move the actual volume structure now */
    bysize_delv(&fromp->vols, vol);
    bysize_insertv(&top->vols, vol);

    /* if from partition is within our destination
     * candidate pool, we assume that this operation may "break 
     * something free" for us and reset our nextvol pointers
     */
    if (OCCUPANCY(fromp->this) < MOVE_BOUND) {
	fromp = head;
	while(fromp) {
	    fromp->nextvol = fromp->vols;
	    fromp = fromp->next;
	}
    }
#if !NEW_MATH
        /* update avg_occ */
    avg_occ = avg_occ - (mover->vol->size/npart)*(1.0/mover->pfrom->size - 1.0/mover->pto->size);
#endif
    return;
}

void bysize_discardv(v)
struct volume *v;
{
    struct bysize_volume *bv;
    struct bysize_partition *bp;

    /* if our mover transaction is open, take this as
     * a hint to give up
     */
    if (bysize_mover != NULL) {
	free(bysize_mover);
	bysize_mover = NULL;
    }

    bv = (struct bysize_volume *) hashtable_find(vol_ht, (key.addr = v, key));
    bp = (struct bysize_partition *) hashtable_find(part_ht, (key.addr = v->phome, key));

    /* make sure we don't do something stupid, like keep a reference to it */
    if (bp->nextvol == bv)
      bp->nextvol = bv->next;

    bysize_delv(&bp->vols, bv);
}

/* dump a report of our state of the world */
void bysize_report()
{
    struct bysize_partition *up;
    struct bysize_volume *uv;

    puts(">>>> bysize");

    up = head;
    while (up) {
	printf("partition /vicep%c of %s, %luK of %luK used (%.2f%% occ)\n",
	       up->this->pid + 'a',
	       up->this->who->who->name,
	       up->this->size - up->this->free,
	       up->this->size,
	       OCCUPANCY(up->this) * 100);
#ifdef DEBUG
	uv = up->vols;
	while (uv) {
	    printf("%s using %luK of %luK quota\n",
		   uv->this->name,
		   uv->this->size,
		   uv->this->maxquota);
	    uv = uv->next;
	}
#endif
	up = up->next;
    }
    printf("TOTAL avg_occ %.2f%% in %lu partitions\n", avg_occ * 100, npart);
}
#endif
