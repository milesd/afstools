#include <afs/param.h>
#include <afs/kautils.h>
  
  struct greet_info {
    char          *name;        /* user name */
    char          *password;     /* user password */
    char          *string;       /* random string */
  };

int try_to_login();

main(argc, argv)
     int argc;
     char **argv;
{
  int code;
  struct greet_info test_greet;
  static char name[128], password[128];
  
  memset(name,NULL,sizeof(name));
  memset(password,NULL,sizeof(password));
  
  printf("Enter AFS User Name: ");
  if( !gets(name) ){
    printf("Unable to grab name from stdin\n");
    perror("Error message returned");
    exit(1);
  }
  printf("Enter Password: ");
  if ( !gets(password) ){
    printf("Unable to grab password from stdin\n");
    perror("Error message returned");
    exit(1);
  }
  
  test_greet.name = name;
  test_greet.password = password;
  
  code = try_to_login(&test_greet);
  
  system ("/usr/ucb/groups;/usr/afsws/bin/tokens");

  return code;
}

int try_to_login (test_g)
     struct greet_info *test_g;
{
  
  int code;
  long password_expires = -1;
  char *reason;
  
  printf("\nName: %s\nPassword: %s\n", test_g->name, test_g->password);
  
  code = ka_UserAuthenticateGeneral(KA_USERAUTH_VERSION+KA_USERAUTH_DOSETPAG,
				    test_g->name,
				    (char *) 0,  /* instance */
				    (char *) 0,  /* cell */
				    test_g->password,
				    0,           /* lifetime, default */
				    &password_expires, /*days 'til it expires*/
				    0,        /* spare 2 */
				    &reason);
  
  if (code != 0)
    printf("AFS Verify failed because %s\n", reason);
  else
    printf("Verify succeeded\n");
  
  return code;
}
