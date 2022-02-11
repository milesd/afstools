%{
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

#include <stdio.h>

#include "balance.h"
#include "afscall.h"
#include "balutil.h"
#include "hash.h"

#include <afs/volint.h>

struct globs {
    struct glob *pos, *neg;
};

static int fatal;
char *infile;
static struct server **cellp;
static struct hashtable *srv_ht = NULL;       /* map of server name -> struct server */
static struct hashtable *srvpartht_ht = NULL; /* map of server name -> hashtable of partitions
					       * for that server */
static union hashtable_key key;

extern int lineno;

static struct server *newserver(char *, char *, struct globs *, struct globs *);
static struct glob *addglob(struct glob **, char *);
int setcell(char *);

#define CHECKCELL if (!setcell(NULL)) \
		     fail(FAIL_EXIT, "%s line %d: cell must be declared before this line\n", infile, lineno)
%}

%union {
    char *string;
    struct server *s;
    struct glob *g;
    struct ioglobs *iog;
    struct globs *gs;
}

%token FS AGENT RUNTIME PROCS TRANSACTIONS VOLTRANSACTIONS CELL
%token <string> STRING AGENTNAME AGENTARGS UQUOTA OUQUOTA

%type <g> globlist plusglob minusglob
%type <gs> inglobs outglobs

%start config

%%

config:    line 
  |        line config
;

line:      fsline
  |        agentline
  |        uquotaline
  |        overduquotaline
  |        runtimeline
  |        procsline
  |        transactionsline
  |        voltransactionsline
  |        cellline
;

cellline: CELL STRING ';' {
    if (setcell($2)) {
	fail(FAIL_EXIT, "%s line %d: cell name redeclared\n", infile, lineno);
    }
    cellname = $2;
}

agentline: AGENT AGENTNAME AGENTARGS ';' {
    if (initialize_agent($2, $3)) {
	fatal++;
	fail(FAIL_NORM, "could not initialize agent %s -- does it exist?\n", $2);
    }
    free($2);
    free($3); 
  }
  | AGENT AGENTNAME ';' {
    if (initialize_agent($2, NULL)) {
	fatal++;
	fail(FAIL_NORM, "could not initialize agent %s -- does it exist?\n", $2);
    }
    free($2); 
  }
;

fsline: FS STRING STRING inglobs outglobs ';' {
      CHECKCELL;
      newserver($2, $3, $4, $5);
      free($2);
      free($3);
      if ($4) free($4);
      if ($5) free($5);
  }
  | FS STRING STRING outglobs inglobs ';' {
      CHECKCELL;
      newserver($2, $3, $5, $4);
      free($2);
      free($3);
      if ($4) free($4);
      if ($5) free($5);
  }
  | FS STRING STRING outglobs ';' {
      CHECKCELL;
      newserver($2, $3, NULL, $4);
      free($2);
      free($3);
      if ($4) free($4);
  }
  | FS STRING STRING inglobs ';' {
      CHECKCELL;
      newserver($2, $3, $4, NULL);
      free($2);
      free($3);
      if ($4) free($4);
  }
  | FS STRING STRING ';' {
      CHECKCELL;
      newserver($2, $3, NULL, NULL);
      free($2);
      free($3);
  }
;

inglobs: '<' plusglob minusglob {
      $$ = (struct globs *) xmalloc(sizeof(struct globs));
      $$->pos = $2;
      $$->neg = $3;
  }
;

outglobs: '>' plusglob minusglob {
      $$ = (struct globs *) xmalloc(sizeof(struct globs));
      $$->pos = $2;
      $$->neg = $3;
  }
;

plusglob: { $$ = (struct glob *) NULL; }
  | '+' globlist {
      $$ = $2;
  }
;

minusglob: { $$ = (struct glob *) NULL; }
  | '-' globlist  {
      $$ = $2;
  }
;

globlist: globlist STRING {
      $$ = addglob(&$1, $2);
  }
  | STRING {
      $$ = addglob((struct glob **) NULL, $1);
  }
;
uquotaline: UQUOTA globlist {
  if ($2) {
    if (size_is_quota) {
      struct glob *tmp;
      tmp=$2;
      while ( tmp->next)
        tmp=tmp->next;
      tmp->next=size_is_quota;
    }
    size_is_quota = $2;
  }
};

overduquotaline: OUQUOTA globlist {
#if BYOVERDRAFT
  if ($2) {
    if (overdraft_use_quota) {
      struct glob *tmp;
      tmp=$2;
      while ( tmp->next)
        tmp=tmp->next;
      tmp->next=overdraft_use_quota;
    }
    overdraft_use_quota = $2;
  }
#else
  fatal++;
  fail(FAIL_NORM, "Used over_use_quota in config, but overdraft agent not configured\n");
#endif
};

runtimeline: RUNTIME '=' STRING ';' {
    if (! (switchflags & SWITCH_NSECS))
      nsecs = parsetime($3);
    if (nsecs < 0) {
	fatal++;
	fail(FAIL_NORM, "time '%s' unparseable in config\n", $3);
    }
    free($3);
}

procsline: PROCS '=' STRING ';' {
    if (! (switchflags & SWITCH_NPROCS))
      nprocs = atoi($3);
    free($3);
}

transactionsline: TRANSACTIONS '=' STRING ';' {
    if (! (switchflags & SWITCH_NTRANSACTIONS))
      ntransactions = atoi($3);
    free($3);
}

voltransactionsline: VOLTRANSACTIONS '=' STRING ';' {
    if (! (switchflags & SWITCH_NVOLTRANS))
      nvoltrans = atoi($3);
    free($3);
}

%%

void get_config(cf, cp)
char *cf;
struct server **cp;
{
    extern FILE *yyin;

    yyin = fopen(cf, "r");
    if (yyin == NULL)
      fail(FAIL_EXIT, "cannot open config file %s\n", cf);

    infile = cf;
    cellp = cp;

    yyparse();

    if (fatal)
      fail(FAIL_EXIT, "errors encountered in config, exiting\n");

    hashtable_clean(srv_ht);
    hashtable_forall(srvpartht_ht, (void (*)(void *))hashtable_clean); /* clean out the hashed hashtables */
    hashtable_clean(srvpartht_ht);
}

static struct server *newserver(name, parts, ing, outg)
char *name, *parts;
struct globs *ing, *outg;
{
    char *c;
    struct pIDs *pidl;
    struct partition *p;
    struct server *s;
    struct host *hent;
    struct hashtable *pht;

    hent = get_host(name);
    if (hent == NULL) {
	fail(FAIL_NORM, "unknown host %s in %s\n", name, infile);
	fatal = 1;

	return (struct server *) NULL;
    }

    if (srv_ht == NULL) srv_ht = hashtable_create(hashfun_string, hashcmp_string,
						  hashdup_string, hashzap_string);

    if (! (s = (struct server *) hashtable_find(srv_ht, (key.s = hent->name, key)))) {
	s = (struct server *) xmalloc(sizeof(struct server));
	s->who = hent;

#ifdef DEBUG
	printf("adding server %s (%x) with parts %s\n", s->who->name, s->who->addr, parts);
#endif

	/* thread onto server list */
	s->next = *cellp;
	*cellp = s;

	hashtable_add(srv_ht, (key.s = hent->name, key), s);
    } else
      free(hent);

    pidl = get_partitions(s->who);

    for (c = parts; *c != '\0'; c++) {
	if (pidl->partIds[*c - 'a'] == -1) {
	    fail(FAIL_NORM, "%s line %d: %s doesn't have /vicep%c\n", infile, lineno, name, *c);
	    fatal = 1;
	    continue;
	}

      	if (srvpartht_ht == NULL) srvpartht_ht = hashtable_create(hashfun_addr, hashcmp_addr,
								  hashdup_addr, hashzap_addr);

	/* look for the per-server hashtable of initialized partitions */
	if (! (pht = (struct hashtable *) hashtable_find(srvpartht_ht, (key.addr = s, key)))) {
	    pht =  hashtable_create(hashfun_addr, hashcmp_addr,
				    hashdup_addr, hashzap_addr);

	    /* and add it in */
	    hashtable_add(srvpartht_ht, (key.addr = s, key), pht);
	}

	/* look for this partition id in the per-server hashtable of partitions (by pid) */
	if (hashtable_find(pht, (key.addr = (void *) (*c - 'a'), key)) != NULL) {
	    /* user is trying to initialize this one multiple times */
	    fatal++;
	    fail(FAIL_NORM, "%s line %d: partition /vicep%c of %s redeclared\n", infile, lineno,
		 *c, s->who->name);
	    continue;
	}

	p = (struct partition *) xmalloc(sizeof(struct partition));

#ifdef DEBUG
	printf("adding /vicep%c\n", *c);
#endif
	/* thread onto partition list */
	p->next = s->parts;
	s->parts = p;
	      
	p->who = s;
	p->pid = *c - 'a';

	/* cache it away so we can detect duplicates */
	hashtable_add(pht, (key.addr = (void *) p->pid, key), p);

	if (ing) {
#ifdef DEBUG
	    {
		struct glob *g;
		
		printf("\tinbound pos globs: ");
		for (g = ing->pos; g; g = g->next)
		  printf("%s ", g->globv);
		puts("");
		printf("\tinbound neg globs: ");
		for (g = ing->neg; g; g = g->next)
		  printf("%s ", g->globv);
		puts("");
	    }
#endif
	    p->inplus = ing->pos;
	    p->inminus = ing->neg;
	}
	if (outg) {
#ifdef DEBUG
	    {
		struct glob *g;
		
		printf("\toutbound pos globs: ");
		for (g = outg->pos; g; g = g->next)
		  printf("%s ", g->globv);
		puts("");
		printf("\toutbound neg globs: ");
		for (g = outg->neg; g; g = g->next)
		  printf("%s ", g->globv);
		puts("");
	    }
#endif
	    p->outplus = outg->pos;
	    p->outminus = outg->neg;
	}
    }

    return s;
}

static struct glob *addglob(list, s)
struct glob **list;
char *s;
{
    struct glob *ng = (struct glob *) xmalloc(sizeof(struct glob));

#ifdef DEBUG
    printf("adding glob %s\n", s);
#endif

    ng->globv = s;
    if (list) {
	ng->next = *list;
	*list = ng;
    }

    return ng;
}

/* set cell exactly once - return boolean true if already set. calling with NULL cellname has
 * the effect of querying the set state
 */
int setcell(cellname)
char *cellname;
{
    static int set = 0;

    /* don't set things twice ... and allow query as to whether we have been set */
    if (set || cellname == NULL)
      return set;

    initialize_afs_guts(cellname);
    set = 1;

    return 0;
}
