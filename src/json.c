#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "json.h"

typedef enum
{
	type_none=0, type_null=1, type_false=2, type_true=3,
	type_integer=4, type_real=5, type_string=6,
	type_object=7, type_array=8
}
 json_type;

#define NAME_SIZE 256

struct _json
{
	char name[NAME_SIZE];		// should be of unlimited size
	json_type type;
	size_t cnt;

	union
	{
		json head;
		char* str;
		long long integer;
		double real;
	};

	json next;
};

#ifdef _WIN32
static long long strtoll(const char* src, char** end, int base)
{
	long long n = 0;
	sscanf(src, "%lld", &n);

	while (isdigit(*src))
		src++;

	*end = (char*)src;
	return n;
}
#endif

const char* json_format_string(const char* src, char* buf, size_t buflen)
{
	char* dst = buf;

	while (*src && (buflen > 3))
	{
		if (*src == '"')
		{
			*dst++ = '\\';
			*dst++ = *src++;
			buflen -= 2;
		}
		else if (*src == '\r')
		{
			*dst++ = '\\';
			*dst++ = 'r';
			src++;
			buflen -= 2;
		}
		else if (*src == '\n')
		{
			*dst++ = '\\';
			*dst++ = 'n';
			src++;
			buflen -= 2;
		}
		else if (*src == '\t')
		{
			*dst++ = '\\';
			*dst++ = 't';
			src++;
			buflen -= 2;
		}
		else if (*src == '\b')
		{
			*dst++ = '\\';
			*dst++ = 'b';
			src++;
			buflen -= 2;
		}
		else if (*src == '\f')
		{
			*dst++ = '\\';
			*dst++ = 'f';
			src++;
			buflen -= 2;
		}
		else if (*src == '\\')
		{
			*dst++ = '\\';
			*dst++ = '\\';
			src++;
			buflen -= 2;
		}
		else
		{
			*dst++ = *src++;
			buflen -= 1;
		}
	}

	*dst = 0;
	return buf;
}

static char* unicode_to_utf8(char* dst, unsigned c)
{
	if (c < 0x80)
		*dst++ = c;
	else if (c < 0x800)
	{
		*dst++ = (unsigned)(192+c/64);
		*dst++ = (unsigned)(128+c%64);
	}
	else if (c-0xd800u < 0x800)
		return dst;
	else if (c < 0x10000)
	{
		*dst++ = (unsigned)(224+c/4096);
		*dst++ = (unsigned)(128+c/64%64);
		*dst++ = (unsigned)(128+c%64);
	}
	else if (c < 0x110000)
	{
		*dst++ = (unsigned)(240+c/262144);
		*dst++ = (unsigned)(128+c/4096%64);
		*dst++ = (unsigned)(128+c/64%64);
		*dst++ = (unsigned)(128+c%64);
	}

	return dst;
}

static void _json_open(char** str, json j, const int is_array)
{
	char* s = (char*)*str;

	if (!*s)
		return;

	while (isspace(*s))
		s++;

	while (*s)
	{
		if ((*s == '}') || (*s == ']'))
		{
			*str = ++s;
			return;
		}

		if (*s == ',')
		{
			s++;
			j->next = (json)calloc(1, sizeof(struct _json));
			if (!j->next) return;
			j = j->next;
		}

		if (!is_array)
		{
			if ((*s == '\"') || (*s == '\''))
			{
				char quote = *s++;
				char* dst = j->name;

				while (*s && (*s != quote))
				{
					char ch = *s++;

					if (ch == '\\')
					{
						ch = *s++;

						if (ch == 'b')
							ch = '\b';
						else if (ch == 'f')
							ch = '\f';
						else if (ch == 'n')
							ch = '\n';
						else if (ch == 'r')
							ch = '\r';
						else if (ch == 't')
							ch = '\t';
						else if (ch == 'u')
						{
							char tmpbuf[10];
							tmpbuf[0] = *s++;
							tmpbuf[1] = *s++;
							tmpbuf[2] = *s++;
							tmpbuf[3] = *s++;
							tmpbuf[4] = 0;
							unsigned val = 0;
							sscanf(tmpbuf, "%X", &val);
							dst = unicode_to_utf8(dst, val);

							if ((dst-j->name) > (NAME_SIZE-4))
								break;

							continue;
						}
					}

					*dst++ = ch;

					if ((dst-j->name) > (NAME_SIZE-1))
						break;
				}

				*dst = 0;
				s++;
			}

			while (isspace(*s))
				s++;

			if (*s == ':')
				s++;

			while (isspace(*s))
				s++;
		}

		if (*s == '{')
		{
			j->type = type_object;
			j->head = (json)calloc(1, sizeof(struct _json));
			if (!j->head) return;
			s++;
			_json_open(&s, j->head, 0);
		}
		else if (*s == '[')
		{
			j->type = type_array;
			j->head = (json)calloc(1, sizeof(struct _json));
			if (!j->head) return;
			s++;
			_json_open(&s, j->head, 1);
		}
		else if ((*s == '\"') || (*s == '\''))
		{
			char quote = *s++;
			j->type = type_string;
			size_t max_bytes = 32;
			j->str = (char*)malloc(max_bytes+4);
			if (!j->str) return;
			char* dst = j->str;
			size_t len = 0;

			while (*s && (*s != quote))
			{
				char ch = *s++;

				if (ch == '\\')
				{
					ch = *s++;

					if (ch == 'b')
						ch = '\b';
					else if (ch == 'f')
						ch = '\f';
					else if (ch == 'n')
						ch = '\n';
					else if (ch == 'r')
						ch = '\r';
					else if (ch == 't')
						ch = '\t';
					else if (ch == 'u')
					{
						char tmpbuf[10];
						tmpbuf[0] = *s++;
						tmpbuf[1] = *s++;
						tmpbuf[2] = *s++;
						tmpbuf[3] = *s++;
						tmpbuf[4] = 0;
						unsigned val = 0;
						sscanf(tmpbuf, "%X", &val);
						char* tmp_dst = unicode_to_utf8(dst, val);
						len += tmp_dst - dst;
						dst = tmp_dst;

						if (len > max_bytes)
						{
							j->str = (char*)realloc(j->str, (max_bytes*=2, max_bytes+4));
							if (!j->str) return;
							dst = j->str+len;
						}

						continue;
					}
				}

				*dst++ = ch;
				len++;

				if (len > max_bytes)
				{
					j->str = (char*)realloc(j->str, (max_bytes*=2, max_bytes+4));
					if (!j->str) return;
					dst = j->str+len;
				}
			}

			*dst = 0;
			s++;
		}
		else if (!strncmp(s, "null", 4))
		{
			j->type = type_null;
			s += 4;
		}
		else if (!strncmp(s, "false", 5))
		{
			j->type = type_false;
			s += 5;
		}
		else if (!strncmp(s, "true", 4))
		{
			j->type = type_true;
			s += 4;
		}
		else
		{
			const char* save_s = s;
			int any = 0;

			while (isdigit(*s) || (*s == '.') || (*s == 'E') || (*s == 'e'))
			{
				if ((*s == '.') || (*s == 'E') || (*s == 'e'))
					any++;

				s++;
			}

			if (any)
			{
				j->type = type_real;
				j->real = strtod(save_s, &s);
			}
			else
			{
				j->type = type_integer;
				j->integer = strtoll(save_s, &s, 10);
			}
		}

		while (isspace(*s))
			s++;
	}

	*str = s;
}

json json_open(const char* str)
{
	if ((*str != '{') && (*str != '['))
		return NULL;

	json j = (json)calloc(1, sizeof(struct _json));
	if (!j) return NULL;
	int is_array = *str++ == '[';

	if (is_array)
		json_set_array(j);
	else
		json_set_object(j);

	j->head = (json)calloc(1, sizeof(struct _json));
	if (!j->head) { free (j); return NULL; }
	_json_open((char**)&str, j->head, is_array);
	return j;
}

json json_init()
{
	return (json)calloc(1, sizeof(struct _json));
}

static json json_add(json j)
{
	if (!j)
		return NULL;

	json last = j;

	while (j)
	{
		last = j;
		j = j->next;
	}

	last->cnt++;
	return last->next = (json)calloc(1, sizeof(struct _json));
}

json json_array_add(json j)
{
	if (!j)
		return NULL;

	if (!j->head)
	{
		j->head = (json)calloc(1, sizeof(struct _json));
		return j->head;
	}

	return json_add(j->head);
}

json json_object_add(json j, const char* name)
{
	if (!j)
		return NULL;

	if (!j->head)
	{
		j = j->head = (json)calloc(1, sizeof(struct _json));
		strncpy(j->name, name, sizeof(j->name)-1);
		j->name[sizeof(j->name)-1] = 0;
		return j;
	}

	j = (json)json_add(j->head);
	strncpy(j->name, name, sizeof(j->name)-1);
	j->name[sizeof(j->name)-1] = 0;
	return j;
}

int json_rem(json ptr, json ptr2)
{
	if (!ptr || !ptr2)
		return 0;

	json* last = &ptr->head;
	json j = ptr->head;

	while (j)
	{
		if (j == ptr2)
		{
			*last = j->next;
			json_close(ptr2);
			ptr->cnt--;
			return 1;
		}

		last = &j;
		j = j->next;
	}

	return 0;
}

json json_create(json j, const char* name)
{
	if (!j)
		return NULL;

	json ptr = (json)json_find(j, name);

	if (ptr)
		return ptr;

	ptr = (json)json_add(j);
	strncpy(ptr->name, name, sizeof(ptr->name)-1);
	ptr->name[sizeof(ptr->name)-1] = 0;
	return ptr;
}

json json_get_object(json j)
{
	if (!j)
		return NULL;

	if (j->type != type_object)
		return NULL;

	return j->head;
}

json json_get_array(json j)
{
	if (!j)
		return NULL;

	if (j->type != type_array)
		return NULL;

	return j->head;
}

json json_find(json j, const char* name)
{
	while (j)
	{
		if (!strcmp(name, j->name))
			return j;

		j = j->next;
	}

	return NULL;
}

size_t json_count(const json j)
{
	if (!j)
		return 0;

	return j->cnt;
}

json json_index(const json ptr, size_t idx)
{
	json j = (json)ptr;

	if (!j)
		return NULL;

	while (j)
	{
		if (idx-- == 0)
			return j;

		j = j->next;
	}

	return NULL;
}

int json_set_integer(json j, long long value)
{
	if (!j)
		return 0;

	if ((j->type == type_array) || (j->type == type_object))
		json_close(j->head);
	else if ((j->type == type_string) && j->str)
		free(j->str);

	j->type = type_integer;
	j->integer = value;
	return 1;
}

int json_set_real(json j, double value)
{
	if (!j)
		return 0;

	if ((j->type == type_array) || (j->type == type_object))
		json_close(j->head);
	else if ((j->type == type_string) && j->str)
		free(j->str);

	j->type = type_real;
	j->real = value;
	return 1;
}

int json_set_string(json j, const char* value)
{
	if (!j || !value)
		return 0;

	if ((j->type == type_array) || (j->type == type_object))
		json_close(j->head);
	else if ((j->type == type_string) && j->str)
		free(j->str);

	j->type = type_string;
	j->str = strdup(value);
	return 1;
}

int json_set_true(json j)
{
	if (!j)
		return 0;

	if ((j->type == type_array) || (j->type == type_object))
		json_close(j->head);
	else if ((j->type == type_string) && j->str)
		free(j->str);

	j->type = type_true;
	return 1;
}

int json_set_false(json j)
{
	if (!j)
		return 0;

	if ((j->type == type_array) || (j->type == type_object))
		json_close(j->head);
	else if ((j->type == type_string) && j->str)
		free(j->str);

	j->type = type_false;
	return 1;
}

int json_set_null(json j)
{
	if (!j)
		return 0;

	if ((j->type == type_array) || (j->type == type_object))
		json_close(j->head);
	else if ((j->type == type_string) && j->str)
		free(j->str);

	j->type = type_null;
	return 1;
}

int json_set_array(json j)
{
	if (!j)
		return 0;

	if ((j->type == type_array) || (j->type == type_object))
		json_close(j->head);
	else if ((j->type == type_string) && j->str)
		free(j->str);

	j->type = type_array;
	j->head = 0;
	return 1;
}

int json_set_object(json j)
{
	if (!j)
		return 0;

	if ((j->type == type_array) || (j->type == type_object))
		json_close(j->head);
	else if ((j->type == type_string) && j->str)
		free(j->str);

	j->type = type_object;
	j->head = 0;
	return 1;
}

long long json_get_integer(const json j)
{
	if (!j)
		return 0;

	if (j->type == type_integer)
		return j->integer;

	return 0;
}

double json_get_real(const json j)
{
	if (!j)
		return 0.0;

	if (j->type == type_real)
		return j->integer;

	return 0.0;
}

const char* json_get_string(const json j)
{
	if (!j)
		return NULL;

	if (j->type == type_string)
		return j->str;

	return NULL;
}

int json_is_integer(const json j)
{
	if (!j)
		return 0;

	return j->type == type_integer;
}

int json_is_real(const json j)
{
	if (!j)
		return 0;

	return j->type == type_real;
}

int json_is_string(const json j)
{
	if (!j)
		return 0;

	return j->type == type_string;
}

int json_is_true(const json j)
{
	if (!j)
		return 0;

	if (j->type == type_true)
		return 1;

	return 0;
}

int json_is_false(const json j)
{
	if (!j)
		return 0;

	if (j->type == type_false)
		return 1;

	return 0;
}

int json_is_null(const json j)
{
	if (!j)
		return 0;

	if (j->type == type_null)
		return 1;

	return 0;
}

int json_get_type(const json j)
{
	if (!j)
		return 0;

	return (int)j->type;
}

void json_close(json j)
{
	if (!j)
		return;

	while (j)
	{
		json save = j;
		j = j->next;

		if ((save->type == type_object) || (save->type == type_array))
			json_close(save->head);
		else if ((save->type == type_string) && save->str)
			free(save->str);

		free(save);
	}
}

size_t _json_print(char** pdst, char* dst, json j, int structure, size_t* bytes_left, size_t* max_len)
{
	char* save_dst = dst;

	while (j)
	{
		size_t i;

		if (j->name[0])
		{
			if (*max_len && (((strlen(j->name)*2)+256) > *bytes_left))
			{
				*max_len *= 2;
				*max_len += strlen(j->name)*2;
				*pdst = (char*)realloc(*pdst, *max_len);
				if (!*pdst) return 0;
				size_t nbytes = dst - save_dst;
				*bytes_left = *max_len - nbytes;
				save_dst = *pdst;
				dst = save_dst + nbytes;
			}

			dst += i = sprintf(dst, "\"");
			*bytes_left -= i;
			const char* src = j->name;
			int i;

			for (i = 0; i < strlen(j->name); i++)
			{
				if (*src == '"')
				{
					*dst++ = '\\';
					*dst++ = *src++;
					*bytes_left -= 2;
				}
				else if (*src == '\\')
				{
					*dst++ = '\\';
					*dst++ = *src++;
					*bytes_left -= 2;
				}
				else if (*src == '\t')
				{
					*dst++ = '\\';
					*dst++ = 't'; src++;
					*bytes_left -= 2;
				}
				else if (*src == '\r')
				{
					*dst++ = '\\';
					*dst++ = 'r'; src++;
					*bytes_left -= 2;
				}
				else if (*src == '\n')
				{
					*dst++ = '\\';
					*dst++ = 'n'; src++;
					*bytes_left -= 2;
				}
				else if (*src == '\b')
				{
					*dst++ = '\\';
					*dst++ = 'b'; src++;
					*bytes_left -= 2;
				}
				else if (*src == '\f')
				{
					*dst++ = '\\';
					*dst++ = 'f'; src++;
					*bytes_left -= 2;
				}
				else
				{
					*dst++ = *src++;
					*bytes_left -= 1;
				}
			}

			dst += i = sprintf(dst, "\":");
			*bytes_left -= i;
		}

		if (j->type == type_null)
		{
			dst += i = sprintf(dst, "null");
			*bytes_left -= i;
		}
		else if (j->type == type_true)
		{
			dst += i = sprintf(dst, "true");
			*bytes_left -= i;
		}
		else if (j->type == type_false)
		{
			dst += i = sprintf(dst, "false");
			*bytes_left -= i;
		}
		else if (j->type == type_integer)
		{
			dst += i = sprintf(dst, "%lld", j->integer);
			*bytes_left -= i;
		}
		else if (j->type == type_real)
		{
			dst += i = sprintf(dst, "%g", j->real);
			*bytes_left -= i;
		}
		else if (j->type == type_string)
		{
			if (*max_len && (((strlen(j->str)*2)+256) > *bytes_left))
			{
				*max_len *= 2;
				*max_len += strlen(j->str)*2;
				*pdst = (char*)realloc(*pdst, *max_len);
				if (!*pdst) return 0;
				size_t nbytes = dst - save_dst;
				*bytes_left = *max_len - nbytes;
				save_dst = *pdst;
				dst = save_dst + nbytes;
			}

			dst += i = sprintf(dst, "\"");
			*bytes_left -= i;
			const char* src = j->str;
			int i;

			for (i = 0; i < strlen(j->str); i++)
			{
				if (*src == '"')
				{
					*dst++ = '\\';
					*dst++ = *src++;
					*bytes_left -= 2;
				}
				else if (*src == '\\')
				{
					*dst++ = '\\';
					*dst++ = *src++;
					*bytes_left -= 2;
				}
				else if (*src == '\t')
				{
					*dst++ = '\\';
					*dst++ = 't'; src++;
					*bytes_left -= 2;
				}
				else if (*src == '\r')
				{
					*dst++ = '\\';
					*dst++ = 'r'; src++;
					*bytes_left -= 2;
				}
				else if (*src == '\n')
				{
					*dst++ = '\\';
					*dst++ = 'n'; src++;
					*bytes_left -= 2;
				}
				else if (*src == '\b')
				{
					*dst++ = '\\';
					*dst++ = 'b'; src++;
					*bytes_left -= 2;
				}
				else if (*src == '\f')
				{
					*dst++ = '\\';
					*dst++ = 'f'; src++;
					*bytes_left -= 2;
				}
				else
				{
					*dst++ = *src++;
					*bytes_left -= 1;
				}
			}

			dst += i = sprintf(dst, "\"");
			*bytes_left -= i;
		}
		else if (j->type == type_object)
		{
			dst += i = sprintf(dst, "{");
			*bytes_left -= i;

			dst += i = _json_print(pdst, dst, j->head, 1, bytes_left, max_len);
			*bytes_left -= i;

			dst += i = sprintf(dst, "}");
			*bytes_left -= i;
		}
		else if (j->type == type_array)
		{
			dst += i = sprintf(dst, "[");
			*bytes_left -= i;

			dst += i = _json_print(pdst, dst, j->head, 1, bytes_left, max_len);
			*bytes_left -= i;

			dst += i = sprintf(dst, "]");
			*bytes_left -= i;
		}

		j = j->next;

		if (j && structure)
		{
			dst += i = sprintf(dst, ",");
			*bytes_left -= i;
		}
		else if (!structure)
		{
			dst += i = sprintf(dst, "\n");
			*bytes_left -= i;
		}

		if (*max_len && (*bytes_left < 32))
		{
			*max_len *= 2;
			*pdst = (char*)realloc(*pdst, *max_len);
			if (!*pdst) return 0;
			size_t nbytes = dst - save_dst;
			*bytes_left = *max_len - nbytes;
			save_dst = *pdst;
			dst = save_dst + nbytes;
		}
	}

	return dst - save_dst;
}

size_t json_print(char** pdst, json j)
{
	if (!pdst || !j) return 0;

	size_t bytes_left = 0, max_len = 0;

	if (!*pdst)
	{
		*pdst = (char*)malloc(max_len=bytes_left=1024);
		if (!*pdst) return 0;
	}

	return _json_print(pdst, *pdst, j, (j->type==type_object)||(j->type==type_array), &bytes_left, &max_len);
}

char* json_to_string(json j)
{
	char* dst = 0;
	json_print(&dst, j);
	return dst;
}
