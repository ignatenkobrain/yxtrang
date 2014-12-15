#ifndef HTTPSERVER_H
#define HTTPSERVER_H

// This module will process query-strings and also POST-data for
// content-type 'application/x-www-form-urlencoded', parsing the
// name-value pairs. Each name will be preceded by an underscore
// in the stash, eg:
//
//    GET xyz?id=123456&blah
//
// is retrieved by:
//
//    const char* filename = session_get_stash(s, "HTTP_FILENAME")
//    long id = session_get_stash(s, "_id")
//
// Other content associated with a request is left unread and should
// be processed by the user.

#include "network.h"

// Session stash variables:

#define HTTP_COMMAND "HTTP_COMMAND"
#define HTTP_RESOURCE "HTTP_RESOURCE"
#define HTTP_VERSION "HTTP_VERSION"
#define HTTP_FILENAME "HTTP_FILENAME"
#define HTTP_QUERY "HTTP_QUERY"
#define HTTP_HOST "HTTP_HOST"
#define HTTP_PORT "HTTP_PORT"

// Session flags:

enum {	HTTP_READY, HTTP_HEAD, HTTP_GET, HTTP_POST,
		HTTP_PUT, HTTP_DELETE, HTTP_v10, HTTP_v11,
		HTTP_PERSIST
};

typedef struct _httpserver* httpserver;

extern httpserver httpserver_create(int (*)(session,void*), void* p1);

extern const char* httpserver_value(session s, const char* name);
extern int httpserver_response(session s, unsigned code, const char* msg, size_t len, const char* content_type);
extern void httpserver_destroy(httpserver h);

// This does all the heavy lifting...

extern int httpserver_handler(session, void*);

#endif
