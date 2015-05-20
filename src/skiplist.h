#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <string.h>

typedef struct skiplist_ skiplist;

// For string  keys use 'strcmp' as the compare function.
// For integer keys use NULL as the compare function.

extern void sl_init(skiplist *d, int dups, int (*compare)(const char*, const char*));
extern int sl_set(skiplist *d, const char *key, void *value);
extern void *sl_rem(skiplist *d, const char *key);
extern void *sl_get(skiplist *d, const char *key);

extern void sl_start(skiplist *d);
extern void *sl_iter(skiplist *d);

extern void sl_done(skiplist *d, int delkey, void (*delval)(void *value));

#endif
