/*
 *	package by Russell J. Yount
 *	(C) Copyright 10/12/86 by Carnegie Mellon University
 */

#include <sys/param.h>
#ifdef ANDREW
#include <sysname.h>
#endif 
#include "node.h"

extern	loadentry();

#ifdef ANDREW
char *cpu = SYS_NAME;
#else 

#ifdef	vax
char	*cpu = "vax";
#endif	
#ifdef	sun
#ifdef	sunV3
#ifdef	mc68020
char	*cpu = "sun3";
#else	
char	*cpu = "sun2";
#endif	
#else	
char	*cpu = "sun";
#endif	
#endif	
#ifdef	ibm032
char	*cpu = "ibm032";
#endif
#endif

char	*conffile = "/etc/package";
int	status = status_noerror;
int	verbose = 0;
int	lazy = 0;
int	silent = 0;
int	re_boot = 1;
int	kflag = 1;
int     absolute = 0;

static int
doargs(int argc, char **argv)
{
  char	*cp;
  
  argv++,argc--;
  while (argc > 0)
    {
      cp = *argv++,argc--;
      if (*cp++ != '-')
	usage();
      while (*cp) {
	  switch (*cp) {
	    case 'c':
	      if (argc <= 0)
		usage();
	      conffile = *argv++,argc--;
	      break;
	    case 'k':
	      kflag = 1;
	      break;
	    case 'y':
	      kflag = 0;
	      break;
	    case 'n':
	      lazy = 1;
	      break;
	    case 's':
	      silent = 1;
	      break;
	    case 'v':
	      verbose = 1;
	      break;
	    case 'r':
	      re_boot = 0;
	      break;
	    case 'a':
	      absolute = 1;
	      break;
	      
	    default:
	      usage();
	  }
	  cp++;
      }
    }
}

static int
usage()
{
  fatal("Usage: package [-kynsv] [-c conffile]");
}

int
main(int argc, char **argv)
{
  char	filename[MAXPATHLEN];
  umask(0);
  doargs(argc,argv);
  initnode();
  if (absolute)
    sprintf(filename,"%s",conffile);
  else
    sprintf(filename,"%s.%s",conffile,cpu);
  if (verbose)
    message("loading %s",filename);
  loadfile(filename,loadentry);
  if (verbose)
    message("checking");
  check();
  if (verbose)
    message("updating");
  update();
  xsync();
  if (verbose)
    message("done");
  exit(status);
}
