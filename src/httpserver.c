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

static int g_debug = 1;

struct _httpserver
{
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

int httpserver_handler(session s, void* p1)
{
	if (!s)
		return 0;

	if (session_on_connect(s))
	{
		if (!g_debug) printf("CONNECTED\n");
		session_set_udata_flag(s, HTTP_READY);
		return 1;
	}

	if (session_on_disconnect(s))
	{
		if (!g_debug) printf("DISCONNECTED\n");
		return 0;
	}

	char* msg;

	if (!session_readmsg(s, &msg))
		return 0;

	if (!g_debug) printf("DATA: %s", msg);

	int is_ready = session_get_udata_flag(s, HTTP_READY);

	// Process command...

	if (is_ready)
	{
		session_clr_udata_flag(s, HTTP_READY);

		char cmd[20], path[8192], ver[20];
		cmd[0] = path[0] = ver[0] = 0;
		sscanf(msg, "%19s %8191s %*[^/]/%19[^\r\n]", cmd, path, ver);
		cmd[sizeof(cmd)-1] = path[sizeof(path)-1] = ver[sizeof(ver)-1] = 0;
		session_set_stash(s, "HTTP_COMMAND", cmd);
		session_set_stash(s, "HTTP_PATH", path);
		session_set_stash(s, "HTTP_VERSION", ver);

		double v = atof(ver);
		session_set_udata_real(s, v);

		char filename[1024], query[8192];
		filename[0] = query[0] = 0;
		sscanf(path, "%1023[^?]?%8191s", filename, query);
		filename[sizeof(filename)-1] = query[sizeof(query)-1] = 0;
		session_set_stash(s, "HTTP_FILENAME", url_decode(filename, path));
		session_set_stash(s, "HTTP_QUERY", url_decode(query, path));
		const char* src = query;
		char tmpbuf2[1024], tmpbuf3[1024];
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
				sscanf(tmpbuf, "%1023[^=]=%1023s", tmpbuf2, tmpbuf3);
				tmpbuf2[sizeof(tmpbuf2)-1] = tmpbuf3[sizeof(tmpbuf3)-1] = 0;
				strcpy(tmpbuf4, "_");
				strcat(tmpbuf4, tmpbuf2);
				session_set_stash(s, tmpbuf4, tmpbuf3);
				dst = tmpbuf;
			}
			else
				*dst++ = *src++;
		}

		*dst = 0;
		tmpbuf2[0] = tmpbuf3[0] = 0;
		sscanf(tmpbuf, "%1023[^=]=%1023s", tmpbuf2, tmpbuf3);
		tmpbuf2[sizeof(tmpbuf2)-1] = tmpbuf3[sizeof(tmpbuf3)-1] = 0;
		strcpy(tmpbuf4, "_");
		strcat(tmpbuf4, tmpbuf2);
		session_set_stash(s, tmpbuf4, tmpbuf3);

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
		session_set_stash(s, name, value);

		if (!strcasecmp(name, "Connection"))
		{
			if (strstri(value, "keep-alive"))
				session_set_udata_flag(s, HTTP_PERSIST);
			else if (strstri(value, "close"))
				session_clr_udata_flag(s, HTTP_PERSIST);
		}

		return 1;
	}

	// Process body...

	httpserver h = (httpserver)p1;
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

httpserver httpserver_create(int (*f)(session,void*), void* p1)
{
	httpserver h = (httpserver)calloc(1, sizeof(struct _httpserver));
	h->f = f;
	h->data = p1;
	return h;
}

void httpserver_destroy(httpserver h)
{
	if (!h)
		return;

	free(h);
}