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
	lock l;

	struct _search
	{
		const char* name;
		const char* key;
		char addr[256];
		unsigned short port;
		int tcp, ssl, local;
	}
	 search;
};

static const int g_debug = 1;

static int uncle_add2(uncle u, const char* name, const char* addr, int local, unsigned short port, int tcp, int ssl)
{
	if (!u)
		return 0;

	char tmpbuf[1024];
	sprintf(tmpbuf, "%s/%s/%d/%u/%d/%d", name, addr, local, port, tcp, ssl);
	lock_lock(u->l);
	sl_string_add(u->db, tmpbuf, tmpbuf);
	lock_unlock(u->l);
	return 1;
}

int uncle_add(uncle u, const char* name, const char* addr, unsigned short port, int tcp, int ssl)
{
	return uncle_add2(u, name, addr, 1, port, tcp, ssl);
}

static int uncle_iter(uncle u, const char* k, const char* v)
{
	char name[256], addr[256];
	name[255] = addr[255] = 0;
	unsigned port = 0;
	int tcp = 0, ssl = 0, local = 0;
	sscanf(v, "%255[^/]/%255[^/]/%d/%u/%d/%d", name, addr, &local, &port, &tcp, &ssl);

	if (u->search.name[0] && strcmp(u->search.name, name))
		return 1;

	if ((u->search.tcp >= 0) && (u->search.tcp != tcp))
		return 1;

	if ((u->search.ssl >= 0) && (u->search.ssl != ssl))
		return 1;

	strcpy(u->search.addr, addr);
	u->search.port = (unsigned short)port;
	u->search.tcp = tcp;
	u->search.ssl = ssl;
	u->search.key = k;
	return 0;
}

int uncle_query(uncle u, const char* name, char* addr, unsigned short* port, int* tcp, int* ssl)
{
	if (!u)
		return 0;

	u->search.name = name;
	u->search.addr[0] = 0;
	u->search.tcp = *tcp;
	u->search.ssl = *ssl;

	lock_lock(u->l);
	sl_string_find(u->db, name, &uncle_iter, u);
	lock_unlock(u->l);

	if (!u->search.addr[0])
		return 0;

	strcpy(addr, u->search.addr);
	*port = u->search.port;
	*tcp = u->search.tcp;
	*ssl = u->search.ssl;
	return 1;
}

int uncle_rem(uncle u, const char* name, const char* addr, unsigned short port, int tcp, int ssl)
{
	if (!u)
		return 0;

	u->search.name = name;
	strcpy(u->search.addr, addr);
	u->search.tcp = tcp;
	u->search.ssl = ssl;
	u->search.addr[0] = 0;

	lock_lock(u->l);
	sl_string_find(u->db, name, &uncle_iter, u);

	if (!u->search.addr[0])
	{
		lock_unlock(u->l);
		return 0;
	}

	sl_string_rem(u->db, u->search.key);
	lock_unlock(u->l);
	return 1;
}

static int uncle_handler(session s, void* data)
{
	uncle u = (uncle)data;
	char* buf = 0;

	if (!session_readmsg(s, &buf))
		return 0;

	const char* addr = session_remote_host(s, 0);
	if (g_debug) printf("UNCLE %s: %s", addr, buf);

	char cmd[256], name[256];
	cmd[0] = name[0] = 0;
	jsonq(buf, "$cmd", cmd, sizeof(cmd));
	jsonq(buf, "$name", name, sizeof(name));
	unsigned short port = (short)jsonq_int(buf, "$port");
	int tcp = -1, ssl = -1;

	if (!jsonq_null(buf, "$tcp"))
		tcp = jsonq_bool(buf, "$tcp");

	if (!jsonq_null(buf, "$ssl"))
		ssl = jsonq_bool(buf, "$ssl");

	if (!strcmp("$cmd", "+"))
	{
		uncle_add2(u, name, addr, 0, port, tcp, ssl);
	}
	else if (!strcmp("$cmd", "-"))
	{
		uncle_rem(u, name, addr, port, tcp, ssl);
	}
	else if (!strcmp("$cmd", "?"))
	{
		char addr[256];

		if (!uncle_query(u, name, addr, &port, &tcp, &ssl))
			return 1;

		char tmpbuf[1024];
		sprintf(tmpbuf,
			"{\"$cmd\":\"+\",\"$name\":\"%s\",\"$port\":%u,\"$tcp\":%s,\"$ssl\":%s}\n",
				name, port, tcp?"true":"false", ssl?"true":"false");
		session_writemsg(s, tmpbuf);
	}

	return 1;
}

static int uncle_wait(void* data)
{
	uncle u = (uncle)data;
	handler_wait(u->h);
	return 1;
}

uncle uncle_create(const char* binding, unsigned short port)
{
	uncle u = (uncle)calloc(1, sizeof(struct _uncle));
	u->db = sl_string_create2();
	u->h = handler_create(0);
	u->l = lock_create();
	handler_add_server(u->h, &uncle_handler, u, binding, port, 0, 0);
	thread_run(&uncle_wait, u);

	char tmpbuf[1024];
	sprintf(tmpbuf,	"{\"$cmd\":\"?\"}\n");

	session s = session_open("255.255.255.255", port, 0, 0);
	session_enable_broadcast(s);
	session_bcastmsg(s, tmpbuf);
	session_close(s);
	return u;
}

void uncle_destroy(uncle u)
{
	if (!u)
		return;

	handler_destroy(u->h);
	sl_string_destroy(u->db);
	lock_destroy(u->l);
	free(u);
}

