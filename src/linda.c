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
	int is_int, is_string;
};

int linda_out(linda l, const char* s)
{
	if (!l)
		return 0;

	json j = json_open(s);
	json jid = json_find(j, "id");
	json juuid = json_find(j, "uuid");
	uuid u;

	if (juuid)
		uuid_from_string(json_get_string(juuid), &u);
	else
		uuid_gen(&u);

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
	json_close(j);
	return 0;
}

static int linda_read(linda l, const char* s, char** dst, int rm, int pred)
{
	return 0;
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

linda linda_open(const char* path, const char* name)
{
	static int first_time = 1;

	if (first_time)
	{
		uuid_seed(time(NULL));
		first_time = 0;
	}

	linda l = (linda)calloc(1, sizeof(struct _linda));
	if (!l) return NULL;
	char filename[1024];
	strcpy(filename, path);
	strcat(filename, SEP);
	strcat(filename, name);
	l->st = store_open(filename, filename, 0);
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
