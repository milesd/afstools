/* This file is part of the balancer
 * Author: Dan Lovinger, del+@cmu.edu
 */

/*
 *        Copyright 1993 by Carnegie Mellon University
 * 
 *                    All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of CMU not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
#ifndef INCLUDE_balance_h
#define INCLUDE_balance_h
#include "xmalloc.h"
#include "match.h"

/* Transarc ... thank you. To shut up rx/rx_user.h */
#define _ANSI_C_SOURCE 1

/* magic number == max # of partitions on a FS */
#define MAXPARTID 26

#if BYWEEKUSE
/* sanity bounds on weekuse numbers - rare corrupted values have been seen 
 * 100 million should be *well* above what anyone could reasonably see
 */
#define MAXWEEKUSE 100000000
#endif

/* default location of the vos utility */
#define VOS "/usr/local/etc/vos"

struct volume {
    char *name;               /* name */
    unsigned long volid;      /* volume id */
    unsigned long size,       /* usage */
                  maxquota;   /* quota */
#if BYWEEKUSE
    unsigned long weekuse;    /* weekuse */
#endif
    int ntimes;               /* number of times it has moved */
    int bkexist;              /* boolean if backup exists */
    struct server *shome;     /* what server */
    struct partition *phome;  /* what partition */
    struct volume *next;
};

struct server {
    struct host *who;                /* who the server is */
    int locked;                      /* lock flag for transactions */
    struct partition *parts;         /* LL of parts */
    struct move_request *to, *from;  /* LL of move_requests (thread by server) */
    struct server *next;
};

struct partition {
    struct server *who;              /* what server */
    int locked;                      /* lock flag so we get move dependency correct */
    struct volume *vols;             /* LL of volumes */
    long pid,                        /* partition ID */
       free, size;                   /* space usage */
#if BYWEEKUSE
    unsigned long weekuse;           /* weekuse */
#endif
#if BYOVERDRAFT
    unsigned long maxquota;   /* quota used */
#endif
    unsigned long nvols;             /* number of volumes */
    struct glob *inplus, *inminus,   /* LLs of plus and minus globs vol must match to move in */
                *outplus, *outminus; /* LLs of plus and minus globs vol must match to move out */
    struct move_request *to, *from;  /* LL of move_requests (thread by partition */
    struct partition *next;
};

struct move_request {
    struct volume *vol;                  /* volume to move */
    struct server *sfrom, *sto;          /* src/dest servers */
    struct partition *pfrom, *pto;       /* src/dest partitions */
    int fd;                              /* fd associated with this transaction */
    int state;                           /* state of transaction */
    struct move_request *fsnext,         /* by from server */ 
                        *fpnext,         /* by from partition */
                        *tsnext,         /* by to server */
                        *tpnext,         /* by to partition */
                        *next;           /* next move request in serial transaction chain */
};

struct transaction {
    int pid;                             /* pid of transaction */
    struct move_request *mr;             /* move request this transaction is satisfying */
};

struct agent {
    char *id;                                      /* agent id (string) */
    struct agent_procs *(*agent_doargs)(char *);   /* pass in arguments from config */
    struct agent_procs *procs;                     /* proc list */
};  

struct agent_procs {
    void (*agent_servconf)(struct server *);          /* accept initial server configuration */
    struct move_request *(*agent_getrequest)();       /* get a new move_request from agent */
    int (*agent_queryrequest)(struct move_request *); /* ask if a move_request is ok */
    void (*agent_setrequest)(struct move_request *);  /* notify of a transaction */
    void (*agent_discardv)(struct volume *);          /* request to discard volume from further consideration */
    void (*agent_report)();                           /* print report about view of the world */
};

extern char *progname;

extern int ntransactions;
extern int nvoltrans;
extern int nprocs;
extern int nsecs;
extern int switchflags;
extern int useserverauth;
extern char *command;
extern char *cellname;
extern struct glob *size_is_quota;
#if BYOVERDRAFT
extern struct glob *overdraft_use_quota;
#endif

#define SWITCH_NTRANSACTIONS   0x01
#define SWITCH_NVOLTRANS       0x02
#define SWITCH_NPROCS          0x04
#define SWITCH_NSECS           0x08

extern int initialize_agent(char *, char *);
#endif /* INCLUDE_balance_h */
