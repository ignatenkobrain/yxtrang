#ifndef STORE_H
#define STORE_H

// A log-structured store with an in-memory index.
//
// Q: Why no permanent disk-based index?
//
// A: To prevent bit-rot. In 20 years when all that
// remains may be the data itself, there is no fancy
// index to reverse-engineer, version to revert to, or
// obsolete/obscure software package to dig out from the archives.
// Also, this is a lot faster.
//
// NOTE: this is not in-itself a general-purpose database.

#include "uuid.h"

typedef struct _store* store;
typedef struct _transaction* transaction;

extern store store_open(const char* path1, const char* path2, int compact);

extern int store_get(const store st, const uuid* u, void** buf, int* len);
extern int store_add(store st, const uuid* u, const void* buf, int len);
extern int store_rem(store st, const uuid* u);
extern unsigned long store_count(const store st);

// Transactions...

extern transaction store_begin(store st, int dbsync);
extern int store_hadd(transaction h, const uuid* u, const void* buf, int len);
extern int store_hrem(transaction h, const uuid* u);
extern int store_cancel(transaction h);					// Rollback
extern int store_end(transaction h);						// Commit

extern int store_close(store st);

#endif

