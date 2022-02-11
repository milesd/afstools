/* ********************************************************************** *\
 *         Copyright IBM Corporation 1988,1989 - All Rights Reserved      *
 *        For full copyright information see:'andrew/config/COPYRITE'     *
\* ********************************************************************** */

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>

static char myname[] = "reauth";
extern int errno;

void usage() {
    fprintf(stderr, "usage: %s <time> principal[.instance] [password]\n", myname);
    exit(1);
}

static int logme(name, inst, password)
char *name, *inst, *password;
{
    char *reason;

#ifdef NOISY
    printf( "Attempting Re-Authentication (%s", name);
    if (*inst)
      printf(".%s", inst);
    puts(")");
#endif

    if(ka_UserAuthenticate(name, inst, /*realm*/0, password, /*sepag*/0, &reason)) {
	printf("Unable to authenticate to AFS because %s\n", reason);
	return 1;
    }

    return 0;
}

main(argc, argv)
int argc;
char *argv[];
{
    register int len, nap, pid = -1, LogStatus;
    struct timeval tv;
    char password[64];	/* Should be big enough */
    char *name, *inst, *c;

    if (argc < 3)
      usage();

    /* Get sleep time */
    nap = atoi(argv[1]);

    if (nap == 0) {
	fprintf(stderr, "%s: error: nap time is 0!\n", myname);
	usage();
    }

    /* rip apart into principal/instance pair */
    name = argv[2];
    if (c = strchr(name, '.')) {
        *c = '\0';
        inst = c+1;
    } else {
        inst = "";
    }

    if (argc == 4) {
	/* copy password from command line & clobber */
	len = strlen(argv[3]);
	if (len >= sizeof(password)) {
	    printf("Password too long: %d\n", len);
	    exit(1);
	}
	strcpy(password, argv[3]);
	memset(argv[3], '\0', len);
    } else {
	/* read from stdin */
	if (isatty(fileno(stdin))) {
	    /* if on a tty, give prompts assuming that we have a user here */
	    if (ka_UserReadPassword("Password:", password, sizeof(password), &c))
	      password[0] = '\0';
	} else {
	    /* else just suck it in */
	    fgets(password, sizeof(password), stdin);
	    len = strlen(password);
	    /* step on spurious newline */
	    if (c = strchr(password, '\n'))
	      *c = '\0';
	}
    }

    signal(SIGHUP, SIG_IGN);

    for (;;) {
	LogStatus = logme(name, inst, password);

	/* Fork & become daemon after first auth */
	if (pid == -1) {
	    pid = fork();
	    if (pid < 0) {
		perror(myname);
		fprintf(stderr, "%s: fork failed\n", myname);
		exit(1);
	    }
	    if (pid > 0) exit(0);

	    setpgrp(0, getpid());
	}

	if (LogStatus == 0) {
	    tv.tv_sec = nap;
	    tv.tv_usec = 0;
#ifdef NOISY
	    printf("Sleeping %d\n", tv.tv_sec);
#endif
	    select(0, NULL, NULL, NULL, &tv);
	} else {
	    /* only five minutes if we weren't able to authenticate */
	    tv.tv_sec = 5 * 60;
	    tv.tv_usec = 0;
#ifdef NOISY
	    printf("Sleeping %d\n", tv.tv_sec);
#endif
	    select(0, NULL, NULL, NULL, &tv);
	}
    }
}
