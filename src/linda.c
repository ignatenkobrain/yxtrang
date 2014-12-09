#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "skiplist_uuid.h"
#include "json.h"
#include "store.h"
#include "linda.h"

struct _linda
{
	store st;
	skiplist sl;
	int is_int, is_string;
};

struct _hlinda
{
	linda l;
	char* dst;
	json jquery;
	uuid oid, last_oid;
	long long int_id;
	const char* string_id;
	int len, rm;
};

int linda_out(hlinda h, const char* s)
{
	if (!h)
		return 0;

	json j = json_open(s);
	json j1 = json_get_object(j);
	json joid = json_find(j1, LINDA_OID);
	uuid u;

	if (joid)
	{
		uuid_from_string(json_get_string(joid), &u);
	}
	else
		uuid_gen(&u);

	json jid = json_find(j1, LINDA_ID);

	if (jid)
	{
		if (json_is_integer(jid))
		{
			long long k = json_get_integer(jid);

			if (h->l->sl && !h->l->is_int)
			{
				printf("linda_out: expected integer id\n");
				return 0;
			}

			if (!h->l->sl)
			{
				h->l->sl = sl_int_uuid_create2();
				h->l->is_int = 1;
			}

			sl_int_uuid_add(h->l->sl, k, &u);
		}
		else if (json_is_string(jid))
		{
			const char* k = json_get_string(jid);

			if (h->l->sl && !h->l->is_string)
			{
				printf("linda_out: expected string id\n");
				return 0;
			}

			if (!h->l->sl)
			{
				h->l->sl = sl_string_uuid_create2();
				h->l->is_string = 1;
			}

			sl_string_uuid_add(h->l->sl, k, &u);
		}
	}

	store_add(h->l->st, &u, s, strlen(s));
	h->last_oid = u;
	json_close(j);
	return 0;
}

int linda_get_length(hlinda h)
{
	if (!h)
		return 0;

	return h->len;
}

const uuid* linda_get_oid(hlinda h)
{
	if (!h)
		return NULL;

	return &h->oid;
}

const uuid* linda_last_oid(hlinda h)
{
	if (!h)
		return NULL;

	return &h->last_oid;
}

void linda_release(hlinda h)
{
	if (!h)
		return;

	if (h->dst)
	{
		free(h->dst);
		h->dst = NULL;
	}
}

static int read_handler(void* arg, void* k, void* v)
{
	hlinda h = (hlinda)arg;
	uuid* u = (uuid*)v;

	if (!store_get(h->l->st, u, (void**)&h->dst, &h->len))
		return 0;

	int match = 1;
	json j1 = json_get_object(h->jquery);
	json jdst = json_open(h->dst);
	json j2 = json_get_object(jdst);
	size_t i, cnt = json_count(j1);

	for (i = 0; i < cnt; i++)
	{
		json j1it = json_index(j1, i);
		const char* name = json_get_string(j1it);

		if (name[0] == '$')
			continue;

		json j2it = json_find(j2, name);

		if (!j2it)
		{
			match = 0;
			continue;
		}

		if (json_is_integer(j1it))
		{
			if (!json_is_integer(j2it))
			{
				match = 0;
				break;
			}

			if (json_get_integer(j1it) != json_get_integer(j2it))
			{
				match = 0;
				break;
			}
		}
		else if (json_is_real(j1it))
		{
			if (!json_is_real(j2it))
			{
				match = 0;
				break;
			}

			if (json_get_real(j1it) != json_get_real(j2it))
			{
				match = 0;
				break;
			}
		}
		else if (json_is_string(j1it))
		{
			if (!json_is_string(j2it))
			{
				match = 0;
				break;
			}

			if (strcmp(json_get_string(j1it), json_get_string(j2it)))
			{
				match = 0;
				break;
			}
		}
		else if (json_is_true(j1it))
		{
			if (!json_is_true(j2it))
			{
				match = 0;
				break;
			}
		}
		else if (json_is_false(j1it))
		{
			if (!json_is_false(j2it))
			{
				match = 0;
				break;
			}
		}
		else if (json_is_null(j1it))
		{
			if (!json_is_null(j2it))
			{
				match = 0;
				break;
			}
		}
	}

	json_close(jdst);

	if (!match)
		return 1;

	h->oid.u1 = u->u1;
	h->oid.u2 = u->u2;
	return 0;
}

static int read_int_handler(void* arg,  int64_t k, uuid* v)
{
	hlinda h = (hlinda)arg;

	if (k != h->int_id)
		return 0;

	return read_handler(arg, (void*)(size_t)k, v);
}

static int read_string_handler(void* arg, const char* k, uuid* v)
{
	hlinda h = (hlinda)arg;

	if (strcmp(k, h->string_id))
		return 0;

	return read_handler(arg, (void*)(size_t)k, v);
}

static int linda_read(hlinda h, const char* s, const char** dst, int rm, int nowait)
{
	json j = json_open(s);
	json j1 = json_get_object(j);
	h->oid.u1 = h->oid.u2 = 0;
	h->rm = rm;
	uuid u;
	int is_int = 0, is_string = 0;

	json joid = json_find(j1, LINDA_OID);

	if (joid)
	{
		uuid_from_string(json_get_string(joid), &u);
		json_close(j);

		if (!store_get(h->l->st, &u, (void**)dst, &h->len))
			return 0;

		if (rm)
		{
			store_rem(h->l->st, &u);
			sl_efface(h->l->sl, &u, (int (*)(const void*,const void*))&uuid_compare);
		}

		h->oid = u;
		return 1;
	}

	json jid = json_find(j1, LINDA_ID);

	if (jid)
	{
		if (h->l->is_int && !json_is_integer(jid))
		{
			printf("linda_read: expected integer id\n");
			json_close(j);
			return 0;
		}
		else if (h->l->is_string && !json_is_string(jid))
		{
			printf("linda_read: expected string id\n");
			json_close(j);
			return 0;
		}

		if (json_is_integer(jid))
		{
			h->int_id = json_get_integer(jid);
			h->jquery = json_open(s);
			sl_int_uuid_find(h->l->sl, h->int_id, &read_int_handler, h);
			json_close(h->jquery);
			is_int = 1;
		}
		else if (json_is_string(jid))
		{
			h->string_id = json_get_string(jid);
			h->jquery = json_open(s);
			sl_string_uuid_find(h->l->sl, h->string_id, &read_string_handler, h);
			json_close(h->jquery);
			is_string = 1;
		}
		else
		{
			json_close(j);
			return 0;
		}
	}
	else
	{
		h->jquery = json_open(s);
		sl_iter(h->l->sl, &read_handler, h);
		json_close(h->jquery);
	}

	json_close(j);

	if (!h->oid.u1 && !h->oid.u2)
		return 0;

	if (rm)
	{
		if (is_int)
		{
			char tmpbuf[1024];
			int tmplen = sprintf(tmpbuf, "{\"%s\":%lld}\n", LINDA_ID, h->int_id);
			store_rem2(h->l->st, &h->oid, tmpbuf, tmplen);
			sl_int_uuid_erase(h->l->sl, h->int_id, &h->oid);
		}
		else if (is_string)
		{
			char tmpbuf[1024], tmpbuf2[1024];
			int tmplen = sprintf(tmpbuf, "{\"%s\":\"%s\"}\n", LINDA_ID, json_escape(h->string_id, tmpbuf2, sizeof(tmpbuf2)));
			store_rem2(h->l->st, &h->oid, tmpbuf, tmplen);
			sl_string_uuid_erase(h->l->sl, h->string_id, &h->oid);
		}
		else
		{
			store_rem(h->l->st, &h->oid);
			sl_uuid_efface(h->l->sl, &h->oid);
		}
	}

	*dst = h->dst;
	return 1;
}

int linda_rd(hlinda h, const char* s, const char** dst)
{
	if (!h)
		return 0;

	return linda_read(h, s, dst, 0, 0);
}

int linda_rdp(hlinda h, const char* s, const char** dst)
{
	if (!h)
		return 0;

	return linda_read(h, s, dst, 0, 1);
}

int linda_in(hlinda h, const char* s, const char** dst)
{
	if (!h)
		return 0;

	return linda_read(h, s, dst, 1, 0);
}

int linda_inp(hlinda h, const char* s, const char** dst)
{
	if (!h)
		return 0;

	return linda_read(h, s, dst, 1, 1);
}

extern hlinda linda_begin(linda l)
{
	if (!l)
		return NULL;

	hlinda h = (hlinda)calloc(1, sizeof(struct _hlinda));
	h->l = l;
	return h;
}

extern void linda_end(hlinda h)
{
	if (!h)
		return;

	linda_release(h);
	free(h);
}

static void linda_store_handler(void* data, const uuid* u, const char* s, int len)
{
	linda l = (linda)data;

	if (len > 0)							// add
	{
		json j = json_open(s);
		json j1 = json_get_object(j);
		json jid = json_find(j1, LINDA_ID);

		if (!jid)
			return;

		if (json_is_integer(jid))
		{
			long long k = json_get_integer(jid);

			if (l->sl && !l->is_int)
			{
				printf("linda_store_handler: expected integer id\n");
				return;
			}

			if (!l->sl)
			{
				l->sl = sl_int_uuid_create2();
				l->is_int = 1;
			}

			sl_int_uuid_add(l->sl, k, u);
		}
		else if (json_is_string(jid))
		{
			const char* k = json_get_string(jid);

			if (l->sl && !l->is_string)
			{
				printf("linda_store_handler: expected string id\n");
				return;
			}

			if (!l->sl)
			{
				l->sl = sl_string_uuid_create2();
				l->is_string = 1;
			}

			sl_string_uuid_add(l->sl, k, u);
		}

		json_close(j);
	}
	else if (len < 0)					// remove (with hint)
	{
		json j = json_open(s);
		json j1 = json_get_object(j);
		json jid = json_find(j1, LINDA_ID);

		if (!jid)
			return;

		if (json_is_integer(jid))
		{
			long long k = json_get_integer(jid);

			if (l->sl && !l->is_int)
			{
				printf("linda_out: expected integer id\n");
				return;
			}

			if (!l->sl)
			{
				l->sl = sl_int_uuid_create2();
				l->is_int = 1;
			}

			sl_int_uuid_erase(l->sl, k, u);
		}
		else if (json_is_string(jid))
		{
			const char* k = json_get_string(jid);

			if (l->sl && !l->is_string)
			{
				printf("linda_out: expected string id\n");
				return;
			}

			if (!l->sl)
			{
				l->sl = sl_string_uuid_create2();
				l->is_string = 1;
			}

			sl_string_uuid_erase(l->sl, k, u);
		}

		json_close(j);
	}
	else 								// remove (brute search)
	{
		sl_uuid_efface(l->sl, u);
	}
}

linda linda_open(const char* path1, const char* path2)
{
	linda l = (linda)calloc(1, sizeof(struct _linda));
	if (!l) return NULL;
	l->st = store_open2(path1, path2, 0, &linda_store_handler, l);
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
