#ifndef BASE64_H
#define BASE64_H

// If user specified '*pdst' buffer, have to trust the length is
// enough and won't overflow. But this allows for using a static or
// stack-based buffer in controlled circumstances. If '*pdst' is zero
// a buffer will be allocated which must subsequently be freed by the
// caller...

void format_base64(const char* src, size_t nbytes, char** pdst, int line_breaks, int cr);
void parse_base64(const char* src, size_t nbytes, char** pdst);

#endif
