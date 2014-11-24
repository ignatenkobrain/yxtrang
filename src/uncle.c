#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "uncle.h"
#include "network.h"
#include "thread.h"
#include "skiplist.h"
#include "jsonq.h"

struct _uncle
{
	handler h;
	skiplist db;
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
	u->db = sl_string_create2();
	u->h = handler_create(0);
	handler_add_server(u->h, &uncle_handler, u, binding, port, 0, 0);
	thread_run(&uncle_wait, u);
	return u;
}

int uncle_add(uncle u, const char* name, const char* addr, short port, int tcp, int ssl)
{
	if (!u)
		return 0;

	return 1;
}

int uncle_query(uncle u, const char* name, const char* addr, short* port, int* tcp, int* ssl)
{
	if (!u)
		return 0;

	return 0;
}

int uncle_rem(uncle u, const char* addr, short port, int cp, int ssl)
{
	if (!u)
		return 0;

	return 0;
}

void uncle_destroy(uncle u)
{
	if (!u)
		return;

	handler_destroy(u->h);
	sl_string_destroy(u->db);
	free(u);
}

