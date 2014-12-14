#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "network.h"

typedef struct _httpserver* httpserver;
enum { HTTP_READY, HTTP_HEAD, HTTP_GET, HTTP_v10, HTTP_v11, HTTP_PERSIST, HTTP_LAST=63 };

httpserver httpserver_create(int (*)(session,void*), void* p1);
int httpserver_handler(session, void*);
void httpserver_destroy(httpserver h);

#endif
