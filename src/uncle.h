#ifndef UNCLE_H
#define UNCLE_H

typedef struct _uncle* uncle;

extern uncle uncle_create(const char* binding, unsigned short port);

// Add, query, remove ephemeral resources.

extern int uncle_add(uncle u, const char* name, const char* addr, unsigned short port, int tcp, int ssl);
extern int uncle_rem(uncle u, const char* addr, unsigned short port, int tcp, int ssl);

// Set port to > 0 to refine search.
// Set tcp and/or ssl to >= 0 to refine search.
// Set addr to a buffer to receive the address string.

extern int uncle_query(uncle u, const char* name, char* addr, unsigned short* port, int* tcp, int* ssl);

extern void uncle_destroy(uncle u);

#endif
