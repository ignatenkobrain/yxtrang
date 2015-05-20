#ifndef SKIPLIST_H
#define SKIPLIST_H

typedef struct skiplist_ skiplist;

extern void sb_init(skiplist *d, int dups);
extern int sb_set(skiplist *d, const char *key, void *value);
extern void *sb_rem(skiplist *d, const char *key);
extern void *sb_get(skiplist *d, const char *key);

extern void sb_start(skiplist *d);
extern void *sb_iter(skiplist *d);

extern void sb_done(skiplist *d, int delkey, void (*delval)(void *value));

#endif
