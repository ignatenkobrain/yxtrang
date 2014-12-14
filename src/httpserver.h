#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "network.h"

// Session stash variables:

#define HTTP_COMMAND "HTTP_COMMAND"
#define HTTP_PATH "HTTP_PATH"
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

httpserver httpserver_create(int (*)(session,void*), void* p1);
int httpserver_handler(session, void*);
const char* httpserver_value(session s, const char* name);
void httpserver_destroy(httpserver h);

#endif
