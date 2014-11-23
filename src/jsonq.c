#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jsonq.h"

const char* jsonq(const char* s, const char* name, char* dstbuf, int dstlen)
{
	const char* src = s;
	char token[256];
	char* dst = token;
	int found = 0, quoted = 0, level = 0, lhs = 1;
	char quote = 0;

	if (*s == '{')
		s++;

	while (*s)
	{
		const char ch = *s++;

		if (!quoted && (ch == '"'))
		{
			quote = '"';
			quoted = 1;
		}
		else if (!quoted && (ch == '\''))
		{
			quote = '\'';
			quoted = 1;
		}
		else if (quoted && (ch == quote))
		{
			quoted = 0;
		}
		else if (!quoted && !found && !level && (ch == ':'))
		{
			*dst = 0;
			dst = token;

			if (!strcmp(name, token))
			{
				found = 1;
				src = s;
			}

			lhs = 0;
		}
		else if (found && !level && ((ch == ',') || (ch == '}')))
		{
			int len = (s-src)-1;

			if (*src == quote)
			{
				src++;
				len -= 2;
			}

			strncpy(dstbuf, src, len<dstlen?len:dstlen);
			dstbuf[len] = 0;
			return dstbuf;
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
			*dst++ = ch;
		}
	}

	return NULL;
}

int64_t jsonq_int(const char* s, const char* name)
{
	char tmpbuf[1024];
	jsonq(s, name, tmpbuf, sizeof(tmpbuf));
	long long v = 0;
	sscanf(tmpbuf, "%lld", &v);
	return v;
}

double jsonq_real(const char* s, const char* name)
{
	char tmpbuf[1024];
	jsonq(s, name, tmpbuf, sizeof(tmpbuf));
	double v = 0.0;
	sscanf(tmpbuf, "%lg", &v);
	return v;
}

int jsonq_bool(const char* s, const char* name)
{
	char tmpbuf[1024];
	jsonq(s, name, tmpbuf, sizeof(tmpbuf));
	return !strcmp(tmpbuf, "true");
}

int jsonq_null(const char* s, const char* name)
{
	char tmpbuf[1024];
	jsonq(s, name, tmpbuf, sizeof(tmpbuf));
	return !strcmp(tmpbuf, "null");
}

