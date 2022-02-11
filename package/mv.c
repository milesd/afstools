/*
 *	package by Russell J. Yount
 *	(C) Copyright 10/12/86 by Carnegie Mellon University
 */
#include <sys/param.h>
#include <errno.h>
#include "node.h"

int 
mv(register char *from, register char *to)
{
  if (!silent)
    message("mv %s %s",from,to);
  if (!lazy && rename(from,to) < 0) {
    if (errno == ETXTBSY) {
      char busy[MAXPATHLEN];
      
      message("rename %s %s; %m", from, to);
      /* should check array bounds here...  */
      if ((strlen(to) + 6) > MAXPATHLEN) { 
	/* if to is too long, truncate it */
	strncpy(busy,to,MAXPATHLEN-6);
      } else {
	strcpy(busy,to);
      }
      strcat(busy,".busy");
      mv(to,busy);
      mv(from,to);
    } else {
      message("rename %s %s; %m",from,to);	
      return -1;
    }
  }
  return 0;
}
