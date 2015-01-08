#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <httpserver.h>
#include <linda.h>
#include <uncle.h>

#define APPLICATION_JSON "application/json"

extern int g_http_debug;

static int linda_request(session s, void *param)
{
	linda l = (linda)param;
	const char *ct = session_get_stash(s, "content-type");

	if (!strstr(ct, APPLICATION_JSON))
	{
		httpserver_response(s, 415, "UNSUPPORTED MEDIA TYPE", 0, NULL);
		return 1;
	}

	char *query = (char*)httpserver_get_content(s);

	if (!query)
	{
		httpserver_response(s, 400, "BAD REQUEST", 0, NULL);
		return 1;
	}

	const int dbsync = 0;
	hlinda h = linda_begin(l);

	if (session_get_udata_flag(s, HTTP_GET))
	{
		const char *buf = NULL;

		if (!linda_rdp(h, query, &buf))
		{
			linda_end(h, dbsync);
			httpserver_response(s, 404, "NOT FOUND", 0, NULL);
			free(query);
			return 1;
		}

		size_t len = linda_get_length(h);
		linda_release(h);					// release the buf
		linda_end(h, dbsync);
		httpserver_response(s, 200, "OK", len, APPLICATION_JSON);
		if (g_http_debug) printf("LINDA: %s", buf);
		free((void*)buf);					// now free the buf
		session_write(s, buf, len);
	}
	else if (session_get_udata_flag(s, HTTP_POST))
	{
		const char *buf = NULL;

		if (!linda_inp(h, query, &buf))
		{
			linda_end(h, dbsync);
			httpserver_response(s, 404, "NOT FOUND", 0, NULL);
			free(query);
			return 1;
		}

		size_t len = linda_get_length(h);
		linda_release(h);					// release the buf
		linda_end(h, dbsync);
		httpserver_response(s, 200, "OK", len, APPLICATION_JSON);
		if (g_http_debug) printf("LINDA: %s", buf);
		free((void*)buf);					// now free the buf
		session_write(s, buf, len);
	}
	else if (session_get_udata_flag(s, HTTP_DELETE))
	{
		if (!linda_rm(h, query))
		{
			linda_end(h, dbsync);
			httpserver_response(s, 404, "NOT FOUND", 0, NULL);
			free(query);
			return 1;
		}

		linda_end(h, dbsync);
		httpserver_response(s, 200, "OK", 0, APPLICATION_JSON);
	}
	else if (session_get_udata_flag(s, HTTP_PUT))
	{
		if (!linda_out(h, query))
		{
			linda_end(h, dbsync);
			httpserver_response(s, 404, "NOT FOUND", 0, NULL);
			free(query);
			return 1;
		}

		linda_end(h, dbsync);
		httpserver_response(s, 200, "OK", 0, APPLICATION_JSON);
	}
	else
	{
		linda_end(h, dbsync);
		httpserver_response(s, 501, "NOT IMPLEMENTED", 0, NULL);
		free(query);
		return 1;
	}

	free(query);
	return 1;
}

static int http_request(session s, void *param)
{
	const char *filename = session_get_stash(s, HTTP_RESOURCE);
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

int main(int ac, char *av[])
{
	const char *binding = NULL;
	unsigned short port = HTTP_DEFAULT_PORT, uncle_port = UNCLE_DEFAULT_PORT;
	int ssl = 0, threads = 0;
	const char *path1 = "./db";
	const char *path2 = NULL;
	int i;

	for (i = 1; i < ac; i++)
	{
		char tmpbuf[256];
		tmpbuf[0] = 0;
		unsigned tmp = 0;

		if (!strncmp(av[i], "--port=", 7))
		{
			sscanf(av[i], "%*[^=]=%u", &tmp);
			port = (short)tmp;
		}
		else if (!strncmp(av[i], "--uncle=", 8))
		{
			sscanf(av[i], "%*[^=]=%u", &tmp);
			uncle_port = (short)tmp;
		}
		else if (!strncmp(av[i], "--threads=", 10))
		{
			sscanf(av[i], "%*[^=]=%u", &tmp);
			threads = (short)tmp;
		}
		else if (!strncmp(av[i], "--ssl=", 6))
		{
			sscanf(av[i], "%*[^=]=%u", &tmp);
			ssl = (short)tmp;
		}
		else if (!strcmp(av[i], "--ssl"))
		{
			ssl = 1;
		}
		else if (!strncmp(av[i], "--tls=", 6))
		{
			sscanf(av[i], "%*[^=]=%u", &tmp);
			ssl = (short)tmp;
		}
		else if (!strcmp(av[i], "--tls"))
		{
			ssl = 1;
		}
		else if (!strncmp(av[i], "--debug=", 8))
		{
			sscanf(av[i], "%*[^=]=%u", &tmp);
			g_http_debug = (short)tmp;
		}
		else if (!strcmp(av[i], "--debug"))
		{
			g_http_debug = 1;
		}
		else if (!strncmp(av[i], "--path=", 7))
		{
			sscanf(av[i], "%*[^=]=%s", tmpbuf);
			tmpbuf[sizeof(tmpbuf)-1] = 0;
			path1 = strdup(tmpbuf);
		}
		else if (!strncmp(av[i], "--path1=", 8))
		{
			sscanf(av[i], "%*[^=]=%s", tmpbuf);
			tmpbuf[sizeof(tmpbuf)-1] = 0;
			path1 = strdup(tmpbuf);
		}
		else if (!strncmp(av[i], "--path2=", 8))
		{
			sscanf(av[i], "%*[^=]=%s", tmpbuf);
			tmpbuf[sizeof(tmpbuf)-1] = 0;
			path2 = strdup(tmpbuf);
		}
	}

	printf("Usage: lindad --port=%u --ssl=%d --debug=%d --threads=%d --path=%s --uncle=%u\n", port, ssl, g_http_debug, threads, path1, uncle_port);

	handler h = handler_create(threads);
	if (!h) return 1;

	if (ssl)
		handler_set_tls(h, "server.pem");

	if (uncle_port != 0)
	{
		if (!handler_add_uncle(h, NULL, (short)uncle_port, SCOPE_DEFAULT))
			return 2;
	}

	linda l = linda_open(path1, path2);
	if (!l) return 3;

	struct httpserver_reqs_ reqs[] =
	{
		{&linda_request, "linda", l},	// http://server/linda
		{&http_request, NULL, NULL},	// ... others ...
		{0}
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
