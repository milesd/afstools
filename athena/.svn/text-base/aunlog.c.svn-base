/* $Header: /afs/athena.mit.edu/astaff/project/afsdev/sandbox/bld.beta/src/log/RCS/unlog.c,v 1.5 89/10/26 14:11:05 mb Exp $ */
/* $Source: /afs/athena.mit.edu/astaff/project/afsdev/sandbox/bld.beta/src/log/RCS/unlog.c,v $ */

#ifndef lint
static char *rcsid = "$Header: /afs/athena.mit.edu/astaff/project/afsdev/sandbox/bld.beta/src/log/RCS/unlog.c,v 1.5 89/10/26 14:11:05 mb Exp $";
#endif


/*

*
*
*  This doesn't work right now.
*   qjb - 2/18/90
*


        unlog -- Tell the Andrew Cache Manager to either clean up
		your connection completely or remove tokens for
		a single cell

        unlog [[-c[ell]] cell] ...

	No arguments means all cells.
*/

#define	VIRTUE	    1
#define	VICE	    1

#include <stdio.h>
#include <potpourri.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <afs/vice.h>
#include <afs/auth.h>
#include <sys/file.h>

#undef VIRTUE
#undef VICE


unlog_from_cell(cell)
  char *cell;
{
    register long code;
    int	cellNum;		/*Cell entry number*/
    int	rc;			/*Return value */
    struct ktc_principal serviceName, clientName; /* service name for ticket */
    struct ktc_token token;	/* the current token */
    int found;			/* Have we found one? */

    cellNum = 0;
    found = 0;
    while (1) {
	rc = ktc_ListTokens(cellNum, &cellNum, &serviceName);
        if (rc)
	    break;
	else {
	    /* get the ticket info itself */
	    rc = ktc_GetToken(&serviceName,&token, sizeof(token), &clientName);
	    if (rc) 
		continue;
	    if (strcmp(serviceName.cell, cell) == 0) {
		found = 1;
		if (code = ktc_ForgetToken(&serviceName)) {
		    fprintf(stderr,
			    "unlog: could not discard for cell %s", cell);
		    fprintf(stderr, ": code %d\n", code);
		    exit(1);
		}
		break;
	    }
	}
    }
    if (! found) {
	fprintf(stderr, "Unable to find tokens for cell %s\n", cell);
    }
    
    return(0);
}

main(argc, argv)
    int argc;
    char *argv[];
{ /* Main routine */
    register long code;
    register int i;

    /* all unlogs do the same thing now */
    if (argc <= 1) {
	if (code = ktc_ForgetAllTokens ()) {
	    printf("unlog: could not discard tickets, code %d\n", code);
	    exit(1);
	}
    }
    else {
	for (i = 1; i < argc; i++) {
	    if (argv[i][0] != '-') {
		unlog_from_cell(argv[i]);
	    }
	}
    }
    exit(0);
} /* Main routine */
