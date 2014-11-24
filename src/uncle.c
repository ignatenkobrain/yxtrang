#include <stdlib.h>
#include <stdio.h>

#include "uncle.h"
#include "network.h"
#include "jsonq.h"

struct _uncle
{
};

uncle uncle_create(const char* binding, short port)
{
	return NULL;
}

int uncle_add(uncle u, const char* name, short port, int tcp, int ssl)
{
	return 0;
}

int uncle_query(uncle u, const char* name, short port, int tcp, int ssl)
{
	return 0;
}

int uncle_rem(uncle u, int id)
{
	return 0;
}

void uncle_destroy(uncle u)
{
}

