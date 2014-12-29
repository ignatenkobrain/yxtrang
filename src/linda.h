#ifndef LINDA_H
#define LINDA_H

// Linda reads and writes JSON tuples.

#include "uuid.h"

#define LINDA_SERVICE "LINDA"
#define LINDA_ID "$id"
#define LINDA_OID "$oid"

typedef struct linda_* linda;
typedef struct hlinda_* hlinda;

extern linda linda_open(const char* path1, const char* path2);
extern hlinda linda_begin(linda l);
extern int linda_out(hlinda l, const char* s);

// These return a managed buffer.
extern int linda_rd(hlinda h, const char* s, const char** buf);
extern int linda_rdp(hlinda h, const char* s, const char** buf);
extern int linda_in(hlinda h, const char* s, const char** buf);
extern int linda_inp(hlinda h, const char* s, const char** buf);

// Release buffer to gain control.
extern void linda_release(hlinda h);

extern int linda_rm(hlinda h, const char* s);	// Must specify OID
extern int linda_get_length(hlinda h);			// of last read
extern const uuid linda_get_oid(hlinda h);		// of last read
extern const uuid linda_last_oid(hlinda h);		// of last write
extern void linda_end(hlinda h, int dbsync);
extern int linda_close(linda l);

#endif
