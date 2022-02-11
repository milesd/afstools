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
#include <stdarg.h>

#include <string.h>
#include <memory.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "balance.h"
#include "balutil.h"

void fail(int mode, ...)
{
    va_list args;
    char *format;
    time_t t;
    static char buf[32];

    va_start(args, mode);
    fprintf(stderr, "%s: ", progname);
    if (mode & FAIL_DATE) {
	t = time(NULL);
	strftime(buf, sizeof(buf), "%c: ", localtime(&t));
	fputs(buf, stderr);
    }
    if (mode & FAIL_AFS) {
	long code;
	char *proc;

	code = va_arg(args, long);
	proc = va_arg(args, char *);

	PrintError(proc, code);
    }
    format = va_arg(args, char *);
    vfprintf(stderr, format, args);
    va_end(args);

    fflush(stderr);

    if (mode & FAIL_EXIT) exit(1);
}

struct host *get_host(host)
char *host;
{
    struct hostent *he;
    struct host *h;

    he = gethostbyname(host);
    if (!he) return (struct host *) NULL;
    if (he->h_length != sizeof(h->addr)) return (struct host *) NULL;

    h = (struct host *) xmalloc(sizeof(struct host));
    h->name = strsav(he->h_name);
    memcpy((char *)&h->addr, (char *)he->h_addr, sizeof(h->addr));

    return h;
}

int countwords(c)
char *c;
{
    int i = 0;

    while (c && *c) {
	if (isspace(*c)) {
	    c++;
	    continue;
	}

	i++;

	while (c && *c) {
	    if (!isspace(*c)) {
		c++;
		continue;
	    }

	    break;
	}
    }

    return i;
}

char **makeargv(argc, args, prog)
int *argc;
char *args;
char *prog;
{
    char **argv, **ap;

    *argc = countwords(args) + 1; /* count in fake progname we add */
    argv = (char **) xmalloc(*argc * sizeof(char *) + 1);
    *argv = prog; /* "progname" */
    ap = argv + 1; /* start adding in at normal arg positions */
    
    while (args && *args) {
	if (isspace(*args)) {
	    *args = '\0';
	    args++;
	    continue;
	}

	*ap = args;
	ap++;

	while (args && *args) {
	    if (!isspace(*args)) {
		args++;
		continue;
	    }

	    break;
	}
    }

    return argv;
}


char *strsav(s)
char *s;
{
    char *c = (char *) xmalloc(strlen(s) + 1);

    strcpy(c, s);
    return c;
}


/* parse a time specification of the form NsNmNhNd
 * (in any order) number of seconds + number of minutes +
 * number of hours + number of days, return total number of
 * of seconds
 */
int parsetime(tstr)
char *tstr;
{
    int total = 0, acc = 0;

    for (;*tstr ; tstr++) {
	if (isdigit(*tstr)) {
	    acc = acc*10 + (*tstr - '0');
	    continue;
	}

	switch (*tstr) {
	case 's':
	case 'S':
	    total += acc;
	    acc = 0;
	    break;

	case 'm':
	case 'M':
	    total += 60 * acc;
	    acc = 0;
	    break;

	case 'h':
	case 'H':
	    total += 60*60 * acc;
	    acc = 0;
	    break;

	case 'd':
	case 'D':
	    total += 24*60*60 * acc;
	    acc = 0;
	    break;

	default:
	    /* illegal character in string */
	    return -1;
	    break;
	}
    }

    /* "unspecified" digits are treated as seconds */
    if (acc) total += acc;

    return total;
}


/* rgets() snatched from niftyclaw(1), which contains
 * the following copyright notice
 */

/* niftyclaw.c -- simultaneous fingering at multiple workstations
 *
 * (c) Copyright 1989, 1990, 1991 Douglas DeCarlo
 * Permission is granted to copy, modify, and use this as long
 * as this message remains intact.  This is a nifty program.
 * The author is not responsible for any damage caused by
 * this program.
 */

/* rgets: fgets() for a file descriptor */
char *rgets(s, n, fd)
char *s;
int n, fd;
{
    int len = 0, res = 0;
    char c, *p;

    /* Read a line character by character */
    for (p = s; len < n && (res = read(fd, &c, 1)) > 0; *p++ = c, len++) {
        if (c == '\n')
          break;
    }
    /* zero pad the remainder of the buffer */
    memset(p, '\0', n - len);

    /* Return NULL if empty string */
    return (res <= 0 && !len) ? NULL: s;
}
