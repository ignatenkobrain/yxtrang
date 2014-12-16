#ifndef STORE_H
#define STORE_H

// A log-structured store with an in-memory UUID index.
// NOTE: this is not in-itself a general-purpose database.

#include "uuid.h"

#define STORE_MAX_WRITELEN (64*1024*1024)			// 64 MB per write

typedef struct _store* store;
typedef struct _hstore* hstore;
typedef struct _hreader* hreader;

extern store store_open(const char* path1, const char* path2, int compact);
extern store store_open2(const char* path1, const char* path2, int compact, void (*)(void*,const uuid,const void*,int), void* p1);

// Non-transactions are NOT thread-safe...

extern int store_get(const store st, const uuid u, void** buf, size_t* len);
extern int store_add(store st, const uuid u, const void* buf, size_t len);
extern int store_rem(store st, const uuid u);
extern int store_rem2(store st, const uuid u, const void* buf, size_t len);
extern unsigned long store_count(const store st);

// Transactions are thread-safe...

extern hstore store_begin(store st, int dbsync);
extern int store_hadd(hstore h, const uuid u, const void* buf, size_t len);
extern int store_hrem(hstore h, const uuid u);
extern int store_hrem2(hstore h, const uuid u, const void* buf, size_t len);
extern int store_cancel(hstore h);
extern int store_end(hstore h);

// Reader...

extern int store_tail(store st, const uuid u, int (*)(void*,const uuid,const void*,int), void* p1);

extern int store_close(store st);

#endif

