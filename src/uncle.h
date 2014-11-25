#ifndef UNCLE_H
#define UNCLE_H

#include "network.h"

typedef struct _uncle* uncle;

#define SCOPE_DEFAULT "test"

// Create using an internal handler and thread.

extern uncle uncle_create(const char* binding, unsigned short port, const char* scope);

// Create but use an existing handler.

extern uncle uncle_create2(handler h, const char* binding, unsigned short port, const char* scope);

// Add, remove ephemeral resources.
// Resources are named and there can be many duplicates, but
// the combination of name/addr/port/tcp is unique.

extern int uncle_add(uncle u, const char* name, const char* addr, unsigned short port, int tcp, int ssl);
extern int uncle_rem(uncle u, const char* name, const char* addr, unsigned short port, int tcp, int ssl);

// Query for named resource.
// A datagram service will normally be returned before a tcp one.
// Set tcp and/or ssl to >= 0 to refine search.
// Set addr to a buffer to receive the address string.

extern int uncle_query(uncle u, const char* name, char* addr, unsigned short* port, int* tcp, int* ssl);

extern void uncle_destroy(uncle u);

#endif
