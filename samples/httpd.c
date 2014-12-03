// For illustration purposes only...

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

#include <network.h>

static int g_quiet = 0;
static const char* g_www = "/var/www";

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

static int on_session(session s, void* param)
{
	enum { CMD=0, HEAD, GET, HTTP10, HTTP11, PERSIST, LAST=63 };

	if (session_on_connect(s))
	{
		if (!g_quiet) printf("CONNECTED\n");
		session_set_udata_flag(s, CMD);
		return 1;
	}

	if (session_on_disconnect(s))
	{
		if (!g_quiet) printf("DISCONNECTED\n");
		return 0;
	}

	char* msg;

	if (!session_readmsg(s, &msg))
		return 0;

	if (!g_quiet) printf("DATA: %s", msg);

	int is_cmd = session_get_udata_flag(s, CMD);

	// Process command...

	if (is_cmd)
	{
		session_clr_udata_flag(s, CMD);

		char cmd[20], path[1024], ver[20];
		cmd[0] = path[0] = ver[0] = 0;
		sscanf(msg, "%19s %1023s %*[^/]/%19[^\r\n]", cmd, path, ver);
		cmd[19] = path[1023] = ver[19] = 0;
		double v = atof(ver);
		session_set_udata_real(s, v);

		if (v == 1.1)
		{
			session_set_udata_flag(s, HTTP11);
			session_set_udata_flag(s, PERSIST);
		}
		else if (v == 1.0)
			session_set_udata_flag(s, HTTP10);

		if (!strcmp(cmd, "HEAD"))
			session_set_udata_flag(s, HEAD);
		else if (!strcmp(cmd, "GET"))
			session_set_udata_flag(s, GET);

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
				session_set_udata_flag(s, PERSIST);
			else if (strstri(value, "close"))
				session_clr_udata_flag(s, PERSIST);
		}

		return 1;
	}

	// Process body...

	char tmpbuf[1024*64], tmpbuf2[1024*64];
	char* dst = tmpbuf;
	char* dst2 = tmpbuf2;
	dst2 += sprintf(dst2, "<html>\n<title>test</title>\n<body><h1>This is a test</h1>\n</body>\n</html>\n");

	dst += sprintf(dst, "HTTP/%1.1f 200 OK\n", session_get_udata_real(s));
	dst += sprintf(dst, "Content-Type: text/html\r\n");

	if (session_get_udata_flag(s, PERSIST))
	{
		dst += sprintf(dst, "Connection: keep-alive\r\n");
		dst += sprintf(dst, "Content-Length: %llu\r\n", (unsigned long long)(dst2-tmpbuf2));
	}
	else
		dst += sprintf(dst, "Connection: close\r\n");

	dst += sprintf(dst, "\r\n");

	// The response-headers...

	if (!session_writemsg(s, tmpbuf))
		return 0;

	// The response-body...

	if (!session_writemsg(s, tmpbuf2))
		return 0;

	if (!g_quiet) printf("%s", tmpbuf);

	// If persistent, clean-up...

	if (session_get_udata_flag(s, PERSIST))
	{
		session_clr_udata_flags(s);
		session_set_udata_flag(s, CMD);
		return 1;
	}

	// Finish-up...

	session_shutdown(s);
	return 1;
}

int main(int ac, char** av)
{
	printf("Usage: httpd [port|8080 [ssl|0 [quiet|0 [threads|0 [www|/var/www]]]]]]\n");
	const char* binding = NULL;
	unsigned short port = (short)(ac>1?atoi(av[1]):8080);
	int ssl = (ac>2?atoi(av[2]):0);
	g_quiet = (ac>3?atoi(av[3]):0);
	int threads = (ac>4?atoi(av[4]):0);
	g_www = (ac>5?av[5]:"/var/www");
	void* param = (void*)0;

	handler h = handler_create(threads);

	if (ssl)
		handler_set_tls(h, "server.pem");

	if (!handler_add_server(h, &on_session, param, binding, port, 1, ssl, NULL))
		return 1;

	handler_wait(h);
	handler_destroy(h);
	return 0;
}
