#ifndef JSONQ_H
#define JSONQ_H

#include <stdint.h>

// JSON quick and dirty search for named value. Calls can be
// concatenated to drill down through object layers.
//
// Accepts single quotes as a convenience.
//
// eg. char* s = "{'a':1,'b'2,'c':{'c1':1,'c1':2,'c3':3},'d',4}";
//
//   char tmp[256];
//   int64_t v = jsonq(s,"c",tmp,sizeof(tmp))->jsonq_int(tmp,"c2");
//
// Doesn't use any memory other than what's on the stack.
//
// TO DO: need to be able to iterate objects and arrays
// and enumerate/index arrays directly.

extern const char* jsonq(const char* s, const char* name, char* dstbuf, int dstlen);
extern int64_t jsonq_int(const char* s, const char* name);
extern double jsonq_real(const char* s, const char* name);
extern int jsonq_bool(const char* s, const char* name);
extern int jsonq_null(const char* s, const char* name);

#endif
