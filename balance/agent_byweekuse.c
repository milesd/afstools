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
#if BYWEEKUSE
#include <stdio.h>

#include "balance.h"
#include "balutil.h"
#include "hash.h"
#include "mygetopt.h"

#include "agent_byweekuse.h"

/* we don't scale weekuse against anything */
#define WEEKUSE(as) ((double)(as)->weekuse)

/* weekuse of a server or partitions with and without volume v */
#define WEEKUSE_WI_V(as, v) (double)((double)(as)->weekuse + (double)(v)->weekuse)
#define WEEKUSE_WO_V(as, v) (double)((double)(as)->weekuse - (double)(v)->weekuse)

/* simple up/down based on precision
 * these evaluate to a percentage occupation
 */
#define UPPER_BOUND (avg_weekuse * (1 + precision))
#define MOVE_BOUND  (avg_weekuse * (1 + (precision/2.0)))
#define LOWER_BOUND (avg_weekuse * (1 - precision)) 

/* a simple wrapper so we can sort partitions */
struct byweekuse_partition {
    struct partition *this;         /* partition here */
    struct byweekuse_volume *vols,  /* sorted list of volumes on this partition */
                         *nextvol;  /* pointer into vols list, next volume we may
				     * want to care about on partition */
    struct byweekuse_partition *next;
};

/* a simple wrapper so we can sort servers */
struct byweekuse_server {
    struct server *this;                 /* server here */
    struct byweekuse_partition *parts;   /* sorted list of partitions */
    unsigned long npart,                 /* # of partitions on this server */
                  weekuse;               /* sum weekuse of partitions */
    struct byweekuse_server *next, *prev;
};

/* a simple wrapper so we can sort volumes */
struct byweekuse_volume {
    struct volume *this;           /* volume here */
    struct byweekuse_volume *next;
};

static struct agent_procs agent_byweekuse_procs = {
    byweekuse_servconf,
    byweekuse_getrequest,
    byweekuse_queryrequest,
    byweekuse_setrequest,
    byweekuse_discardv,
    byweekuse_report
};

/*******************************
 * Byweekuse Agent Global Data *
 *******************************/

static struct byweekuse_server *head = NULL, 
                               *tail = NULL;     /* sorted chain of servers */
static double avg_weekuse = 0;                   /* average usage of *partitions* */
static unsigned long npart = 0;                  /* number of *partitions* we have */
static double precision = 0.20;                  /* get partitions to within 20% by default */
static struct hashtable *part_ht = NULL;         /* struct partition -> struct byweekuse_partition map */
static struct hashtable *vol_ht = NULL;          /* struct volume -> struct byweekuse_volume map */
static struct hashtable *srv_ht = NULL;          /* struct server -> struct byweekuse_server map */
static struct move_request *byweekuse_mover = NULL; /* cached move request we currently care about */
static union hashtable_key key;                  /* generic hashtable key */

struct agent_procs *byweekuse_doargs(args)
char *args;
{
    int argc, error, c;
    char **argv;
    extern char *optarg;

    if (args != NULL) {
#ifdef DEBUG
	printf("%s processing %s\n", BYWEEKUSE_NAME, args);
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

    return &agent_byweekuse_procs;
}

/* insert a server, sorted, into the server chain */
static void byweekuse_inserts(foresp, tailsp, adds)
struct byweekuse_server **foresp, **tailsp, *adds;
{
    adds->prev = NULL;

    while (*foresp) {
	/* if elt has less WEEKUSE than what we are adding 
	 * we've found the slot to drop into
	 */
	if (WEEKUSE(*foresp) <= WEEKUSE(adds))
	  break;

	/* if we're about to break out, set the prev pointer now */
	if ((*foresp)->next == NULL)
	  adds->prev = *foresp;

	foresp = &(*foresp)->next;
    }

    /* thread onto the forward chain */
    adds->next = *foresp;
    *foresp = adds;
    if (adds->next) {
	/* thread onto the backward chain */
	adds->prev = adds->next->prev;
	adds->next->prev = adds;
    } else {
	/* we are at the end of the forward chain,
	 * so update tail pointer
	 */
	*tailsp = adds;
    }
}

/* delete a server from the server chain */
static void byweekuse_dels(foresp, tailsp, adds)
struct byweekuse_server **foresp, **tailsp, *adds;
{
    if (adds->prev) {
	/* not at top of the list */
	adds->prev->next = adds->next;
    } else {
	/* update top of the list */
	*foresp = adds->next;
    }

    if (adds->next) {
	/* not at end of the list */
	adds->next->prev = adds->prev;
    } else {
	/* update end of the list */
	*tailsp = adds->prev;
    }
}

/* insert a partition, sorted, into a partition chain */
static void byweekuse_insertp(lpp, addp)
struct byweekuse_partition **lpp, *addp;
{
    while (*lpp) {
	/* if elt has more usage than the one we're
	 * adding, we've found the slot to drop into
	 *
	 * we reverse sort this because candidate dest partitions
	 * should be considered from least used to most used on
	 * a server
	 */
	if (WEEKUSE((*lpp)->this) > WEEKUSE(addp->this))
	  break;

	lpp = &(*lpp)->next;
    }

    /* thread onto the chain */
    addp->next = *lpp;
    *lpp = addp;
}

/* delete a volume from a volume chain */
static void byweekuse_delp(lpp, addp)
struct byweekuse_partition **lpp, *addp;
{
    while (*lpp) {
	if (*lpp == addp) {
	    *lpp = addp->next;
	    addp->next = (struct byweekuse_partition *) NULL;
	    return;
	}

	lpp = &(*lpp)->next;
    }
}

/* insert a volume, sorted, into a volume chain */
static void byweekuse_insertv(lvp, addv)
struct byweekuse_volume **lvp, *addv;
{
    while (*lvp) {
	/* if elt has less usage than the one we're
	 * adding, we've found the slot to drop into
	 */
	if ((*lvp)->this->weekuse <= addv->this->weekuse)
	  break;

	lvp = &(*lvp)->next;
    }

    /* thread onto the chain */
    addv->next = *lvp;
    *lvp = addv;
}

/* delete a volume from a volume chain */
static void byweekuse_delv(lvp, addv)
struct byweekuse_volume **lvp, *addv;
{
    while (*lvp) {
	if (*lvp == addv) {
	    *lvp = addv->next;
	    addv->next = (struct byweekuse_volume *) NULL;
	    return;
	}

	lvp = &(*lvp)->next;
    }
}

void byweekuse_servconf(cell)
struct server *cell;
{
    struct partition *pp;
    struct volume *vp;
    struct byweekuse_partition *up;
    struct byweekuse_volume *uv;
    struct byweekuse_server *us;

    while (cell) {
	us = (struct byweekuse_server *) xmalloc(sizeof(struct byweekuse_server));

	us->this = cell;

	/* index struct byweekuse_server by struct server */
	if (srv_ht == NULL) srv_ht = hashtable_create(hashfun_addr, hashcmp_addr,
						      hashdup_addr, hashzap_addr);
	hashtable_add(srv_ht, (key.addr = cell, key), us);

	pp = cell->parts;

	/* walk along partitions, adding them into sorted local structs */
	while (pp) {
	    up = (struct byweekuse_partition *) xmalloc(sizeof(struct byweekuse_partition));

	    up->this = pp;
	    byweekuse_insertp(&us->parts, up);

	    /* index struct byweekuse_partition by struct partition */
	    if (part_ht == NULL) part_ht = hashtable_create(hashfun_addr, hashcmp_addr,
							    hashdup_addr, hashzap_addr);
	    hashtable_add(part_ht, (key.addr = pp, key), up);

	    /* accumulate into server stats */
	    us->npart++;
	    
	    vp = pp->vols;

	    /* walk along volumes, adding them into sorted local structs */
	    while (vp) {
		uv = (struct byweekuse_volume *) xmalloc(sizeof(struct byweekuse_volume));

		uv->this = vp;
		byweekuse_insertv(&up->vols, uv);

		/* index struct byweekuse_volume by struct volume */
		if (vol_ht == NULL) vol_ht = hashtable_create(hashfun_addr, hashcmp_addr,
							      hashdup_addr, hashzap_addr);
		hashtable_add(vol_ht, (key.addr = vp, key), uv);

		vp = vp->next;
	    }

	    /* init next volume pointer */
	    up->nextvol = up->vols;

	    /* accumulate into server stats */
	    us->weekuse += pp->weekuse;

	    /* update WEEKUSE counters */
	    npart++;
	    avg_weekuse = ((avg_weekuse * (npart - 1)) + WEEKUSE(up->this))/npart;

	    pp = pp->next;
	}

	/* roll into list after we know contents of partitions */
	byweekuse_inserts(&head, &tail, us);

	cell = cell->next;
    }
}

/* find a place to move this volume, NULL if we didn't
 * find anything
 *
 * start at "start", so we can keep polling for servers
 * in our searcher
 */
static struct byweekuse_partition *byweekuse_findest(sp, pp, v)
struct byweekuse_server *sp;
struct byweekuse_partition *pp;
struct volume *v;
{
    while (sp) {
	/* key point: do not allow moves into a server
	 * that is above the mark regardless of whether
	 * there may be a partition here that may be
	 * otherwise eligible
	 */
	if (WEEKUSE(sp)/sp->npart < UPPER_BOUND)
	  while (pp) {
	      /* if this partition can take this volume without
	       * pushing it above the upper precision limit
	       */
	      if (WEEKUSE_WI_V(pp->this, v) < MOVE_BOUND)
		/* let's do it */
		return pp;

	      pp = pp->next;
	  }

	sp = sp->prev;
    }

    return (struct byweekuse_partition *) NULL;
}

struct move_request *byweekuse_getrequest()
{
    struct byweekuse_partition *pp, *destp;
    struct byweekuse_server *sp = head;
    struct byweekuse_volume *mv = NULL;

    /* if we are currently trying to move a volume */
    if (byweekuse_mover != NULL) {
	/* starting from our current spot, attempt find a place
	 * we want to move this thing to
	 */
	sp = (struct byweekuse_server *)hashtable_find(srv_ht, (key.addr = byweekuse_mover->sto, key));
	pp = (struct byweekuse_partition *)hashtable_find(part_ht, (key.addr = byweekuse_mover->pto, key));
	destp = byweekuse_findest(sp, pp->next, byweekuse_mover->vol);

	/* if we didn't fail ... */
	if (destp != NULL) {
	    /* update our transaction */
	    byweekuse_mover->sto = destp->this->who;
	    byweekuse_mover->pto = destp->this;

	    /* and try again */
	    return byweekuse_mover;
	}

	/* oh well, we have to punt on this volume now */
	/* find the byweekuse_server and partition this volume is on 
	 * to discover our place in the search
	 */
	pp = (struct byweekuse_partition *)hashtable_find(part_ht, (key.addr = byweekuse_mover->pfrom, key));
	sp = (struct byweekuse_server *)hashtable_find(srv_ht, (key.addr = byweekuse_mover->sfrom, key));

#ifdef DEBUG
	printf("byweekuse: rejection moving %s from %s/%c\n",
	       byweekuse_mover->vol->name,
	       byweekuse_mover->sfrom->who->name,
	       byweekuse_mover->pfrom->pid + 'a');
#endif

	/* fall off into finding another volume */
    }

    /* for all servers while we don't have a volume to move */
    while (sp && !mv) {
	pp = sp->parts;

	/* another key point: regardless of the usage on a server,
	 * it may be possible that it has a partition that needs
	 * to be lowered
	 */

	/* for all partitions while we don't have a volume to move */
	while (pp && !mv) {
	    /* if we need to balance this partition */
	    if (WEEKUSE(pp->this) > MOVE_BOUND) {
		/* for unconsidered volumes on this partition ... */
		while (pp->nextvol && !mv) {
		    /* if moving this volume will make the partition more
		     * happy while not driving the partition under the lower
		     * precision limit (to try to prevent oscillation), and
		     * we can find a place to move it to.
		     *
		     * and weekuse is non-zero. other agents didn't have to worry
		     * about this.
		     */
		    if (pp->nextvol->this->weekuse != 0 &&
			WEEKUSE_WO_V(pp->this, pp->nextvol->this) > LOWER_BOUND &&
			(destp = byweekuse_findest(tail, tail->parts, pp->nextvol->this)))
		      /* got us a volume to move */
		      mv = pp->nextvol;

		    /* push along the volume list */
		    pp->nextvol = pp->nextvol->next;
		}
	    }

	    /* push along partition list */
	    pp = pp->next;
	}

	/* push along server list */
	sp = sp->next;
    }

    if (mv) {
	if (byweekuse_mover == NULL)
	  byweekuse_mover = (struct move_request *) xmalloc(sizeof(struct move_request));

	/* fill in the move structure */
	byweekuse_mover->vol = mv->this;
	byweekuse_mover->sfrom = byweekuse_mover->vol->shome;
	byweekuse_mover->pfrom = byweekuse_mover->vol->phome;
	byweekuse_mover->sto = destp->this->who;
	byweekuse_mover->pto = destp->this;

	return byweekuse_mover;
    }

    /* Give Up */
    if (byweekuse_mover) {
	free(byweekuse_mover);
	byweekuse_mover = NULL;
    }
    return (struct move_request *) NULL;
}

/* a tester function, returning boolean true if we are OK with
 * the propsed move
 */
int byweekuse_queryrequest(mover)
struct move_request *mover;
{
    /* yeah, yeah */
    if (mover == byweekuse_mover) return 1;

    if (WEEKUSE_WI_V(mover->pto, mover->vol) < UPPER_BOUND)
      return 1;

#ifdef DEBUG
    printf("REJECT: byweekuse onto a partition %f >= %f\n",WEEKUSE_WI_V(mover->pto, mover->vol),UPPER_BOUND);
#endif

    return 0;
}

/* get notified of transactions that are taking place,
 * update internal data structures
 *
 * by the time we are called, the master structure is updated
 * IN PARTICULAR the partition info is correct
 */
void byweekuse_setrequest(mover)
struct move_request *mover;
{
    struct byweekuse_partition *fromp, *top;
    struct byweekuse_server *froms, *tos;
    struct byweekuse_volume *vol;

    /* means our transaction has been accepted, so we will want to
     * get a new transaction to meddle with 
     */
    if (mover == byweekuse_mover)
      byweekuse_mover = NULL;

    /* find the byweekuse_partition, server and volume equivalents */
    fromp = (struct byweekuse_partition *)hashtable_find(part_ht, (key.addr = mover->pfrom, key));
    top = (struct byweekuse_partition *)hashtable_find(part_ht, (key.addr = mover->pto, key));
    froms = (struct byweekuse_server *)hashtable_find(srv_ht, (key.addr = mover->sfrom, key));
    tos = (struct byweekuse_server *)hashtable_find(srv_ht, (key.addr = mover->sto, key));
    vol = (struct byweekuse_volume *)hashtable_find(vol_ht, (key.addr = mover->vol, key));
    
    /* update the weekuse counters */
    froms->weekuse -= mover->vol->weekuse;
    tos->weekuse += mover->vol->weekuse;

    /* this is kinda sleezy, but is easy to grok - we're just resorting */

    /* take 'em out */
    byweekuse_dels(&head, &tail, froms);
    if (froms != tos)
      byweekuse_dels(&head, &tail, tos);
    byweekuse_delp(&froms->parts, fromp);
    byweekuse_delp(&tos->parts, top);

    /* put 'em back in */
    byweekuse_inserts(&head, &tail, froms);
    if (froms != tos)
      byweekuse_inserts(&head, &tail, tos);
    byweekuse_insertp(&froms->parts, fromp);
    byweekuse_insertp(&tos->parts, top);

    /* move the actual volume structure now */
    byweekuse_delv(&fromp->vols, vol);
    byweekuse_insertv(&top->vols, vol);

    /* if from partition is within our destination
     * candidate pool, we assume that this operation may "break 
     * something free" for us and reset our nextvol pointers
     */
    if (WEEKUSE(froms)/froms->npart < MOVE_BOUND && WEEKUSE(fromp->this) < MOVE_BOUND) {
	froms = head;
	while (froms) {
	    fromp = froms->parts;
	    while(fromp) {
		fromp->nextvol = fromp->vols;
		fromp = fromp->next;
	    }

	    froms = froms->next;
	}
    }

    /* avg_weekuse doesn't change */

    return;
}

void byweekuse_discardv(v)
struct volume *v;
{
    struct byweekuse_volume *bv;
    struct byweekuse_partition *bp;

    /* if our mover transaction is open, take this as
     * a hint to give up
     */
    if (byweekuse_mover != NULL) {
	free(byweekuse_mover);
	byweekuse_mover = NULL;
    }

    bv = (struct byweekuse_volume *) hashtable_find(vol_ht, (key.addr = v, key));
    bp = (struct byweekuse_partition *) hashtable_find(part_ht, (key.addr = v->phome, key));

    /* make sure we don't do something stupid, like keep a reference to it */
    if (bp->nextvol == bv)
      bp->nextvol = bv->next;

    byweekuse_delv(&bp->vols, bv);
}

/* dump a report of our state of the world */
void byweekuse_report()
{
    struct byweekuse_server *us;
    struct byweekuse_partition *up;
    struct byweekuse_volume *uv;

    puts(">>>> byweekuse");

    us = head;
    while (us) {
	printf("server %s, weekuse %lu\n",
	      us->this->who->name,
	      us->weekuse);

	up = us->parts;
	while (up) {
	    printf("\tpartition /vicep%c, weekuse %lu\n",
		   up->this->pid + 'a',
		   up->this->weekuse);
#ifdef DEBUG
	    uv = up->vols;
	    while (uv) {
		printf("\t\t%s with weekuse %lu using %luK of %luK quota\n",
		       uv->this->name,
		       uv->this->weekuse,
		       uv->this->size,
		       uv->this->maxquota);
		uv = uv->next;
	    }
#endif

	    up = up->next;
	}

	us = us->next;
    }
    printf("TOTAL avg_weekuse %.2f in %lu partitions\n", avg_weekuse, npart);
}
#endif
