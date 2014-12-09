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

// Iterate over the whole range. f returns zero to halt.

extern void sl_iter(const skiplist s, int (*f)(void*, void*, void*), void* arg);

// Iterate over >= key. f returns zero to halt.

extern void sl_find(const skiplist s, const void* key, int (*f)(void*, void*, void*), void* arg);

// This is quite inefficient as it involves a linear search.

extern int sl_erase(skiplist s, const void* value, int (*compare)(const void*, const void*));

extern void sl_destroy(skiplist s);


#endif
