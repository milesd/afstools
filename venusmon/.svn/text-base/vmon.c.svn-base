/* vmon.c: display venus monitor messages
 *
 * (c) Copyright 1990 by Douglas DeCarlo
 * Permission is granted to copy, modify, and use this as long
 * as this message remains intact.
 * I looked at the console code, and it was soooooooooo gross; so I rewrote it.
 *
 * This program reads the cache manager monitoring port.  Be sure to use
 * a command such as "fs monitor localhost" to turn on monitoring or no
 * output will ever be produced.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

#define VMON_SOCKET	2106
#define UDP_PROTO	17
#define OUTFD		1

#define STORESTR	"store$"
#define FETCHSTR	"fetch$"
#define STOREOFFT	14
#define FETCHOFFT	15

static char *usage = "USAGE: venusmon [-d(ate)] [-f fetchmsg] [-s storemsg]";

#define TRUE	1
#define FALSE	0

/* getSocket:  Get and bind a socket for the venus monitor information
   return the socket, or -1 if an error occurred
   Do not use getservent() for port number because /etc/services has tcp
   */
int getSocket()
{
    struct protoent *p;
    int s, protocol, portNum = VMON_SOCKET;
    struct sockaddr_in name;

    /* Get protocol for udp socket */
    protocol = (p = getprotobyname("udp")) == NULL ? UDP_PROTO : p->p_proto;

    /* Get socket */
    if ((s = socket(AF_INET, SOCK_DGRAM, protocol)) < 0) {
        puts("venusmon: socket failed\n");
        return -1;
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(portNum);

    /* Bind socket */
    if (bind(s, &name, sizeof(struct sockaddr_in)) < 0) {
	puts("venusmon: bind failed\n");
        return -1;
    }

    return s;
}

/* write_date: Write the current date (hh:mm:ss A/PM) to
   the file desc fd
   */
void write_date(fd)
int fd;
{
    struct tm *tmp;
    char buf[32];
    int hrs, am = TRUE;
    long tim;

    tim = time(0);
    tmp = localtime(&tim);
    hrs = tmp->tm_hour;

    if (!(am = (hrs < 12))) {
	hrs -= 12;
    }
    if (!hrs)
      hrs = 12;
 
    sprintf(buf, " (%d:%02d:%02d %s)\n", hrs, tmp->tm_min, tmp->tm_sec, am ? "AM":"PM");

    (void)write(fd, buf, strlen(buf));
}

main(argc, argv)
int argc;
char *argv[];
{
    int s, n, date = FALSE;
    char buffer[BUFSIZ], *fetchMsg = "F: ", *storeMsg = "S: ";

    /* Parse command line switches */
    for (; --argc > 0; ) {
	if (**++argv == '-') {
	    switch ((*argv)[1]) {
	      case 'f':
		if (--argc > 0) {
		    fetchMsg = *++argv;
		} else {
		    puts(usage);
		    exit(-1);
		}
		break;
	      case 's':
		if (--argc > 0) {
		    storeMsg = *++argv;
		} else {
		    puts(usage);
		    exit(-1);
		}
		break;
	      case 'd':
		date = TRUE;
		break;
	      default:
		puts(usage);
		exit(-1);
	    }
	} else {
	    puts(usage);
	    exit(-1);
	}
    }

    /* Connect to venus mon and get packets as they come in */
    if ((s = getSocket()) >= 0) {
	for (;;) {
	    /* Read info from venus socket */
	    if (n = read(s, buffer, BUFSIZ)) {
		if (!strncmp(buffer, FETCHSTR, sizeof(FETCHSTR)-1)) {
		    write(OUTFD, fetchMsg, strlen(fetchMsg));
		    (void)write(OUTFD, buffer + FETCHOFFT, n - FETCHOFFT - 1);
		} else if (!strncmp(buffer, STORESTR, sizeof(STORESTR)-1)) {
		    write(OUTFD, storeMsg, strlen(storeMsg));
		    (void)write(OUTFD, buffer + STOREOFFT, n - STOREOFFT - 1);
		} else {
		    (void)write(OUTFD, buffer, n - 1);
		}
	    }
 
	    /* Output date if desired */
	    if (date) {
		write_date(OUTFD);
	    } else {
		*buffer = '\n';
		(void)write(OUTFD, buffer, 1);
	    }
	}
    }
}
