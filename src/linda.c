#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "skiplist_int.h"
#include "skiplist_string.h"
#include "json.h"
#include "store.h"
#include "uuid.h"
#include "linda.h"

#ifdef _WIN32
#define SEP "\\"
#else
#define SEP "/"
#endif

struct _linda
{
	store st;
	skiplist sl;
	int is_int, is_string, len, rm;
	long long int_id;
	const char* string_id;
	uuid uuid, last_uuid;
};

int linda_out(linda l, const char* s)
{
	if (!l)
		return 0;

	json j = json_open(s);
	json j1 = json_get_object(j);
	json juuid = json_find(j1, LINDA_UUID);
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
				l->sl = sl_int_create();
				l->is_int = 1;
			}

			sl_int_add(l->sl, k, &u);
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
				l->sl = sl_int_create();
				l->is_string = 1;
			}

			sl_string_add(l->sl, k, &u);
		}
	}

	store_add(l->st, &u, s, strlen(s));
	l->last_uuid = u;
	json_close(j);
	return 0;
}

int linda_get_length(linda l)
{
	if (!l)
		return 0;

	return l->len;
}

const uuid* linda_get_uuid(linda l)
{
	if (!l)
		return NULL;

	return &l->uuid;
}

const uuid* linda_get_last_uuid(linda l)
{
	if (!l)
		return NULL;

	return &l->last_uuid;
}

static int read_handler(void* arg, void* k, void* v)
{
	linda l = (linda)arg;
	l->uuid = *((uuid*)v);
	return 1;
}

static int read_int_handler(void* arg, void* _k, void* v)
{
	long long k = (long long)_k;
	linda l = (linda)arg;

	if (k != l->int_id)
		return 0;

	l->uuid = *((uuid*)v);
	return 1;
}

static int read_string_handler(void* arg, void* _k, void* v)
{
	const char* k = (const char*)_k;
	linda l = (linda)arg;

	if (strcmp(k, l->string_id))
		return 0;

	l->uuid = *((uuid*)v);
	return 0;
}

static int linda_read(linda l, const char* s, char** dst, int rm, int nowait)
{
	json j = json_open(s);
	json j1 = json_get_object(j);
	l->uuid.u1 = l->uuid.u2 = 0;
	l->rm = rm;
	uuid u;
	json juuid = json_find(j1, LINDA_UUID);

	if (juuid)
	{
		uuid_from_string(json_get_string(juuid), &u);
		json_close(j);

		if (!store_get(l->st, &u, (void**)dst, &l->len))
			return 0;

		if (rm)
			store_rem(l->st, &u);

		l->uuid = u;
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
			sl_int_find(l->sl, l->int_id, &read_int_handler, l);
		}
		else if (json_is_string(jid))
		{
			l->string_id = json_get_string(jid);
			sl_string_find(l->sl, l->string_id, &read_string_handler, l);
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

	if (!l->uuid.u1 && !l->uuid.u2)
		return 0;

	printf("HERE3: %s", s);

	if (!store_get(l->st, &l->uuid, (void**)dst, &l->len))
		return 0;

	if (rm)
		store_rem(l->st, &l->uuid);

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
	static int first_time = 1;

	if (first_time)
	{
		uuid_seed(time(NULL));
		first_time = 0;
	}

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
