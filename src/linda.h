#ifndef LINDA_H
#define LINDA_H

// Linda reads and writes JSON tuples.
// String ID has 256 byte length limit

#include "uuid.h"

#define LINDA_ID "$id"
#define LINDA_OID "$oid"

typedef struct _linda* linda;
typedef struct _hlinda* hlinda;

extern linda linda_open(const char* path1, const char* path2);

// Begin a sequence of one or more operations.

extern hlinda linda_begin(linda l, int tran);

extern int linda_out(hlinda l, const char* s);

// These all return a managed pointer to a read buffer.

extern int linda_rd(hlinda h, const char* s, const char** buf);
extern int linda_rdp(hlinda h, const char* s, const char** buf);
extern int linda_in(hlinda h, const char* s, const char** buf);
extern int linda_inp(hlinda h, const char* s, const char** buf);

// Call only to take control of read buffer.

extern void linda_release(hlinda h);

// Non-standard extension. Must specify an OID.

extern int linda_rm(hlinda h, const char* s);

extern int linda_get_length(hlinda h);			// of last read
extern const uuid linda_get_oid(hlinda h);		// of last read
extern const uuid linda_last_oid(hlinda h);		// of last write

extern void linda_end(hlinda h);

extern int linda_close(linda l);

#endif
