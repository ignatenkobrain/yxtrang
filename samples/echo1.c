#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <network.h>

// Synchronous echo client...

int main(int ac, char** av)
{
	printf("Usage: echo1 [host|localhost [port|12345 [tcp|1 [ssl|0 [loops|10 [quiet|0]]]]]]\n");
	const char* host = ac>1?av[1]:"localhost";
	unsigned short port = (short)(ac>2?atoi(av[2]):12345);
	int tcp = (ac>3?atoi(av[3]):1);
	int ssl = (ac>4?atoi(av[4]):0);
	int loops = (ac>5?atoi(av[5]):10);
	int quiet = (ac>6?atoi(av[6]):0);

	static const char* hello = "thequickbrownfoxjumpedoverthelazydog\n";
	int hello_len = strlen(hello);
	session s = session_open(host, port, tcp, ssl);
	if (!s) return 1;
	int i = 0;

	while (i++ < loops)
	{
		if (session_write(s, hello, hello_len) <= 0)
		{
			printf("write: %s\n", strerror(errno));
			break;
		}

		char* msg;

		if (!session_readmsg(s, &msg))
			break;

		if (!quiet) printf("CLIENT: %s", msg);
	}

	printf("DONE\n");
	return 0;
}
