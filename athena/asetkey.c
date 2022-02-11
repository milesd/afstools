#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <krb.h>
	
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
	char tkey[8], name[255], inst[255], realm[255];
	int kvno;
	if (argc != 5) {
	    printf("setkey add: usage is 'setkey add <kvno> <keyfile> <princ>\n");
	    exit(1);
	}
	kvno = atoi(argv[2]);
	bzero(tkey, sizeof(tkey));
	code = kname_parse(name, inst, realm, argv[4]);
	if (code != 0) {
		printf("Invalid kerberos name\n");
		exit(1);
	}
	code = read_service_key(name, inst, realm, kvno, argv[3], tkey);
	if (code != 0) {
		printf("Can't find key in %s\n", argv[3]);
		exit(1);
	}
	code = afsconf_AddKey(tdir, kvno, tkey);
	if (code) {
	    printf("setkey: failed to set key, code %d.\n", code);
	    exit(1);
	}
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
	register int i;
	char tbuffer[9];
	
	code = afsconf_GetKeys(tdir, &tkeys);
	if (code) {
	    printf("setkey: failed to get keys, code %d\n", code);
	    exit(1);
	}
	for(i=0;i<tkeys.nkeys;i++) {
	    if (tkeys.key[i].kvno != -1) {
		bcopy(tkeys.key[i].key, tbuffer, 8);
		tbuffer[8] = 0;
		printf("kvno %4d: key is '%s'\n", tkeys.key[i].kvno, tbuffer);
	    }
	}
	printf("All done.\n");
    }
    else {
	printf("setkey: unknown operation '%s', type 'setkey' for assistance\n");
	exit(1);
    }
    exit(0);
}
