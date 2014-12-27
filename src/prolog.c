#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "prolog.h"

void do_prolog(const char* filename)
{
	FILE* f = fopen(filename, "r");
	char* buf;

	while (getline(&buf, NULL, f) > 0)
	{
	}

	fclose(f);
}
