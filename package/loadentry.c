/*
 * package by Russell J. Yount (C) Copyright 10/12/86 by Carnegie Mellon
 * University
 */

#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "node.h"
#if defined(_IBMR2) || defined(__hpux) || defined(__linux__)
#include <sys/sysmacros.h>
#endif				/* defined(_IBMR2) || ...  */
#ifdef USES_SYSMKDEV
#include <sys/mkdev.h>
#endif

#ifdef _IBMR2
#define MAJOR_DEV   65535
#define MINOR_DEV   65535
#else
#define MAJOR_DEV   255		/* BSD style char major devs  */
#ifdef __hpux			/* hp/ux is special and has 3 byte minor devs */
#define MINOR_DEV 16777215
#else				/* __hpux */
#define MINOR_DEV 255		/* BSD style minor devs */
#endif				/* __hpux */
#endif				/* _IBMR2 */

extern char *strsav();
extern struct node *lookuppath();

struct type {
  unsigned int mode;
  unsigned int flag;
};

struct nodetype {
  char *name;
  unsigned int mode;
  unsigned int mask;
};

static struct nodetype nodetypes[]=
{
  "B", S_IFBLK, flag_win,
  "C", S_IFCHR, flag_win,
  "D", S_IFDIR, flag_abs | flag_clean | flag_copy | flag_big | flag_update | flag_copyattr | flag_win,
  "F", S_IFREG, flag_abs | flag_init | flag_old | flag_stat | flag_update | flag_win,
  "L", S_IFLNK, flag_abs | flag_init | flag_win,
  "N", 0, flag_no | flag_win,
  "P", S_IFIFO, flag_win,
  "S", S_IFSOCK, flag_win,
  "W", 0, 0,
  0, 0, 0
};

struct subtype {
  char *name;
  unsigned int flag;
};

static struct subtype subtypes[]=
{
  "A", flag_abs,
  "C", flag_copy,
  "I", flag_init,
  "O", flag_old,
  "Q", flag_stat,
  "R", flag_clean,
  "T", flag_copyattr,
  "U", flag_update,
  "X", flag_big,
  "W", flag_win,
  0, 0
};

int
lookuptype(char *name, struct type *tp)
{
  register struct nodetype *ntp;
  register struct subtype *stp;
  register char *flagbit;

  /* Need to come up with a faster way, but it works */
  for (ntp = nodetypes; ntp->name; ntp++) {
    if (strncmp(ntp->name, name, 1) == 0) {
      tp->mode = ntp->mode;
      tp->flag = flag_type;
      if (strlen(name) > 1) 
	for (flagbit = &name[1]; *flagbit; flagbit++) 
	  for (stp = subtypes; stp->name; stp++) 
	    if (strncmp(stp->name, flagbit, 1) == 0)
	      if (stp->flag & ntp->mask)
		tp->flag |= stp->flag;
	      else {
		message("lookuptype: flag %c invalid with type %c", flagbit[0], name[0]);
		return 1;
	      }
      return 0;
    }
  }
  return 1;
}

static int
lookupuser(register char *name, register uid_t *uidp)
{
  static struct passwd *pw = 0;

  if (strcmp(name, "root") == 0) {
    *uidp = 0;
    return 0;
  }
  if (pw == 0 || strcmp(pw->pw_name, name))
    pw = getpwnam(name);
  if (pw != 0) {
    *uidp = pw->pw_uid;
    return 0;
  }
  return -1;
}

static int
lookupgroup(register char *name, register gid_t *gidp)
{
  static struct group *gr = 0;

  if (strcmp(name, "wheel") == 0) {
    *gidp = 0;
    return 0;
  }
  if (gr == 0 || strcmp(gr->gr_name, name))
    gr = getgrnam(name);
  if (gr != 0) {
    *gidp = gr->gr_gid;
    return 0;
  }
  return -1;
}

int
loadentry(register int nfield, register char **field, int lineno)
{
  register struct node *np;
  register struct type *tp;
  struct type typ;
  char *path;

  tp = &typ;

  if (nfield < 1)
    return -1;
  if (lookuptype(*field, tp) != 0)
    fatal("%s; invalid type [line %d]", *field, lineno);
  field++, nfield--;
  if (nfield < 1)
    return -1;
  path = *field++, nfield--;
  if (*path != '/')
    fatal("%s; bad path [line %d]", path, lineno);
  if ((np = lookuppath(root, path, create)) == 0)
    fatal("%s; invalid path [line %d]", path, lineno);
  if ((np->flag & flag_type) && (np->mode & S_IFMT) != tp->mode) {
    if ((np->flag & flag_win) ^ (tp->flag & flag_win)) {
      if (np->flag & flag_win)
	return 0;
      else 
	memset((char *)np, 0, sizeof(struct node));
    } else
      fatal("loadentry: %s; type conflict with line %d [line %d]", path, np->lineno, lineno);
  }
  if ((np->flag & flag_proto) && ((np->flag & flag_abs) != 
				  (tp->flag & flag_abs))) {
    if ((np->flag & flag_win) ^ (tp->flag & flag_win)) {
      if (np->flag & flag_win)
	return 0;
      else 
	memset((char *)np, 0, sizeof(struct node));
    }
#if 0
else
      fatal("loadentry: %s; proto conflict; not abs [line %d]",path, np->lineno);
#endif
  }
  np->mode |= tp->mode;
  np->flag |= tp->flag;
  if (tp->mode == S_IFCHR || tp->mode == S_IFBLK) {
    int maj, min;

    if (nfield > 2
	&& ('0' <= field[0][0] && field[0][0] <= '9')
	&& ('0' <= field[1][0] && field[1][0] <= '9')) {
      if ((sscanf(*field, "%d", &maj) != 1)
	  || (0 > maj || maj > MAJOR_DEV))
	fatal("%s; bad major %s [line %d]", path, *field, lineno);
      field++, nfield--;
      if ((sscanf(*field, "%d", &min) != 1)
	  || (0 > min || min > MINOR_DEV))
	fatal("%s; bad minor %s [line %d]", path, *field, lineno);
      field++, nfield--;
      if (np->flag & flag_rdev)
	if (np->rdev != makedev(maj, min))
	  fatal("%s; mode conflict with line %d [line %d]", path, np->lineno, lineno);
      np->rdev = makedev(maj, min);
      np->flag |= flag_rdev;
    }
  } else if (nfield > 0 && **field == '/') {
    if (np->flag & flag_proto)
      /* Nothing I can do here. You lose, you lose. */
      if (strcmp(*field, np->proto))
	fatal("%s; proto conflict with line %d [line %d]", path, np->lineno, lineno);
    np->proto = strsav(*field);
    field++, nfield--;
    np->flag |= flag_proto;
  }
  if (nfield > 0 && 'a' <= **field && **field <= 'z') {
    uid_t uid;

    if (lookupuser(*field, &uid))
      fatal("%s; unknown user %s [line %d]", path, *field, lineno);
    if (np->flag & flag_uid)
      if (np->uid != uid)
	fatal("%s; uid conflict with line %d [line %d]", path, np->lineno, lineno);
    field++, nfield--;
    np->uid = uid;
    np->flag |= flag_uid;
  }
  if (nfield > 0 && 'a' <= **field && **field <= 'z') {
    gid_t gid;

    if (lookupgroup(*field, &gid)) {
      if (verbose)
	message("WARNING unknown group %s for %s", *field, path);
      gid = 0;
    }
    if (np->flag & flag_gid)
      if (np->gid != gid)
	fatal("%s; gid conflict with line %d [line %d]",
		path, np->lineno, lineno);
    field++, nfield--;
    np->gid = gid;
    np->flag |= flag_gid;
  }
  if (nfield > 0 && '0' <= **field && **field <= '7') {
    mode_t mod;
    char *modetype;

    if (sizeof(mode_t) == sizeof(short))
	modetype = "%ho";
    else if (sizeof(mode_t) == sizeof(long))
	modetype = "%lo";
    else fatal("panic: mode_t is of unknown size! [line %d]", np->lineno);

    if (sscanf(*field, modetype, &mod) != 1
	|| mod > (mode_t) ~S_IFMT)
	fatal("%s; bad mode %s [line %d]", path, *field, lineno);
    if (np->flag & flag_mode)
      if ((np->mode & ~S_IFMT) != mod)
	fatal("%s; mode conflict with line %d [line %d]", path, np->lineno, lineno);
    field++, nfield--;
    np->mode |= mod;
    np->flag |= flag_mode;
  }
#ifdef HAVE_MD5
  if (nfield > 0 && (np->flag & flag_type) && 
      ((np->mode & S_IFMT) == S_IFREG)) {
    int foo;
    foo = sscanf(*field, "%s", np->hash);
    if (foo != 1)
      fatal("%s; bad hash %s [line %d]", path, *field, lineno);
    field++, nfield--;
    np->flag |= flag_hash;
  }
#endif
  np->lineno = lineno;
  return nfield != 0;
}
