#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "skiplist_int.h"
#include "skiplist_string.h"
#include "jsonq.h"
#include "json.h"
#include "store.h"
#include "linda.h"

struct _linda
{
	store st;
	skiplist sl;
};

int linda_out(linda l, const char* s)
{
	if (!l)
		return 0;

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
