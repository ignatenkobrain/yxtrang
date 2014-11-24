#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

typedef struct _session* session;
typedef struct _handler* handler;

// Sessions are message oriented (ie. newline delimited text string)
// of arbitrary and unlimited length, and can traverse TCP or UDP. This
// might be different to expectations with UDP, but allows messages to
// span hardware limitations...

// Client sessions are created by a 'session_open' call and must be
// disposed of by a 'session_close' call.
//
// Specify 'tcp' to enable stream-oriented TCP use or datagram-oriented
// UDP.
//
// Use 'ssl' to immediately enable TLS (SSL fallback is not supported)
// on the new connection.

extern session session_open(const char* host, short port, int tcp, int ssl);

// Or application-level control of TLS.

extern int session_enable_tls(session s, const char* certfile, int level);

// With UDP this will enable broadcast sending.

extern int session_enable_broadcast(session s);

// With UDP this will enable multicast sending.
// LOOP = 1 (the default) allow loopback to same host.
// HOPS = 0 same host only, 1 (the default) same sub-net etc.

extern int session_enable_multicast(session s, int loop, int hops);

// With UDP this will enable multicast receiving on an address.

extern int join_multicast(session s, const char* addr);
extern int leave_multicast(session s, const char* addr);

// Check connect and/or disconnect state change.

extern int session_on_connect(session s);
extern int session_on_disconnect(session s);

extern const char* session_remote_host(session s, int /*resolve*/);

extern int session_set_sndbuffer(session s, int bufsize);
extern int session_set_rcvbuffer(session s, int bufsize);

extern int session_write(session s, const void* buf, size_t len);
extern int session_writemsg(session s, const char* buf);
extern int session_bcast(session s, const void* buf, size_t len);
extern int session_bcastmsg(session s, const char* buf);
extern int session_read(session s, void* buf, size_t len);

// Readmsg returns 1 on a complete message being read. It will
// return 0 otherwise. Readmsg allocates and disposes of the buffer
// internally.

extern int session_readmsg(session s, char** buf);

// User flags.

extern void session_clr_udata_flags(session s);
extern void session_clr_udata_flag(session s, int flag);   // flag=0..63
extern void session_set_udata_flag(session s, int flag);   // flag=0..63
extern int session_get_udata_flag(session s, int flag);    // flag=0..63

// User data (int/real are separate values).

extern void session_set_udata_int(session s, uint64_t data);
extern uint64_t session_get_udata_int(session s);
extern void session_set_udata_real(session s, double data);
extern double session_get_udata_real(session s);

// User key-values. Note: makes internal copies and returned
// 'value' on the get points to it.

extern void session_clr_stash(session s);
extern void session_set_stash(session s, const char* key, const char* value);
extern void session_del_stash(session s, const char* key);
extern const char* session_get_stash(session s, const char* key);

// Increment/decrement use count. Use to manage life-cycle.

extern void session_share(session s);
extern void session_unshare(session s);

// Graceful network shutdown.

extern int session_shutdown(session s);

// Close, and possibly (depending on use count) free session.

extern int session_close(session s);

// Handlers use thread-pools to manage sessions asynchronously.
// Such sockets are set non-blocking.
// Note: data available does not mean a complete message is
// available for processing.

extern handler handler_create(int threads);

extern int handler_set_tls(handler h, const char* keyfile);

// Server sessions are created and disposed of automatically by the
// connection handler, accessing the user-supplied callback function.
// Callbacks happen in-line if 'threads' is zero.
// Specify 'tcp' to enable TCP streams or UDP datagrams.
// Use 'ssl' to immediately enable TLS (SSL is not supported).

extern int handler_add_server(handler h, int (*f)(session, void* data), void* data, const char* binding, short port, int tcp, int ssl);

// Established client session can be added to a handler to be managed.

extern int handler_add_client(handler h, int (*f)(session, void* data), void* data, session s);

// There is where the action occurs. It will not return until there
// are no more sockets to monitor.

extern int handler_wait(handler h);

extern int handler_destroy(handler h);

#endif
