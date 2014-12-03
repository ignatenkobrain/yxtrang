#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <string.h>

typedef struct _skiplist* skiplist;

extern skiplist sl_create(int (*compare)(const void*, const void*), void* (*copykey)(const void*), void (*freekey)(void*));

// Makes internal copies. Returned 'value' points to it
// so make own copy if needed for long-term use.

extern skiplist sl_create2(int (*compare)(const void*, const void*), void* (*copykey)(const void*), void (*freekey)(void*), void* (*copyval)(const void*), void (*freeval)(void*));

extern int sl_add(skiplist s, const void* key, const void* value);
extern int sl_rem(skiplist s, const void* key);
extern int sl_get(const skiplist s, const void* key, const void** value);
extern unsigned long sl_count(const skiplist s);

// Iterate over the whole range. f returns zero to halt.

extern void sl_iter(const skiplist s, int (*f)(void*, void*, void*), void* arg);

// Iterate over >= key. f returns zero to halt.

extern void sl_find(const skiplist s, const void* key, int (*f)(void*, void*, void*), void* arg);

extern void sl_destroy(skiplist s);

// Specialized variant:
//
// sl_int_create - int key, int value
// sl_int_create2 - int key, string value

#define sl_int_create() sl_create(NULL, NULL, NULL)
#define sl_int_create2() sl_create2((NULL, NULL, NULL, (void* (*)(const void*))&strdup, &free)
#define sl_int_add(s,k,v) sl_add(s, (const void*)(size_t)k, (const void*)(size_t)v)
#define sl_int_rem(s,k) sl_rem(s, (const void*)(size_t)k)
#define sl_int_get(s,k,v) sl_get(s, (const void*)(size_t)k, (const void**)v)
#define sl_int_iter(s,f,a) sl_iter(s, (int (*)(void*, void*, void*))f, (void*)a)
#define sl_int_find(s,k,f,a) sl_find(s, (const void*)k, (int (*)(void*, void*, void*))f, (void*)a)
#define sl_int_count sl_count
#define sl_int_destroy sl_destroy

// Specialized variant:
//
// sl_string_create - string key, int value
// sl_string_create2 - string key, string value

#define sl_string_create() sl_create((int (*)(const void*, const void*))&strcmp, (void* (*)(const void*))&strdup, &free)
#define sl_string_create2() sl_create2((int (*)(const void*, const void*))&strcmp, (void* (*)(const void*))&strdup, &free, (void* (*)(const void*))&strdup, &free)
#define sl_string_add(s,k,v) sl_add(s, (const void*)k, (const void*)v)
#define sl_string_rem(s,k) sl_rem(s, (const void*)k)
#define sl_string_get(s,k,v) sl_get(s, (const void*)k, (const void**)v)
#define sl_string_iter(s,f,a) sl_iter(s, (int (*)(void*, void*, void*))f, (void*)a)
#define sl_string_find(s,k,f,a) sl_find(s, (const void*)k, (int (*)(void*, void*, void*))f, (void*)a)
#define sl_string_count sl_count
#define sl_string_destroy sl_destroy

#endif
