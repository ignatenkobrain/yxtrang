#ifndef UNCLE_H
#define UNCLE_H

typedef struct _uncle* uncle;

extern uncle uncle_create(const char* binding, short port);

// Add, query, remove ephemeral resources.

extern int uncle_add(const char* name, short port, int tcp, int ssl);
extern int uncle_query(const char* name, short port, int tcp, int ssl);
extern int uncle_rem(int id);

extern void uncle_destroy();

#endif
