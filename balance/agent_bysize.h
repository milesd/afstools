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
#ifndef INCLUDE_agent_bysize_h
#define INCLUDE_agent_bysize_h
extern void bysize_servconf(struct server *);
extern struct agent_procs *bysize_doargs(char *);
extern struct move_request *bysize_getrequest();
extern int bysize_queryrequest(struct move_request *);
extern void bysize_setrequest(struct move_request *);
extern void bysize_discardv(struct volume *);
extern void bysize_report();

#define BYSIZE_NAME "bysize"
#endif /* INCLUDE_agent_bysize_h */
