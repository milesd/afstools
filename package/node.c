/*
 *	package by Russell J. Yount
 *	(C) Copyright 10/12/86 by Carnegie Mellon University
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "node.h"

struct node *root;

static int 
namehash(register char *name)
{
  register int hash;

  for (hash = 0; *name; hash += (hash << 6) + *name++)
    continue;
  return hash;
}

static struct node *
allocnode()
{
  register struct node *np;

  if ((np = (struct node *)malloc(sizeof(struct node))) == 0)
    fatal("malloc; %m");
  memset((char *)np, 0, sizeof(struct node));
  return np;
}

void
freenode(struct node *np)
{
  memset((char *)np, 0, sizeof(struct node));
  free(np);
}

struct node *
lookupindir(register struct node *dp, register char *name, int flag)
{
  register int hash;
  register struct entry *ep, *lp;

  if ((dp->mode & S_IFMT) != S_IFDIR)
    return 0;
  
  hash = namehash(name);
  for (lp = ep = dp->file.dirp; ep; lp = ep, ep = ep->nextp)
    {
      if (ep->hash != hash)
	continue;
      if (strcmp(ep->name,name))
	continue;
      return ep->nodep;
    }

  if (flag == noop)
    return 0;
  
  if (flag == create) {
    ep = (struct entry *)malloc((unsigned)sizeof(struct entry)+strlen(name));
    if (ep == 0)
      fatal("malloc; %m");
    ep->nodep = allocnode();
    ep->hash = hash;
    (void)strcpy(ep->name,name);
    ep->nextp = dp->file.dirp;
    dp->file.dirp = ep;
  
    return ep->nodep;
  } 
  if (flag == delete) {
    lp->nextp = ep->nextp;
    freenode(ep);
    return (struct node *)NULL;
  }
}

struct node *lookuppath(dp,path,flag)
register struct node *dp;
register char *path;
register int flag;
{
  register char *name;
  register char ch;
  
  while (*path == '/')
    path++;
  while (*path != 0 && dp != 0)
    {
      name = path;
      while (*path != 0 && *path != '/')
	path++;
      ch = *path;
      *path = 0;
      if (flag && (dp->flag & flag_type) == 0)
	dp->mode |= S_IFDIR;
      dp = lookupindir(dp,name,flag);
      *path = ch;
      while (*path == '/')
	path++;
    }
  return dp;
}

int
initnode()
{
  root = allocnode();
}
