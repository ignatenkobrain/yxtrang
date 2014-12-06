#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "skiplist_int.h"
#include "skiplist_string.h"
#include "json.h"
#include "store.h"
#include "linda.h"

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
	json j2 = json_find(j, "id");
	uuid u = {0};

	if (j2)
	{
		if (json_is_integer(j2))
		{
			long long k = json_get_integer(j2);

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
		else if (json_is_string(j2))
		{
			const char* k = json_get_string(j2);

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

	json_close(j);
	return 0;
}

int linda_in(linda l, const char* s, char** dst)
{
	if (!l)
		return 0;

	return 0;
}

int linda_inp(linda l, const char* s, char** dst)
{
	if (!l)
		return 0;

	return 0;
}

int linda_rd(linda l, const char* s, char** dst)
{
	if (!l)
		return 0;

	return 0;
}

int linda_rdp(linda l, const char* s, char** dst)
{
	if (!l)
		return 0;

	return 0;
}

linda linda_open(const char* name)
{
	linda l = (linda)calloc(1, sizeof(struct _linda));
	l->st = store_open(name, name, 0);
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
