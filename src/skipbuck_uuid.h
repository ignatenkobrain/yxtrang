#ifndef SKIPBUCK_UUID_H
#define SKIPBUCK_UUID_H

#include "skipbuck.h"

// Specialized variant:
//
// sb_int_uuid_create - int key, uuid value
// sb_int_uuid_create2 - int key, uuid value

#define sb_int_uuid_create() sb_create(NULL, NULL, NULL)
#define sb_int_uuid_create2() sb_create2(NULL, NULL, NULL, (void *(*)(const void*))&uuid_copy, &free)
#define sb_int_uuid_add(s,k,v) sb_add(s, (const void*)(size_t)k, (const void*)(size_t)v)
#define sb_int_uuid_get(s,k,v) sb_get(s, (const void*)(size_t)k, (const void**)v)
#define sb_int_uuid_rem(s,k) sb_rem(s, (const void*)(size_t)k)
#define sb_int_uuid_erase(s,k,v) sb_erase(s, (const void*)(size_t)k, (const void*)v, (int (*)(const void*,const void*))&uuid_compare)
#define sb_int_uuid_efface(s,v) sb_efface(s, (const void*)v, (int (*)(const void*,const void*))&uuid_compare)
#define sb_int_uuid_iter(s,f,a) sb_iter(s, (int (*)(void*, void*, void*))f, (void*)a)
#define sb_int_uuid_find(s,k,f,a) sb_find(s, (const void*)(size_t)k, (int (*)(void*, void*, void*))f, (void*)a)
#define sb_int_uuid_count sb_count
#define sb_int_uuid_destroy sb_destroy

// Specialized variant:
//
// sb_string_uuid_create - string key, uuid value
// sb_string_uuid_create2 - string key, uuid value

#define sb_string_uuid_create() sb_create((int (*)(const void*, const void*))&strcmp, (void *(*)(const void*))&strdup, &free)
#define sb_string_uuid_create2() sb_create2((int (*)(const void*, const void*))&strcmp, (void *(*)(const void*))&strdup, &free, (void *(*)(const void*))&uuid_copy, &free)
#define sb_string_uuid_add(s,k,v) sb_add(s, (const void*)k, (const void*)v)
#define sb_string_uuid_get(s,k,v) sb_get(s, (const void*)k, (const void**)v)
#define sb_string_uuid_rem(s,k) sb_rem(s, (const void*)k)
#define sb_string_uuid_erase(s,k,v) sb_erase(s, (const void*)k, (const void*)v, (int (*)(const void*,const void*))&uuid_compare)
#define sb_string_uuid_efface(s,v) sb_efface(s, (const void*)v, (int (*)(const void*,const void*))&uuid_compare)
#define sb_string_uuid_iter(s,f,a) sb_iter(s, (int (*)(void*, void*, void*))f, (void*)a)
#define sb_string_uuid_find(s,k,f,a) sb_find(s, (const void*)k, (int (*)(void*, void*, void*))f, (void*)a)
#define sb_string_uuid_count sb_count
#define sb_string_uuid_destroy sb_destroy

#define sb_uuid_efface(s,v) sb_efface(s, (const void*)v, (int (*)(const void*,const void*))&uuid_compare)

#endif
