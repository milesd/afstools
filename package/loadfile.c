/*
 *	package by Russell J. Yount
 *	(C) Copyright 10/12/86 by Carnegie Mellon University
 */

#include <sys/types.h>
#include <sys/file.h>
#include <fcntl.h>

#define	entrysize	512
#define	buffersize	8192
#define	maxfield	32

int
loadfile(char *filename, int (*loadfunc)())
{
  char	buffer[buffersize+entrysize+1], *field[maxfield];
  int	fd, lineno, cc;
  register char	*tailp, *headp;
  register int	nfield, count;

  if ((fd = open(filename,O_RDONLY)) < 0)
    fatal("open %s; %m",filename);
  lineno = 0;
  cc = buffersize;
  tailp = headp = buffer;
  for (;;)
    {
      if ((count = tailp - headp) < entrysize && cc > 0)
	{
	  if ((headp - buffer) > 0)
	    {
	      memcpy(buffer, headp, count);
	      headp = buffer;
	      tailp = buffer + count;
	    }
	  if ((cc = read(fd,tailp,buffersize)) > 0)
	    tailp += cc;
	  *tailp = 0;
	}
      if (tailp == headp)
	break;
      for (nfield = 0; nfield < maxfield; nfield++)
	{
	  while (*headp == ' ' || *headp == '\t')
	    headp++;
	  if (*headp == '#')
	    {
	      *headp++ = 0;
	      while (*headp != 0 && *headp != '\n')
		headp++;
	    }
	  if (*headp == '\n')
	    {
	      lineno++;
	      *headp++ = 0;
	      break;
	    }
	  if (*headp == 0)
	    break;
	  field[nfield] = headp;
	  while (*headp != 0)
	    {
	      if (*headp == '\n' || *headp == '#')
		break;
	      if (*headp == ' ' || *headp == '\t')
		{
		  *headp++ = 0;
		  break;
		}
	      headp++;
	    }
	}
      if (nfield == 0)
	continue;
      if ((*loadfunc)(nfield,field, lineno))
	fatal("%s; syntax error [line %d]",filename,lineno);
    }
  if (close(fd) < 0)
    fatal("close %s; %m",filename);
}
