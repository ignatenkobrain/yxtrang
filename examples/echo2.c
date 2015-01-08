#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <network.h>

// Asynchronous echo client...

static int g_quiet = 0;
static int g_loops = 10;

static const char *hello = "thequickbrownfoxjumpedoverthelazydog\n";

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

	if (!session_readmsg(s, &msg))         // read the echo
		return 0;

	if (!g_quiet) printf("DATA: %s", msg);

	int count = session_get_udata_int(s);

	if (count++ == g_loops)
	{
		session_shutdown(s);
		return 0;
	}

	session_set_udata_int(s, count);

	if (session_writemsg(s, hello) <= 0)
	{
		printf("write: %s\n", strerror(errno));
		return 0;
	}

	return 1;
}


int main(int ac, char *av[])
{
	printf("Usage: echo2 [host|localhost [port|12345 [tcp|1 [ssl|0 [loops|10 [quiet|0 [threads:0]]]]]]]\n");
	const char *host = ac>1?av[1]:"localhost";
	unsigned short port = (short)(ac>2?atoi(av[2]):12345);
	int tcp = (ac>3?atoi(av[3]):1);
	int ssl = (ac>4?atoi(av[4]):0);
	g_loops = (ac>5?atoi(av[5]):10);
	g_quiet = (ac>6?atoi(av[6]):0);
	int threads = (ac>7?atoi(av[7]):0);

	session s = session_open(host, port, tcp, ssl);
	if (!s) return 1;

	handler h = handler_create(threads);
	handler_add_client(h, &on_session, NULL, s);

	// Start the loop...

	if (session_writemsg(s, hello) <= 0)
	{
		printf("write: %s\n", strerror(errno));
		return 1;
	}

	handler_wait(h);
	printf("DONE\n");
	handler_destroy(h);
	return 0;
}
