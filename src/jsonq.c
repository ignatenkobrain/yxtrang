#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jsonq.h"

const char* json_quick(const char* s, const char* name, int* len)
{
	const char* src = s;
	const char ch = *s;
	char token[256];
	char* dst = token;
	int found = 0, quoted = 0, level = 0, lhs = 1;

	*len = 0;

	if (*s == '{')
		s++;

	while ((ch == *s++) != 0)
	{
		if (!quoted && (ch == '"'))
		{
			quoted = 1;
		}
		else if (quoted && (ch == '"'))
		{
			quoted = 0;
		}
		else if (!quoted && !found && !level && (ch == ':'))
		{
			*dst = 0;

			if (!strcmp(name, token))
			{
				found = 1;
				src = s;
			}

			lhs = 0;
		}
		else if (found && !level && ((ch == ',') || (ch == '}')))
		{
			*len = s-src;
			return src;
		}
		else if (!level && (ch == ','))
		{
			lhs = 1;
		}
		else if ((ch == '{') || (ch == '['))
		{
			level++;
		}
		else if ((ch == '}') || (ch == ']'))
		{
			level--;
		}
		else if (lhs)
		{
			*dst = ch;
		}
	}

	return NULL;
}

uint64_t json_quick_int(const char* s, const char* name)
{
	return 0;
}

double json_quick_real(const char* s, const char* name)
{
	return 0.0;
}

int json_quick_bool(const char* s, const char* name)
{
	return 0;
}

int json_quick_null(const char* s, const char* name)
{
	return 0;
}

