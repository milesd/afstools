/*
 *	package by Russell J. Yount
 *	(C) Copyright 10/12/86 by Carnegie Mellon University
 */

#include <sys/types.h>

#define	noop 0
#define	create 1
#define delete 2

#define flag_null       0000000
#define	flag_type	0000001
#define	flag_proto	0000002
#define	flag_rdev	0000004
#define	flag_uid	0000010
#define	flag_gid	0000020
#define	flag_mode	0000040
#define	flag_mtime	0000100

#define	flag_clean	0000200	/* remove unspecified files */
#define	flag_old	0000400	/* rename old version with .old */
#define	flag_big	0001000	/* for lost+found directories */
#define	flag_init	0002000	/* initalize only */
#define	flag_abs	0004000	/* flag absolute */
#define flag_stat	0010000	/* reboot if modified */
#define flag_copy       0020000  /* copy files instead of links */
#define flag_no         0040000  /* leave it the hell alone */
#define flag_update     0100000  /* what do you mean inhibited?? */
#define flag_hash       0200000  /* compare with known hash of this file */
#define flag_copyattr   0400000  /* use modemask and owner from top directory
                                    (for D with src) */
#define flag_win        01000000  /* allow this to override another entry */

#define	status_noerror	0
#define	status_error	2
#define	status_reboot	4

struct entry
{
	struct entry	*nextp;
	struct node	*nodep;
	int	        hash;
	char		name[1];
};

struct node
{
	unsigned int	flag;
	unsigned int	mode;
	unsigned int	modemask;
	uid_t	uid;
	gid_t	gid;
	dev_t   rdev;
	time_t	mtime;
	off_t   size;
	char	*proto;
        unsigned int lineno;
        union
	{
		struct entry *dirp;
	} file;
	unsigned char hash[32];
};

extern struct node *root;
extern int lazy;
extern int verbose;
extern int silent;
extern int status;
extern int re_boot;
extern int kflag;

