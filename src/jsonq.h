#ifndef JSONQ_H
#define JSONQ_H

#include <stdint.h>

extern const char* json_quick(const char* s, const char* name, int* len);
extern uint64_t json_quick_int(const char* s, const char* name);
extern double json_quick_real(const char* s, const char* name);
extern int json_quick_bool(const char* s, const char* name);
extern int json_quick_null(const char* s, const char* name);

#endif
