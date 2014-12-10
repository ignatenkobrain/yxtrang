#ifndef SKIPLIST_UUID_H
#define SKIPLIST_UUID_H

#include "skiplist.h"

// Specialized variant:
//
// sl_int_uuid_create - int key, uuid value
// sl_int_uuid_create2 - int key, uuid value

#define sl_int_uuid_create() sl_create(NULL, NULL, NULL)
#define sl_int_uuid_create2() sl_create2(NULL, NULL, NULL, (void* (*)(const void*))&uuid_copy, &free)
#define sl_int_uuid_add(s,k,v) sl_add(s, (const void*)(size_t)k, (const void*)(size_t)v)
#define sl_int_uuid_get(s,k,v) sl_get(s, (const void*)(size_t)k, (const void**)v)
#define sl_int_uuid_rem(s,k) sl_rem(s, (const void*)(size_t)k)
#define sl_int_uuid_erase(s,k,v) sl_erase(s, (const void*)(size_t)k, (const void*)v, (int (*)(const void*,const void*))&uuid_compare)
#define sl_int_uuid_efface(s,v) sl_efface(s, (const void*)v, (int (*)(const void*,const void*))&uuid_compare)
#define sl_int_uuid_iter(s,f,a) sl_iter(s, (int (*)(void*, void*, void*))f, (void*)a)
#define sl_int_uuid_find(s,k,f,a) sl_find(s, (const void*)(size_t)k, (int (*)(void*, void*, void*))f, (void*)a)
#define sl_int_uuid_count sl_count
#define sl_int_uuid_destroy sl_destroy

// Specialized variant:
//
// sl_string_uuid_create - string key, uuid value
// sl_string_uuid_create2 - string key, uuid value

#define sl_string_uuid_create() sl_create((int (*)(const void*, const void*))&strcmp, (void* (*)(const void*))&strdup, &free)
#define sl_string_uuid_create2() sl_create2((int (*)(const void*, const void*))&strcmp, (void* (*)(const void*))&strdup, &free, (void* (*)(const void*))&uuid_copy, &free)
#define sl_string_uuid_add(s,k,v) sl_add(s, (const void*)k, (const void*)v)
#define sl_string_uuid_get(s,k,v) sl_get(s, (const void*)k, (const void**)v)
#define sl_string_uuid_rem(s,k) sl_rem(s, (const void*)k)
#define sl_string_uuid_erase(s,k,v) sl_erase(s, (const void*)k, (const void*)v, (int (*)(const void*,const void*))&uuid_compare)
#define sl_string_uuid_efface(s,v) sl_efface(s, (const void*)v, (int (*)(const void*,const void*))&uuid_compare)
#define sl_string_uuid_iter(s,f,a) sl_iter(s, (int (*)(void*, void*, void*))f, (void*)a)
#define sl_string_uuid_find(s,k,f,a) sl_find(s, (const void*)k, (int (*)(void*, void*, void*))f, (void*)a)
#define sl_string_uuid_count sl_count
#define sl_string_uuid_destroy sl_destroy

#define sl_uuid_efface(s,v) sl_efface(s, (const void*)v, (int (*)(const void*,const void*))&uuid_compare)

#endif
