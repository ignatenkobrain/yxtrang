#ifndef SKIPLIST_INT_H
#define SKIPLIST_INT_H

#include "skiplist.h"

// Specialized variant:
//
// sl_int_create - int key, int value
// sl_int_create2 - int key, string value

#define sl_int_create() sl_create(NULL, NULL, NULL)
#define sl_int_create2() sl_create2(NULL, NULL, NULL, (void* (*)(const void*))&strdup, &free)
#define sl_int_add(s,k,v) sl_add(s, (const void*)(size_t)k, (const void*)(size_t)v)
#define sl_int_get(s,k,v) sl_get(s, (const void*)(size_t)k, (const void**)v)
#define sl_int_rem(s,k) sl_rem(s, (const void*)(size_t)k)
#define sl_int_erase(s,k,v,f) sl_erase(s, (const void*)(size_t)k, (const void*)v, (int (*)(const void*,const void*))f)
#define sl_int_efface(s,v,f) sl_efface(s, (const void*)v, (int (*)(const void*,const void*))f)
#define sl_int_iter(s,f,a) sl_iter(s, (int (*)(void*, void*, void*))f, (void*)a)
#define sl_int_find(s,k,f,a) sl_find(s, (const void*)k, (int (*)(void*, void*, void*))f, (void*)a)
#define sl_int_count sl_count
#define sl_int_destroy sl_destroy

#endif
