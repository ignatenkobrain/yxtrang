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

extern int g_http_debug;
static const char* g_www = "/var/www";

static int http_request(session s, void* param)
{
	const char* filename = session_get_stash(s, HTTP_RESOURCE);
	char body[1024];
	const size_t len =
		sprintf(body,
		"<html>\n<title>404 NOT FOUND</title>\n<body>\n<h1>404 NOT FOUND: '%s'</h1>\n</body>\n</html>\n",
		filename
		);

	httpserver_response(s, 404, "NOT FOUND", len, "text/html");
	if (g_http_debug) printf("HTTP: %s", body);
	return session_write(s, body, len);
}

int main(int ac, char** av)
{
	printf("Usage: httpd [port|%u [ssl|0 [quiet|0 [threads|0 [www|/var/www]]]]]]\n", HTTP_DEFAULT_PORT);
	const char* binding = NULL;
	unsigned short port = (short)(ac>1?atoi(av[1]):HTTP_DEFAULT_PORT);
	int ssl = (ac>2?atoi(av[2]):0);
	g_http_debug = !(ac>3?atoi(av[3])>0?1:0:0);
	int threads = (ac>4?atoi(av[4]):0);
	g_www = (ac>5?av[5]:"/var/www");
	void* param = (void*)0;

	handler h = handler_create(threads);
	if (!h) return 1;

	if (ssl)
		handler_set_tls(h, "server.pem");

	httpserver http = httpserver_create(&http_request, NULL);
	if (!http) return 2;

	if (!handler_add_server(h, &httpserver_handler, http, binding, port, 1, ssl, NULL))
		return 3;

	handler_wait(h);
	handler_destroy(h);
	httpserver_destroy(http);
	return 0;
}
