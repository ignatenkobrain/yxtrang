#ifndef JSONQ_H
#define JSONQ_H

#include <stdint.h>

extern const char* json_quick(const char* s, const char* name);
extern uint64_t json_quick_int(const char* s, const char* name);
extern double json_quick_real(const char* s, const char* name);
extern int json_quick_bool(const char* s, const char* name);
extern int json_quick_null(const char* s, const char* name);

extern const char* json_quick_array(const char* s, int n);
extern uint64_t json_quick_array_int(const char* s, int n);
extern double json_quick_array_real(const char* s, int n);
extern int json_quick_array_bool(const char* s, int n);
extern int json_quick_array_null(const char* s, int n);

#endif
