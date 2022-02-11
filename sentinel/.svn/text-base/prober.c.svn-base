/***********************************************************
        Copyright 1991 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/
#include <sys/time.h>

#include <afs/cellconfig.h>
#include <afs/fsprobe.h>

extern struct hostent *hostutil_GetHostByName();

static struct sockaddr_in *make_sockets();
static int probe_handler();

static int num_servers;

main(argc, argv)
int argc;
char *argv[];
{
    struct sockaddr_in *sockets;
    struct timeval tv;
    int code;

    if (argc < 3) {
	fprintf(stderr, "Usage: sentinel-prober delay server1 [server2 ...]\n");
	exit(-1);
    }

    num_servers = argc - 2;
    sockets = make_sockets(num_servers, argv + 2);
    if (fsprobe_Init(num_servers, sockets, atoi(argv[1]), probe_handler, 0))
    {
	fputs("Couldn't initialize file server probe.\n", stderr);
	exit(-1);
    }

    for (;;)
    {
	tv.tv_sec  = 60*60;	/*Sleep for an hour at a time*/
	tv.tv_usec = 0;
	code = IOMGR_Select(0,	/*Num fds*/
			    0,	/*Descriptors ready for reading*/
			    0,	/*Descriptors ready for writing*/
			    0,	/*Descriptors with exceptional conditions*/
			    &tv);	/*Timeout structure*/
    } /*Sleep forever*/
}

static struct sockaddr_in *make_sockets(count, servers)
int count;
char *servers[];
{
    struct sockaddr_in *sockets;
    struct hostent *he;
    int ii;

    /* Set up the sockets */
    sockets = (struct sockaddr_in *)malloc(count * sizeof(struct sockaddr_in));
    if (!sockets)
    {
	fprintf(stderr, "prober: malloc failed\n");
	exit(-1);
    }

    bzero(sockets, count * sizeof(struct sockaddr_in));

    for (ii = 0; ii < count; ii++)
    {
	sockets[ii].sin_family = htons(AF_INET);
	sockets[ii].sin_port = htons(AFSCONF_FILEPORT);
	he = hostutil_GetHostByName(servers[ii]);
	if (!he)
	{
	    fprintf(stderr, "Invalid host name: %s\n", servers[ii]);
	    exit(-1);
	}
	bcopy(he->h_addr, &sockets[ii].sin_addr.s_addr, sizeof(he->h_addr));
    }

    return sockets;
}

static int probe_handler()
{
    int code;

    /* Just shove everything out to stdout */
    code = write(1, (char *)&fsprobe_Results.probeNum, sizeof(fsprobe_Results.probeNum));
    if (code == -1) exit(-1);

    code = write(1, (char *)&fsprobe_Results.probeTime, sizeof(fsprobe_Results.probeTime));
    if (code == -1) exit(-1);

    code = write(1, (char *)fsprobe_Results.stats, num_servers * sizeof(fsprobe_Results.stats[0]));
    if (code == -1) exit(-1);

    code = write(1, (char *)fsprobe_Results.probeOK, num_servers * sizeof(fsprobe_Results.probeOK[0]));
    if (code == -1) exit(-1);

    return 0;
}
