/*
 * package by Russell J. Yount (C) Copyright 10/12/86 by Carnegie Mellon
 * University
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/file.h>
#ifdef USES_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
#ifdef USES_UTIME
#include <utime.h>
#endif				/* USES_UTIME */
#include "node.h"
#ifdef USES_SYSMKDEV
#include <sys/mkdev.h>
#endif
#ifdef USES_SYSMACROS
#include <sys/sysmacros.h>
#endif
#ifdef HAVE_MD5
#include <stdio.h>
#include <md5global.h>
#include <md5.h>
#endif

extern struct node *lookupindir();

static struct stat stb;
static char path[MAXPATHLEN];

int
update()
{
  strcpy(path, "");
  traverse2(root);
}

static int
update2(register struct node *np)
{
  if ((np->flag & flag_no) != 0)
    return;
  switch (np->mode & S_IFMT) {
  case S_IFSOCK:
    updatesocket(np);
    break;
  case S_IFCHR:
  case S_IFBLK:
    updatespec(np);
    break;
  case S_IFLNK:
    updatelink(np);
    break;
  case S_IFDIR:
    updatedir(np);
    break;
  case S_IFREG:
    updatereg(np);
    break;
  case S_IFIFO:
    updatepipe(np);
    break;
  }
}

static int
traverse2(register struct node *np)
{
  register char *endp;
  register struct entry *ep;
  register int len;

  len = strlen(path);
  if (len == 0)
    strcpy(path, "/");
  update2(np);
  if (len == 0)
    strcpy(path, "");
  if ((np->mode & S_IFMT) != S_IFDIR)
    return;
  endp = path + len;
  *endp++ = '/';
  *endp = 0;
  for (ep = np->file.dirp; ep; ep = ep->nextp) {
    strcpy(endp, ep->name);
    traverse2(ep->nodep);
  }
  *--endp = 0;
}

static int
updatesocket(register struct node *np)
{
  (void) dochtyp(np);
}

static int
updatepipe(register struct node *np)
{
  (void) dochtyp(np);
}

static int
updatespec(register struct node *np)
{
  register int ret;

  ret = dochtyp(np);
  if (ret == 1)
    return;
  if ((np->flag & flag_rdev) != 0) {
    if (ret >= 0) {
      if (np->rdev != stb.st_rdev) {
	rm(path, 0);
	ret = -1;
      }
    }
    if (ret < 0) {
      char *type;
      type = ((np->mode & S_IFMT) == S_IFBLK) ? "b" : "c";
      if (!silent)
	message("mknod %s %d %d %s",
		type, major(np->rdev), minor(np->rdev), path);
      if (!lazy && mknod(path, (int) np->mode, (int) np->rdev) < 0)
	message("mknod %s %d %d %s; %m",
		type, major(np->rdev), minor(np->rdev), path);
      if (!lazy && (ret = lstat(path, &stb)) < 0)
	message("lstat %s; %m", path);
    }
  }
  if (ret >= 0) {
    dochmod(np);
    dochown(np);
  }
}


static int
updatelink(register struct node *np)
{
  register int ret;
  char temp[MAXPATHLEN], temp2[MAXPATHLEN];
  int cc;

  ret = dochtyp(np);
  if (ret == 1)
    return;
  if ((np->flag & flag_proto) == 0)
    return;
  if (np->flag & flag_abs)
    sprintf(temp, "%s", np->proto);
  else
    sprintf(temp, "%s%s", np->proto, path);
  if (ret >= 0) {
    if ((np->flag & flag_init) != 0) 
	return;
    if ((cc = readlink(path, temp2, sizeof(temp2) - 1)) < 0) {
      message("readlink %s; %m", path);
      return;
    }
    temp2[cc] = 0;
    if (strcmp(temp2, temp)) {
      rm(path, 0);
      ret = -1;
    }
  }
  if (ret < 0) {
    if (!silent)
      message("ln %s %s", path, temp);
    if (!lazy && symlink(temp, path) < 0)
      message("symlink %s %s; %m", temp, path);
  }
}


static int
updatedir(register struct node *np)
{
  register int ret;

  ret = dochtyp(np);
  if (ret == 1)
    return;
  if (ret < 0) {
    if (!silent)
      message("mkdir %s", path);
    if (!lazy && mkdir(path, (int) np->mode & ~S_IFMT) < 0)
      message("mkdir %s; %m", path);
    if (!lazy && (ret = lstat(path, &stb)) < 0)
      message("lstat %s; %m", path);
  }
  if (np->flag & flag_big)
    (void) fixbigdir(np);
  if (np->flag & flag_clean)
    (void) fixdir(np);
  if (ret >= 0) {
    dochmod(np);
    dochown(np);
  }
}

#ifdef HAVE_MD5
static int
  gethash(char *filename, unsigned char *md)
{
  FILE *file;
  MD5_CTX ctx;
  int len;
  unsigned int tmp;
  unsigned char buf[1024];
  unsigned char digest[16];

  if ((file = fopen (filename, "rb")) == NULL) {
    md = (unsigned char *)0;
    return 0;
  }
  else {
    MD5Init(&ctx);
    while (len = fread (buf, 1, 1024, file))
      MD5Update (&ctx, buf, len);
    MD5Final (digest, &ctx);
    fclose (file);

    for (tmp = 0; tmp < 16; tmp++) 
      sprintf(&buf[tmp*2], "%02x", digest[tmp]);

    memcpy(md, buf, 32);

    return 1;
  }
}
#endif

static int
updatereg(register struct node *np)
{
  register int ret;

  ret = dochtyp(np);
  if (ret == 1)
    return;
  if ((np->flag & flag_proto) != 0) {
    if (ret < 0)
      np->flag &= ~flag_old;
    if (ret >= 0) {
      if ((np->flag & flag_init) == 0) {
	if (np->mtime != stb.st_mtime)
	  ret = -1;
	if ((ret != -1) && (np->size != stb.st_size)) {
	  ret = -1;
	}
#ifdef HAVE_MD5
	if ((ret != -1) && (np->flag & flag_hash)) {
	  unsigned char thishash[32];
	  gethash(path, thishash);
	  if (strncmp(np->hash, thishash, 32)) {
	    ret = -1;
	  }
	}
#endif
      }
    }
    if (ret < 0) {
      if ((ret = fixreg(np)) >= 0)
	ret = lstat(path, &stb);
      if (ret >= 0)
	dochtim(np);
    }
  }
  if (ret >= 0) {
    dochmod(np);
    dochown(np);
  }
}

static int
dochtyp(register struct node *np)
{
  if (lstat(path, &stb) < 0)
    return -1;
  if ((kflag && (stb.st_mode & 0222) == 0) && (np->flag & flag_update == 0)) {
    if (!silent)
      message("INHIBIT %s updating", path);
    return 1;
  }
  if ((stb.st_mode & S_IFMT) == (np->mode & S_IFMT))
    return 0;
  rm(path, (np->flag & flag_update != 0));
  return -1;
}

static int
dochmod(register struct node *np)
{
  if ((np->flag & flag_mode) == 0)
    return;
  if ((np->mode & ~S_IFMT) == (stb.st_mode & ~S_IFMT))
    return;
  if (!silent)
    message("chmod %s %o", path, np->mode & ~S_IFMT);
  if (!lazy && chmod(path, (int) np->mode & ~S_IFMT) < 0)
    message("chmod %s; %m", path);
}

static int
dochown(register struct node *np)
{
  if ((np->flag & flag_uid) == 0)
    np->uid = stb.st_uid;
  if ((np->flag & flag_gid) == 0)
    np->gid = stb.st_gid;
  if (np->uid == stb.st_uid && np->gid == stb.st_gid)
    return;
  if (!silent)
    message("chown %s %d %d", path, np->uid, np->gid);
  if (!lazy && chown(path, np->uid, np->gid) < 0)
    message("chown %s; %m", path);
}

static int
dochtim(register struct node *np)
{
  struct timeval tm[2];
#ifdef USES_UTIME
  struct utimbuf ut;
#endif				/* USES_UTIME */

  if (np->mtime == stb.st_mtime)
    return;
  tm[0].tv_sec = tm[1].tv_sec = np->mtime;
  tm[0].tv_usec = tm[1].tv_usec = 0;
  if (!silent) {
    char *date;
    date = ctime((time_t *) & np->mtime);
    date[24] = 0;
#ifdef USES_UTIME
    message("utime %s [%s]", path, date);
  }
  ut.modtime = ut.actime = tm[0].tv_sec + tm[0].tv_usec;
  if (!lazy && utime(path, &ut) < 0)
       message("utime %s; %m", path);

#else				/* USES_UTIME */
    message("utimes %s [%s]", path, date);
  }
  if (!lazy && utimes(path, tm) < 0)
       message("utimes %s; %m", path);

#endif				/* USES_UTIME */
}

static int
fixbigdir(struct node *np)
{
  if (stb.st_size >= 3584)
    return 0;
  return mklostfound(path);
}

static int
fixdir(register struct node *np)
{
  register DIR *dp;
#ifdef USES_DIRENT
  register struct dirent *de;
#else
  register struct direct *de;
#endif
  register char *endp;

  if (verbose)
    message("cleandir %s", path);
  if ((dp = opendir(path)) == 0) {
    message("opendir %s; %m", path);
    return -1;
  }
  endp = path + strlen(path);
  *endp++ = '/';
  while ((de = readdir(dp)) != 0) {
    if (de->d_name[0] == '.') {
      if (de->d_name[1] == 0)
	continue;
      if (de->d_name[1] == '.' && de->d_name[2] == 0)
	continue;
    }
    if (lookupindir(np, de->d_name, noop) != 0)
      continue;
    strcpy(endp, de->d_name);
    rm(path, (np->flag & flag_update != 0));
  }
  *--endp = 0;
  closedir(dp);
  return 0;
}

static int
fixreg(register struct node *np)
{
  char new[MAXPATHLEN], old[MAXPATHLEN], temp[MAXPATHLEN];

  sprintf(new, "%s.new", path);
  if (np->flag & flag_abs)
    sprintf(temp, "%s", np->proto);
  else
    sprintf(temp, "%s%s", np->proto, path);
  if (cp(temp, new))
    return -1;
  if (np->flag & flag_old) {
    sprintf(old, "%s.old", path);
    (void) rm(old, 0);
    (void) ln(path, old);
  }
  if (mv(new, path))
    return -1;
  if (np->flag & flag_stat)
    status = status_reboot;
  return 0;
}
