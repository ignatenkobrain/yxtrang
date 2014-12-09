#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <string.h>

typedef struct _skiplist* skiplist;

// Makes internal copies. Returned 'value' points to it.

extern skiplist sl_create(int (*compare)(const void*, const void*), void* (*copykey)(const void*), void (*freekey)(void*));

// Make own copy if needed for long-term use.

extern skiplist sl_create2(int (*compare)(const void*, const void*), void* (*copykey)(const void*), void (*freekey)(void*), void* (*copyval)(const void*), void (*freeval)(void*));

extern int sl_add(skiplist s, const void* key, const void* value);
extern int sl_rem(skiplist s, const void* key);
extern int sl_get(const skiplist s, const void* key, const void** value);
extern unsigned long sl_count(const skiplist s);

// Iterate over the whole range. f returns -1, to delete, <= 0 to halt, 1 to continue.

extern void sl_iter(skiplist s, int (*f)(void*, void*, void*), void* arg);

// Iterate over >= key. f returns -1 to delete, <= 0 to halt, 1 to continue.

extern void sl_find(skiplist s, const void* key, int (*f)(void*, void*, void*), void* arg);

extern void sl_destroy(skiplist s);


#endif
