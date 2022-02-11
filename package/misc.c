/*
 *	package by Russell J. Yount
 *	(C) Copyright 10/12/86 by Carnegie Mellon University
 */

#include <sys/param.h>
#include <sys/types.h>
#include <varargs.h>
#include <stdio.h>
#include "node.h"

#ifdef __GLIBC__
#include <errno.h>
#else
extern int errno;
extern int sys_nerr;
extern char *sys_errlist[];
#endif
	  
static char *
putnum(register char *dp, register unsigned n, register int b)
{	
  register int s;

  for (s = b; n / s; s *= b)
    continue;
  s /= b;
  while (s > 0) {
    *dp++ = '0' + (n / s);
    n %= s;
    s /= b;
  }
  return dp;
}

static char *
putstr(register char *dp, register char *s)
{
  while (*s)
    *dp++ = *s++;
  return dp;
}

static char *
putformat(register char *dp, register char *fp,	register va_list ap)
{
  while (*fp) {
    if (*fp == '%') {
      switch (*++fp) {
      case 'c':
	{
	  char	c;
	  
	  c = va_arg(ap,int);
	  *dp++ = c;
	  fp++;
	  break;
	}
      case 'd':
	{
	  int	d;
	  
	  d = va_arg(ap,int);
	  if (d < 0) {
	    *dp++ = '-';
	    d = -d;
	  }
	  dp = putnum(dp,(unsigned)d,10);
	  fp++;
	  break;
	}
      case 'm':
	{
	  if (errno >= 0 && errno < sys_nerr)
	    dp = putstr(dp,sys_errlist[errno]);
	  else {
	    dp = putstr(dp,"Unknown error (errorno =");
	    dp = putnum(dp,(unsigned)errno,10);
	    dp = putstr(dp,")");
	  }
	  fp++;
	  break;
	}
      case 'o':
	{
	  unsigned o;
	  
	  o = va_arg(ap,int);
	  dp = putnum(dp,o,8);
	  fp++;
	  break;
	}
      case 's':
	{
	  char	*s;
	  
	  s = va_arg(ap,char *);
	  dp = putstr(dp,s);
	  fp++;
	  break;
	}
      case 'u':
	{
	  unsigned u;
	  
	  u = va_arg(ap,int);
	  dp = putnum(dp,u,10);
	  fp++;
	  break;
	}
      case 'x':
	{
	  unsigned x;
	  
	  x = va_arg(ap,int);
	  dp = putnum(dp,x,16);
	  fp++;
	  break;
	}
      }
      continue;
    }
    if (*fp == '\\') {
	switch (*++fp) {
	  case '\\':
	    *dp++ = '\\';
	    fp++;
	    break;
	    
	  case 'f':
	    *dp++ = '\f';
	    fp++;
	    break;
	    
	  case 'n':
	    *dp++ = '\n';
	    fp++;
	    break;
	    
	  case 'r':
	    *dp++ = '\r';
	    fp++;
	    break;
	    
	  case 't':
	    *dp++ = '\t';
	    fp++;
	    break;
	    
	}
	continue;
    }
    *dp++ = *fp++;
  }
  return dp;
}

/* Better safe */
#define	maxline	(5*MAXPATHLEN)

int
fatal(va_alist)
va_dcl
{
  va_list	ap;
  char	*dp, *fp;
  char	line[maxline];
  
  va_start(ap);
  fp = va_arg(ap,char *);
  dp = putformat(line,fp,ap);
  *dp++ = '\n';
  write(2,line,dp-line);
  exit(status_error);
}


int
message(va_alist)
va_dcl
{
  va_list	ap;
  char	*dp, *fp;
  char	line[maxline];
  
  va_start(ap);
  fp = va_arg(ap,char *);
  dp = putformat(line,fp,ap);
  *dp++ = '\n';
  write(1,line,dp-line);
}

