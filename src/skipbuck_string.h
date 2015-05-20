#ifndef SKIPBUCK_STRING_H
#define SKIPBUCK_STRING_H

#include "skipbuck.h"

// Specialized variant:
//
// sb_string_create - string key, int value
// sb_string_create2 - string key, string value

#define sb_string_create() sb_create((int (*)(const void*, const void*))&strcmp, (void *(*)(const void*))&copy_string, &free)
#define sb_string_create2() sb_create2((int (*)(const void*, const void*))&strcmp, (void *(*)(const void*))&copy_string, &free, (void *(*)(const void*))&copy_string, &free)
#define sb_string_add(s,k,v) sb_add(s, (const void*)k, (const void*)v)
#define sb_string_get(s,k,v) sb_get(s, (const void*)k, (const void**)v)
#define sb_string_rem(s,k) sb_rem(s, (const void*)k)
#define sb_string_erase(s,k,v,f) sb_erase(s, (const void*)k, (const void*)v, (int (*)(const void*,const void*))f)
#define sb_string_efface(s,v,f) sb_efface(s, (const void*)v, (int (*)(const void*,const void*))f)
#define sb_string_iter(s,f,a) sb_iter(s, (int (*)(void*, void*, void*))f, (void*)a)
#define sb_string_find(s,k,f,a) sb_find(s, (const void*)k, (int (*)(void*, void*, void*))f, (void*)a)
#define sb_string_count sb_count
#define sb_string_destroy sb_destroy

extern char *copy_string(const char *s);

#endif
