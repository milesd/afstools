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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "balance.h"
#include "config.h"
#include "balutil.h"
#include "afscall.h"
#include "hash.h"
#include "agent.h"
#include "mygetopt.h"
#include "state.h"

#include <afs/volint.h>
#include <afs/voldefs.h>
#include <afs/volser.h>

static struct server *cell = NULL;                   /* list of servers */
char *cellname = NULL;                               /* text cell name */
struct glob *size_is_quota = NULL;
#if BYOVERDRAFT
struct glob *overdraft_use_quota = NULL;
#endif

static struct move_request *move_head = NULL,        /* list of transactions, in order */ 
                           **move_tail = &move_head; /* & to stuff next transaction into */
int ntransactions = 300;                             /* number of transactions to do */
int nvoltrans = 1;                                   /* number of transactions to allow on
						      * a single volume 
						      */
int nprocs = 4;                                      /* number of simultaneous move
						      * transactions we're allowed to have
						      */
int nsecs = (60*60)*4;                               /* allowed runtime in secs */
static time_t startime;                              /* start time of program */
static int echo_mode = 0;                            /* echo mode - just print what
						      * we would have done
						      */
static int doreports = 0;                            /* flag for printing reports in log */

int switchflags = 0;                                 /* flags to tell us which switches were
						      * set on the command line
						      */
int useserverauth = 0;                               /* use local auth on server (from KeyFile) */
char *command = VOS;                                 /* command to use (can be overidden) */

char *progname;
char *voldumpdir=NULL;

int aborted = 0;                                     /* ext. for signalling */

#ifndef FNDELAY
#define FNDELAY O_NDELAY
#endif

void usage() {
  fprintf(stderr, "To execute volume transactions immediately:\n");
  
  fprintf(stderr, "usage: %s [-c /path/to/vos] [-s] [-e] [-r] -f config [-l limit] [-t ntrans]\n", progname);
  fprintf(stderr, "      [-v nvoltrans] [-p nprocs] [-D /path/to/volume/dumps]\n", progname);
  fprintf(stderr, "To store volume transactions in a database\n");
  fprintf(stderr, "usage: %s [-e] [-r] -f config [-v nvoltrans] -o dbpath [-D /path/to/volume/dumps]\n", progname);
  fprintf(stderr, "To execute volume transactions from a database\n");
  fprintf(stderr, "usage: %s [-c /path/to/vos] [-s] [-e] [-r] -f config [-l limit] [-t ntrans]\n", progname);
  fprintf(stderr, "      [-p nprocs] -i dbpath [-D /path/to/volume/dumps]\n", progname);
  
  fprintf(stderr, "\tntrans defaults to %d\n\tnvoltrans defaults to %d\n\trun limit defaults to %d secs\n\tnprocs defaults to %d\n",
          ntransactions,
          nvoltrans,
          nsecs,
          nprocs);
  exit(1);
}

void initialize_cell(sp)
struct server *sp;
{
    struct partition *pp;
    struct volume *vp;
    struct diskPartition *partinf;
    struct volEntries *vols;
    struct hashtable *ro_ht, *bk_ht;
    union hashtable_key key;
    int fatal = 0, i;
    char *c;

    while (sp) {
	pp = sp->parts;

	while (pp) {
	    partinf = get_partition_info(sp->who, pp->pid);
	    if (partinf == NULL) {
		fatal = 1;
		continue;
	    }

	    pp->free = partinf->free;
	    pp->size = partinf->minFree;

#ifdef DEBUG
	    printf("initializing %s of %s\n\t%dK free\n\t%dKtotal\n\n",
		   partinf->name,
		   sp->who->name,
		   pp->free,
		   pp->size);
#endif

	    vols = get_partition_volumes(sp->who, pp->pid);
	    if (vols == NULL) {
		fatal = 1;
		continue;
	    }

	    pp->nvols = vols->volEntries_len;

	    ro_ht = hashtable_create(hashfun_string, hashcmp_string,
				     hashdup_string, hashzap_string);
	    bk_ht = hashtable_create(hashfun_string, hashcmp_string,
				     hashdup_string, hashzap_string);

	    /* make a first pass looking for RO replicated volumes
	     * so we leave their (possible) RW counterparts alone
	     * and backup volumes so we know that we should recreate
	     * them at the destination
	     */
	    for (i = 0; i < pp->nvols; i++) {
		if (vols->volEntries_val[i].type != RWVOL) {
		    /* look for an extension */
		    c = strrchr(vols->volEntries_val[i].name, '.');

		    /* if no extension, we'll just ignore this volume */
		    if (!c) continue;
		    
		    /* if we have a replicated volume, we will not
		     * want to move around its RW on the same partition
		     * so store the name of the readwrite volume for
		     * lookup
		     *
		     * type = RO is not good enough - in backup systems
		     * volumes are often restored RO, but without the
		     * .readonly extension
		     */
		    if (!strcmp(c, ".readonly")) {
			*c = '\0';
			hashtable_add(ro_ht, (key.s = vols->volEntries_val[i].name, key), (void *)1);
#ifdef DEBUG
			printf("hashed up %s for RO elimination\n", vols->volEntries_val[i].name);
#endif
		    }
		    if (vols->volEntries_val[i].type == BACKVOL) {
			*c = '\0';
			hashtable_add(bk_ht, (key.s = vols->volEntries_val[i].name, key), (void *)1);
#ifdef DEBUG
			printf("hashed up %s for BK preservation\n", vols->volEntries_val[i].name);
#endif
		    }

		    *c = '.'; /* restore it */
		}
	    }

	    for (i = 0; i < pp->nvols; i++) {
		/* even if we ignore this volume, statistics must be kept for it */
#if BYWEEKUSE
		/* enforce a sanity check on weekuse */
		if (vols->volEntries_val[i].spare1 > MAXWEEKUSE) {
		    fail(FAIL_NORM|FAIL_DATE, "volume %lu had weekuse of %lu - zeroed\n", 
			 vols->volEntries_val[i].volid,
			 vols->volEntries_val[i].spare1);
		    vols->volEntries_val[i].spare1 = 0;
		}
		pp->weekuse += vols->volEntries_val[i].spare1;
#endif

		if (vols->volEntries_val[i].volid != 
		    vols->volEntries_val[i].backupID &&
                    size_is_quota &&
                    matchlist(size_is_quota, vols->volEntries_val[i].name))
                  {
#if DEBUG
                    printf("Adjusting volume %s, actual usage is %d/%d\n",
                           vols->volEntries_val[i].name,
                           vols->volEntries_val[i].size,
                           vols->volEntries_val[i].maxquota);
#endif
                    /* check for 0 quota or overfull volumes */
                    vols->volEntries_val[i].maxquota =
                      max(vols->volEntries_val[i].maxquota,
                          vols->volEntries_val[i].size);
                    /* Pretend tha this volume is completely full */
                    pp->free -= vols->volEntries_val[i].maxquota -
                      vols->volEntries_val[i].size;
                    vols->volEntries_val[i].size =
                      vols->volEntries_val[i].maxquota;
                  }
                
#if BYOVERDRAFT
		if (vols->volEntries_val[i].volid != 
		    vols->volEntries_val[i].backupID
                    /* && vols->volEntries_val[i].cloneID != 0 */ )
                  {
                    if (overdraft_use_quota &&
                        matchlist(overdraft_use_quota,
                                  vols->volEntries_val[i].name))
                      pp->maxquota += vols->volEntries_val[i].maxquota;
                    else
                      pp->maxquota += vols->volEntries_val[i].size;
                  }
#endif
                      
                      
                        

		/* if volume is not online OR volume status is non-BUSY (VOK)
		 * OR non-readwrite, OR not on the positive outbound list 
		 * (null list matches!) OR the volume is on the minus outbound list
		 * and the minus list exists, OR we find it on the exclude
		 * hashtable, we aren't interested in this volume
		 *
		 * this optimizes us somewhat, so agents don't waste time considering
		 * volumes that we don't want to move anyway
		 */
		if (vols->volEntries_val[i].inUse == 0 ||
		    vols->volEntries_val[i].status != VOK ||
		    vols->volEntries_val[i].type != RWVOL ||
		    !matchlist(pp->outplus, vols->volEntries_val[i].name) ||
		    (pp->outminus != NULL && 
		     matchlist(pp->outminus, vols->volEntries_val[i].name)) ||
		    hashtable_find(ro_ht, (key.s = vols->volEntries_val[i].name, key))) {
#ifdef DEBUG
		    printf("ignoring %s\n", vols->volEntries_val[i].name);
#endif
		    continue;
	        }

		vp = (struct volume *) xmalloc(sizeof(struct volume));
		vp->next = pp->vols;
		pp->vols = vp;

		vp->name = strsav(vols->volEntries_val[i].name);
		vp->volid = vols->volEntries_val[i].volid;
		vp->size = vols->volEntries_val[i].size;
		vp->maxquota = vols->volEntries_val[i].maxquota;
#if BYWEEKUSE
		vp->weekuse = vols->volEntries_val[i].spare1;
#endif
		vp->shome = sp;
		vp->phome = pp;
		vp->bkexist = (hashtable_find(bk_ht, (key.s = vp->name, key)) == (void *) 1 ?
			       1 : 0);

#ifdef DEBUG
		printf("volume %s using %dK of %dK\n", vols->volEntries_val[i].name,
		       vp->size,
		       vp->maxquota);
#endif
	    }

	    hashtable_clean(bk_ht);
	    hashtable_clean(ro_ht);
	    pp = pp->next;
	}

	sp = sp->next;
    }

    if (fatal)
      fail(FAIL_EXIT, "errors encountered, exiting\n");
}
	      
/* attempt to intialize an agent with args
 * return boolean true if we fail (agent could have
 * rejected args or we may not have found the named agent)
 */
int initialize_agent(agent, args)
char *agent;
char *args;
{
    struct agent **agp = all_agents;

    while (*agp) {
	if (!strcmp((*agp)->id, agent)) {
	    (*agp)->procs = (*(*agp)->agent_doargs)(args);
	    return (*agp)->procs == NULL;   /* return true if agent didn't init */
	}
	agp++;
    }
    
    return 1;  /* didn't intialize anything */
}

/* give all of the agents the server configurations */
void initialize_agent_servconf(cell)
struct server *cell;
{
    struct agent **agp = all_agents;

    for (agp = all_agents; *agp; agp++) {
	if (! (*agp)->procs) continue;
	(*(*agp)->procs->agent_servconf)(cell);
    }
}

#define UNTHREAD(pp, listpp, elt, next) { \
				      (pp) = (listpp); \
				      while (*(pp)) { \
					  if (*(pp) == (elt)) \
					    break; \
					  (pp) = &(*pp)->next; \
				      } \
				      *(pp) = (*(pp))->next; \
                                  }

/* update global datastructures to reflect this transaction.
 * return boolean true if this transaction struct can be
 * discarded.
 *
 * note that this has minor hooks for transaction compression
 * i.e. returning 1 to indicate it was compressed. Compression
 * is a much harder problem than I first thought.
 */
int domove(mover)
struct move_request *mover;
{
    struct volume **v;
    int retval = 0;

    /* explicitly do not attempt to compress transactions so
     * that a -> b, b -> c, results in a -> c. The agents
     * present transactions serially, so we must process them
     * serially in order to guarantee success
     */

    /* thread onto toserver to transaction list */
    mover->tsnext = mover->sto->to;
    mover->sto->to = mover;
    
    /* thread onto topartition to transaction list */
    mover->tpnext = mover->pto->to;
    mover->pto->to = mover;

    /* thread onto fromserver from transaction list */
    mover->fsnext = mover->sfrom->from;
    mover->sfrom->from = mover;

    /* thread onto frompartition from transaction list */
    mover->fpnext = mover->pfrom->from;
    mover->pfrom->from = mover;

    /* update partitions */
    mover->pfrom->free += mover->vol->size;
    mover->pto->free -= mover->vol->size;
#if BYWEEKUSE
    mover->pfrom->weekuse -= mover->vol->weekuse;
    mover->pto->weekuse += mover->vol->weekuse;
#endif
#if BYOVERDRAFT
    if (overdraft_use_quota &&
        matchlist(overdraft_use_quota,
                  mover->vol->name)) {
     mover->pfrom->maxquota -= mover->vol->maxquota;
     mover->pto->maxquota += mover->vol->maxquota;
    } else {
      mover->pfrom->maxquota -= mover->vol->size;
      mover->pto->maxquota += mover->vol->size;
    }
#endif
    
    
    /* move the volume structure around */
    /* disregard complexity of backup volumes in volume counts */

    /* unthread from frompartition */
    UNTHREAD(v, &mover->pfrom->vols, mover->vol, next);
    mover->pfrom->nvols--;

    /* thread onto topartition */
    mover->vol->next = mover->pto->vols;
    mover->pto->vols = mover->vol;
    mover->pto->nvols++;

    /* update volume's idea of it's location */
    mover->vol->shome = mover->sto;
    mover->vol->phome = mover->pto;

    /* thread transaction onto list */
    *move_tail = mover;
    move_tail = &mover->next;

    /* done ! */
    return retval;
}

/* do the balance loop */
void balance()
{
    int activity, doit, zapit;
    struct agent **iquery, **iget;
    struct move_request *mover;

    if (doreports) {
	/* issue reports */
	puts("----- BEFORE BALANCE REPORTS -----");
	for (iquery = all_agents; *iquery; iquery++) {
	    if (! (*iquery)->procs) continue;
	    (*(*iquery)->procs->agent_report)();
	}
	puts("----------------------------------");
	fflush(stdout);
    }

    /* do loop while we have had activity */
    do {
	activity = 0;

	/* loop across all agents */
	for (iget = all_agents; *iget; iget++) {
	    if (! (*iget)->procs) continue;

	    doit = 0;

	    /* keep bothering an agent until it either gives up (returns
	     * a NULL mover) or gives us something we can do something with
	     */
	    do {
		/* get move request from agent */
		mover = (*(*iget)->procs->agent_getrequest)();

		/* if agent wants to do anything */
		if (mover) {

		    /* check that volume matches the inbound patterns for this
		     * partition
		     */

		    if (!matchlist(mover->pto->inplus, mover->vol->name) ||
			(mover->pto->inminus != NULL && 
			 matchlist(mover->pto->inminus, mover->vol->name))) {
			/* reject this - partition doesn't want the volume */
#ifdef DEBUG
			printf("REJECT: partition /vicep%c of %s\n",
			       mover->pto->pid + 'a',
			       mover->sto->who->name);
#endif
			continue;
		    }

		    /* ask if move is ok with the agents */
		    for (iquery = all_agents; *iquery; iquery++) {
			if (! (*iquery)->procs) continue;
			if (!(doit = (*(*iquery)->procs->agent_queryrequest)(mover)))
			  break;
		    }

		    /* perform some sanity checks - this is where we can insert
		     * pre-emptive strikes (NOT! :)
		     */
		    if (doit) {
			/* if we're moving, check that this volume hasn't moved too many times
			 * and make sure the volume isn't locked ("or this will be the shortest
			 * attack of all time")
			 */
			if (mover->vol->ntimes == nvoltrans || afs_volislocked(mover->vol)) {
			    /* bzzt. not happening */
#ifdef DEBUG
			    printf("REJECT: volume %s cannot be moved\n",
				   mover->vol->name,
				   mover->vol->ntimes);
#endif
			    /* make everyone throw this volume away */
			    for (iquery = all_agents; *iquery; iquery++) {
				if (! (*iquery)->procs) continue;
				(*(*iquery)->procs->agent_discardv)(mover->vol);
			    }

			    /* reset doit counter */
			    doit = 0;
			    /* go back for another try */
			    continue;
			}

			mover->vol->ntimes++;
		    }
		}
	    } while (mover && !doit);

	    /* if everyone agrees */
	    if (doit) {
		activity++; /* log that something happened this round */
		
#ifdef DEBUG
		printf("agent %10s: %-23s %s/%c -> %s/%c (%dK)\n",
		       (*iget)->id,
		       mover->vol->name,
		       mover->sfrom->who->name,
		       mover->pfrom->pid + 'a',
		       mover->sto->who->name,
		       mover->pto->pid + 'a',
		       mover->vol->size);
#endif

		/* panic out if an agent tells us to do nothing to a volume */
		if (mover->sfrom == mover->sto && mover->pfrom == mover->pto)
		  fail(FAIL_EXIT, "panic: %s returned a null transaction\n", (*iget)->id);

		/* update global structures */
		zapit = domove(mover);

		/* and notify all agents */
		for (iquery = all_agents; *iquery; iquery++) {
		    if (! (*iquery)->procs) continue;
		    (*(*iquery)->procs->agent_setrequest)(mover);
		}

		/* see if this volume has moved it's limit */
		if (mover->vol->ntimes == nvoltrans) {
		    /* go tell everyone to stop thinking about it */
		    for (iquery = all_agents; *iquery; iquery++) {
			if (! (*iquery)->procs) continue;
			(*(*iquery)->procs->agent_discardv)(mover->vol);
		    }
		}		    

		/* domove said kill it? */
		if (zapit)
		  free(mover);

		/* bust out if we hit the transaction limit */
		if (! --ntransactions) goto done;
	    }
	}
    } while (activity);

 done:
    if (doreports) {
	/* issue reports */
	puts("------ AFTER BALANCE REPORTS -----");
	for (iquery = all_agents; *iquery; iquery++) {
	    if (! (*iquery)->procs) continue;
	    (*(*iquery)->procs->agent_report)();
	}
	puts("----------------------------------");
	fflush(stdout);
    }
}


/* true if the current time is over the requested run period */
int overtime()
{
    if (nsecs && time(NULL) > startime + nsecs) return 1;

    return 0;
}


/* handle the mechanics of carrying out the transactions we've
 * generated.
 *
 * the current implementation of the "state engine" is extremely
 * simple and would not be suitable for anything more complex
 * than it is currently doing - rw volumes. see state.c.
 */

int do_transactions(mrpp)
struct move_request **mrpp;
{
    int pid, status, shutdown = 0, abend = 0, pfd[2], i, flag = 0;
    char buf[256], *c;
    struct hashtable *trans_ht;
    struct move_request **tpp, *trans, *trans_fd[FD_SETSIZE];
    union hashtable_key key;
    fd_set rfds;
    struct timeval tv;

    /* create pid -> transaction mapping */
    trans_ht = hashtable_create(hashfun_addr, hashcmp_addr,
				hashdup_addr, hashzap_addr);

    /* NULL out the array of fd -> transaction mappings */
    memset((char *)trans_fd, '\0', sizeof(trans_fd));

    while (1) {
	/* if we are over our time limit, initiate shutdown */
	if (overtime() && !echo_mode) shutdown = 1;

	tpp = mrpp;

	/* go and dispatch all of the transactions we possibly can */
	while (*tpp && nprocs && !shutdown && !aborted) {
	    /* is this transaction in a terminal state? */
	    if (state_terminal(*tpp)) {
		/* snip the transaction from the list */
		trans = *tpp;
		*tpp = (*tpp)->next;
		free(trans);
		continue;
	    }

	    /* if the servers are open */
	    if (state_lock(*tpp, SLOCK_AVAIL)) {
		/* allow as to what we're about to try to do */
		if (c = state_text(*tpp))
		  fail(FAIL_DATE, c);

		/* if we're in echo_mode, transition and roll around.
		 */
		if (echo_mode) {
		    state_transition(*tpp);
		    continue;
		}

		if (pipe(pfd)) {
		    /* blow out if pipe fails */
		    perror("pipe");
		    abend = shutdown = 1;
		    break;
		}

		pid = fork();
		if (pid == 0) {
                    /* Protect the children from our user trying to stop us */
                    signal(SIGINT, SIG_IGN);
                  
		    /* snatch stdout & stderr so we can monitor output */
		    dup2(pfd[1], fileno(stdout));
		    dup2(pfd[1], fileno(stderr));

		    /* dispatch the state function */
		    state_dispatch(*tpp);

		    /* bummer - can't exec ... */
		    perror("execl");
		    exit(1);
		}

		/* shutdown our handle on the other side of the pipe */
		close(pfd[1]);

		if (pid < 0) {
		    /* some fork error. bust out. this will usually
		     * be recoverable after waiting for a child to
		     * finish. If not, we fail here until the pid
		     * test automatically breaks us out.
		     */
		    perror("fork");
		    break;
		}

		nprocs--;         /* decrease our allowance */

		/* store, keyed by pid, the transaction */
		hashtable_add(trans_ht, (key.addr = (void *)pid, key), *tpp);

		/* set up for reading */
		if (fcntl(pfd[0], F_SETFL, FNDELAY) < 0) {
		    /* something ugly is going on - leave */
		    perror("fcntl");
		    abend = shutdown = 1;
		    break;
		}
		trans_fd[pfd[0]] = *tpp;
		(*tpp)->fd = pfd[0];             /* note in transaction so we can close it */

		/* lock the servers appropriate to the state */
		if (!state_lock(*tpp, SLOCK_LOCK)) {
		    fail(FAIL_EXIT|FAIL_DATE, "lock failed\n");
		}

		/* be careful not to skip over next transaction! */
		continue;
	    }

	    /* push along list of transactions */
	    tpp = &(*tpp)->next;
	}

	/* loop around for process output and termination ... */
	pid = 0;
	while (1) {
	    /* use pid as a flag to stop this IO pass - if waitpid() didn't see anything */
	    if (pid)
	      break;
	 
	    pid = waitpid(-1, &status, WNOHANG);

	    /* half second timeouts */
	    tv.tv_sec = 0;
	    tv.tv_usec = 500000;

	    /* set up fd_sets */
	    FD_ZERO(&rfds);
	    for (i = 0; i< FD_SETSIZE; i++)
	      if (trans_fd[i] != NULL)
		FD_SET(i, &rfds);

	    /* hang out for command output */
	    i = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);

	    /* find out who peeped (if anyone) */
	    if (i > 0)
	      for (i = 0; i < FD_SETSIZE; i++) {
		  /* if this is a descriptor we know about and select flagged it */
		  if (trans_fd[i] != NULL && FD_ISSET(i, &rfds)) {
		      /* pull stuff in */
		      while (rgets(buf, sizeof(buf), i) != NULL) {
			  /* step on newline if it exists */
			  if ((c = strchr(buf, '\n')) != NULL)
			    *c = '\0';

			  /* and tag the output appropriately */
			  fail(FAIL_DATE, "%s: %s\n", trans_fd[i]->vol->name, buf);
		      }
		  }
	      }

	    if (i < 0) {
		/* prepare to punt if select bombs */
                if (aborted > shutdown) /* Assume select punted due to ^C */
                  fprintf(stderr, "Shutting down.  Waiting for children.\n");
                else
		  perror("select");
                abend |= !(aborted-shutdown);
                shutdown = aborted;
	    }

	}

	/* if we have no children, we weren't able to dispatch anything OR
	 * everything is done. In either case, time to give up.
	 */
	if (pid < 0) break;

	/* find the transaction in the transaction hashtable */
	trans = (struct move_request *) hashtable_del(trans_ht, (key.addr = (void *)pid, key));

	/* see what actually happened ... */
	if (WIFEXITED(status)) {
	    /* initiate shutdown if a vos croaked */
	    if (WEXITSTATUS(status) != 0) {
		fail(FAIL_DATE, "FAIL: move of %s failed (exit status = %d)\n", trans->vol->name,
		     WEXITSTATUS(status));
		abend = shutdown = 1;
	    }
	} else {
	    /* WIFSIGNALED is true */
	    fail(FAIL_DATE, "FAIL: move of %s was signaled to death (signal %d)\n",
		 trans->vol->name, WTERMSIG(status));
	    abend = shutdown = 1;
	}

	if (!state_lock(trans, SLOCK_FREE)) {
	    fail(FAIL_EXIT|FAIL_DATE, "free lock failed\n");
	}	    
	state_transition(trans);

	/* something finished, bump allowed transactions  */
	nprocs++;

	/* discard FD references */
	close(trans->fd);
	trans_fd[trans->fd] = NULL;
    }

    if (abend) {
	fail(FAIL_DATE, "FAIL: run terminated by SHUTDOWN triggered by abnormal exit condition\n");
	fail(FAIL_DATE, "FAIL: cleanup may be neccesary - see log for failed transactions\n");
    }

    if (overtime() && !echo_mode)
      fail(FAIL_DATE, "TIME: run shutdown initiated by going over time limit\n");

    if (*mrpp && !echo_mode) {
	fail(FAIL_DATE, "FAIL: the following transactions were NOT processed\n");

	tpp = mrpp;
	while (*tpp) {
	    do {
		/* run through the state transitions and dump text */
		if (c = state_text(*tpp))
		  fail(FAIL_NORM, "UNDONE: %s", c);
		state_transition(*tpp);
	    } while (c != NULL);

	    tpp = &(*tpp)->next;
	}
    }

    hashtable_clean(trans_ht);

    if ((abend || overtime() || *mrpp) && !echo_mode)
      return 1;

    return 0;
}


void bal_sigint(foo)
int foo;
{
  aborted++;
}

void main(argc, argv)
int argc;
char **argv;
{
    char *confile = NULL;
    int c, error = 0;
    extern char *optarg;

    progname = argv[0];
    startime = time(NULL);

    while ((c = mygetopt(argc, argv, "f:t:v:p:l:ersc:D:")) != -1) {
	switch (c) {
	case 'f':
	    confile = optarg;
	    break;
	case 't':
	    ntransactions = atoi(optarg);
	    switchflags |= SWITCH_NTRANSACTIONS;
	    break;
	case 'v':
	    nvoltrans = atoi(optarg);
	    switchflags |= SWITCH_NVOLTRANS;
	    break;
	case 'p':
	    nprocs = atoi(optarg);
	    switchflags |= SWITCH_NPROCS;
	    break;
	case 'l':
	    nsecs = parsetime(optarg);
	    if (nsecs < 0) {
		fail(FAIL_NORM, "time '%s' unparseable\n", optarg);
		usage();
	    }
	    switchflags |= SWITCH_NSECS;
	    break;;
	case 'e':
	    echo_mode = 1;
	    break;
	case 'r':
	    doreports = 1;
	    break;
	case 's':
	    useserverauth = 1;
	    break;
	case 'c':
	    command = optarg;
	    break;
        case 'D':
            voldumpdir = optarg;
            break;
        default:
	    error++;
	}
    }
    if (error || confile == NULL) usage();

    /* process config, setting up skeleton server configuration
     * and doing first pass intialization of the agents
     */
    get_config(confile, &cell);

    /* fill in the server configurations */
    initialize_cell(cell);

    /* initialize the agents' views of the world */
    initialize_agent_servconf(cell);

    /* run the balance loop */
    balance();

    /* roll out the transactions */
    fprintf(stderr, "About to execute the transactions.\nType ^C to shutdown gracefully.\n");
    signal(SIGINT, bal_sigint);
    exit(do_transactions(&move_head));
}
