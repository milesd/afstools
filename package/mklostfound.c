/*
 *	package by Russell J. Yount
 *	(C) Copyright 10/12/86 by Carnegie Mellon University
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include "node.h"

int mklostfound(register char *path)
{
  register char *u, *l, *endp;
  register int f;
  struct stat stb;

  if (!silent)
    message("mklost+found %s",path);
  if (lazy)
    return 0;
  endp = path + strlen(path);
  *endp++ = '/';
  endp[2] = 0;
  for (u = "0123456789abcdef"; *u; u++) {
    for (l = "0123456789abcdef"; *l; l++) {
      endp[0] = *u;
      endp[1] = *l;
      f = open(path,O_CREAT|O_TRUNC|O_WRONLY,0666);
      if (f < 0) {
	message("open %s; %m",path);
	continue;
      }
      (void)close(f);
    }
  }
  for (u = "0123456789abcdef"; *u; u++) {
		for (l = "0123456789abcdef"; *l; l++) {
		  endp[0] = *u;
		  endp[1] = *l;
		  if (lstat(path,&stb) >= 0)
		    if ((stb.st_mode & S_IFMT) != S_IFDIR)
		      if (unlink(path) < 0)
			message("unlink %s; %m",path);
		}
  }
  *--endp = 0;
  return 0;
}
