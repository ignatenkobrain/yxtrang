#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <string.h>

typedef struct skiplist_* skiplist;

extern skiplist sl_create(int (*compare)(const void*, const void*), void* (*copykey)(const void*), void (*freekey)(void*));
extern skiplist sl_create2(int (*compare)(const void*, const void*), void* (*copykey)(const void*), void (*freekey)(void*), void* (*copyval)(const void*), void (*freeval)(void*));

extern int sl_add(skiplist s, const void* key, const void* value);
extern int sl_get(const skiplist s, const void* key, const void** value);
extern int sl_rem(skiplist s, const void* key);

extern int sl_erase(skiplist s, const void* key, const void* value, int (*compare)(const void*,const void*));
extern int sl_efface(skiplist s, const void* value, int (*compare)(const void*,const void*));
extern void sl_iter(const skiplist s, int (*)(void*,void*,void*), void* p1);
extern void sl_find(const skiplist s, const void* key, int (*)(void*,void*,void*), void* p1);
extern unsigned long sl_count(const skiplist s);

extern void sl_destroy(skiplist s);


#endif
