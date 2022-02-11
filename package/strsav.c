/*
 *	package by Russell J. Yount
 *	(C) Copyright 10/12/86 by Carnegie Mellon University
 */

char *strsav(register char *s)
{
  register char *p;

  if ((p = malloc((unsigned)1+strlen(s))) == 0)
    fatal("malloc; %m");
  strcpy(p,s);
  return p;
}
