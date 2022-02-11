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
#ifndef INCLUDE_balutil_h
#define INCLUDE_balutil_h
extern void fail(int, ...);
extern struct host *get_host(char *);
int countwords(char *);
char **makeargv(int *, char *, char *);
char *strsav(char *);
int parsetime(char *);
char *rgets(char *, int, int);

/* for fail */
#define FAIL_NORM 0x00
#define FAIL_EXIT 0x01
#define FAIL_AFS  0x02
#define FAIL_DATE 0x04

struct host {
    long addr;    /* internet address */
    char *name;   /* FQDN of the machine */
};

#endif /* INCLUDE_balutil_h */
