#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lindatp.h"
#include "httpserver.h"
#include "linda.h"

extern int g_http_debug;

struct _lindatp
{
	linda l;
};

int lindatp_request(session s, void* param)
{
	const char* filename = session_get_stash(s, HTTP_FILENAME);
	//lindatp l = (lindatp)param;

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

	return 1;
}

lindatp lindatp_create(const char* path1, const char* path2)
{
	if (!path1)
		return NULL;

	lindatp l = (lindatp)calloc(1, sizeof(struct _lindatp));
	l->l = linda_open(path1, path2);
	return l;
}

void lindatp_destroy(lindatp l)
{
	if (!l)
		return;

	linda_close(l->l);
	free(l);
}
