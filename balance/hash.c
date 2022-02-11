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
#include <string.h>

#include "hash.h"
#include "xmalloc.h"
#include "balutil.h"

#define HASH_DEFAULT_SIZE (unsigned long)128

/******************
 * String Hashing *
 ******************/

unsigned long hashfun_string(ht, key)
struct hashtable *ht;
union hashtable_key key;
{
    unsigned long hash = 0;

    /* a cheesy default */
    for(; *key.s; key.s++)
      hash += (int)*key.s;

    return (unsigned long)(hash % ht->size);
}

unsigned hashcmp_string(val1, val2)
union hashtable_key val1, val2;
{
    return !strcmp(val1.s, val2.s);
}

union hashtable_key hashdup_string(val)
union hashtable_key val;
{
    union hashtable_key k;

    k.s = strsav(val.s);
    return k;
}

void hashzap_string(val)
union hashtable_key val;
{
    free(val.s);
}

/*******************
 * Address Hashing *
 *******************/

unsigned long hashfun_addr(ht, key)
struct hashtable *ht;
union hashtable_key key;
{
    /* throw away the bottom 3 bits because most structures will
     * be aligned in the lower 3 bits, i.e. lower order 3 bits
     * are not sufficiently random for us.
     */
    return (unsigned long)(((unsigned long)key.addr >> 3) % ht->size);
}

unsigned hashcmp_addr(val1, val2)
union hashtable_key val1, val2;
{
    return val1.addr == val2.addr;
}

union hashtable_key hashdup_addr(val)
union hashtable_key val;
{
    union hashtable_key k;

    /* this can't quite be a NOOP */
    k.addr = val.addr;
    return k;
}

void hashzap_addr(val)
union hashtable_key val;
{
    return;
}

/**************************************
 * end of type-hashing specific stuff *
 **************************************/

struct hashtable *hashtable_create(keyfn, cmpfn, dupfn, zapfn)
unsigned long (*keyfn)(struct hashtable *, union hashtable_key);
unsigned (*cmpfn)(union hashtable_key, union hashtable_key);
union hashtable_key (*dupfn)(union hashtable_key);
void (*zapfn)(union hashtable_key);
{
    struct hashtable *nt = (struct hashtable *) xmalloc(sizeof(struct hashtable));

    nt->size = HASH_DEFAULT_SIZE;
    nt->buckets = 
      (struct hashbucket **) xmalloc(nt->size*sizeof(struct hashbucket *));
    nt->keyfun = keyfn;
    nt->cmpfun = cmpfn;
    nt->dupkey = dupfn;
    nt->zapkey = zapfn;

    return nt;
}

void *hashtable_find(ht, key)
struct hashtable *ht;
union hashtable_key key;
{
    struct hashbucket *b = ht->buckets[(*ht->keyfun)(ht, key)];

    while(b) {
	if ((*ht->cmpfun)(key, b->key)) return b->elt;
	b = b->next;
    }

    return (void *) NULL;
}

void *hashtable_update(ht, key, e)
struct hashtable *ht;
union hashtable_key key;
void *e;
{
    struct hashbucket *b = ht->buckets[(*ht->keyfun)(ht, key)];

    while(b) {
	if ((*ht->cmpfun)(key, b->key)) {
	    /* update datum element */
	    b->elt = e;
	    return e;
	}
	b = b->next;
    }

    return (void *) NULL;
}

void *hashtable_add(ht, key, e)
struct hashtable *ht;
union hashtable_key key;
void *e;
{
    struct hashbucket **blist = &ht->buckets[(*ht->keyfun)(ht, key)];
    struct hashbucket *nb = (struct hashbucket *) xmalloc(sizeof(struct hashbucket));

    /* no dynamic resize yet ... */
    /* we also don't care if the key is already in the table - leave
     * that to the caller to make sure about.
     */
    nb->elt = e;
    nb->key = (*ht->dupkey)(key); /* key contents must *not* get modified out from under us */
    nb->next = *blist;
    *blist = nb;

    return e;
}

void *hashtable_del(ht, key)
struct hashtable *ht;
union hashtable_key key;
{
    struct hashbucket **b = &ht->buckets[(*ht->keyfun)(ht, key)];
    struct hashbucket *t;
    void *e;

    while(*b) {
	if ((*ht->cmpfun)(key, (*b)->key)) {
	    t = *b;
	    e = (*b)->elt;
	    *b = (*b)->next;
	    free(t);
	    return e;
	}

	b = &(*b)->next;
    }

    return (void *) NULL;
}

void hashtable_clean(ht)
struct hashtable *ht;
{
    int i;
    struct hashbucket *b, *n;

    for(i = 0; i < ht->size; i++) {
	b = ht->buckets[i];

	/* chase the list carefully */
	while(b) {
	    n = b->next;
	    (*ht->zapkey)(b->key);
	    free(b);
	    b = n;
	}
    }

    free(ht->buckets);
    free(ht);
}

void hashtable_forall(ht, proc)
struct hashtable *ht;
void (*proc)(void *);
{
    int i;
    struct hashbucket *b;

    for(i = 0; i < ht->size; i++) {
	b = ht->buckets[i];

	/* chase the list */
	while(b) {
	    (*proc)(b->elt);
	    b = b->next;
	}
    }
}

void hashtable_stats(ht)
struct hashtable *ht;
{
    int i, j;
    struct hashbucket *b;

    puts("-------------");

    for (i = 0; i < ht->size; i++) {
	b = ht->buckets[i];
	
	j = 0;
	while(b) {
	    j++;
#ifdef DEBUG
	    printf("elt %x\n", b->elt);
#endif
	    b = b->next;
	}

	printf("hashtable: slot %u has %u elt\n", i, j);
    }
}
