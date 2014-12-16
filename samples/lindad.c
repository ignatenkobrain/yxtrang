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
#include <uncle.h>

#define HTTP_DEFAULT_PORT 8080
#define APPLICATION_JSON "application/json"

extern int g_http_debug;

static int linda_request(session s, void* param)
{
	linda l = (linda)param;

	if (!session_get_udata_flag(s, HTTP_GET))
	{
		httpserver_response(s, 501, "METHOD NOT IMPLEMENTED", 0, NULL);
		return 1;
	}

	const char* ct = session_get_stash(s, "content-type");

	if (!strstr(ct, APPLICATION_JSON))
	{
		httpserver_response(s, 415, "BAD MEDIA TYPE", 0, NULL);
		return 1;
	}

	char* query = (char*)httpserver_get_content(s);

	if (!query)
	{
		httpserver_response(s, 400, "NO DATA", 0, NULL);
		return 1;
	}

	//////////////////////////////////////
	//
	// Linda/Store is not yet thread-safe
	//
	session_lock(s);

	hlinda h = linda_begin(l, 0);
	const char* buf = NULL;

	if (!linda_rdp(h, query, &buf))
	{
		linda_end(h);
		session_unlock(s);
		free(query);
		httpserver_response(s, 404, "NOT FOUND", 0, NULL);
		return 1;
	}

	size_t len = linda_get_length(h);
	linda_release(h);					// release the buf
	linda_end(h);
	session_unlock(s);
	//
	//////////////////////////////////////

	free(query);
	httpserver_response(s, 200, "OK", len, APPLICATION_JSON);
	if (g_http_debug) printf("SEND: %s", buf);
	session_write(s, buf, len);
	free((void*)buf);					// now free the buf
	return 1;
}

static int http_request(session s, void* param)
{
	const char* filename = session_get_stash(s, HTTP_FILENAME);
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

int main(int ac, char** av)
{
	printf("Usage: lindad [port|%u [ssl|0 [quiet|0 [threads|0 [path1 [path2 [uncle|%u]]]]]]]\n", HTTP_DEFAULT_PORT, UNCLE_DEFAULT_PORT);
	const char* binding = NULL;
	unsigned short port = (short)(ac>1?atoi(av[1]):HTTP_DEFAULT_PORT);
	int ssl = (ac>2?atoi(av[2]):0);
	g_http_debug = !(ac>3?atoi(av[3])>0?1:0:0);
	int threads = (ac>4?atoi(av[4]):0);
	const char* path1 = (ac>5?av[5]:"./db");
	const char* path2 = (ac>6?av[6]:NULL);
	unsigned short uncle_port = (short)(ac>7?atoi(av[7]):UNCLE_DEFAULT_PORT);
	void* param = (void*)0;

	handler h = handler_create(threads);
	if (!h) return 1;

	if (ssl)
		handler_set_tls(h, "server.pem");

	if (!handler_add_uncle(h, NULL, (short)uncle_port, SCOPE_DEFAULT))
		return 2;

	linda l = linda_open(path1, path2);
	if (!l) return 3;

	struct _httpserver_reqs reqs[] =
	{
		{&linda_request, "linda", l},	// http://server/linda
		{&http_request, NULL, NULL},	// ... others ...
		0
	};

	httpserver http = httpserver_create2(reqs);
	if (!http) return 4;

	if (!handler_add_server(h, &httpserver_handler, http, binding, port, 1, ssl, LINDA_SERVICE))
		return 5;

	handler_wait(h);
	handler_destroy(h);
	httpserver_destroy(http);
	linda_close(l);
	return 0;
}
