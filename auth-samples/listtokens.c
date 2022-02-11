/* Copyright (C) 1989 Transarc Corporation - All rights reserved */
/*
 * P_R_P_Q_# (C) COPYRIGHT IBM CORPORATION 1987
 * LICENSED MATERIALS - PROPERTY OF IBM
 * REFER TO COPYRIGHT INSTRUCTIONS FORM NUMBER G120-2083
 */

#include <afs/param.h>
#include <stdio.h>
#include <afs/auth.h>
#include <time.h>		/* ctime() */

char *ctime();

main(argc, argv)
    int argc;
    char **argv;

{
  int	    cellNum;			/*Cell entry number*/
  int	    rc;				/*Return value from U_CellGetLocalTokens*/
  int	    tokenExpireTime;		/*When token expires*/
  char	    *expireString;		/*Char string of expiration time*/
  char	    UserName[16];		/*Printable user name*/
  struct ktc_principal serviceName, clientName;	/* service name for ticket */
  struct ktc_token token;			/* the token we're printing */
  
  cellNum = 0;
  
  while (1) {
    rc = ktc_ListTokens(cellNum, &cellNum, &serviceName);
    if (rc) {
      /* only error is now end of list */
      printf("   --End of list--\n");
      break;
    }
    else {
      /* get the ticket info itself */
      rc = ktc_GetToken(&serviceName, &token, sizeof(token), &clientName);
      if (rc) {
	printf("%s: failed to get token info for service %s.%s.%s (code %d)\n",
	       argv[0], serviceName.name, serviceName.instance,
	       serviceName.cell, rc);
	continue;
      }
      tokenExpireTime = token.endTime;
      
      strcpy(UserName, clientName.name);
      if (clientName.instance[0] != 0) {
	strcat(UserName, ".");
	strcat(UserName, clientName.instance);
      }
      if (UserName[0] == 0)
	printf("Tokens");
      else if (strncmp(UserName, "AFS ID", 6) == 0) {
	printf("User's (%s) tokens", UserName);
      }
      else if (strncmp(UserName, "Unix UID", 8) == 0) {
	printf("Tokens");
      }
      else
	printf("User %s's tokens", UserName);
      printf(" for %s%s%s@%s ",
	     serviceName.name,
	     serviceName.instance[0] ? "." : "",
	     serviceName.instance,
	     serviceName.cell);
      expireString = ctime(&tokenExpireTime);
      expireString += 4; /*Move past the day of week*/
      expireString[12] = '\0';
      printf("[Expires %s]\n",
	     expireString);
    }
  }
}
