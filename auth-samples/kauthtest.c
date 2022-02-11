/*
** Dan Hamel
** March 25, 1991
** kauthtest
**
** 15-Feb-1994 jackson: Updated for AFS 3.3's password_expires feature
**
** This is an example of how you might include AFS authentication in an
** application that needs to write into protected areas of the AFS name
** space.
**
** Notice that the setpag() will get a new Process Authentication Group
** (PAG) for this process, and therefore the user's tokens won't be
** affected by the ka_UserAuthenticateGeneral or ktc_ForgetAllTokens
** calls.
**
** To compile, use the associated makefile:  make -f kauthtest.makefile
*/

#include <afs/param.h>
#include <afs/kautils.h>
#include <stdio.h>


void klog(pw_name)
   char *pw_name;
{
   long code;
   long password_expires = -1;
   char *reason, pword[64];
  
   printf("\nklog: %s\n", pw_name);
   code = ka_UserReadPassword("Password:", pword, sizeof(pword), &reason);

   if(code) {
      printf("Unable to read password because %s\n", reason);
      exit(1);
   }		     

   if(ka_UserAuthenticateGeneral(
        KA_USERAUTH_VERSION + KA_USERAUTH_DOSETPAG,
        pw_name,        /* kerberos name */
        (char *)0,      /* instance */
        (char *)0,      /* cell */
        pword,          /* password */
        0,              /* default lifetime */
        &password_expires, /* days until the password expires */
        0,              /* spare 2 */
        &reason         /* error string */
        )) {
            printf("ERR: Unable to authenticate to AFS because %s\n",
                    reason);
            exit(1);
         } 
   printf("kauth succeeded for %s\n", pw_name);
   printf("The password for %s ", pw_name);
   if (password_expires < 0)
     printf("never expires\n");
   else if (password_expires == 255) {
     printf("expires in 255 days OR you are running an AFS 3.2 kaserver\n");
   } else
     printf("expires in %d days\n", password_expires);
}


void try_access(fname, msg)
   char *fname, *msg;
{
   FILE *fd;

   if((fd = fopen(fname, "w")) != NULL) {
      printf("open succeeded %s\n", msg);
      close(fd);
   } else
      printf("open failed %s\n", msg);
}


void unlog() {
   int code;

   code = ktc_ForgetAllTokens ();
   if(code) {
      printf("\nERR: can't discard tokens, code=%d\n", code);
      exit(1);
   }
   printf("\nUnlog succeeded!\n");
}


main(argc, argv) 
   int argc;
   char *argv[];
{
   char *uname;
   char *file;

   if (argc != 3) {
      printf("Usage: klog-test <user-name> <file name>\n\n");
      exit (1);
   }
   uname = argv[1];
   file = argv[2];
   printf("\nTesting with name=%s, file=%s\n", uname, file);

   setpag();           /* get unique PAG for this process, no tokens */
   try_access(file, "without token");   
   klog(uname);
   try_access(file, "with token");
   unlog();
   try_access(file, "after unlog");
}
