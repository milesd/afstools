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
/* modes for state_lock() */

#define SLOCK_LOCK    0
#define SLOCK_FREE    1
#define SLOCK_HELD    2
#define SLOCK_AVAIL   3

/* size of state_text()'s static buffer */
#define STEXT_BUFSIZE 256

void state_transition(struct move_request *);
int state_terminal(struct move_request *);
int state_lock(struct move_request *, int);
void state_dispatch(struct move_request *);
char *state_text(struct move_request *);
