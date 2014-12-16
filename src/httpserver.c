#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

#include "httpserver.h"

#define PREFIX "_"
#define MAX_REQS 64

int g_http_debug = 0;

struct _httpserver
{
	struct _httpserver_reqs reqs[MAX_REQS];
	int (*f)(session,void*);
	void* data;
};

static char* strstri(const char* src, const char* s)
{
	while (*src)
	{
		const char* c1 = src;
		const char* c2 = s;

		while (tolower(*c1) == tolower(*c2))
		{
			c1++; c2++;

			if (!*c2)
				return (char*)src;
		}

		src++;
	}

	return 0;
}

static const char* url_decode(const char* src, char* dst)
{
	const char* save_dst = dst;

	while (*src)
	{
		if (*src == '+')
		{
			*dst++ = ' ';
			src++;
		}
		else if (*src == '%')
		{
			src++;
			char buf[3];
			buf[0] = *src++;
			buf[1] = *src++;
			buf[2] = 0;
			int num;
			sscanf(buf, "%2X", &num);
			char ch = num;
			*dst++ = ch;
		}
		else
			*dst++ = *src++;
	}

	*dst = 0;
	return save_dst;
}

static char* lower(char* src)
{
	char* save_src = src;

	while (*src)
	{
		*src = _tolower(*src);
		src++;
	}

	return save_src;
}

static void decode_data(session s, const char* str)
{
	const char* src = str;
	char tmpbuf2[1024], tmpbuf3[8192], tmpbuf5[8192];
	char tmpbuf4[1024*2];
	char tmpbuf[1024];
	char* dst = tmpbuf;

	while (*src)
	{
		if (*src == '&')
		{
			src++;
			*dst = 0;
			tmpbuf2[0] = tmpbuf3[0] = 0;
			sscanf(tmpbuf, "%1023[^=]=%8191s", tmpbuf2, tmpbuf3);
			tmpbuf2[sizeof(tmpbuf2)-1] = tmpbuf3[sizeof(tmpbuf3)-1] = 0;
			strcpy(tmpbuf4, PREFIX);
			strcat(tmpbuf4, url_decode(tmpbuf2, tmpbuf5));
			session_set_stash(s, tmpbuf4, url_decode(tmpbuf3, tmpbuf5));
			dst = tmpbuf;
		}
		else
			*dst++ = *src++;
	}

	*dst = 0;
	tmpbuf2[0] = tmpbuf3[0] = 0;
	sscanf(tmpbuf, "%1023[^=]=%8191s", tmpbuf2, tmpbuf3);
	tmpbuf2[sizeof(tmpbuf2)-1] = tmpbuf3[sizeof(tmpbuf3)-1] = 0;
	strcpy(tmpbuf4, PREFIX);
	strcat(tmpbuf4, url_decode(tmpbuf2, tmpbuf5));
	session_set_stash(s, tmpbuf4, url_decode(tmpbuf3, tmpbuf5));
}

void* httpserver_get_content(session s)
{
	long len = atol(session_get_stash(s, "content-length"));
	if (!len) return NULL;
	char* data = (char*)malloc(len+1);

	if (!session_read(s, data, len))
	{
		free(data);
		return NULL;
	}

	data[len] = 0;
	return data;
}

static int get_postdata(session s)
{
	const char* ct = session_get_stash(s, "content-type");

	if (!strstri(ct, "application/x-www-form-urlencoded"))
		return 0;

	char* query = (char*)httpserver_get_content(s);
	decode_data(s, query);
	free(s);
	return 1;
}

int httpserver_handler(session s, void* p1)
{
	if (!s)
		return 0;

	if (session_on_connect(s))
	{
		if (g_http_debug) printf("CONNECTED\n");
		session_set_udata_flag(s, HTTP_READY);
		return 1;
	}

	if (session_on_disconnect(s))
	{
		if (g_http_debug) printf("DISCONNECTED\n");
		return 0;
	}

	char* msg;

	if (!session_readmsg(s, &msg))
		return 0;

	if (g_http_debug) printf("DATA: %s", msg);

	int is_ready = session_get_udata_flag(s, HTTP_READY);

	// Process command...

	if (is_ready)
	{
		session_clr_udata_flag(s, HTTP_READY);

		char cmd[20], path[8192], ver[20];
		cmd[0] = path[0] = ver[0] = 0;
		sscanf(msg, "%19s /%8191s %*[^/]/%19[^\r\n]", cmd, path, ver);
		cmd[sizeof(cmd)-1] = path[sizeof(path)-1] = ver[sizeof(ver)-1] = 0;
		session_set_stash(s, HTTP_COMMAND, cmd);
		session_set_stash(s, HTTP_VERSION, ver);
		double v = atof(ver);
		char filename[1024], query[8192];
		filename[0] = query[0] = 0;
		sscanf(path, "%1023[^?]?%8191s", filename, query);
		filename[sizeof(filename)-1] = query[sizeof(query)-1] = 0;
		session_set_stash(s, HTTP_RESOURCE, url_decode(filename, path));
		session_set_stash(s, HTTP_QUERYSTRING, query);
		decode_data(s, query);

		if (v == 1.1)
		{
			session_set_udata_flag(s, HTTP_v11);
			session_set_udata_flag(s, HTTP_PERSIST);
		}
		else if (v == 1.0)
			session_set_udata_flag(s, HTTP_v10);

		if (!strcasecmp(cmd, "HEAD"))
			session_set_udata_flag(s, HTTP_HEAD);
		else if (!strcasecmp(cmd, "GET"))
			session_set_udata_flag(s, HTTP_GET);
		else if (!strcasecmp(cmd, "POST"))
			session_set_udata_flag(s, HTTP_POST);
		else if (!strcasecmp(cmd, "PUT"))
			session_set_udata_flag(s, HTTP_PUT);
		else if (!strcasecmp(cmd, "DELETE"))
			session_set_udata_flag(s, HTTP_DELETE);

		return 1;
	}

	// Process headers...

	const char* src = msg;

	if (*src == '\r')
		src++;

	if (*src != '\n')
	{
		char name[1024], value[8192];
		name[0] = value[0] = 0;
		sscanf(msg, "%1023[^:]: %8191[^\r\n]", name, value);
		name[sizeof(name)-1] = value[sizeof(value)-1] = 0;
		session_set_stash(s, lower(name), value);

		if (!strcasecmp(name, "connection"))
		{
			if (strstri(value, "keep-alive"))
				session_set_udata_flag(s, HTTP_PERSIST);
			else if (strstri(value, "close"))
				session_clr_udata_flag(s, HTTP_PERSIST);
		}
		else if (!strcasecmp(name, "host"))
		{
			char tmpbuf2[256], tmpbuf3[256];
			tmpbuf2[0] = tmpbuf3[0] = 0;
			sscanf(value, "%255[^:]:%255s", tmpbuf2, tmpbuf3);
			tmpbuf2[sizeof(tmpbuf2)-1] = tmpbuf3[sizeof(tmpbuf3)-1] = 0;
			session_set_stash(s, HTTP_HOST, tmpbuf2);
			session_set_stash(s, HTTP_PORT, tmpbuf3);
		}

		return 1;
	}

	// Process body...

	if (session_get_udata_flag(s, HTTP_POST))
		get_postdata(s);

	httpserver h = (httpserver)p1;

	if (!h->f)
	{
		const char* filename = session_get_stash(s, HTTP_RESOURCE);

		size_t i = 0;

		while (h->reqs[i].f)
		{
			if (!h->reqs[i].path)
			{
				h->reqs[i].f(s, h->reqs[i].data);
				break;
			}

			if (!strcmp(h->reqs[i].path, filename))
			{
				h->reqs[i].f(s, h->reqs[i].data);
				break;
			}

			i++;

			if (i == MAX_REQS)
				break;
		}
	}
	else
		h->f(s, h->data);

	// If persistent then reset...

	if (session_get_udata_flag(s, HTTP_PERSIST))
	{
		session_clr_udata_flags(s);
		session_set_udata_flag(s, HTTP_READY);
		return 1;
	}

	// Otherwise...

	session_shutdown(s);
	return 1;
}

const char* httpserver_value(session s, const char* name)
{
	if (!s)
		return NULL;

	char tmpbuf[1024];
	strcpy(tmpbuf, PREFIX);
	strcat(tmpbuf, name);
	return session_get_stash(s, tmpbuf);
}

int httpserver_response(session s, unsigned code, const char* msg, size_t len, const char* content_type)
{
	if (!s || !msg)
		return 0;

	char headers[1024];
	char* dst = headers;
	dst += sprintf(dst, "HTTP/%s %u %s\n", session_get_stash(s, HTTP_VERSION), code, msg);

	if (content_type && *content_type)
		dst += sprintf(dst, "Content-Type: %s\r\n", content_type);

	if (session_get_udata_flag(s, HTTP_PERSIST))
	{
		dst += sprintf(dst, "Connection: keep-alive\r\n");
		dst += sprintf(dst, "Content-Length: %u\r\n", (unsigned)len);
	}
	else
		dst += sprintf(dst, "Connection: close\r\n");

	dst += sprintf(dst, "\r\n");

	if (g_http_debug) printf("SEND: %s", headers);

	if (!session_writemsg(s, headers))
		return 0;

	long ct_len = atol(session_get_stash(s, "content-length"));

	if ((code > 299) && (ct_len != 0))
		session_clr_udata_flag(s, HTTP_PERSIST);

	return 1;
}

httpserver httpserver_create(int (*f)(session,void*), void* p1)
{
	httpserver h = (httpserver)calloc(1, sizeof(struct _httpserver));
	h->f = f;
	h->data = p1;
	return h;
}

httpserver httpserver_create2(struct _httpserver_reqs* reqs)
{
	httpserver h = (httpserver)calloc(1, sizeof(struct _httpserver));
	size_t i = 0;

	while (reqs->f)
	{
		h->reqs[i].f = reqs->f;
		h->reqs[i].path = reqs->path;
		h->reqs[i].data = reqs->data;
		i++;
	}

	h->reqs[i].f = NULL;
	return h;
}

void httpserver_destroy(httpserver h)
{
	if (!h)
		return;

	free(h);
}
