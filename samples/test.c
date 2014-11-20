#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <float.h>

#include <json.h>
#include <base64.h>
#include <tree.h>
#include <store.h>
#include <network.h>
#include <scriptlet.h>
#include <skiplist.h>

#define PORT 6199
static int debug = 0, port = PORT;

static int on_server_session(session s, void* v)
{
	if (session_on_connect(s))
	{
		printf("SERVER: connected '%s'\n", session_remote_host(s, 0));
		return 1;
	}

	if (session_on_disconnect(s))
	{
		printf("SERVER: disconnected\n");
		return 0;
	}

	char* buf = 0;

	// Check for complete message:

	if (!session_readmsg(s, &buf))
		return 0;

	if (debug) printf("SERVER: %s", buf);

	// Echo it back...

	return session_writemsg(s, buf);
}

static void do_server(int tcp, int ssl, int threads)
{
	handler h = handler_create(threads);

	if (ssl)
		handler_set_tls(h, "server.pem");

	if (tcp)
	{
		if (!handler_add_server(h, &on_server_session, NULL, NULL, (short)port, 1, ssl))
		{
			printf("server failed\n");
			return;
		}
	}
	else
	{
		if (!handler_add_server(h, &on_server_session, NULL, NULL, (short)port, 0, 0))
		{
			printf("server failed\n");
			return;
		}
	}

	handler_wait(h);
	handler_close(h);
}

static void do_client(const char* host, long cnt, int tcp, int ssl, int broadcast)
{
	session s = session_open(host, (short)port, tcp, ssl);
	if (!s) { printf("CLIENT: session failed\n"); return; }

	printf("CLIENT: connected '%s'\n", session_remote_host(s, 0));
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

		char* buf = 0;

		if (!session_readmsg(s, &buf))
		{
			printf("Read error\n");
			break;
		}

		if (debug) printf("CLIENT: %s", buf);
		tot++;

		if (!session_readmsg(s, &buf))
		{
			printf("Read error\n");
			break;
		}

		if (debug) printf("CLIENT: %s", buf);
		tot++;

		if (!session_readmsg(s, &buf))
		{
			printf("Read error\n");
			break;
		}

		if (debug) printf("CLIENT: %s", buf);
		tot++;
	}

	printf("Sent/received %ld msgs\n", tot);
	session_close(s);
}

static void do_script(long cnt)
{
	const char* text = "\n\tX = 3.14159265358979323846;\n\tY = X ** 2;\n\tprint Y;";

	printf("Scriptlet compile: '%s'\n", text);
	scriptlet s = scriptlet_open(text);

	printf("Scriptlet run\n");
	runtime r = scriptlet_prepare(s);
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

static void do_store(long cnt, int vfy, int compact)
{
	store st = store_open("./db", 0, compact);
	time_t t = time(NULL);

#if 0
	printf("Writing...\n");
	handle h = store_begin(st, 0);
	long i;

	for (i = 1; i <= cnt; i++)
	{
		char tmpbuf[1024];
		int len = sprintf(tmpbuf, "{'name':'test','i':%ld}\n", i);
		uuid u = {i, t};

		if (!store_hadd(h, &u, tmpbuf, len))
			printf("ADD failed: %ld\n", i);

		if (!(i%10))
		{
			store_end(h);
			h = store_begin(st, 0);
		}
	}

	store_end(h);
#else
	printf("Writing...\n");
	long i;

	for (i = 1; i <= cnt; i++)
	{
		char tmpbuf[1024];
		int len = sprintf(tmpbuf, "{'name':'test','i':%ld}\n", i);
		uuid u = {i, t};

		if (!store_add(st, &u, tmpbuf, len))
			printf("ADD failed: %ld\n", i);
	}
#endif

	printf("Store: %ld items\n", (long)store_count(st));

	if (vfy)
	{
		printf("Verifying...\n");

		for (i = 1; i <= cnt; i++)
		{
			char tmpbuf[1024];
			void* buf = &tmpbuf;
			int len = sizeof(tmpbuf);
			long k = (rand()%cnt)+1;
			uuid u = {k, t};

			if (!store_get(st, &u, &buf, &len))
				printf("GET failed: %ld\n", k);
		}
	}

	if (vfy)
	{
		printf("Deleting...\n");

		for (i = 1; i <= cnt; i++)
		{
			char tmpbuf[1024];
			void* buf = &tmpbuf;
			int len = sizeof(tmpbuf);
			long k = (rand()%cnt)+1;
			uuid u = {k, t};
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
			void* buf = &tmpbuf;
			int len = sizeof(tmpbuf);
			long k = (rand()%cnt)+1;
			uuid u = {k, t};
			store_get(st, &u, &buf, &len);
		}
	}

	store_close(st);
}

#define SKIP_RANDOM 0

static void do_skip(long cnt)
{
	extern void sl_dump(const skiplist sptr);
	skiplist sl = sl_int_open();
	long i;

	printf("Writing...\n");

#if !SKIP_RANDOM && 1
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

	sl_dump(sl);

#if 1
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
#endif

#if 1
	sl_dump(sl);

	// Try some deletes...

	printf("Deleting...\n");

	while (sl_count(sl) > 0)
	{
		for (i = 1; i <= cnt; i++)
		{
			int k = (rand()%cnt)+1;

			if (!sl_int_rem(sl, k))
				;//printf("Del failed: %llu\n", (unsigned long long)k);
			else
				;//printf("Del k=%llu\n", (unsigned long long)k);
		}

		//sl_dump(sl);
		printf("Count: %llu\n", (unsigned long long)sl_count(sl));
	}

#endif

	sl_close(sl);
}

#define TREE_RANDOM 0

void do_tree(long cnt, int rnd)
{
	tree tptr = tree_open();
	long i;

	if (!rnd)
	{
		for (i = 1; i <= cnt; i++)
		{
			uuid u = uuid_set(i, 1);
			tree_add(tptr, &u, u.u1);
		}
	}
	else
	{
		for (i = 1; i <= cnt; i++)
		{
			uint64_t k = rand()%10;
			uuid u = uuid_set(((i+k)%cnt)+1, 1);
			tree_add(tptr, &u, u.u1);
		}
	}

	// Spot check...

	for (i = 1; i <= cnt; i++)
	{
		uuid u = uuid_set((rand()%cnt)+1,1);
		uint64_t v = 0;

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

	long trunks, branches, leafs;
	tree_stats(tptr, &trunks, &branches, &leafs);
	printf("Stats: Trunks: %ld, Branches: %ld, Leafs: %ld\n", trunks, branches, leafs);

	// Random deletes...

	while (tree_count(tptr) > 0)
	{
		for (i = 1; i <= cnt; i++)
		{
			uuid u = uuid_set((rand()%cnt)+1, 1);

			if (!tree_del(tptr, &u))
			{
				//if (!rnd)
				//	printf("Del failed: %llu\n", (unsigned long long)u.u1);
			}
			else
				;//printf("Del k=%llu\n", (unsigned long long)u.u1);
		}

		tree_stats(tptr, &trunks, &branches, &leafs);
		printf("Stats: Trunks: %ld, Branches: %ld, Leafs: %ld\n", trunks, branches, leafs);
	}

	tree_close(tptr);
}

static void do_base64()
{
	const char* s = "the quick brown fox jumped over the lazy dog";
	printf("%s =>\n", s);

	char dst[1024];
	char* pdst = dst;
	format_base64(s, strlen(s), &	pdst, 0, 0);
	printf("%s =>\n", dst);

	char dst2[1024];
	pdst = dst2;
	parse_base64(dst, strlen(dst), &pdst);
	printf("%s\n", dst2);
}

static void do_json()
{
	// Do it the easy way...

	const char* s = "{'a':1,'b':2.2,"
		"'c':'the quick brown fox jumped over the lazy dog',"
		"'d':true,'e':false,'f':null,"
		"'g':[11,22],"
		"'h':{'h1':33,'h2':44}"
		"}";

	json ptr = json_open(s);
	char* pdst;
	printf("%s\n", pdst=json_to_string(ptr));
	free(pdst);
	json_close(ptr);

	// Do it the hard way...

	ptr = json_init();
	json_set_object(ptr);

	json_set_integer(json_object_add(ptr, "a"), 1);
	json_set_real(json_object_add(ptr, "b"), 2.2);
	json_set_string(json_object_add(ptr, "c"), "the quick brown fox jumped over the lazy dog");
	json_set_true(json_object_add(ptr, "d"));
	json_set_false(json_object_add(ptr, "e"));
	json_set_null(json_object_add(ptr, "f"));

	json jptr;
	jptr = json_object_add(ptr, "g");
	json_set_array(jptr);
	json_set_integer(json_array_add(jptr), 11);
	json_set_integer(json_array_add(jptr), 22);

	jptr = json_object_add(ptr, "h");
	json_set_object(jptr);
	json_set_integer(json_object_add(jptr, "h1"), 33);
	json_set_integer(json_object_add(jptr, "h2"), 44);

	printf("%s\n", pdst=json_to_string(ptr));
	free(pdst);
	json_close(ptr);
}

int main(int ac, char* av[])
{
	int loops = 10*1000, loops2 = 10, test_tree = 0, test_store = 0;
	int compact = 0, vfy = 0, srvr = 0, client = 0, tcp = 1, ssl = 0;
	int quiet = 0, test_json = 0, test_base64 = 0, rnd = 0, test_skiplist = 0;
	int broadcast = 0, threads = 0, test_script = 0;
	char host[1024];
	srand(time(0)|1);
	int i;

	sprintf(host, "localhost");

	for (i = 1; i < ac; i++)
	{
		if (!strncmp(av[i], "--debug=", 8))
			sscanf(av[i], "%*[^=]=%d", &debug);

		if (!strncmp(av[i], "--loops=", 8))
			sscanf(av[i], "%*[^=]=%d", &loops);

		if (!strncmp(av[i], "--loops2=", 9))
			sscanf(av[i], "%*[^=]=%d", &loops2);

		if (!strncmp(av[i], "--host=", 7))
			sscanf(av[i], "%*[^=]=%s", host);

		if (!strncmp(av[i], "--port=", 7))
			sscanf(av[i], "%*[^=]=%d", &port);

		if (!strncmp(av[i], "--threads=", 10))
			sscanf(av[i], "%*[^=]=%d", &threads);

		if (!strcmp(av[i], "--skip"))
			test_skiplist = 1;

		if (!strcmp(av[i], "--tree"))
			test_tree = 1;

		if (!strcmp(av[i], "--json"))
			test_json = 1;

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

		if (!strcmp(av[i], "--quiet"))
			quiet = 1;
	}

	if (test_json)
	{
		do_json();
		return 0;
	}

	if (test_base64)
	{
		do_base64();
		return 0;
	}

	if (test_store)
	{
		do_store(loops, vfy, compact);
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
		do_server(tcp, ssl, threads);
		return 0;
	}

	if (client)
	{
		do_client(host, loops, tcp, ssl, broadcast);
		return 0;
	}

	return 0;
}
