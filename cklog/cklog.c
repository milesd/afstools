/*
 * cklog: a program for getting cross-realm tickets
 *
 */
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

#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <afs/auth.h>
#include <afs/cellconfig.h>
#include <afs/ptclient.h>
#include <afs/pterror.h>

#include <des.h>
#include <krb.h>

extern int errno;

main(argc, argv)
    int argc;
    char **argv;

{
    char pname[ANAME_SZ];
    char pinst[INST_SZ];
    char prealm[REALM_SZ];
    char namebuf[ANAME_SZ+1+INST_SZ+1+REALM_SZ+1];
    struct ktc_token token;
    struct ktc_principal afs, me;
    int rc, id;
    extern char  *krb_err_txt[];
    struct namelist nl;
    struct idlist idl;

    KTEXT_ST auth;
    CREDENTIALS c;

    if (argc != 2) {
	fprintf(stderr, "Usage: %s <realm>\n", argv[0]);
	exit(1);
    }

    rc = krb_get_tf_fullname(TKT_FILE, pname, pinst, prealm);
    if (rc != 0) {
	fprintf(stderr, "%s: unable to find ticket file: %s\n", argv[0], krb_err_txt[rc]);
	exit(1);
    }

    foldup(argv[1], argv[1]);
    rc = krb_mk_req(&auth, "afs", "", argv[1]);
    if (rc != 0) {
	fprintf(stderr, "%s: unable to request ticket for afs@%s service: %s\n",
 		argv[0], argv[1],krb_err_txt[rc]);
	exit(1);
    }

    rc = krb_get_cred("afs", "", argv[1], &c);
    if (rc != 0) {
	fprintf(stderr,"%s: krb_get_cred: %s\n", argv[0], krb_err_txt[rc]);
	exit(1);
    }

    (void)strcpy(afs.name, c.service);
    (void)strcpy(afs.instance, c.instance);
    folddown(afs.cell, c.realm);
    (void)strcpy(me.name, c.pname);
    (void)strcpy(me.instance, c.pinst);
    folddown(me.cell, prealm);

    token.ticketLen = c.ticket_st.length;
    bcopy((char *)&c.ticket_st.dat[0], &token.ticket[0], token.ticketLen);
    bcopy((char *)&c.session[0], (char *)&token.sessionKey, sizeof(token.sessionKey));
    token.kvno = c.kvno;
    token.startTime = c.issue_date;
    token.endTime = krb_life_to_time(c.issue_date,c.lifetime);
    rc = ktc_SetToken(&afs, &token, &me);
    if (rc) {
	fprintf(stderr,"%s: could not store token for afs@%s: %s\n",
		argv[0], argv[1], krb_err_txt[rc]);
	exit(1);
    }

    folddown(argv[1], argv[1]);
    rc = pr_Initialize(1L, AFSCONF_CLIENTNAME, argv[1]);
    if (rc) {
	fprintf(stderr,"%s: could not intialize protection package\n",argv[0]);
	fprintf(stderr,"%s: remote cell may be down\n",argv[1]);
	exit(1);
    }

    nl.namelist_len = 1;
    strcpy(namebuf, pname);
    if (*pinst) {
	strcat(namebuf, ".");
	strcat(namebuf, pinst);
    }
    strcat(namebuf, "@");
    strcat(namebuf, me.cell);
    nl.namelist_val = (prname *)namebuf;
    idl.idlist_len = 0;
    idl.idlist_val = NULL;

    id = 0;

    rc = pr_NameToId(&nl, &idl);
    if (rc) {
	com_err(argv[0], PRNOENT, "so cannot confirm cross cell token for %s", namebuf);
	fprintf(stderr,"%s: remote cell may be down\n",argv[1]);
	exit(1);
    }
    /* entry DNE */
    if (idl.idlist_val[0] == ANONYMOUSID) {
	printf("doing first-time registration of %s at %s\n", namebuf, argv[1]);
	rc = pr_CreateUser(namebuf, &id);

	if (rc) {
	    com_err (argv[0], rc, "so could not create cross cell entry for %s", namebuf);
	    fprintf(stderr,"%s: remote cell may be down\n",argv[1]);
	    exit(1);
	}

	printf("created cross cell entry for %s at %s\n", namebuf, argv[1]);
    }
    rx_Finalize();
}

foldup (out,in)
char *in,*out;
{
    for (;*in; in++) {
	if (islower(*in))
	    *out = toupper(*in);
	else
	    *out = *in;
	out++;
    }
    *out = 0;
}

folddown (out,in)
char *in,*out;
{
    for (;*in; in++) {
	if (isupper(*in))
	    *out = tolower(*in);
	else
	    *out = *in;
	out++;
    }
    *out = 0;
}
