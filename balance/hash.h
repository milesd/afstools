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
#ifndef INCLUDE_hash_h
#define INCLUDE_hash_h

union hashtable_key {
    char *s;
    void *addr;
};

struct hashbucket {
    void *elt;
    union hashtable_key key;
    struct hashbucket *next;
};

struct hashtable {
    struct hashbucket **buckets;
    unsigned long size;
    unsigned long (*keyfun)(struct hashtable *, union hashtable_key);
    unsigned (*cmpfun)(union hashtable_key, union hashtable_key);
    union hashtable_key (*dupkey)(union hashtable_key);
    void (*zapkey)(union hashtable_key);
};

extern struct hashtable *hashtable_create(unsigned long (*)(struct hashtable *, union hashtable_key),
					  unsigned (*)(union hashtable_key, union hashtable_key),
					  union hashtable_key (*)(union hashtable_key),
					  void (*)(union hashtable_key));
extern void *hashtable_find(struct hashtable *, union hashtable_key);
extern void *hashtable_update(struct hashtable *, union hashtable_key, void *);
extern void *hashtable_add(struct hashtable *, union hashtable_key, void *);
extern void *hashtable_del(struct hashtable *, union hashtable_key);
extern void hashtable_clean(struct hashtable *);
extern void hashtable_forall(struct hashtable *, void (*)(void *));
extern void hashtable_stats(struct hashtable *);

/* these are library-supplied hashing functions */

/* hash a string */
extern unsigned long hashfun_string(struct hashtable *, union hashtable_key);
extern unsigned hashcmp_string(union hashtable_key, union hashtable_key);
extern union hashtable_key hashdup_string(union hashtable_key);
extern void hashzap_string(union hashtable_key);

/* hash an address */
extern unsigned long hashfun_addr(struct hashtable *, union hashtable_key);
extern unsigned hashcmp_addr(union hashtable_key, union hashtable_key);
extern union hashtable_key hashdup_addr(union hashtable_key);
extern void hashzap_addr(union hashtable_key);

#endif /* INCLUDE_hash_h */
