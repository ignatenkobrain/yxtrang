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

	struct _search
	{
		const char* name;
		char addr[256];
		unsigned short port;
		int tcp, ssl;
	}
	 search;
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

uncle uncle_create(const char* binding, unsigned short port)
{
	uncle u = (uncle)calloc(1, sizeof(struct _uncle));
	u->db = sl_string_create2();
	u->h = handler_create(0);
	handler_add_server(u->h, &uncle_handler, u, binding, port, 0, 0);
	thread_run(&uncle_wait, u);
	return u;
}

int uncle_add(uncle u, const char* name, const char* addr, unsigned short port, int tcp, int ssl)
{
	if (!u)
		return 0;

	char tmpbuf[1024];
	sprintf(tmpbuf, "%s/%s/%u/%d/%d", name, addr, port, tcp, ssl);
	sl_string_add(u->db, name, tmpbuf);
	return 1;
}

static int uncle_iter(uncle u, const char* name, const char* v)
{
	if (strcmp(name, u->search.name))
		return 1;

	char tmpbuf[256];
	int port, tcp, ssl;
	sscanf(v, "%*[^/]/%255[^/]/%d/%d/%d", tmpbuf, &port, &tcp, &ssl);

	if (u->search.port && (u->search.port != port))
		return 1;

	if (u->search.tcp && (u->search.tcp != tcp))
		return 1;

	if (u->search.ssl && (u->search.ssl != ssl))
		return 1;

	strcpy(u->search.addr, tmpbuf);
	u->search.name = name;
	u->search.port = (unsigned short)port;
	u->search.tcp = tcp;
	u->search.ssl = ssl;
	return 0;
}

int uncle_query(uncle u, const char* name, char* addr, unsigned short* port, int* tcp, int* ssl)
{
	if (!u)
		return 0;

	const char* tmpbuf;

	if (!sl_string_get(u->db, name, &tmpbuf))
		return 0;

	u->search.addr[0] = 0;
	u->search.name = name;
	u->search.port = *port;
	u->search.tcp = *tcp;
	u->search.ssl = *ssl;
	sl_string_iter(u->db, &uncle_iter, u);
	addr = u->search.addr;
	*port = u->search.port;
	*tcp = u->search.tcp;
	*ssl = u->search.ssl;
	return 1;
}

int uncle_rem(uncle u, const char* addr, unsigned short port, int tcp, int ssl)
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

