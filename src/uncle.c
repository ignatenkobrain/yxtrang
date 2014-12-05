#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

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
	session s;				// used to broadcast on
	char scope[256];
	time_t unique;

	struct _search
	{
		const char* name;
		const char* key;
		char addr[256];
		unsigned short port;
		int tcp, ssl, local;
		session s;
	}
	 search;
};

static const int g_debug = 0;

static int uncle_iter(uncle u, const char* k, const char* v)
{
	char name[256], addr[256];
	name[255] = addr[255] = 0;
	unsigned port = 0;
	int tcp = 0, ssl = 0, local = 0;
	sscanf(k, "%255[^/]/%255[^/]/%d/%u/%d/%d", name, addr, &local, &port, &tcp, &ssl);

	if (u->search.name[0] && strcmp(u->search.name, name))
		return 0;

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

	if (name[0])
		sl_string_find(u->db, name, &uncle_iter, u);
	else
		sl_string_iter(u->db, &uncle_iter, u);

	lock_unlock(u->l);

	if (!u->search.addr[0])
		return 0;

	strcpy(addr, u->search.addr);
	*port = u->search.port;
	*tcp = u->search.tcp;
	*ssl = u->search.ssl;
	return 1;
}

static int uncle_add2(uncle u, const char* name, const char* addr, int local, unsigned short port, int tcp, int ssl)
{
	if (!u)
		return 0;

	char tmpbuf[1024];
	sprintf(tmpbuf, "%s/%s/%d/%u/%d/%d", name, addr, local, port, tcp, ssl);
	lock_lock(u->l);
	sl_string_add(u->db, tmpbuf, tmpbuf);
	lock_unlock(u->l);

	if (local)
	{
		sprintf(tmpbuf,
			"{\"$scope\":\"%s\",\"$unique\":%llu,\"$cmd\":\"+\",\"$name\":\"%s\",\"$port\":%u,\"$tcp\":%s,\"$ssl\":%s}\n",
				u->scope, (unsigned long long)u->unique, name, port,
				tcp?"true":"false", ssl?"true":"false");
		session_writemsg(u->s, tmpbuf);
	}

	return 1;
}

int uncle_add(uncle u, const char* name, const char* addr, unsigned short port, int tcp, int ssl)
{
	return uncle_add2(u, name, addr, 1, port, tcp, ssl);
}

static int uncle_rem2(uncle u, const char* name, const char* addr, int local, unsigned short port, int tcp)
{
	if (!u)
		return 0;

	u->search.name = name;
	strcpy(u->search.addr, addr);
	u->search.tcp = tcp;
	u->search.ssl = -1;
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

	if (local)
	{
		char tmpbuf[1024];
		sprintf(tmpbuf,
			"{\"$scope\":\"%s\",\"$unique\":%llu,\"$cmd\":\"-\",\"$name\":\"%s\",\"$port\":%u,\"$tcp\":%s}\n",
				u->scope, (unsigned long long)u->unique, name, port, tcp?"true":"false");
		session_writemsg(u->s, tmpbuf);
	}

	return 1;
}

int uncle_rem(uncle u, const char* name, const char* addr, unsigned short port, int tcp)
{
	return uncle_rem2(u, name, addr, 1, port, tcp);
}

static int uncle_iter2(uncle u, const char* k, const char* v)
{
	char name[256], addr[256];
	name[255] = addr[255] = 0;
	unsigned port = 0;
	int tcp = 0, ssl = 0, local = 0;
	sscanf(v, "%255[^/]/%255[^/]/%d/%u/%d/%d", name, addr, &local, &port, &tcp, &ssl);

	if (u->search.name[0] && strcmp(u->search.name, name))
		return 0;

	if ((u->search.tcp >= 0) && (u->search.tcp != tcp))
		return 1;

	if ((u->search.ssl >= 0) && (u->search.ssl != ssl))
		return 1;

	char tmpbuf[1024];
	sprintf(tmpbuf,
		"{\"$scope\":\"%s\",\"$unique\":%llu,\"$cmd\":\"+\",\"$name\":\"%s\",\"$port\":%u,\"$tcp\":%s,\"$ssl\":%s}\n",
			u->scope, (unsigned long long)u->unique, name, port, tcp?"true":"false", ssl?"true":"false");
	session_writemsg(u->search.s, tmpbuf);
	if (g_debug) printf("SND: %s", tmpbuf);
	return 1;
}

static int uncle_handler(session s, void* data)
{
	uncle u = (uncle)data;
	char* buf = 0;

	if (!session_readmsg(s, &buf))
		return 0;

	const char* addr = session_get_remote_host(s, 0);
	if (g_debug) printf("UNCLE RCV %s: %s", addr, buf);
	char scope[256];

	if (jsonq_int(buf, "$unique") == u->unique)
		return 1;

	if (strcmp(jsonq(buf, "$scope", scope, sizeof(scope)), u->scope))
		return 1;

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

	if (!strcmp(cmd, "+"))
	{
		uncle_add2(u, name, addr, 0, port, tcp, ssl);
	}
	else if (!strcmp(cmd, "-"))
	{
		uncle_rem2(u, name, addr, 0, port, tcp);
	}
	else if (!strcmp(cmd, "?"))
	{
		u->search.name = name;
		u->search.addr[0] = 0;
		u->search.tcp = tcp;
		u->search.ssl = ssl;
		u->search.s = s;

		lock_lock(u->l);

		if (name[0])
			sl_string_find(u->db, name, &uncle_iter2, u);
		else
			sl_string_iter(u->db, &uncle_iter2, u);

		lock_unlock(u->l);
	}

	return 1;
}

uncle uncle_create2(handler h, const char* binding, unsigned short port, const char* scope)
{
	if (!h || !port)
		return NULL;

	uncle u = (uncle)calloc(1, sizeof(struct _uncle));
	u->db = sl_string_create2();
	u->h = h;
	u->l = lock_create();
	u->unique = time(NULL);
	strcpy(u->scope, scope?scope:SCOPE_DEFAULT);
	handler_add_server(u->h, &uncle_handler, u, binding, port, 0, 0, NULL);
	u->s = session_open("255.255.255.255", port, 0, 0);
	session_enable_broadcast(u->s);
	handler_add_client(h, &uncle_handler, u, u->s);

	char tmpbuf[1024];
	sprintf(tmpbuf,	"{\"$scope\":\"%s\",\"$unique\":%llu,\"$cmd\":\"?\"}\n",
		u->scope, (unsigned long long)u->unique);
	session_writemsg(u->s, tmpbuf);

	return u;
}

static int uncle_wait(void* data)
{
	uncle u = (uncle)data;
	handler_wait(u->h);
	return 1;
}

uncle uncle_create(const char* binding, unsigned short port, const char* scope)
{
	if (!port)
		return NULL;

	handler h = handler_create(0);
	uncle u = uncle_create2(h, binding, port, scope);
	thread_run(&uncle_wait, u);
	return u;
}

const char* uncle_get_scope(uncle u)
{
	return u->scope;
}

void uncle_destroy(uncle u)
{
	if (!u)
		return;

	handler_destroy(u->h);
	sl_string_destroy(u->db);
	lock_destroy(u->l);
	session_close(u->s);
	free(u);
}

