#ifndef STORE_H
#define STORE_H

// A log-structured store with an in-memory UUID index.
// NOTE: this is not in-itself a general-purpose database.

#include "uuid.h"

typedef struct _store* store;
typedef struct _hstore* hstore;

extern store store_open(const char* path1, const char* path2, int compact);
extern store store_open2(const char* path1, const char* path2, int compact, void (*)(void*,const uuid_t*,const void*,int), void* data);

extern int store_get(const store st, const uuid_t* u, void** buf, int* len);
extern int store_add(store st, const uuid_t* u, const void* buf, int len);
extern int store_rem(store st, const uuid_t* u);
extern unsigned long store_count(const store st);

// Optionally store information that caused the delete.

extern int store_rem2(store st, const uuid_t* u, const void* buf, int len);

// Transactions...

extern hstore store_begin(store st, int dbsync);
extern int store_hadd(hstore h, const uuid_t* u, const void* buf, int len);
extern int store_hrem2(hstore h, const uuid_t* u, const void* buf, int len);
extern int store_hrem(hstore h, const uuid_t* u);
extern int store_cancel(hstore h);					// Rollback
extern int store_end(hstore h);						// Commit

extern int store_close(store st);

#endif

