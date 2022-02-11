/*
 * This file replaces some of the routines in the Kerberos utilities.
 * It is based on the Kerberos library modules:
 * 	send_to_kdc.c
 * 
 * Copyright 1987, 1988, 1992 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 */

#ifndef lint
static char rcsid_send_to_kdc_c[] =
"$Id: krb_util.c,v 1.3 1998/01/08 20:33:05 vwelch Exp $";
#endif /* lint */

#include <kerberosIV/mit-copyright.h>
#include <krb5.h>
#include <kerberosIV/krb.h> 

#ifdef WINDOWS

#include "aklog.h"		/* for struct afsconf_cell */

#else /* !WINDOWS */

#include <afs/param.h>
#include <afs/cellconfig.h>

#endif /* WINDOWS */

#define S_AD_SZ sizeof(struct sockaddr_in)

char *afs_realm_of_cell(context, cellconfig)
    krb5_context context;
    struct afsconf_cell *cellconfig;
{
    char krbhst[MAX_HSTNM];
    static char krbrlm[REALM_SZ+1];
	char **hrealms = 0;
	krb5_error_code retval;

    if (!cellconfig)
	return 0;
    if (retval = krb5_get_host_realm(context,
				cellconfig->hostName[0], &hrealms))
		return 0; 
	if(!hrealms[0]) return 0;
	strcpy(krbrlm, hrealms[0]);

	if (hrealms) krb5_xfree(hrealms);
    
    return krbrlm;
}
