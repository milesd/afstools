#include <stdio.h>
#include <ctype.h>
#include <rx/rx.h>
#include <rx/xdr.h>
#include <afs/cellconfig.h>
#include <afs/prclient.h>
#include <afs/prerror.h>
#include <strings.h>
	
main(argc,argv)
long argc;
char **argv;
{
    long line, ngid;
    register long code;
    char name[PR_MAXNAMELEN];
    long id, gid;
    char buf[BUFSIZ];
    FILE *fp;
    char *ptr;
    char *aptr;
    char *tmp;
    long i;
    long verbose = 0;
    char *cellname;
    extern struct ubik_client *pruclient;
    
    if (argc < 2) {
	fprintf(stderr,"Usage: readcred [-v] [-c cellname] credfile.\n");
	exit(1);
    }
    cellname = 0;
    for (i = 1;i<argc;i++) {
	if (!strcmp(argv[i],"-v"))
	    verbose = 1;
	else {
	    if (!strcmp(argv[i],"-c")) {
		cellname = (char *)malloc(100);
		strncpy(cellname,argv[++i],100);
	    }
	    else
		strncpy(buf,argv[i],sizeof(buf));
	}
    }
    code = pr_Initialize(2,AFSCONF_CLIENTNAME, cellname); 
    if (code) {
	fprintf(stderr,"pr_Initialize failed, code %d.\n",code);
	exit(1);
    }

    if ((fp= fopen(buf,"r")) == NULL) {
	fprintf(stderr,"Couldn't open %s.\n",argv[1]);
	exit(2);
    }
    while ((tmp = fgets(buf,sizeof(buf),fp)) != NULL) {
	line++;
	bzero(name,PR_MAXNAMELEN);
	ptr = index(buf,':');
	if (ptr == NULL) {
	    fprintf(stderr, "Bad format on line %d\n", line);
	    continue;
	}
	strncpy(name,buf,ptr-buf);
	ptr += 1;
	if (!isdigit(ptr[0])) {
	    fprintf(stderr, "Bad format on line %d\n", line);
	    continue;
	}
	id = atoi(ptr);
	if (verbose)
	    printf("Adding %s with id %d.\n",name,id);
	code = pr_CreateUser(name,&id);
	if (code && code != PRIDEXIST) {
	    fprintf(stderr,"Failed to add user %s with id %d!\n",name,id);
	    fprintf(stderr,"%s (%d).\n",pr_ErrorMsg(code),code);
	}
	/* Parse the group list */
	ngid = 0;
        for (ptr = index (ptr, ':'); ptr != NULL; ptr = index (ptr, ':')) {
	    ngid++;
	    ptr += 1;
	    if (!isdigit(ptr[0])) {
		fprintf(stderr, "Invalid GID on line %d (field %d)\n", line, ngid);
		continue;
	    }
	    gid = atoi(ptr);
	    gid = -gid;
	    if (verbose) 
		printf("Adding %s to group %d\n", name, gid);
	    code = ubik_Call (PR_AddToGroup, pruclient, 0, id, gid);
	    if (code != 0 && code != PRIDEXIST) {
		fprintf(stderr,"Failed to add user %s (%d) to group with id %d!\n",name,id, gid);
		fprintf(stderr,"%s (%d).\n",pr_ErrorMsg(code),code);
	    }
	}
    }
}
