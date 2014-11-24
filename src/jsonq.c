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
	char quote = 0, ch;

	if (*s == '{')
		s++;

	while ((ch = *s++) != 0)
	{
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
		else if (quoted && (ch == '\\'))
		{
			ch = *s++;

			if (ch == '"')
				*dst++ = ch;
			else if (ch == '\'')
				*dst++ = ch;
			else if (ch == '\\')
				*dst++ = ch;
			else if (ch == '/')
				*dst++ = ch;
			else if (ch == 'b')
				*dst++ = '\b';
			else if (ch == 'f')
				*dst++ = '\f';
			else if (ch == 'n')
				*dst++ = '\n';
			else if (ch == 'r')
				*dst++ = '\r';
			else if (ch == 't')
				*dst++ = '\t';
			else
				*dst++ = ch;
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

						if (ch == '"')
							*dst++ = ch;
						else if (ch == '\'')
							*dst++ = ch;
						else if (ch == '\\')
							*dst++ = ch;
						else if (ch == '/')
							*dst++ = ch;
						else if (ch == 'b')
							*dst++ = '\b';
						else if (ch == 'f')
							*dst++ = '\f';
						else if (ch == 'n')
							*dst++ = '\n';
						else if (ch == 'r')
							*dst++ = '\r';
						else if (ch == 't')
							*dst++ = '\t';
						else
							*dst++ = ch;
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

	*dstbuf = 0;
	return dstbuf;
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

