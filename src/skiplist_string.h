#ifndef SKIPLIST_STRING_H
#define SKIPLIST_STRING_H

#include "skiplist.h"

// Specialized variant:
//
// sl_string_create - string key, int value
// sl_string_create2 - string key, string value

#define sl_string_create() sl_create((int (*)(const void*, const void*))&strcmp, (void *(*)(const void*))&copy_string, &free)
#define sl_string_create2() sl_create2((int (*)(const void*, const void*))&strcmp, (void *(*)(const void*))&copy_string, &free, (void *(*)(const void*))&copy_string, &free)
#define sl_string_add(s,k,v) sl_add(s, (const void*)k, (const void*)v)
#define sl_string_get(s,k,v) sl_get(s, (const void*)k, (const void**)v)
#define sl_string_rem(s,k) sl_rem(s, (const void*)k)
#define sl_string_erase(s,k,v,f) sl_erase(s, (const void*)k, (const void*)v, (int (*)(const void*,const void*))f)
#define sl_string_efface(s,v,f) sl_efface(s, (const void*)v, (int (*)(const void*,const void*))f)
#define sl_string_iter(s,f,a) sl_iter(s, (int (*)(void*, void*, void*))f, (void*)a)
#define sl_string_find(s,k,f,a) sl_find(s, (const void*)k, (int (*)(void*, void*, void*))f, (void*)a)
#define sl_string_count sl_count
#define sl_string_destroy sl_destroy

extern char *copy_string(const char *s);

#endif
