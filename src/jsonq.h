#ifndef JSONQ_H
#define JSONQ_H

#include <stdint.h>

// JSON quick parse

extern const char* jsonq(const char* s, const char* name, char* dstbuf);
extern int64_t jsonq_int(const char* s, const char* name);
extern double jsonq_real(const char* s, const char* name);
extern int jsonq_bool(const char* s, const char* name);
extern int jsonq_null(const char* s, const char* name);

#endif
