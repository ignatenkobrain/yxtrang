#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "network.h"

typedef struct _httpserver* httpserver;

httpserver httpserver_create(int (*)(session,void*), void* p1);
int httpserver_session(session, void*);
void httpserver_destroy(httpserver h);

#endif
