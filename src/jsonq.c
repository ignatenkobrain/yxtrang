#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jsonq.h"

static const char *escapes = "\a\f\b\t\v\r\n\0";
static const char *anti_escapes = "afbtvrn0";

const char *jsonq(const char *s, const char *name, char *dstbuf, int dstlen)
{
	const char *src = s;
	char tmpbuf[1024];
	char *dst = tmpbuf;
	int found = 0, quoted = 0, level = 0, lhs = 1;
	char ch;

	if (*s == '{')
		s++;

	while ((ch = *s++) != 0)
	{
		if (!quoted && (ch == '"'))
			quoted = 1;
		else if (quoted && (ch == '"'))
			quoted = 0;
		else if (quoted && lhs && (ch == '\\'))
		{
			ch = *s++;
			const char *ptr = strchr(anti_escapes, ch);
			if (ptr) *dst++ = escapes[ptr-anti_escapes];
			else *dst++ = ch;
		}
		else if (!quoted && lhs && !found && !level && (ch == ':'))
		{
			*dst = 0;
			dst = tmpbuf;

			if (!strcmp(name, tmpbuf))
			{
				found = 1;
				src = s;
			}

			lhs = 0;
		}
		else if (found && !level && ((ch == ',') || (ch == '}')))
		{
			int len = (s-src)-1;

			if (*src == '"')
			{
				dst = dstbuf;
				src++;
				len -= 2;

				while (len-- > 0)
				{
					ch = *src++;

					if (ch == '\\')
					{

						ch = *src++;
						len--;
						const char *ptr = strchr(anti_escapes, ch);
						if (ptr) *dst++ = escapes[ptr-anti_escapes];
						else *dst++ = ch;
					}
					else
						*dst++ = ch;
				}
			}
			else
				strncpy(dstbuf, src, len<dstlen?len:dstlen);

			dstbuf[len] = 0;
			return dstbuf;
		}
		else if (!level && (ch == ','))
			lhs = 1;
		else if ((ch == '{') || (ch == '['))
			level++;
		else if ((ch == '}') || (ch == ']'))
			level--;
		else if (lhs)
			*dst++ = ch;
	}

	*dstbuf = 0;
	return dstbuf;
}

long long jsonq_int(const char *s, const char *name)
{
	char tmpbuf[1024];
	jsonq(s, name, tmpbuf, sizeof(tmpbuf));
	long long v = 0;
	sscanf(tmpbuf, "%lld", &v);
	return v;
}

double jsonq_real(const char *s, const char *name)
{
	char tmpbuf[1024];
	jsonq(s, name, tmpbuf, sizeof(tmpbuf));
	double v = 0.0;
	sscanf(tmpbuf, "%lg", &v);
	return v;
}

int jsonq_bool(const char *s, const char *name)
{
	char tmpbuf[1024];
	jsonq(s, name, tmpbuf, sizeof(tmpbuf));
	return !strcmp(tmpbuf, "true");
}

int jsonq_null(const char *s, const char *name)
{
	char tmpbuf[1024];

	if (!*jsonq(s, name, tmpbuf, sizeof(tmpbuf)))
		return 1;

	return !strcmp(tmpbuf, "null");
}

