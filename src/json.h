#ifndef JSON_H
#define JSON_H

// This parser also accepts single quotes as
// a convenience with certain languages.

typedef struct _json* json;

extern json json_open(const char* s);	// create from string, OR
extern json json_init(void);				// create top-level object or array

extern int json_set_array(json ptr);
extern json json_array_add(json jptr);

extern int json_set_object(json ptr);
extern json json_object_add(json jptr, const char* name);

extern json json_find(json ptr, const char* name);
extern size_t json_count(const json jptr);				// N
extern json json_index(const json ptr, size_t idx);		// 0..N-1

extern int json_set_integer(json ptr, long long value);
extern int json_set_real(json ptr, double value);
extern int json_set_string(json ptr, const char* value);
extern int json_set_true(json ptr);
extern int json_set_false(json ptr);
extern int json_set_null(json ptr);

extern int json_is_integer(const json ptr);
extern int json_is_real(const json ptr);
extern int json_is_string(const json ptr);
extern int json_is_true(const json ptr);
extern int json_is_false(const json ptr);
extern int json_is_null(const json ptr);

extern int json_get_type(const json ptr);

extern json json_get_object(const json ptr);
extern json json_get_array(const json ptr);

extern long long json_get_integer(const json ptr);
extern double json_get_real(const json ptr);
extern const char* json_get_string(const json ptr);

extern int json_rem(json ptr, json ptr2);

extern const char* json_escape(const char* src, char* dst, size_t maxlen);

// If user specified '*pdst' buffer, have to trust the length is
// enough and won't overflow. But this allows for using a static or
// stack-based buffer in controlled circumstances. If '*pdst' is zero
// a buffer will be allocated which must subsequently be freed by the
// caller...

extern size_t json_print(char** pdst, json ptr);
extern char* json_to_string(json ptr);

extern void json_close(json ptr);

#endif
