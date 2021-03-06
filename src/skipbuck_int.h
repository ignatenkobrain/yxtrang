#ifndef SKIPBUCK_INT_H
#define SKIPBUCK_INT_H

#include "skipbuck.h"

// Specialized variant:
//
// sb_int_create - int key, int value
// sb_int_create2 - int key, string value

#define sb_int_create() sb_create(NULL, NULL, NULL)
#define sb_int_create2() sb_create2(NULL, NULL, NULL, (void *(*)(const void*))&strdup, &free)
#define sb_int_set(s,k,v) sb_set(s, (const void*)(size_t)k, (const void*)(size_t)v)
#define sb_int_get(s,k,v) sb_get(s, (const void*)(size_t)k, (const void**)v)
#define sb_int_del(s,k) sb_del(s, (const void*)(size_t)k)
#define sb_int_erase(s,k,v,f) sb_erase(s, (const void*)(size_t)k, (const void*)v, (int (*)(const void*,const void*))f)
#define sb_int_efface(s,v,f) sb_efface(s, (const void*)v, (int (*)(const void*,const void*))f)
#define sb_int_iter(s,f,a) sb_iter(s, (int (*)(void*, void*, void*))f, (void*)a)
#define sb_int_find(s,k,f,a) sb_find(s, (const void*)(size_t)k, (int (*)(void*, void*, void*))f, (void*)a)
#define sb_int_count sb_count
#define sb_int_destroy sb_destroy

#endif
