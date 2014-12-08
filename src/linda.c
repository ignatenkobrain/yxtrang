#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "skiplist_uuid.h"
#include "json.h"
#include "store.h"
#include "uuid.h"
#include "linda.h"

struct _linda
{
	store st;
	skiplist sl;
	int is_int, is_string, len, rm;
	long long int_id;
	const char* string_id;
	uuid oid, last_oid;
};

int linda_out(linda l, const char* s)
{
	if (!l)
		return 0;

	json j = json_open(s);
	json j1 = json_get_object(j);
	json juuid = json_find(j1, LINDA_OID);
	uuid u;

	if (juuid)
	{
		uuid_from_string(json_get_string(juuid), &u);
	}
	else
		uuid_gen(&u);

	json jid = json_find(j1, LINDA_ID);

	if (jid)
	{
		if (json_is_integer(jid))
		{
			long long k = json_get_integer(jid);

			if (l->sl && !l->is_int)
			{
				printf("linda_out: expected integer id\n");
				return 0;
			}

			if (!l->sl)
			{
				l->sl = sl_int_uuid_create2();
				l->is_int = 1;
			}

			char tmpbuf[256];
			printf("linda_out: id=%lld, UUID=%s\n", k, uuid_to_string(&u, tmpbuf));
			sl_int_uuid_add(l->sl, k, &u);
		}
		else if (json_is_string(jid))
		{
			const char* k = json_get_string(jid);

			if (l->sl && !l->is_string)
			{
				printf("linda_out: expected string id\n");
				return 0;
			}

			if (!l->sl)
			{
				l->sl = sl_string_uuid_create2();
				l->is_string = 1;
			}

			sl_string_uuid_add(l->sl, k, &u);
		}
	}

	store_add(l->st, &u, s, strlen(s));
	l->last_oid = u;
	json_close(j);
	return 0;
}

int linda_get_length(linda l)
{
	if (!l)
		return 0;

	return l->len;
}

const uuid* linda_get_oid(linda l)
{
	if (!l)
		return NULL;

	return &l->oid;
}

const uuid* linda_get_last_uuid(linda l)
{
	if (!l)
		return NULL;

	return &l->last_oid;
}

static int read_handler(void* arg, void* k, void* v)
{
	linda l = (linda)arg;
	uuid* u = (uuid*)v;
	l->oid.u1 = u->u1;
	l->oid.u2 = u->u2;
	return 1;
}

static int read_int_handler(void* arg, void* _k, void* v)
{
	long long k = (long long)_k;
	linda l = (linda)arg;

	if (k != l->int_id)
		return 0;

	uuid* u = (uuid*)v;
	l->oid.u1 = u->u1;
	l->oid.u2 = u->u2;
	return 0;
}

static int read_string_handler(void* arg, void* _k, void* v)
{
	const char* k = (const char*)_k;
	linda l = (linda)arg;

	if (strcmp(k, l->string_id))
		return 0;

	uuid* u = (uuid*)v;
	l->oid.u1 = u->u1;
	l->oid.u2 = u->u2;
	return 0;
}

static int linda_read(linda l, const char* s, char** dst, int rm, int nowait)
{
	json j = json_open(s);
	json j1 = json_get_object(j);
	l->oid.u1 = l->oid.u2 = 0;
	l->rm = rm;
	uuid u;
	json juuid = json_find(j1, LINDA_OID);

	if (juuid)
	{
		uuid_from_string(json_get_string(juuid), &u);
		json_close(j);

		if (!store_get(l->st, &u, (void**)dst, &l->len))
			return 0;

		if (rm)
			store_rem(l->st, &u);

		l->oid = u;
		return 1;
	}

	json jid = json_find(j1, LINDA_ID);

	if (jid)
	{
		if (l->is_int && !json_is_integer(jid))
		{
			printf("linda_read: expected integer id\n");
			json_close(j);
			return 0;
		}
		else if (l->is_string && !json_is_string(jid))
		{
			printf("linda_read: expected string id\n");
			json_close(j);
			return 0;
		}

		if (json_is_integer(jid))
		{
			l->int_id = json_get_integer(jid);
			printf("linda_read: find=%lld\n", l->int_id);
			sl_int_uuid_find(l->sl, l->int_id, &read_int_handler, l);
		}
		else if (json_is_string(jid))
		{
			l->string_id = json_get_string(jid);
			sl_string_uuid_find(l->sl, l->string_id, &read_string_handler, l);
		}
		else
		{
			json_close(j);
			return 0;
		}
	}
	else
	{
		sl_iter(l->sl, &read_handler, l);
	}

	json_close(j);

	if (!l->oid.u1 && !l->oid.u2)
		return 0;

	char tmpbuf[256];
	printf("HERE3: UUID=%s\n", uuid_to_string(&l->oid, tmpbuf));

	if (!store_get(l->st, &l->oid, (void**)dst, &l->len))
	{
		char tmpbuf[256];
		printf("linda_read: UUID=%s not found\n", uuid_to_string(&l->oid, tmpbuf));
		return 0;
	}

	if (rm)
		store_rem(l->st, &l->oid);

	return 1;
}

int linda_rd(linda l, const char* s, char** dst)
{
	if (!l)
		return 0;

	return linda_read(l, s, dst, 0, 0);
}

int linda_rdp(linda l, const char* s, char** dst)
{
	if (!l)
		return 0;

	return linda_read(l, s, dst, 0, 1);
}

int linda_in(linda l, const char* s, char** dst)
{
	if (!l)
		return 0;

	return linda_read(l, s, dst, 1, 0);
}

int linda_inp(linda l, const char* s, char** dst)
{
	if (!l)
		return 0;

	return linda_read(l, s, dst, 1, 1);
}

linda linda_open(const char* path1, const char* path2)
{
	linda l = (linda)calloc(1, sizeof(struct _linda));
	if (!l) return NULL;
	l->st = store_open(path1, path2, 0);
	return l;
}

int linda_close(linda l)
{
	if (!l)
		return 0;

	if (l->st)
		store_close(l->st);

	if (l->sl)
		sl_destroy(l->sl);

	free(l);
	return 1;
}
