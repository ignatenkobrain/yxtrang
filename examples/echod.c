#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <network.h>

static int g_quiet = 0;

static int on_session(session s, void *param)
{
	if (session_on_connect(s))
	{
		printf("CONNECTED\n");
		return 1;
	}

	if (session_on_disconnect(s))
	{
		printf("DISCONNECTED\n");
		return 0;
	}

	char *msg;

	if (!session_readmsg(s, &msg))         // read in a msg
		return 0;

	if (!g_quiet) printf("DATA: %s", msg);
	return session_writemsg(s, msg);       // echo it back
}

int main(int ac, char *av[])
{
	printf("Usage: echod [port|12345 [tcp|1 [ssl|0 [quiet|0 [threads|0]]]]]\n");
	const char *binding = NULL;
	unsigned short port = (short)(ac>1?atoi(av[1]):12345);
	int tcp = (ac>2?atoi(av[2]):1);
	int ssl = (ac>3?atoi(av[3]):0);
	g_quiet = (ac>4?atoi(av[4]):0);
	int threads = (ac>5?atoi(av[5]):0);
	void *param = (void*)0;

	handler h = handler_create(threads);

	if (ssl)
		handler_set_tls(h, "server.pem");

	if (!handler_add_server(h, &on_session, param, binding, port, tcp, ssl, NULL))
		return 1;

	handler_wait(h);
	handler_destroy(h);
	return 0;
}
