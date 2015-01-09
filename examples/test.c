#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <float.h>

#include <json.h>
#include <jsonq.h>
#include <base64.h>
#include <tree.h>
#include <store.h>
#include <linda.h>
#include <network.h>
#include <scriptlet.h>
#include <skiplist_int.h>
#include <uncle.h>

#define SERVER_PORT 6198

#ifdef _WIN32
#define sleep _sleep
#else
#include <unistd.h>
#endif

static int g_debug = 0, g_quiet = 1;
static unsigned short g_uncle = UNCLE_DEFAULT_PORT;
static const char *g_service = "TEST";
static const char *qbf = "the quick brown fox jumped over the lazy dog";

static int on_server_session(session s, void *v)
{
	if (session_on_connect(s))
	{
		printf("SERVER: connected '%s'\n", session_get_remote_host(s, 0));
		return 1;
	}

	if (session_on_disconnect(s))
	{
		printf("SERVER: disconnected\n");
		return 0;
	}

	char *buf = 0;

	if (!session_readmsg(s, &buf))
		return 0;

	if (g_debug) printf("SERVER: %s", buf);
	return session_writemsg(s, buf);
}

static void do_server(unsigned short port, int tcp, int ssl, int threads)
{
	handler h = handler_create(threads);

	if (ssl)
		handler_set_tls(h, "server.pem");

	if (!handler_add_uncle(h, NULL, (short)g_uncle, SCOPE_DEFAULT))
	{
		printf("add uncle failed\n");
		return;
	}

	if (!handler_add_server(h, &on_server_session, NULL, NULL, port, tcp, ssl, g_service))
	{
		printf("add server failed\n");
		return;
	}

	handler_wait(h);
	handler_destroy(h);
}

static void do_client(long cnt, const char *host, unsigned short port, int tcp, int ssl, int broadcast)
{
	session s = session_open(host, port, tcp, ssl);
	if (!s) { printf("CLIENT: session failed\n"); return; }

	printf("CLIENT: connected '%s'\n", session_get_remote_host(s, 0));
	if (broadcast) session_enable_broadcast(s);

	long tot = 0;
	int i;

	for (i = 0; i < cnt; i += 3)
	{
		// Send 3 overlapped messages to test things out...

		if (session_writemsg(s, "Hello, world1!\nHello, world2") <= 0)
		{
			printf("Write error\n");
			break;
		}

		if (session_writemsg(s, "!\nHello, world3!\n") <= 0)
		{
			printf("Write error\n");
			break;
		}

		// Get 'em back...

		char *buf = 0;

		if (!session_readmsg(s, &buf))
		{
			printf("Read error\n");
			break;
		}

		if (g_debug) printf("CLIENT: %s", buf);
		tot++;

		if (!session_readmsg(s, &buf))
		{
			printf("Read error\n");
			break;
		}

		if (g_debug) printf("CLIENT: %s", buf);
		tot++;

		if (!session_readmsg(s, &buf))
		{
			printf("Read error\n");
			break;
		}

		if (g_debug) printf("CLIENT: %s", buf);
		tot++;
	}

	printf("Sent/received %ld msgs\n", tot);
	session_close(s);
}

static void do_script(long cnt)
{
	const char *text = "\n\tX = 3.14159265358979323846;\n\tY = X  **2;\n\tprint Y;";

	printf("Scriptlet compile: '%s'\n", text);
	scriptlet s = scriptlet_open(text);

	printf("Scriptlet run\n");
	hscriptlet r = scriptlet_prepare(s);
	scriptlet_run(r);
	double v = 0.0;

	if (scriptlet_get_real(r, "X", &v))
		printf("Scriptlet result X=%.*g\n", DBL_DIG, (double)v);
	else
		printf("ERROR\n");

	v = 0.0;

	if (scriptlet_get_real(r, "Y", &v))
		printf("Scriptlet result Y=%.*g\n", DBL_DIG, (double)v);
	else
		printf("ERROR\n");

	scriptlet_done(r);
	scriptlet_close(s);
}

static void do_linda_out(long cnt)
{
	linda *l = linda_open("./db", NULL);
	const int dbsync = 0;
	hlinda *h = linda_begin(l);
	long i;

	for (i = 0; i < cnt; i++)
	{
		int id = rand() % 1000;
		char tmpbuf[1024];
		sprintf(tmpbuf, "{\"%s\":%d,\"nbr\":%ld,\"text\":\"%s\"}\n", LINDA_ID, id, i, qbf);
		linda_out(h, tmpbuf);

		if (!(i%10))
		{
			linda_end(h, dbsync);
			h = linda_begin(l);
		}
	}

	// Return by id (indexed)...

	for (i = 0; i < 100; i++)
	{
		const char *buf = NULL;
		char tmpbuf[1024];
		sprintf(tmpbuf, "{\"%s\":%ld}\n", LINDA_ID, i);

		if (!linda_rdp(h, tmpbuf, &buf))
			continue;

		printf("GOT: %s=%ld => %s", LINDA_ID, i, buf);
	}

	linda_end(h, dbsync);
	linda_close(l);
}

static void do_linda_in()
{
	linda *l = linda_open("./db", NULL);
	hlinda *h = linda_begin(l);
	long i;

	// Return by id (indexed)...

	for (i = 0; i < 10; i++)
	{
		const char *buf = NULL;
		char tmpbuf[1024];
		sprintf(tmpbuf, "{\"%s\":%ld}\n", LINDA_ID, i);

		if (!linda_rdp(h, tmpbuf, &buf))
			continue;

		printf("GOT1: %s=%ld => %s", LINDA_ID, i, buf);
	}

	// Return first 100 by param (not indexed)...

	for (i = 0; i < 10; i++)
	{
		const char *buf;
		char tmpbuf[1024];
		sprintf(tmpbuf, "{\"nbr\":%d}\n", rand()%1000);

		if (!linda_rdp(h, tmpbuf, &buf))
			continue;

		printf("GOT2: %s => %s", tmpbuf, buf);
	}

	linda_end(h, 0);
	linda_close(l);
}

static void do_store(long cnt, int vfy, int compact, int tran)
{
	store st = store_open("./db", 0, compact);
	time_t t = time(NULL);
	printf("Store: %ld items\n", (long)store_count(st));

	printf("Writing...\n");
	hstore h = tran ? store_begin(st) : NULL;
	long i;

	for (i = 1; i <= cnt; i++)
	{
		char tmpbuf[1024];
		int len = sprintf(tmpbuf, "{'name':'test','i':%ld}\n", i);
		uuid_t u = {i, t};

		if (tran)
		{
			if (!store_hadd(h, &u, tmpbuf, len))
				printf("ADD failed: %ld\n", i);
		}
		else
		{
			if (!store_add(st, &u, tmpbuf, len))
				printf("ADD failed: %ld\n", i);
		}

		if (tran && !(i%10))
		{
			store_end(h, 0);
			h = store_begin(st);
		}
	}

	if (h) store_end(h, 0);
	printf("Store: %ld items\n", (long)store_count(st));

	if (vfy)
	{
		printf("Verifying...\n");

		for (i = 1; i <= cnt; i++)
		{
			char tmpbuf[1024];
			void *buf = &tmpbuf;
			size_t len = sizeof(tmpbuf);
			long k = (rand()%cnt)+1;
			uuid_t u = {k, t};

			if (!store_get(st, &u, &buf, &len))
				printf("HTTP_GET failed: %ld\n", k);
		}
	}

	if (vfy)
	{
		printf("Deleting...\n");

		for (i = 1; i <= cnt; i++)
		{
			long k = (rand()%cnt)+1;
			uuid_t u = {k, t};
			store_rem(st, &u);
		}

		printf("Store: %ld items\n", (long)store_count(st));
	}

	if (vfy)
	{
		printf("Reading...\n");

		for (i = 1; i <= cnt; i++)
		{
			char tmpbuf[1024];
			void *buf = &tmpbuf;
			size_t len = sizeof(tmpbuf);
			long k = (rand()%cnt)+1;
			uuid_t u = {k, t};
			store_get(st, &u, &buf, &len);
		}
	}

	store_close(st);
}

#define SKIP_RANDOM 0

static void do_skip(long cnt)
{
	extern void sl_dump(const skiplist sptr);
	skiplist sl = sl_int_create();
	long i;

	printf("Writing...\n");

#if !SKIP_RANDOM && 1
	for (i = 1; i <= cnt; i++)
		sl_int_add(sl, i, i);

	printf("Duplicates...\n");

	for (i = 1; i <= cnt; i++)
	{
		int k = (rand()%cnt)+1;
		sl_int_add(sl, k, k);
	}
#elif !SKIP_RANDOM && 1
	for (i = 1; i <= cnt; i++)
		sl_int_add(sl, i, i);
#elif !SKIP_RANDOM && 0
	for (i = cnt; i > 0; i--)
		sl_int_add(sl, i, i);
#else
	for (i = 1; i <= cnt; i++)
	{
		int k = (rand()%cnt)+1;
		sl_int_add(sl, k, k);
	}
#endif

	if (!g_quiet)
		sl_dump(sl);

	// Spot check...

	printf("Reading...\n");

	for (i = 1; i <= cnt; i++)
	{
		int k = (rand()%cnt)+1;
		int v = -1;

		if (!sl_int_get(sl, k, &v))
#if !SKIP_RANDOM
			printf("Get failed: %llu\n", (unsigned long long)(size_t)k);
#else
			;
#endif
		else if (v != k)
			printf("Get bad match: k=%llu v=%llu\n", (unsigned long long)(size_t)k, (unsigned long long)(size_t)v);
		else
			;//printf("Get k=%llu v=%llu\n", (unsigned long long)(size_t)k, (unsigned long long)(size_t)v);
	}

	if (!g_quiet)
		sl_dump(sl);

	// Try some deletes...

	printf("Deleting...\n");

	while (sl_count(sl) > 0)
	{
		for (i = 1; i <= cnt; i++)
		{
			int k = (rand()%cnt)+1;

#if 1
			if (!sl_int_rem(sl, k))
#elif 0
			if (!sl_int_erase(sl, k, (size_t)k, NULL))
#else
			if (!sl_int_efface(sl, (size_t)k, NULL))
#endif
				;//printf("Del failed: %llu\n", (unsigned long long)k);
			else
				;//printf("Del k=%llu\n", (unsigned long long)k);
		}

		//sl_dump(sl);
		printf("Count: %llu\n", (unsigned long long)sl_count(sl));
	}

	sl_destroy(sl);
}

#define TREE_RANDOM 0

void do_tree(long cnt, int rnd)
{
	tree tptr = tree_create();
	long i;

	if (!rnd)
	{
		for (i = 1; i <= cnt; i++)
		{
			uuid_t u = uuid_set(i, 1);
			tree_add(tptr, &u, u.u1);
		}
	}
	else
	{
		for (i = 1; i <= cnt; i++)
		{
			uint64_t k = rand()%10;
			uuid_t u = uuid_set(((i+k)%cnt)+1, 1);
			tree_add(tptr, &u, u.u1);
		}
	}

	// Spot check...

	for (i = 1; i <= cnt; i++)
	{
		uuid_t u = uuid_set((rand()%cnt)+1,1);
		unsigned long long v = 0;

		if (!tree_get(tptr, &u, &v))
		{
			if (!rnd)
				printf("Get failed: %llu\n", (unsigned long long)u.u1);
		}
		else if (u.u1 != v)
			printf("Get bad match: k=%llu v=%llu\n", (unsigned long long)u.u1, (unsigned long long)v);
		else
			;//printf("Get k=%llu v=%llu\n", (unsigned long long)u.u1, (unsigned long long)v);
	}

	size_t trunks, branches, leafs;
	tree_stats(tptr, &trunks, &branches, &leafs);
	printf("Stats: Trunks: %lld, Branches: %lld, Leafs: %lld\n", (long long)trunks, (long long)branches, (long long)leafs);

	// Random deletes...

	while (tree_count(tptr) > 0)
	{
		for (i = 1; i <= cnt; i++)
		{
			uuid_t u = uuid_set((rand()%cnt)+1, 1);

			if (!tree_del(tptr, &u))
			{
				//if (!rnd)
				//	printf("Del failed: %llu\n", (unsigned long long)u.u1);
			}
			else
				;//printf("Del k=%llu\n", (unsigned long long)u.u1);
		}

		tree_stats(tptr, &trunks, &branches, &leafs);
		printf("Stats: Trunks: %lld, Branches: %lld, Leafs: %lld\n", (long long)trunks, (long long)branches, (long long)leafs);
	}

	tree_destroy(tptr);
}

static void do_base64()
{
	const char *s = qbf;
	printf("%s =>\n", s);

	char dst[1024];
	char *pdst = dst;
	format_base64(s, strlen(s), &	pdst, 0, 0);
	printf("%s =>\n", dst);

	char dst2[1024];
	pdst = dst2;
	parse_base64(dst, strlen(dst), &pdst);
	printf("%s\n", dst2);
}

static void do_json()
{
	const char *s = "{'a':1,'b':2.2,"
		"'c':'the quick brown fox jumped over the lazy dog',"
		"'d':true,'e':false,'f':null,"
		"'g':[11,22],"
		"'h':{'h1':33,'h2':44}"
		"}";

	printf("ORIG: %s\n", s);
	char *pdst;

	// Parse and reformat...

	json *j = json_open(s);
	printf("PARSED: %s\n", pdst=json_to_string(j)); free(pdst);
	json_close(j);

	// Build it manually...

	j = json_init();
	json_set_object(j);

	json_set_integer(json_object_add(j, "a"), 1);
	json_set_real(json_object_add(j, "b"), 2.2);
	json_set_string(json_object_add(j, "c"), "the quick brown fox jumped over the lazy dog");
	json_set_true(json_object_add(j, "d"));
	json_set_false(json_object_add(j, "e"));
	json_set_null(json_object_add(j, "f"));

	json *j0 = json_object_add(j, "g");
	json_set_array(j0);
	json_set_integer(json_array_add(j0), 11);
	json_set_integer(json_array_add(j0), 22);

	j0 = json_object_add(j, "h");
	json_set_object(j0);
	json_set_integer(json_object_add(j0, "h1"), 33);
	json_set_integer(json_object_add(j0, "h2"), 44);

	printf("BUILT: %s\n", pdst=json_to_string(j)); free(pdst);
	json_close(j);

	j = json_open(s);
	json *j1 = json_get_object(j);
	json *j2 = json_find(j1, "c");

	if (j2)
		printf("QUERY: 'c' = '%s'\n", json_get_string(j2));
	else
		printf("Cannot find 'c'\n");

	json_close(j);
}

static void do_jsonq(const char *name)
{
	const char *s = "{"
		"'a':1,"
		"'b':2.2,"
		"'c':'the quick brown \\\"fox\\\" jumped over the lazy dog',"
		"'d':true,"
		"'e':false,"
		"'f':null,"
		"'g':[11,22],"
		"'h':{'h1':33,'h2':44,h3':'this is a \\\"quote\\\" character'}"
		"}";

	printf("%s\n", s);

	char tmp[1024];
	printf("%s = %s\n", name, jsonq(s, name, tmp, sizeof(tmp)));
}

int main(int ac, char *av[])
{
	int loops = 10000, loops2 = 10, test_tree = 0, test_store = 0;
	int compact = 0, vfy = 0, srvr = 0, client = 0, tcp = 1, ssl = 0;
	int test_json = 0, test_base64 = 0, rnd = 0, test_skiplist = 0;
	int broadcast = 0, threads = 0, test_script = 0, test_jsonq = 0;
	int discovery = 0, test_linda_out = 0, test_linda_in = 0;
	int tran = 0;
	unsigned short port = SERVER_PORT;
	int i;

	srand(time(0)|1);
	char host[256];
	host[0] = 0;
	sprintf(host, "localhost");

	for (i = 1; i < ac; i++)
	{
		if (!strncmp(av[i], "--debug=", 8))
			sscanf(av[i], "%*[^=]=%d", &g_debug);

		if (!strncmp(av[i], "--loops=", 8))
			sscanf(av[i], "%*[^=]=%d", &loops);

		if (!strncmp(av[i], "--loops2=", 9))
			sscanf(av[i], "%*[^=]=%d", &loops2);

		if (!strncmp(av[i], "--host=", 7))
			sscanf(av[i], "%*[^=]=%s", host);

		if (!strncmp(av[i], "--name=", 7))
			sscanf(av[i], "%*[^=]=%s", host);

		if (!strncmp(av[i], "--uncle=", 8))
		{
			unsigned tmp_port;
			sscanf(av[i], "%*[^=]=%u", &tmp_port);
			g_uncle = (unsigned short)tmp_port;
		}

		if (!strncmp(av[i], "--port=", 7))
		{
			unsigned tmp_port;
			sscanf(av[i], "%*[^=]=%u", &tmp_port);
			port = (unsigned short)tmp_port;
		}

		if (!strncmp(av[i], "--threads=", 10))
			sscanf(av[i], "%*[^=]=%d", &threads);

		if (!strcmp(av[i], "--skip"))
			test_skiplist = 1;

		if (!strcmp(av[i], "--tree"))
			test_tree = 1;

		if (!strcmp(av[i], "--json"))
			test_json = 1;

		if (!strcmp(av[i], "--jsonq"))
			test_jsonq = 1;

		if (!strcmp(av[i], "--linda-out"))
			test_linda_out = 1;

		if (!strcmp(av[i], "--linda-in"))
			test_linda_in = 1;

		if (!strcmp(av[i], "--store"))
			test_store = 1;

		if (!strcmp(av[i], "--base64"))
			test_base64 = 1;

		if (!strcmp(av[i], "--script"))
			test_script = 1;

		if (!strcmp(av[i], "--rnd"))
			rnd = 1;

		if (!strcmp(av[i], "--vfy"))
			vfy = 1;

		if (!strcmp(av[i], "--bcast"))
			broadcast = 1;

		if (!strcmp(av[i], "--compact"))
			compact = 1;

		if (!strcmp(av[i], "--tran"))
			tran = 1;

		if (!strcmp(av[i], "--srvr"))
			srvr = 1;

		if (!strcmp(av[i], "--client"))
			client = 1;

		if (!strcmp(av[i], "--tcp"))
			tcp = 1;

		if (!strcmp(av[i], "--notcp"))
			tcp = 0;

		if (!strcmp(av[i], "--udp"))
			tcp = 0;

		if (!strcmp(av[i], "--ssl"))
			ssl = 1;

		if (!strcmp(av[i], "--disc") || !strcmp(av[i], "--discovery"))
			discovery = 1;

		if (!strcmp(av[i], "--quiet"))
			g_quiet = 1;

		if (!strcmp(av[i], "--noquiet"))
			g_quiet = 0;
	}

	if (client && discovery)
	{
		uncle u = uncle_create(NULL, g_uncle, SCOPE_DEFAULT);
		sleep(1);

		if (uncle_query(u, g_service, host, &port, &tcp, &ssl))
			printf("DISCOVERY: service=%s, host=%s, port=%u, tcp=%d, ssl=%d\n", g_service, host, port, tcp, ssl);

		uncle_destroy(u);
	}

	if (test_json)
	{
		do_json();
		return 0;
	}

	if (test_jsonq)
	{
		do_jsonq(host);
		return 0;
	}

	if (test_base64)
	{
		do_base64();
		return 0;
	}

	if (test_linda_out)
	{
		do_linda_out(loops);
		return 0;
	}

	if (test_linda_in)
	{
		do_linda_in();
		return 0;
	}

	if (test_store)
	{
		do_store(loops, vfy, compact, tran);
		return 0;
	}

	if (test_skiplist)
	{
		do_skip(loops);
		return 0;
	}

	if (test_tree)
	{
		do_tree(loops, rnd);
		return 0;
	}

	if (test_script)
	{
		do_script(loops);
		return 0;
	}

	if (srvr)
	{
		do_server(port, tcp, ssl, threads);
		return 0;
	}

	if (client)
	{
		do_client(loops, host, port, tcp, ssl, broadcast);
		return 0;
	}

	return 0;
}
