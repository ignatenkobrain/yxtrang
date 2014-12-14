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

#include <httpserver.h>

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
	char body[1024*64];
	char* dst = body;
	dst += sprintf(dst, "<html>\n<title>test</title>\n<body><h1>This is a test</h1>\n</body>\n</html>\n");
	size_t len = dst - body;

	char headers[1024*64];
	dst = headers;
	dst += sprintf(dst, "HTTP/%1.1f 200 OK\n", session_get_udata_real(s));
	dst += sprintf(dst, "Content-Type: text/html\r\n");

	if (session_get_udata_flag(s, HTTP_PERSIST))
	{
		dst += sprintf(dst, "Connection: keep-alive\r\n");
		dst += sprintf(dst, "Content-Length: %u\r\n", (unsigned)len);
	}
	else
		dst += sprintf(dst, "Connection: close\r\n");

	dst += sprintf(dst, "\r\n");

	if (!session_writemsg(s, headers))
		return 0;

	if (!g_quiet) printf("%s", headers);

	// The response-body...

	if (!session_writemsg(s, body))
		return 0;

	if (!g_quiet) printf("%s", body);
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

	httpserver http = httpserver_create(&on_session, NULL);

	if (!handler_add_server(h, &httpserver_handler, http, binding, port, 1, ssl, NULL))
		return 1;

	handler_wait(h);
	handler_destroy(h);
	httpserver_destroy(http);
	return 0;
}
