/*
  crosslog.c
  Joseph Jackson
  27-Jan-1994

  Replacement for CMU's cklog program.  This one relies on Rx
  protocols from libkauth instead of UDP protocols from libkrb.
  Since we don't provide libkrb to customers, cklog is hard to
  compile.  This one should easily link with libkauth and friends.

*/


#include <stdio.h>		/* for good measure */
#include <stdlib.h>		/* for getenv */
#include <afs/param.h>		/* always required by AFS code */
#include <afs/kautils.h>	/* for all ka_ routines */

#include <afs/cellconfig.h>	/* for local copy of ka_GetAFSTicket */
#include <afs/ptserver.h>
#include <afs/pterror.h>
  
char *pn;

#define CHECK(code,whoami) \
  if (code) { \
    fprintf (stderr, "%s: %s failed with code %d\n%s\n", \
	     pn, whoami, code, error_message(code)); \
    dump_tokens(); \
    return code; \
  } else { \
    fprintf (stderr, "%s finished okay\n", whoami); \
    fflush (stderr); \
  }

char *
format_princ (principal)
     struct ktc_principal *principal;
{
  static char return_string[MAXKTCNAMELEN*3+2];

  sprintf(return_string, "%s%s%s@%s",
	  principal->name,
	  principal->instance[0] ? "." : "",
	  principal->instance,
	  principal->cell);
  return return_string;
}

int
dump_tokens ()
{
  struct ktc_principal serviceName, clientName;
  struct ktc_token token;
  int code;
  int tokenNum = 0;

  while (1) {
    code = ktc_ListTokens(tokenNum, &tokenNum, &serviceName);
    if (code) {
      printf("[ End ]\n");
      break;
    } else {
      code = ktc_GetToken(&serviceName, &token, sizeof(token), &clientName);
      if (code) {
	fprintf (stderr, "%s: ktc_GetToken failed for %s\n", 
		 pn, format_princ(serviceName));
	continue;
      }
      printf ("[%5d] ", tokenNum);
      printf ("C: %-30s ", format_princ(&clientName));
      printf ("S: %s\n", format_princ(&serviceName));
    }
  }
  return 0;
}

int
main(argc, argv)
     int argc;
     char **argv;
{
  char   name[MAXKTCNAMELEN];
  char   inst[MAXKTCNAMELEN];
  char   cell[MAXKTCNAMELEN];
  char  realm[MAXKTCNAMELEN];
  char *passwd;
  struct ktc_encryptionKey key;
  struct ktc_token token;
  Date lifetime = MAXKTCTICKETLIFETIME;
  int  code;
  long pwdexpired;
  
  pn = argv[0];
  if (argc != 5) {
    fprintf (stderr, "Usage: %s <name> <inst> <cell> <realm>\n", pn);
    return 1;
  }
  strcpy (name, argv[1]);
  strcpy (inst, argv[2]);
  strcpy (cell, argv[3]);
  strcpy (realm, argv[4]);
  passwd = getenv("PASSWD");
  if (!passwd || !passwd[0]) {
    fprintf (stderr, "%s: PASSWD environment variable not set\n", pn);
    passwd = 0;
  }
  printf ("Name: %s\nInst: %s\nCell: %s\nRealm: %s\n\n",
	  name, inst[0] ? inst : "(none)", cell, realm);
  
  code = dump_tokens();
  CHECK (code, "dump_tokens");

  code = ka_Init(0);
  CHECK (code, "ka_Init");

  if (passwd) {
    ka_StringToKey (passwd, cell, &key);

    code = ka_GetAuthToken (name, inst, cell, &key, lifetime, &pwdexpired);
    CHECK (code, "ka_GetAuthToken");
  }

/*code = ka_GetServerToken ("afs" , "", realm, lifetime, &token, 1); */

  code = ka_GetAFSTicket (name, inst, realm, lifetime, 0);
  CHECK (code, "ka_GetAFSTicket");

  code = dump_tokens();
  CHECK (code, "dump_tokens");

  return 0;
}

/*
ktc_GetToken(aserver, atoken, atokenLen, aclient)
struct ktc_principal *aserver, *aclient;
int atokenLen;
struct ktc_token *atoken; {
*/
