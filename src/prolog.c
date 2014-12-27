#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "prolog.h"

char* get_line(char* src, char** buf)
{
	if (!src)
		return NULL;

	if (!*src)
		return NULL;

	size_t len = 0, max_len = 1024;
	char* dst = malloc(max_len);
	char ch;

	while ((ch = *src++) != 0)
	{
		if (ch == '%')
		{
			while ((ch = *src++) != 0)
			{
				if (ch == '\n')
					break;
			}

			break;
		}

		if (ch == '\r')
			continue;

		if (ch == '\n')
			break;

		if (++len == max_len)
		{
			max_len *= 2;
			dst = realloc(dst, max_len);
			len = 1;
		}

		*dst++ = ch;
	}

	*dst = '\0';
	*buf = dst;
	return src;
}

void test(const char* f)
{
}
