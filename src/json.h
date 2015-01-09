#ifndef JSON_H
#define JSON_H

// This parser also accepts single quotes as
// a convenience with certain languages.

typedef struct json_ json;

extern json *json_open(const char *s);		// create from string, OR
extern json *json_init(void);				// create top-level object or array

extern int json_set_array(json *j);
extern json *json_array_add(json *j);

extern int json_set_object(json *j);
extern json *json_object_add(json *j, const char *name);

extern json *json_find(json *j, const char *name);
extern size_t json_count(const json *j);				// N
extern json *json_index(const json *j, size_t idx);		// 0..N-1

extern int json_set_integer(json *j, long long value);
extern int json_set_real(json *j, double value);
extern int json_set_string(json *j, const char *value);
extern int json_set_true(json *j);
extern int json_set_false(json *j);
extern int json_set_null(json *j);

extern int json_is_integer(const json *j);
extern int json_is_real(const json *j);
extern int json_is_string(const json *j);
extern int json_is_true(const json *j);
extern int json_is_false(const json *j);
extern int json_is_null(const json *j);

extern int json_get_type(const json *j);

extern json *json_get_object(const json *j);
extern json *json_get_array(const json *j);

extern long long json_get_integer(const json *j);
extern double json_get_real(const json *j);
extern const char *json_get_string(const json *j);

extern int json_rem(json *j, json *ptr2);

extern const char *json_format_string(const char *src, char *dst, size_t maxlen);

// If user specified '*pdst' buffer, have to trust the length is
// enough and won't overflow. But this allows for using a static or
// stack-based buffer in controlled circumstances. If '*pdst' is zero
// a buffer will be allocated which must subsequently be freed by the
// caller...

extern size_t json_print(char **pdst, json *j);
extern char *json_to_string(json *j);

extern void json_close(json *j);

#endif
