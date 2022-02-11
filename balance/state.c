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

#include "balance.h"
#include "balutil.h"
#include "state.h"

/* note that there is an implicit assumption right now that
 * the zero state is always the initial state - this would
 * change if RO volumes were to start moving around (for instance)
 * and require explicit setting of start states as opposed to
 * allowing the zero'ing of the move_request structure to do it
 * for us.
 *
 * state_transition would also start taking arguments to choose
 * the path in the state diagram.
 */

/* state_transition encodes the state machine for
 * volume operations.
 *
 * state 0: rw vos move, go to 1 if a backup vol existed, otherwise terminate
 * state 1: rw vos backup, go to terminate
 * state 2: terminal state, go to self
 */

/* This really is quite primitive code. Expediency wins in this case. */

static int lockid = 1; /* increasing lock number */

void state_transition(mrp)
struct move_request *mrp;
{
    switch (mrp->state) {
    case 0:
	if (mrp->vol->bkexist)
	  mrp->state = 1;
	else
	  mrp->state = 2;
	break;
    case 1:
	mrp->state = 2;
	break;
    default:
	mrp->state = 2;
	break;
    }
}


/* query if state is terminal (boolean) */

int state_terminal(mrp)
struct move_request *mrp;
{
    switch (mrp->state) {
    case 2:
	return 1;
	break;
    default:
	break;
    }

    return 0;
}


/* return a textual representation of what this state does */

char *state_text(mrp)
struct move_request *mrp;
{
    static char buf[STEXT_BUFSIZE];

    switch (mrp->state) {
    case 0:
	if (useserverauth)
	  sprintf(buf, "%s move %s %s %c %s %c -cell %s -localauth\n",
		  command,
		  mrp->vol->name,
		  mrp->sfrom->who->name,
		  mrp->pfrom->pid + 'a',
		  mrp->sto->who->name,
		  mrp->pto->pid + 'a',
		  cellname);
	else
	  sprintf(buf, "%s move %s %s %c %s %c -cell %s\n",
		  command,
		  mrp->vol->name,
		  mrp->sfrom->who->name,
		  mrp->pfrom->pid + 'a',
		  mrp->sto->who->name,
		  mrp->pto->pid + 'a',
		  cellname);
	break;
    case 1:
	if (useserverauth)
	  sprintf(buf, "%s backup %s -cell %s -localauth\n",
		  command,
		  mrp->vol->name,
		  cellname);
	else
	  sprintf(buf, "%s backup %s -cell %s\n",
		  command,
		  mrp->vol->name,
		  cellname);
	break;
    default:
	return NULL;
	break;
    }

    return buf;
}


/* dispatch a state's executor - this is a twin to state_text */

void state_dispatch(mrp)
struct move_request *mrp;
{
    char fromp[2], top[2];

    switch (mrp->state) {
    case 0:
	fromp[0] = mrp->pfrom->pid + 'a';
	top[0] = mrp->pto->pid + 'a';
	fromp[1] = top[1] = '\0';

	if (useserverauth)
	  execl(command, command, "move", mrp->vol->name,
		mrp->sfrom->who->name, fromp,
		mrp->sto->who->name, top, "-cell", cellname, "-localauth",
		(char *) NULL);
	else
	  execl(command, command, "move", mrp->vol->name,
		mrp->sfrom->who->name, fromp,
		mrp->sto->who->name, top, "-cell", cellname,
		(char *) NULL);
	break;
    case 1:
	if (useserverauth)
	  execl(command, command, "backup", mrp->vol->name, "-cell", cellname, "-localauth",
		(char *) NULL);
	else
	  execl(command, command, "backup", mrp->vol->name, "-cell", cellname,
		(char *) NULL);
	break;
    default:
	fail(FAIL_EXIT|FAIL_DATE, "attempt to dispatch a non-executor state\n");
	break;
    }
}


/* state_lock handles all of the server locking operations:
 * SLOCK_LOCK: get locks if avaliable. return 1 if we got them, 0 if not.
 * SLOCK_FREE: free locks if held. return 1 if freed, 0 if we didn't have them.
 * SLOCK_HELD: return 1 if we hold them, 0 if not.
 * SLOCK_AVAIL: return 1 if we can get them, 0 if not.
 */

int state_lock(mrp, mode)
struct move_request *mrp;
int mode;
{
    switch(mrp->state) {
    case 0:
	/* state 0 (rw vos move) needs locks on src and dest servers */
	switch(mode) {
	case SLOCK_LOCK:
	    if (state_lock(mrp, SLOCK_AVAIL)) {
		mrp->sfrom->locked = 1;
		mrp->sto->locked = 1;
		/* increment partition lockid so that the effect
		 * is that we re-figure the locks on each lock change
		 */
		lockid++;
		return 1;
	    } else
	      return 0;
	    break;
	case SLOCK_FREE:
	    if (state_lock(mrp, SLOCK_HELD)) {
		mrp->sfrom->locked = 0;
		mrp->sto->locked = 0;
		/* and diddle the lockid on the lock change */
		lockid++;
		return 1;
	    } else
	      return 0;
	    break;
	case SLOCK_HELD:
	    if (mrp->sfrom->locked && mrp->sto->locked) 
	      return 1;
	    else
	      return 0;
	    break;
	case SLOCK_AVAIL:
	    if (!mrp->sfrom->locked && !mrp->sto->locked && mrp->pto->locked < lockid) 
	      return 1;
	    else {
		/* if destintation server is locked, the source partition
		 * needs to be locked down so that we don't mistakenly
		 * put another volume onto it and break the serial nature
		 * of the transaction list.
		 */
		if (!mrp->sfrom->locked && mrp->sto->locked && mrp->pfrom->locked < lockid)
		    mrp->pfrom->locked = lockid;
		return 0;
	    }
	    break;
	default:
	    fail(FAIL_EXIT, "bogus mode in state_lock\n");
	    break;
	}
	break;
    case 1:
	/* state 0 (rw vos backup) needs locks on dest server */
	switch(mode) {
	case SLOCK_LOCK:
	    if (state_lock(mrp, SLOCK_AVAIL)) {
		mrp->sto->locked = 1;
		return 1;
	    } else
	      return 0;
	    break;
	case SLOCK_FREE:
	    if (state_lock(mrp, SLOCK_HELD)) {
		mrp->sto->locked = 0;
		return 1;
	    } else
	      return 0;
	    break;
	case SLOCK_HELD:
	    if (mrp->sto->locked == 1) 
	      return 1;
	    else
	      return 0;
	    break;
	case SLOCK_AVAIL:
	    if (mrp->sto->locked == 0) 
	      return 1;
	    else
	      return 0;
	    break;
	default:
	    fail(FAIL_EXIT, "bogus mode in state_lock\n");
	    break;
	}
	break;
    default:
	/* terminal state can't do any locking */
	break;
    }

    return 0;
}
