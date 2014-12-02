#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

#include "uncle.h"

typedef struct _session* session;
typedef struct _handler* handler;

extern const char* hostname(void);

extern session session_open(const char* host, unsigned short port, int tcp, int ssl);

extern int session_on_connect(session s);
extern int session_on_disconnect(session s);

// With UDP this will enable multicast sending.
// LOOP = 1 (the default) allow loopback to same host.
// HOPS = 0 same host only, 1 (the default) same sub-net etc.

extern int session_enable_multicast(session s, int loop, int hops);
extern int session_enable_broadcast(session s);

extern int session_enable_tls(session s, const char* certfile, int level);

extern const char* session_get_remote_host(session s, int /*resolve*/);

extern int session_set_sndbuffer(session s, int bufsize);
extern int session_set_rcvbuffer(session s, int bufsize);

extern int session_write(session s, const void* buf, size_t len);
extern int session_writemsg(session s, const char* buf);

extern int session_read(session s, void* buf, size_t len);

// Readmsg returns 1 on a complete message being read.
// Readmsg returns 0 otherwise.
// Readmsg allocates and disposes of the buffer internally.

extern int session_readmsg(session s, char** buf);

extern void session_clr_udata_flags(session s);
extern void session_clr_udata_flag(session s, int flag);   // flag=0..63
extern void session_set_udata_flag(session s, int flag);   // flag=0..63
extern int session_get_udata_flag(session s, int flag);    // flag=0..63

extern void session_set_udata_int(session s, uint64_t data);
extern uint64_t session_get_udata_int(session s);

extern void session_set_udata_real(session s, double data);
extern double session_get_udata_real(session s);

extern void session_clr_stash(session s);
extern void session_set_stash(session s, const char* key, const char* value);
extern void session_del_stash(session s, const char* key);
extern const char* session_get_stash(session s, const char* key);

extern void session_lock(session s);
extern void session_unlock(session s);

extern int session_shutdown(session s);

extern void session_share(session s);
extern void session_unshare(session s);

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

// Add socket-based services to the handler.

extern int handler_add_uncle(handler h, const char* binding, unsigned short port, const char* scope);

// If name is not NULL then it is added to the uncle as a named service.

extern int handler_add_multicast(handler h, int (*f)(session, void* data), void* data, const char* binding, unsigned short port, const char* maddr6, const char* maddr4, const char* name);
extern int handler_add_server(handler h, int (*f)(session, void* data), void* data, const char* binding, unsigned short port, int tcp, int ssl, const char* name);
extern int handler_add_client(handler h, int (*f)(session, void* data), void* data, session s);

// There is where the action occurs. It will not return until
// there are no more sockets to monitor.

extern int handler_wait(handler h);

extern int handler_destroy(handler h);

#endif
