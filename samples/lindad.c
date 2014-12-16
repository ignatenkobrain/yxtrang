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
#include <linda.h>

extern int g_http_debug;

static int request(session s, void* param)
{
	linda l = (linda)param;
	const char* filename = session_get_stash(s, HTTP_FILENAME);

	if (strcmp(filename, "linda"))
	{
		char body[1024];
		const size_t len =
			sprintf(body,
			"<html><title>test</title>\n<body><h1>Request for: '%s'</h1></body>\n</html>\n",
			filename
			);

		httpserver_response(s, 404, "NOT FOUND", len, "text/html");
		if (g_http_debug) printf("SEND: %s", body);
		return session_write(s, body, len);
	}

	if (!session_get_udata_flag(s, HTTP_GET))
	{
		httpserver_response(s, 501, "METHOD NOT SUPPORTED", 0, NULL);
		return 1;
	}

	hlinda h = linda_begin(l, 0);
	const char* buf = NULL;
	char* query = httpserver_get_content(s);

	if (!query)
	{
		httpserver_response(s, 400, "NO DATA", 0, NULL);
		return 1;
	}

	if (!linda_rdp(h, query, &buf))
	{
		httpserver_response(s, 404, "NOT FOUND", 0, NULL);
		free(query);
		return 1;
	}

	free(query);
	const char* body = buf;
	size_t len = linda_get_length(h);
	linda_end(h);

	httpserver_response(s, 200, "OK", len, "application/json");
	if (g_http_debug) printf("SEND: %s", body);
	return session_write(s, body, len);
	return 1;
}

int main(int ac, char** av)
{
	printf("Usage: lindad [port|8080 [ssl|0 [quiet|0 [threads|0 [path1 [path2]]]]]]\n");
	const char* binding = NULL;
	unsigned short port = (short)(ac>1?atoi(av[1]):8080);
	int ssl = (ac>2?atoi(av[2]):0);
	g_http_debug = !(ac>3?atoi(av[3])>0?1:0:0);
	int threads = (ac>4?atoi(av[4]):0);
	const char* path1 = (ac>5?av[5]:"./db");
	const char* path2 = (ac>6?av[6]:NULL);
	void* param = (void*)0;

	handler h = handler_create(threads);
	if (!h) return 1;

	if (ssl)
		handler_set_tls(h, "server.pem");

	linda l = linda_open(path1, path2);
	if (!l) return 2;

	httpserver http = httpserver_create(&request, l);
	if (!http) return 3;

	if (!handler_add_server(h, &httpserver_handler, http, binding, port, 1, ssl, NULL))
		return 1;

	handler_wait(h);
	handler_destroy(h);
	httpserver_destroy(http);
	linda_close(l);
	return 0;
}
