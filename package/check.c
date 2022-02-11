/*
 *	package by Russell J. Yount
 *	(C) Copyright 10/12/86 by Carnegie Mellon University
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef USES_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
#include "node.h"


extern struct node *lookupindir();
extern char *strsav();

static char path[MAXPATHLEN];
static char path2[MAXPATHLEN];
static char path3[MAXPATHLEN];

int
check()
{
	strcpy(path,"");
	traverse1(root);
}

static int
check3(struct node *np)
{
	struct stat stb, stb2;
	char dir[MAXPATHLEN], parent[MAXPATHLEN];
	register char *ep, *dp, *sp;
	int ret;

	ret = 0;
	ep = strrchr(path,'/');
	for (sp = path, dp = dir; sp < ep; *dp++ = *sp++);
	if (dp == dir)
		*dp++ = '/';
	*dp = 0;
	ep = strrchr(dir,'/');
	for (sp = dir, dp = parent; sp < ep; *dp++ = *sp++);
	if (dp == parent)
		*dp++ = '/';
	*dp = 0;
	ret = 0;
	if (strcmp(dir,"/")) {
		if (stat(dir,&stb) < 0)
			ret = -1;
		if (stat(parent,&stb2) < 0)
			ret = -1;
		if ((stb.st_mode & S_IFMT) != S_IFDIR)
			ret = -1;
		if ((stb2.st_mode & S_IFMT) != S_IFDIR)
			ret = -1;
		if (stb2.st_dev == stb.st_dev)
			ret = -1;
	}
	if (ret < 0)
		fatal("%s; not mounted [line %d]",dir, np->lineno);
	if (verbose)
		message("mount %s",dir);
}

static int
check2(register struct node *np)
{
	register struct node *np2;
#ifdef USES_DIRENT
	register struct dirent *de;
#else
	register struct direct *de;
#endif
	DIR	*dp;
	struct stat stb;
	int cc;
	struct node oldnp2;

	if ((np->flag & (flag_type|flag_big)) == (flag_type|flag_big))
		check3(np);
	if ((np->flag & flag_proto) == 0)
	{
		if ((np->flag & flag_type) == 0)
			fatal("%s; incomplete [line %d]",path, np->lineno);
		return;
	}
	if (np->flag & flag_abs)
		sprintf(path2,"%s",np->proto);	
	else		
		sprintf(path2,"%s%s",np->proto,path);
	if (! (np->flag & flag_type) && ! (np->flag & flag_copy )) {
	  if (lstat(path2,&stb) < 0)
	    if (verbose)
	      fatal("lstat %s; %m [line %d]", path2, np->lineno);
	  if ((stb.st_mode & S_IFMT) == S_IFLNK) {
	    np->proto = (char *) malloc(MAXPATHLEN +1);
            cc = readlink(path2,np->proto,MAXPATHLEN);
	    np->proto[cc]=0;
	    np->flag |= flag_abs;

	  }
	}
	else {
	  if (stat(path2,&stb) < 0)
	    if ((np->mode & S_IFMT) == S_IFLNK)
	      {
		if (lstat(path2,&stb) < 0)
		  if (verbose)
		    message("WARNING %s not found", path2);
	      }
	    else
	      fatal("stat %s; %m [line %d]",path2, np->lineno);
	}

	if (np->flag & flag_type) 
	  {
		if ((np->mode & S_IFMT) == S_IFLNK)
			return;
		if ((stb.st_mode & S_IFMT) != (np->mode & S_IFMT))
			fatal("check2 %s; type conflict [line %d]",path2, np->lineno);
	  } 
	else 
	  {
		np->mode |= stb.st_mode & S_IFMT;
		np->flag |= flag_type;

	  }
	switch (np->mode & S_IFMT)
	{
		case S_IFCHR:
		case S_IFBLK:
			if ((np->flag & flag_rdev) == 0)
			{
				np->rdev = stb.st_rdev;
				np->flag |= flag_rdev;
			}
                case S_IFREG:
			np->size = stb.st_size;
	}
	if ((np->flag & flag_mode) == 0)
	{
               if (np->flag & flag_copyattr) {
                      np->mode |= stb.st_mode & ~S_IFMT & np->modemask;
                      np->mode |= stb.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH);
               } else
                      np->mode |= stb.st_mode & ~S_IFMT;
	       
		np->flag |= flag_mode;
	}
	if ((np->flag & flag_uid) == 0)
	{
		np->uid = stb.st_uid;
		np->flag |= flag_uid;
	}
	if ((np->flag & flag_gid) == 0)
	{
#ifdef	VICE
		np->gid = (stb.st_gid == 32767) ? 0 : stb.st_gid;
#else	VICE
		np->gid = stb.st_gid;
#endif	VICE
		np->flag |= flag_gid;
	}
	if ((np->flag & flag_mtime) == 0)
	{
		np->mtime = stb.st_mtime;
		np->flag |= flag_mtime;
	}
	if ((np->mode & S_IFMT) != S_IFDIR)
		return;
	if (verbose)
		message("scandir %s",path2);
	if ((dp = opendir(path2)) == 0)
		fatal("opendir %s; %m [line %d]",path2, np->lineno);
	while ((de = readdir(dp)) != 0)
	{
		if (de->d_name[0] == '.')
		{
			if (de->d_name[1] == 0)
				continue;
			if (de->d_name[1] == '.' && de->d_name[2] == 0)
				continue;
		}
		if ((np2 = lookupindir(np,de->d_name,create)) == 0)
			fatal("%s/%s; bad path [line %d]",path,de->d_name, np->lineno);
		/* Keep a backup so we can return to previous state */
		memcpy(&oldnp2, np2, sizeof(struct node));
	      redo:
		/* Important not to copy this for entries
		   not fully inherited at this point */
		if ((np2->flag & flag_type) && (np->flag & flag_win))
		  np2->flag |= flag_win;
		if (np->flag & flag_clean) 
		  np2->flag |= flag_clean;
		if (np->flag & flag_copy)
		  np2->flag |= flag_copy;
                if (np->flag & flag_copyattr) {
                  np2->flag |= (flag_copyattr|flag_uid|flag_gid);
                  if (np2->flag & flag_mode)
                    np2->mode = np->mode & ~(S_IFMT|S_IXUSR|S_IXGRP|S_IXOTH) |
                                np2->mode & (S_IFMT|S_IXUSR|S_IXGRP|S_IXOTH);
                  else
                    np2->modemask = np->mode &~(S_IFMT|S_IXUSR|S_IXGRP|S_IXOTH);
                  np2->uid = np->uid;
                  np2->gid = np->gid;
                }
		
		if ((np2->flag & flag_proto) == 0) {
			if (np->flag & flag_abs) {
				np2->flag |= flag_abs;
				sprintf(path3,"%s/%s",np->proto,de->d_name);
				np2->proto = strsav(path3);
			} else {
				np2->proto = np->proto;
			}
			np2->flag |= flag_proto;
			continue;
		}
		if ((np2->flag & flag_abs) != (np->flag & flag_abs)) {
		  if ((np->flag & flag_win) ^ (np2->flag & flag_win)) {
		    if (np->flag & flag_win) {
		      /* Wipe the old entry and use new one */
		      memset((char *)np2, 0, sizeof(struct node));
		      goto redo;
		    } else 
		      /* Back out, do not augment */
		      memcpy(np2, &oldnp2, sizeof(struct node));
		  } else
		    fatal("check: %s/%s; proto conflict; not abs [line %d]",path,de->d_name, np->lineno);
		}		

		if (np->flag & flag_abs) {
			sprintf(path3,"%s/%s",np->proto,de->d_name);
			if (strcmp(path3,np2->proto)) 
			  if ((np->flag & flag_win) ^ (np2->flag & flag_win)) {
			    if (np->flag & flag_win) {
			      /* Wipe the old entry and use new one */
			      memset((char *)np2, 0, sizeof(struct node));
			      goto redo;
			    } else 
			      /* Back out, do not augment */
			      memcpy(np2, &oldnp2, sizeof(struct node));
			  } else
			    fatal("%s/%s; proto conflict; %s != %s [line %d]",path,de->d_name, path3, np2->proto, np->lineno);
		} else {
			if (strcmp(np->proto,np2->proto)) {
			  if ((np->flag & flag_win) ^ (np2->flag & flag_win)) {
			    if (np->flag & flag_win) {
			      /* Wipe the old entry and use new one */
			      memset((char *)np2, 0, sizeof(struct node));
			      goto redo;
			    } else 
			      /* Back out, do not augment */
			      memcpy(np2, &oldnp2, sizeof(struct node));
			  } else
			    fatal("%s/%s; proto conflict; %s != %s [line %d]",path,de->d_name, np->proto, np2->proto, np->lineno);
			}
		}
		/* Ok to inherit here if we got here since the parent 
		   must have won anyhow */
		if (np->flag & flag_win)
		  np2->flag |= flag_win;
	}
	closedir(dp);
}


static int
traverse1(register struct node *np)
{
	register char *endp;
	register struct entry *ep;
	register int len;

	len = strlen(path);
	if (len == 0)
		strcpy(path,"/");
	check2(np);
	if (len == 0)
		strcpy(path,"");
	if ((np->mode & S_IFMT) != S_IFDIR)
		return;
	endp = path + len;
	*endp++ = '/';
	*endp = 0;
	for (ep = np->file.dirp; ep; ep = ep->nextp)
	{
		strcpy(endp,ep->name);
		traverse1(ep->nodep);
	}
	*--endp = 0;
}
