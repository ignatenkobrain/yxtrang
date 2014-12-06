#ifndef LINDA_H
#define LINDA_H

typedef struct _linda* linda;

extern linda linda_open(const char* path, const char* name);

extern int linda_out(linda l, const char* s);
extern int linda_rd(linda l, const char* s, char** dst);
extern int linda_rdp(linda l, const char* s, char** dst);
extern int linda_in(linda l, const char* s, char** dst);
extern int linda_inp(linda l, const char* s, char** dst);

extern int linda_close(linda l);

#endif
