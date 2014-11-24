#include <stdlib.h>
#include <stdio.h>

#include "uncle.h"
#include "network.h"
#include "thread.h"
#include "jsonq.h"

struct _uncle
{
	handler h;
};

static int uncle_wait(void* data)
{
	uncle u = (uncle)data;
	handler_wait(u->h);
	return 1;
}

static int uncle_handler(session s, void* data)
{
	//uncle u = (uncle)data;
	return 1;
}

uncle uncle_create(const char* binding, short port)
{
	uncle u = (uncle)calloc(1, sizeof(struct _uncle));
	u->h = handler_create(0);
	handler_add_server(u->h, &uncle_handler, u, binding, port, 0, 0);
	thread_run(&uncle_wait, u);
	return u;
}

int uncle_add(uncle u, const char* name, short port, int tcp, int ssl)
{
	return 0;
}

int uncle_query(uncle u, const char* name, short port, int tcp, int ssl)
{
	return 0;
}

int uncle_rem(uncle u, int id)
{
	return 0;
}

void uncle_destroy(uncle u)
{
	handler_destroy(u->h);
	free(u);
}

