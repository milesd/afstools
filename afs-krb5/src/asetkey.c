/*
 * $Id: asetkey.c,v 1.5 1997/03/31 16:37:50 kenh Exp $
 *
 * asetkey - Manipulates an AFS KeyFile
 *
 * Updated for Kerberos 5
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <krb5.h>

#include <afs/stds.h>
#include <afs/cellconfig.h>
#include <afs/keys.h>

main(argc, argv)
int argc;
char **argv; {
    struct afsconf_dir *tdir;
    register long code;

    if (argc == 1) {
	printf("setkey: usage is 'setkey <opcode> options, e.g.\n");
	printf("    setkey add <kvno> <keyfile> <princ>\n");
	printf("    setkey delete <kvno>\n");
	printf("    setkey list\n");
	exit(1);
    }

    tdir = afsconf_Open(AFSCONF_SERVERNAME);
    if (!tdir) {
	printf("setkey: can't initialize conf dir '%s'\n", AFSCONF_SERVERNAME);
	exit(1);
    }
    if (strcmp(argv[1], "add")==0) {
	krb5_context context;
	krb5_principal principal;
	krb5_keyblock *key;
	krb5_error_code retval;
	int kvno;

	if (argc != 5) {
	    printf("setkey add: usage is 'setkey add <kvno> <keyfile> <princ>\n");
	    exit(1);
	}

	krb5_init_context(&context);

	kvno = atoi(argv[2]);
	retval = krb5_parse_name(context, argv[4], &principal);
	if (retval != 0) {
		com_err(argv[0], retval, "while parsing AFS principal");
		exit(1);
	}
	retval = krb5_kt_read_service_key(context, argv[3], principal, kvno,
					  ENCTYPE_DES_CBC_CRC, &key);
	if (retval != 0) {
		com_err(argv[0], retval, "while extracting AFS service key");
		exit(1);
	}

	if (key->length != 8) {
		printf("Key length should be 8, but is really %d!\n",
		       key->length);
		exit(1);
	}

	code = afsconf_AddKey(tdir, kvno, key->contents);
	if (code) {
	    printf("setkey: failed to set key, code %d.\n", code);
	    exit(1);
	}
	krb5_free_principal(context, principal);
	krb5_free_keyblock(context, key);
    }
    else if (strcmp(argv[1], "delete")==0) {
	long kvno;
	if (argc != 3) {
	    printf("setkey delete: usage is 'setkey delete <kvno>\n");
	    exit(1);
	}
	kvno = atoi(argv[2]);
	code = afsconf_DeleteKey(tdir, kvno);
	if (code) {
	    printf("setkey: failed to delete key %d, (code %d)\n", kvno, code);
	    exit(1);
	}
    }
    else if (strcmp(argv[1], "list") == 0) {
	struct afsconf_keys tkeys;
	register int i, j;
	
	code = afsconf_GetKeys(tdir, &tkeys);
	if (code) {
	    printf("setkey: failed to get keys, code %d\n", code);
	    exit(1);
	}
	for(i=0;i<tkeys.nkeys;i++) {
	    if (tkeys.key[i].kvno != -1) {
		printf("kvno %4d: key is: ", tkeys.key[i].kvno);
		for (j = 0; j < 8; j++)
			printf("%02x", (unsigned char) tkeys.key[i].key[j]);
		printf("\n");
	    }
	}
	printf("All done.\n");
    }
    else {
	printf("setkey: unknown operation '%s', type 'setkey' for assistance\n", argv[1]);
	exit(1);
    }
    exit(0);
}
