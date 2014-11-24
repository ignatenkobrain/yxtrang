#include <stdlib.h>
#include <stdio.h>

#include "uncle.h"
#include "network.h"
#include "jsonq.h"

struct _uncle
{
};

uncle uncle_create(const char* binding, short port);

// Add, query, remove ephemeral resources.

int uncle_add(const char* name, short port, int tcp, int ssl)
{
	return 0;
}

int uncle_query(const char* name, short port, int tcp, int ssl)
{
	return 0;
}

int uncle_rem(int id)
{
	return 0;
}

void uncle_destroy()
{
}

