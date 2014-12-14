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
#include "skiplist_string.h"

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

	int is_cmd = session_get_udata_flag(s, HTTP_READY);

	// Process command...

	if (is_cmd)
	{
		session_clr_udata_flag(s, HTTP_READY);

		char cmd[20], path[1024], ver[20];
		cmd[0] = path[0] = ver[0] = 0;
		sscanf(msg, "%19s %1023s %*[^/]/%19[^\r\n]", cmd, path, ver);
		cmd[19] = path[1023] = ver[19] = 0;
		double v = atof(ver);
		session_set_udata_real(s, v);

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
