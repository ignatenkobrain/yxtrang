#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <httpserver.h>

extern int g_http_debug;
static const char* g_www_root = "/var/www";

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

int main(int ac, char* av[])
{
	const char* binding = NULL;
	unsigned short port = HTTP_DEFAULT_PORT;
	int ssl = 0, threads = 0;
	int i;

	for (i = 1; i < ac; i++)
	{
		char tmpbuf[256];
		tmpbuf[0] = 0;

		if (!strncmp(av[i], "--port=", 7))
		{
			unsigned tmp;
			sscanf(av[i], "%*[^=]=%u", &tmp);
			port = (short)tmp;
		}
		else if (!strncmp(av[i], "--threads=", 10))
		{
			unsigned tmp;
			sscanf(av[i], "%*[^=]=%u", &tmp);
			threads = (short)tmp;
		}
		else if (!strncmp(av[i], "--ssl=", 6))
		{
			unsigned tmp;
			sscanf(av[i], "%*[^=]=%u", &tmp);
			ssl = (short)tmp;
		}
		else if (!strcmp(av[i], "--ssl"))
		{
			ssl = 1;
		}
		else if (!strncmp(av[i], "--tls=", 6))
		{
			unsigned tmp;
			sscanf(av[i], "%*[^=]=%u", &tmp);
			ssl = (short)tmp;
		}
		else if (!strcmp(av[i], "--tls"))
		{
			ssl = 1;
		}
		else if (!strncmp(av[i], "--debug=", 8))
		{
			unsigned tmp;
			sscanf(av[i], "%*[^=]=%u", &tmp);
			g_http_debug = (short)tmp;
		}
		else if (!strcmp(av[i], "--debug"))
		{
			g_http_debug = 1;
		}
		else if (!strncmp(av[i], "--www=", 6))
		{
			sscanf(av[i], "%*[^=]=%s", tmpbuf);
			tmpbuf[sizeof(tmpbuf)-1] = 0;
			g_www_root = strdup(tmpbuf);
		}
	}

	printf("Usage: httpd --port=%u --ssl=%d --debug=%d --threads=%d --www=%s\n", port, ssl, g_http_debug, threads, g_www_root);

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
