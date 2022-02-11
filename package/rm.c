/*
 *	package by Russell J. Yount
 *	(C) Copyright 10/12/86 by Carnegie Mellon University
 */

#include <sys/types.h>
#include <sys/stat.h>
#ifdef USES_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
#include <sys/param.h>
#include <errno.h>
#include "node.h"

extern char *strcpy();

int rm(path, noinhibit)
	register char *path;
        int noinhibit;
{
	register char *endp;
#ifdef USES_DIRENT
	register struct dirent *de;
#else
	register struct direct *de;
#endif
	register DIR *dp;
	struct stat stb;

	if (lstat(path,&stb) < 0)
	{
		/* message("lstat %s; %m",path); */
		return;
	}
	if (kflag && !noinhibit && (stb.st_mode & 0222) == 0)
	{
		if (!silent)
			message("INHIBIT %s removal",path);
		return;
	}
	if ((stb.st_mode & S_IFMT) != S_IFDIR)
	{
		if (!silent)
			message("rm %s",path);
		if (!lazy && unlink(path) < 0) {
		  if (errno == ETXTBSY) {
		    char busy[MAXPATHLEN];

		    message("rm %s ; %m",path);
		    /* if path is too long, truncate it */
		    if ((strlen(path) + 6) > MAXPATHLEN) { 
		      strncpy(busy,path,MAXPATHLEN-6);
		    } else {
		      strcpy(busy,path);
		    }
		    (void)strcat(busy,".busy");
		    mv(path,busy);
		  } else {
		    message("unlink %s; %m",path);
		    return;
		  }
		}
		return;
	}
	endp = path + strlen(path);
	if ((dp = opendir(path)) == 0)
	{
		message("opendir %s; %m",path);
		return;
	}
	*endp++ = '/';
	while ((de = readdir(dp)) != 0)
	{
		if (de->d_name[0] == '.')
		{
			if (de->d_name[1] == 0)
				continue;
			if (de->d_name[1] == '.' && de->d_name[2] == 0)
				continue;
		}
		(void)strcpy(endp,de->d_name);
		(void)rm(path, noinhibit);
	}
	*--endp = 0;
	(void)closedir(dp);
	if (!silent)
		message("rmdir %s",path);
	if (!lazy && rmdir(path) < 0)
	{
		message("rmdir %s; %m",path);
		return;
	}
	return;
}

