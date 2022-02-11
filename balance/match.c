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
/* match.c -- globbing for niftyclean
 *
 * (C) Copyright 1989, by Jay Laefer and Mike Darweesh
 * Permission is granted to copy, modify, and use this as long
 * as this message remains intact.  This is a nifty program.
 * The authors are not responsible for any damage caused by
 * this program.
 */

#include <stdio.h>
#include "match.h"

static int umatch(char *, char *);

/* match() takes a string and a pattern.  It does some funky
   globbing that I don't fully understand and returns 0 if the
   match fails.
   */
int match(as, ap)
char *as, *ap;
{
    register char *s, *p;
    register scc;
    int c, cc, ok, lc;
    
    s = as;
    p = ap;
    if (scc = *s++)
      if ((scc &= 0177) == 0)
	scc = 0200;
    switch (c = *p++) {
	
    case '[':
	ok = 0;
	lc = 077777;
	while (cc = *p++) {
	    if (cc == ']') {
		if (ok)
		  return(match(s, p));
		else
		  return(0);
	    } else if (cc == '-') {
		if (lc <= scc && scc <= (c = *p++))
		  ok++;
	    } else
	      if (scc == (lc = cc))
		ok++;
	}
	return(0);
	
    default:
	if (c != scc)
	  return(0);
	
    case '?':
	if (scc)
	  return(match(s, p));
	return(0);
	
    case '*':
	return(umatch(--s, p));
	
    case '\0':
	return(!scc);
    }
}

/* umatch() takes a string and a pattern.  If the pattern
   is empty, return true (1).  Otherwise, call amatch()
   until the string is empty or amatch() succeeds.
   */
static int umatch(s, p)
char *s, *p;
{
    if(*p == 0)
      return(1);
    while(*s)
      if (match(s++,p))
	return(1);
    return(0);
}

/* match a string against a list of globs. short circuits
 * on a match, returning boolean true, else false.
 */
int matchlist(gl, s)
struct glob *gl;
char *s;
{
    if (gl == NULL) return 1; /* the empty list matches */

    while(gl) {
	if (match(s, gl->globv)) return 1;
	gl = gl->next;
    }

    return 0;
}
