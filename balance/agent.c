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
#include "agent.h"

#if BYSIZE
static struct agent agent_bysize = { 
    BYSIZE_NAME,
    bysize_doargs,
    (struct agent_procs *) NULL
};
#endif

#if BYNUMBER
static struct agent agent_bynumber = { 
    BYNUMBER_NAME,
    bynumber_doargs,
    (struct agent_procs *) NULL
};
#endif

#if BYWEEKUSE
static struct agent agent_byweekuse = { 
    BYWEEKUSE_NAME,
    byweekuse_doargs,
    (struct agent_procs *) NULL
};
#endif

#if BYOVERDRAFT
static struct agent agent_byoverdraft = {
    BYOVERDRAFT_NAME,
    byoverdraft_doargs,
    (struct agent_procs *) NULL
};
#endif

struct agent *all_agents[] = {
#if BYSIZE
    &agent_bysize,
#endif
#if BYNUMBER
    &agent_bynumber,
#endif
#if BYWEEKUSE
    &agent_byweekuse,
#endif
#if BYOVERDRAFT
    &agent_byoverdraft,
#endif
    (struct agent *) NULL
};
