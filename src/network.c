// The handler will select the optimal mechanism for the platform:
//
//		BSD				- kqueue
//		Linux 			- epoll
//		Posix/Vista+	- poll
//		Other/XP		- select

#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__) && !defined(__bsdi__)
#define _POSIX_C_SOURCE 200112L
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
#define ioctl ioctlsocket
#define poll WSAPoll
#define sleep _sleep
#define msleep Sleep
#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH
#define MSG_NOSIGNAL 0
#ifdef errno
#undef errno
#endif
#define errno WSAGetLastError()
#ifdef EWOULDBLOCK
#undef EWOULDBLOCK
#endif
#define EWOULDBLOCK WSAEWOULDBLOCK
#undef EINTR
#define EINTR WSAEINTR
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#define msleep(ms) usleep(ms*1000)
#endif

#ifndef USE_SSL
#define USE_SSL 1
#endif

#if USE_SSL
#include "openssl/rsa.h"
#include "openssl/crypto.h"
#include "openssl/x509.h"
#include "openssl/pem.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#endif

#ifndef WANT_POLL
#define WANT_POLL 0
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__)
#include <sys/event.h>
#endif

#if defined(__linux__)
#include <sys/epoll.h>
#endif

#if !defined(_WIN32) && WANT_POLL
#define _GNU_SOURCE
#include <poll.h>
#endif

#include "network.h"
#include "skiplist.h"
#include "thread.h"
#include "uncle.h"

#define BUFLEN 1500
#define FD_POLLSIZE 10000
#define MAX_SERVERS 1024
#define SESSION_TIMEOUT_SECONDS 90
#define MAX_EVENTS 1000

#ifndef POLLRDHUP
#define POLLRDHUP 0
#endif

static const int g_debug = 0;

struct _server
{
	int (*f)(session, void*);
	void* v;
	int fd, port, tcp, ssl, ipv4;
};

typedef struct _server* server;

struct _handler
{
	skiplist fds, badfds;
	lock strand;
	thread_pool tp;
	uncle u[MAX_SERVERS];
	fd_set rfds;
	struct _server srvs[MAX_SERVERS];
#if defined(POLLIN) && WANT_POLL
	struct pollfd rpollfds[FD_POLLSIZE];
#endif
	void* ctx;
	int cnt, hi, fd, threads, uncs;
	volatile int halt, use;
};

struct _session
{
	handler h;
	skiplist stash;
	char* remote;
	char srcbuf[BUFLEN];
	const char* src;
	char* dstbuf;
	char* dst;
	lock strand;
	uint64_t udata_flags;
	unsigned long long udata_int;
	double udata_real;
	int connected, disconnected, len, busy;
	int fd, port, tcp, is_ssl, ipv4, idx, use_cnt;
	int (*f)(session, void*);
	void* ssl;
	void* ctx;
	void* v;

	union
	{
		struct sockaddr_in addr4;
		struct sockaddr_in6 addr6;
	};
};

#if USE_SSL
///////////////////////////////////////////////////////////////////////////////

/* ----BEGIN GENERATED SECTION-------- */

/*
** Diffie-Hellman-Parameters: (512 bit)
**     prime:
**         00:d4:bc:d5:24:06:f6:9b:35:99:4b:88:de:5d:b8:
**         96:82:c8:15:7f:62:d8:f3:36:33:ee:57:72:f1:1f:
**         05:ab:22:d6:b5:14:5b:9f:24:1e:5a:cc:31:ff:09:
**         0a:4b:c7:11:48:97:6f:76:79:50:94:e7:1e:79:03:
**         52:9f:5a:82:4b
**     generator: 2 (0x2)
** Diffie-Hellman-Parameters: (1024 bit)
**     prime:
**         00:e6:96:9d:3d:49:5b:e3:2c:7c:f1:80:c3:bd:d4:
**         79:8e:91:b7:81:82:51:bb:05:5e:2a:20:64:90:4a:
**         79:a7:70:fa:15:a2:59:cb:d5:23:a6:a6:ef:09:c4:
**         30:48:d5:a2:2f:97:1f:3c:20:12:9b:48:00:0e:6e:
**         dd:06:1c:bc:05:3e:37:1d:79:4e:53:27:df:61:1e:
**         bb:be:1b:ac:9b:5c:60:44:cf:02:3d:76:e0:5e:ea:
**         9b:ad:99:1b:13:a6:3c:97:4e:9e:f1:83:9e:b5:db:
**         12:51:36:f7:26:2e:56:a8:87:15:38:df:d8:23:c6:
**         50:50:85:e2:1f:0d:d5:c8:6b
**     generator: 2 (0x2)
*/

static unsigned char dh512_p[] =
{
    0xD4, 0xBC, 0xD5, 0x24, 0x06, 0xF6, 0x9B, 0x35, 0x99, 0x4B, 0x88, 0xDE,
    0x5D, 0xB8, 0x96, 0x82, 0xC8, 0x15, 0x7F, 0x62, 0xD8, 0xF3, 0x36, 0x33,
    0xEE, 0x57, 0x72, 0xF1, 0x1F, 0x05, 0xAB, 0x22, 0xD6, 0xB5, 0x14, 0x5B,
    0x9F, 0x24, 0x1E, 0x5A, 0xCC, 0x31, 0xFF, 0x09, 0x0A, 0x4B, 0xC7, 0x11,
    0x48, 0x97, 0x6F, 0x76, 0x79, 0x50, 0x94, 0xE7, 0x1E, 0x79, 0x03, 0x52,
    0x9F, 0x5A, 0x82, 0x4B,
};
static unsigned char dh512_g[] =
{
    0x02,
};

static DH *get_dh512()
{
    DH *dh;

    if ((dh = DH_new()) == NULL)
        return (NULL);
    dh->p = BN_bin2bn(dh512_p, sizeof(dh512_p), NULL);
    dh->g = BN_bin2bn(dh512_g, sizeof(dh512_g), NULL);
    if ((dh->p == NULL) || (dh->g == NULL))
        return (NULL);
    return (dh);
}
static unsigned char dh1024_p[] =
{
    0xE6, 0x96, 0x9D, 0x3D, 0x49, 0x5B, 0xE3, 0x2C, 0x7C, 0xF1, 0x80, 0xC3,
    0xBD, 0xD4, 0x79, 0x8E, 0x91, 0xB7, 0x81, 0x82, 0x51, 0xBB, 0x05, 0x5E,
    0x2A, 0x20, 0x64, 0x90, 0x4A, 0x79, 0xA7, 0x70, 0xFA, 0x15, 0xA2, 0x59,
    0xCB, 0xD5, 0x23, 0xA6, 0xA6, 0xEF, 0x09, 0xC4, 0x30, 0x48, 0xD5, 0xA2,
    0x2F, 0x97, 0x1F, 0x3C, 0x20, 0x12, 0x9B, 0x48, 0x00, 0x0E, 0x6E, 0xDD,
    0x06, 0x1C, 0xBC, 0x05, 0x3E, 0x37, 0x1D, 0x79, 0x4E, 0x53, 0x27, 0xDF,
    0x61, 0x1E, 0xBB, 0xBE, 0x1B, 0xAC, 0x9B, 0x5C, 0x60, 0x44, 0xCF, 0x02,
    0x3D, 0x76, 0xE0, 0x5E, 0xEA, 0x9B, 0xAD, 0x99, 0x1B, 0x13, 0xA6, 0x3C,
    0x97, 0x4E, 0x9E, 0xF1, 0x83, 0x9E, 0xB5, 0xDB, 0x12, 0x51, 0x36, 0xF7,
    0x26, 0x2E, 0x56, 0xA8, 0x87, 0x15, 0x38, 0xDF, 0xD8, 0x23, 0xC6, 0x50,
    0x50, 0x85, 0xE2, 0x1F, 0x0D, 0xD5, 0xC8, 0x6B,
};
static unsigned char dh1024_g[] =
{
    0x02,
};

static DH *get_dh1024()
{
    DH *dh;

    if ((dh = DH_new()) == NULL)
        return (NULL);
    dh->p = BN_bin2bn(dh1024_p, sizeof(dh1024_p), NULL);
    dh->g = BN_bin2bn(dh1024_g, sizeof(dh1024_g), NULL);
    if ((dh->p == NULL) || (dh->g == NULL))
        return (NULL);
    return (dh);
}
/* ----END GENERATED SECTION---------- */

static DH *ssl_dh_GetTmpParam(int nKeyLen)
{
    DH *dh;

    if (nKeyLen == 512)
        dh = get_dh512();
    else if (nKeyLen == 1024)
        dh = get_dh1024();
    else
        dh = get_dh1024();
    return dh;
}

/*
 * Hand out the already generated DH parameters...
 */

static DH *ssl_callback_TmpDH(SSL *pSSL, int nExport, int nKeyLen)
{
   return ssl_dh_GetTmpParam(nKeyLen);
}

/*  _________________________________________________________________
**
**  OpenSSL Callback Functions
**  _________________________________________________________________
*/

/*
 * Hand out temporary RSA private keys on demand
 *
 * The background of this as the TLSv1 standard explains it:
 *
 * | D.1. Temporary RSA keys
 * |
 * |    US Export restrictions limit RSA keys used for encryption to 512
 * |    bits, but do not place any limit on lengths of RSA keys used for
 * |    signing operations. Certificates often need to be larger than 512
 * |    bits, since 512-bit RSA keys are not secure enough for high-value
 * |    transactions or for applications requiring long-term security. Some
 * |    certificates are also designated signing-only, in which case they
 * |    cannot be used for key exchange.
 * |
 * |    When the public key in the certificate cannot be used for encryption,
 * |    the server signs a temporary RSA key, which is then exchanged. In
 * |    exportable applications, the temporary RSA key should be the maximum
 * |    allowable length (i.e., 512 bits). Because 512-bit RSA keys are
 * |    relatively insecure, they should be changed often. For typical
 * |    electronic commerce applications, it is suggested that keys be
 * |    changed daily or every 500 transactions, and more often if possible.
 * |    Note that while it is acceptable to use the same temporary key for
 * |    multiple transactions, it must be signed each time it is used.
 * |
 * |    RSA key generation is a time-consuming process. In many cases, a
 * |    low-priority process can be assigned the task of key generation.
 * |    Whenever a new key is completed, the existing temporary key can be
 * |    replaced with the new one.
 *
 * So we generated 512 and 1024 bit temporary keys on startup
 * which we now just hand out on demand....
 *
 */

static RSA* tmprsakey512 = 0;
static RSA* tmprsakey1024 = 0;

static RSA *ssl_callback_TmpRSA(SSL *pSSL, int nExport, int nKeyLen)
{
	if (nExport && (nKeyLen == 512))
		return tmprsakey512;
	else if (nExport && (nKeyLen == 1024))
		return tmprsakey1024;
	else
		return tmprsakey1024;
}

static int SSL_smart_shutdown(SSL *ssl)
{
	int i, rc = 0;

	for (i = 0; i < 4; i++)
	{
		if ((rc = SSL_shutdown(ssl)))
			break;
	}

	return rc;
}
#endif

const char* hostname(void)
{
	static char tmpbuf[256] = {0};

	if (tmpbuf[0])
		return tmpbuf;

	if (gethostname(tmpbuf, sizeof(tmpbuf)) == 0)
		return tmpbuf;
	else
		return "LOCALHOST.LOCALDOMAIN";
}

static int _parse_addr4(const char* host, struct sockaddr_in* addr4, int numeric)
{
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
	hints.ai_flags = (numeric?AI_NUMERICHOST:0) | (host?0:AI_PASSIVE);

	struct addrinfo* AddrInfo;
	int status;

	if ((status = getaddrinfo(host, 0, &hints, &AddrInfo)) != 0)
		return 0;

	struct addrinfo* AI;
	int i = 0;

	for (AI = AddrInfo; AI != 0; AI = AI->ai_next)
	{
		if (AI->ai_family != AF_INET)
			continue;

		memcpy(addr4, AI->ai_addr, AI->ai_addrlen);
		i++;
	}

	freeaddrinfo(AddrInfo);
	return i;
}

static int parse_addr4(const char* host, struct sockaddr_in* addr4)
{
	if (_parse_addr4(host, addr4, 0) || _parse_addr4(host, addr4, 1))
		return 1;
	else
		return 0;
}

static int _parse_addr6(const char* host, struct sockaddr_in6* addr6, int numeric)
{
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET6;
	hints.ai_flags = (numeric?AI_NUMERICHOST:0) | (host?0:AI_PASSIVE);

	struct addrinfo* AddrInfo;
	int status;

	if ((status = getaddrinfo(host, 0, &hints, &AddrInfo)) != 0)
		return 0;

	struct addrinfo* AI;
	int i = 0;

	for (AI = AddrInfo; AI != 0; AI = AI->ai_next)
	{
		if (AI->ai_family != AF_INET6)
			continue;

		memcpy(addr6, AI->ai_addr, AI->ai_addrlen);
		i++;
	}

	freeaddrinfo(AddrInfo);
	return i;
}

static int parse_addr6(const char* host, struct sockaddr_in6* addr6)
{
	if (_parse_addr6(host, addr6, 0) || _parse_addr6(host, addr6, 1))
		return 1;
	else
		return 0;
}

session session_open(const char* host, unsigned short port, int tcp, int ssl)
{
	if (!host || (port == 0))
		return 0;

#ifdef _WIN32
	static int cnt = 0;

	if (!cnt++)
	{
		WORD wVersionRequested = MAKEWORD(2,2);
		WSADATA wsaData;

		if (WSAStartup(wVersionRequested, &wsaData) != 0)
		{
		    WSACleanup();
		    return NULL;
		}
	}
#endif

	int fd = -1, try_ipv6 = 1, try_ipv4 = 1;
	struct sockaddr_in6 addr6 = {0};
	struct sockaddr_in addr4 = {0};

	while (try_ipv6 || try_ipv4)
	{
		if (try_ipv6)
		{
			addr6.sin6_family = AF_INET6;

			if (parse_addr6(host, &addr6))
			{
				addr6.sin6_port = htons(port);
				fd = socket(AF_INET6, tcp?SOCK_STREAM:SOCK_DGRAM, 0);

				if (fd < 0)
				{
					printf("socket6 failed: %s\n", strerror(errno));
					try_ipv6 = 0;
					continue;
				}

				int flag = 1;
				setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));

				if (tcp)
				{
					if (connect(fd, (struct sockaddr*)&addr6, sizeof(addr6)) != 0)
					{
						printf("connect6 failed: %s\n", strerror(errno));
						close(fd);
						fd = -1;
						try_ipv6 = 0;
						continue;
					}
				}

				break;
			}
			else
				try_ipv6 = 0;
		}

		if (try_ipv4)
		{
			addr4.sin_family = AF_INET;

			if (parse_addr4(host, &addr4))
			{
				addr4.sin_port = htons(port);
				fd = socket(AF_INET, tcp?SOCK_STREAM:SOCK_DGRAM, 0);

				if (fd < 0)
				{
					printf("socket4 failed: %s\n", strerror(errno));
					try_ipv4 = 0;
					continue;
				}

				int flag = 1;
				setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));

				if (tcp)
				{
					if (connect(fd, (struct sockaddr*)&addr4, sizeof(addr4)) != 0)
					{
						printf("connect4 failed: %s\n", strerror(errno));
						close(fd);
						fd = -1;
						try_ipv4 = 0;
						continue;
					}
				}

				break;
			}
			else
				try_ipv4 = 0;
		}
	}

	if (fd == -1)
		return NULL;

	session s = (session)calloc(1, sizeof(struct _session));
	if (!s) return NULL;
	session_share(s);
	s->strand = lock_create();
	s->connected = 1;
	s->fd = fd;
	s->ipv4 = !try_ipv6;
	s->port = port;
	s->tcp = tcp;
	s->src = s->srcbuf;

	if (s->ipv4)
		s->addr4 = addr4;
	else
		s->addr6 = addr6;

	if (tcp)
	{
		struct linger linger;
		linger.l_onoff = 0;
		linger.l_linger = 1;
		setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
		int flag = 1;
		setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(flag));
		flag = 1;
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
	}

#if USE_SSL
	static int init = 0;

	if (!init)
	{
		SSL_load_error_strings();
		SSLeay_add_all_algorithms();		// ???
		SSL_library_init();
		init = 1;
	}
#endif

	if (ssl)
	{
		if (!session_enable_tls(s, NULL, 0))
		{
			session_close(s);
			return NULL;
		}
	}

	return s;
}

int session_enable_tls(session s, const char* certfile, int level)
{
	if (!s || s->is_ssl)
		return 0;

#if USE_SSL
	if (!s->ctx)
	{
		s->ctx = (SSL_CTX*)SSL_CTX_new(s->h?TLSv1_2_server_method():TLSv1_2_client_method());

		if (!s->ctx)
		{
			printf("SSL new client context failed\n");
			ERR_print_errors_fp(stderr);
			return 0;
		}

		SSL_CTX_set_options((SSL_CTX*)s->ctx, SSL_OP_ALL);
		SSL_CTX_set_cipher_list((SSL_CTX*)s->ctx, SSL_DEFAULT_CIPHER_LIST);
		SSL_CTX_set_options((SSL_CTX*)s->ctx, SSL_OP_SINGLE_DH_USE);

		if (certfile)
		{
			if (!SSL_CTX_use_certificate_file((SSL_CTX*)s->ctx, (char*)certfile, SSL_FILETYPE_PEM))
				printf("SSL load certificate failed\n");

			//if (!SSL_CTX_set_default_verify_paths((SSL_CTX*)s->ctx))
			//	printf("SSL set_default_verify_paths failed\n");
		}
	}

	s->ssl = (SSL*)SSL_new((SSL_CTX*)s->ctx);
	if (!s->ssl) return 0;

	SSL_set_ssl_method((SSL*)s->ssl, s->h?TLSv1_2_server_method():TLSv1_2_client_method());

	if ((level > 0) && certfile)
		SSL_set_verify((SSL*)s->ssl, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 0);
	else
		SSL_set_verify((SSL*)s->ssl, SSL_VERIFY_NONE, 0);

	SSL_set_fd((SSL*)s->ssl, s->fd);

	if (s->h)
	{
		if (SSL_accept((SSL*)s->ssl) == -1)
		{
			printf("SSL_accept failed\n");
			ERR_print_errors_fp(stderr);
			return 0;
		}
	}
	else
	{
		if (SSL_connect((SSL*)s->ssl) == -1)
		{
			printf("SSL_connect failed\n");
			ERR_print_errors_fp(stderr);
			return 0;
		}
	}

	char cipher[256];
	cipher[0] = 0;
	sscanf(SSL_get_cipher((SSL*)s->ssl), "%255s", cipher);
	cipher[255] = 0;
	//printf("CIPHER: %s\n", cipher);

	X509* server_cert = SSL_get_peer_certificate((SSL*)s->ssl);

	if (server_cert)
	{
		char buf[1024];
		//char* str;
		//buf[0] = 0;
		//str = X509_NAME_oneline(X509_get_subject_name(server_cert), buf, sizeof(buf));
		buf[0] = 0;
		X509_NAME_oneline(X509_get_issuer_name(server_cert), buf, sizeof(buf));
		const char* ptr = strstr(buf, "/CN=");
		char common_name[256];
		common_name[0] = 0;
		if (ptr) sscanf(ptr+4, "%255[^/]", common_name);
		common_name[255] = 0;
		//printf("SSL Server Common name = '%s'\n", common_name);
		X509_free(server_cert);
	}
	else
	{
		if (level > 0)
			printf("SSL No server certificate\n");
	}

	SSL_set_read_ahead((SSL*)s->ssl, s->h?0:0);
	s->is_ssl = 1;
	return 1;
#else
	return 0;
#endif
}

int session_on_connect(session s)
{
	if (!s)
		return 0;

	if (s->connected)
	{
		s->connected = 0;
		return 1;
	}

	return 0;
}

int session_on_disconnect(session s)
{
	if (!s)
		return 0;

	return s->disconnected;
}

void session_lock(session s)
{
	lock_lock(s->h->strand);
}

void session_unlock(session s)
{
	lock_unlock(s->h->strand);
}

#ifdef _WIN32
const char* inet_ntop(int family, void* address, char* buffer, socklen_t len)
{
	DWORD buflen = 256;

    if (family == AF_INET6)
    {
		struct sockaddr_in6 sin6 = {0};
		sin6.sin6_family = family;
		sin6.sin6_addr = *((struct in6_addr*)address);

		if (WSAAddressToString((struct sockaddr*)&sin6,
								   sizeof(sin6),
								   NULL,       // LPWSAPROTOCOL_INFO
								   buffer,
								   &buflen) == SOCKET_ERROR)
				strcpy(buffer, "");
	}
	else
    {
		struct sockaddr_in sin4 = {0};
		sin4.sin_family = family;
		sin4.sin_addr = *((struct in_addr*)address);

		if (WSAAddressToString((struct sockaddr*)&sin4,
								   sizeof(sin4),
								   NULL,       // LPWSAPROTOCOL_INFO
								   buffer,
								   &buflen) == SOCKET_ERROR)
				strcpy(buffer, "");
	}

    return buffer;
}
#endif

void session_set_udata_flag(session s, int flag)
{
	if (!s || (flag < 0) || (flag > 63))
		return;

	s->udata_flags |= ((uint64_t)1) << flag;
}

void session_clr_udata_flag(session s, int flag)
{
	if (!s || (flag < 0) || (flag > 63))
		return;

	s->udata_flags &= ~(((uint64_t)1) << flag);
}

void session_clr_udata_flags(session s)
{
	if (!s)
		return;

	s->udata_flags = 0;
}

int session_get_udata_flag(session s, int flag)
{
	if (!s || (flag < 0) || (flag > 63))
		return 0;

	return s->udata_flags & (((uint64_t)1) << flag);
}

void session_set_udata_int(session s, long long data)
{
	if (!s)
		return;

	s->udata_int = data;
}

long long session_get_udata_int(session s)
{
	if (!s)
		return 0;

	return s->udata_int;
}

void session_set_udata_real(session s, double data)
{
	if (!s)
		return;

	s->udata_real = data;
}

double session_get_udata_real(session s)
{
	if (!s)
		return 0;

	return s->udata_real;
}

void session_clr_stash(session s)
{
	if (!s)
		return;

	if (!s->stash)
		return;

	sl_string_destroy(s->stash);
	s->stash = 0;
}

void session_set_stash(session s, const char* key, const char* value)
{
	if (!s)
		return;

	if (!s->tcp)
		return;

	if (!s->stash)
		s->stash = sl_string_create2();

	sl_string_add(s->stash, key, value);
}

void session_del_stash(session s, const char* key)
{
	if (!s)
		return;

	if (!s->stash)
		return;

	sl_string_rem(s->stash, key);
}

const char* session_get_stash(session s, const char* key)
{
	if (!s)
		return NULL;

	if (!s->stash)
		return NULL;

	void* v = NULL;
	sl_string_get(s->stash, key, &v);
	return (const char*)v;
}

int session_enable_broadcast(session s)
{
	if (!s)
		return 0;

	if (s->disconnected)
		return 0;

	int flag = 1;
	return setsockopt(s->fd, SOL_SOCKET, SO_BROADCAST, (char*)&flag, sizeof(flag));
}

int session_enable_multicast(session s, int loop, int hops)
{
	if (!s)
		return 0;

	if (s->disconnected)
		return 0;

	int status;

	if (!s->ipv4)
	{
		setsockopt(s->fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (const char*)&loop, sizeof(loop));
		status = setsockopt(s->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (const char*)&hops, sizeof(hops));
	}
	else
	{
		setsockopt(s->fd, IPPROTO_IP, IP_MULTICAST_LOOP, (const char*)&loop, sizeof(loop));
		status = setsockopt(s->fd, IPPROTO_IP, IP_MULTICAST_TTL, (const char*)&hops, sizeof(hops));
	}

	return status;
}

int session_set_sndbuffer(session s, int bufsize)
{
	if (!s)
		return 0;

	setsockopt(s->fd, SOL_SOCKET, SO_SNDBUF, (const char*)&bufsize, sizeof(bufsize));
	return 1;
}

int session_set_rcvbuffer(session s, int bufsize)
{
	if (!s)
		return 0;

	setsockopt(s->fd, SOL_SOCKET, SO_RCVBUF, (const char*)&bufsize, sizeof(bufsize));
	return 1;
}

const char* session_get_remote_host(session s, int resolve)
{
	if (!s)
		return "";

	if (s->disconnected)
		return "";

	if (!s->remote)
		s->remote = (char*)malloc(256);

	if (s->tcp && s->ipv4)
	{
		socklen_t len = sizeof(struct sockaddr_in);

		if (getpeername(s->fd, (struct sockaddr*)&s->addr4, &len) == 0)
			return inet_ntop(AF_INET, &s->addr4.sin_addr, s->remote, INET_ADDRSTRLEN);
	}
	else if (s->tcp)
	{
		socklen_t len = sizeof(struct sockaddr_in6);

		if (getpeername(s->fd, (struct sockaddr*)&s->addr6, &len) == 0)
			return inet_ntop(AF_INET6, &s->addr6.sin6_addr, s->remote, INET6_ADDRSTRLEN);
	}
	else if (s->ipv4)
	{
		return inet_ntop(AF_INET, &s->addr4.sin_addr, s->remote, INET_ADDRSTRLEN);
	}
	else
	{
		return inet_ntop(AF_INET6, &s->addr6.sin6_addr, s->remote, INET6_ADDRSTRLEN);
	}

	return "";
}

int session_write(session s, const void* _buf, size_t len)
{
	const char* buf = (const char*)_buf;

	if (!s || !buf || !len)
		return 0;

	if (s->disconnected)
		return 0;

	time_t started = 0;

	while (len > 0)
	{
		int wlen = 0;

#if USE_SSL
		if (s->is_ssl)
		{
			wlen = SSL_write((SSL*)s->ssl, buf, len);

			if (wlen < 0)
				s->disconnected = 1;
		}
		else
#endif
		if (s->tcp)
		{
			wlen = send(s->fd, (const char*)buf, len, MSG_NOSIGNAL);

			if (wlen < 0)
				s->disconnected = 1;
		}
		else if (s->ipv4)
		{
			socklen_t alen = sizeof(struct sockaddr_in);
			wlen = sendto(s->fd, (const char*)buf, len, MSG_NOSIGNAL, (struct sockaddr*)&s->addr4, alen);
		}
		else
		{
			socklen_t alen = sizeof(struct sockaddr_in6);
			wlen = sendto(s->fd, (const char*)buf, len, MSG_NOSIGNAL, (struct sockaddr*)&s->addr6, alen);
		}

		if (wlen == len)
			break;

		if (errno == EINTR)		// With SSL only?
			continue;

		if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
			return 0;

		if (wlen > 0)
		{
			len -= wlen;
			buf += wlen;
		}

		if (!started)
			started = time(NULL);
		else if ((time(NULL) - started) >= SESSION_TIMEOUT_SECONDS)
		{
			s->disconnected = 1;
			shutdown(s->fd, SHUT_RDWR);
			return 0;
		}

		msleep(1);
	}

	return 1;
}

int session_writemsg(session s, const char* buf)
{
	return session_write(s, buf, strlen(buf));
}

int session_bcast(session s, const void* buf, size_t len)
{
	if (!s || !buf || !len)
		return 0;

	if (s->tcp)
		return 0;

	struct sockaddr_in addr4 = {0};
	addr4.sin_family = AF_INET;
	addr4.sin_port = htons(s->port);
	addr4.sin_addr.s_addr = -1;
	socklen_t alen = sizeof(struct sockaddr_in);
	int wlen = sendto(s->fd, (const char*)buf, len, MSG_NOSIGNAL, (struct sockaddr*)&addr4, alen);
	return wlen > 0;
}

int session_bcastmsg(session s, const char* buf)
{
	return session_bcast(s, buf, strlen(buf));
}

int session_read(session s, void* buf, size_t len)
{
	if (!s || !buf || !len)
		return 0;

	if (s->disconnected)
		return 0;

	int rlen;

#if USE_SSL
	if (s->is_ssl)
	{
		rlen = SSL_read((SSL*)s->ssl, s->srcbuf, BUFLEN-1);
	}
	else
#endif
	if (s->tcp)
	{
		rlen = recv(s->fd, (char*)buf, len, 0);
	}
	else if (s->ipv4)
	{
		socklen_t alen = sizeof(struct sockaddr_in);
		rlen = recvfrom(s->fd, (char*)buf, len, 0, (struct sockaddr*)&s->addr4, &alen);
	}
	else
	{
		socklen_t alen = sizeof(struct sockaddr_in6);
		rlen = recvfrom(s->fd, (char*)buf, len, 0, (struct sockaddr*)&s->addr6, &alen);
	}

	if ((rlen < 0) &&
		((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)))
		return 0;

	if (rlen <= 0)
	{
		if (s->tcp)
			s->disconnected = 1;

		return 0;
	}

	return rlen > 0;
}

int session_readmsg(session s, char** buf)
{
	if (!s || !buf)
		return 0;

	if (s->disconnected)
		return 0;

	// Allocate destination message buffer
	// if one doesn't already exist.

	if (!s->dstbuf)
	{
		s->len = BUFLEN;
		s->dstbuf = (char*)malloc(s->len);
		if (!s->dstbuf) return 0;
		s->dst = s->dstbuf;
	}

	// Read until end of input buffer or newline,
	// whichever comes first.

	while (*s->src && (*s->src != '\n'))
	{
		*s->dst++ = *s->src++;

		// Allow space for \n\0

		if ((s->dst - s->dstbuf) == (s->len-2))
		{
			int save_len = s->len;
			s->len += BUFLEN;
			s->dstbuf = (char*)realloc(s->dstbuf, s->len);
			if (!s->dstbuf) return 0;
			s->dst = s->dstbuf + save_len;
		}
	}

	// If we have a newline then we have a
	// complete message to return.

	if (*s->src == '\n')
	{
		*s->dst++ = *s->src++;
		*s->dst = 0;
		*buf = s->dstbuf;
		size_t len = s->dst - s->dstbuf;
		s->dst = s->dstbuf;
		return len;
	}

	// If not then read some more and repeat.

	int rlen;
	s->srcbuf[0] = 0;
	s->src = s->srcbuf;

#if USE_SSL
	if (s->is_ssl)
	{
		rlen = SSL_read((SSL*)s->ssl, s->srcbuf, BUFLEN-1);
	}
	else
#endif
	if (s->tcp)
	{
		rlen = recv(s->fd, s->srcbuf, BUFLEN-1, 0);
	}
	else if (s->ipv4)
	{
		socklen_t slen = sizeof(struct sockaddr_in);
		rlen = recvfrom(s->fd, s->srcbuf, BUFLEN-1, 0, (struct sockaddr*)&s->addr4, &slen);
	}
	else
	{
		socklen_t slen = sizeof(struct sockaddr_in6);
		rlen = recvfrom(s->fd, s->srcbuf, BUFLEN-1, 0, (struct sockaddr*)&s->addr6, &slen);
	}

	if ((rlen < 0) &&
		((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)))
		return 0;

	if (rlen <= 0)
	{
		if (s->tcp)
			s->disconnected = 1;

		return 0;
	}

	s->srcbuf[rlen] = 0;
	return session_readmsg(s, buf);
}

int session_shutdown(session s)
{
	if (!s)
		return 0;

#if USE_SSL
	if (s->is_ssl)
		SSL_smart_shutdown(s->ssl);
#endif

	if (s->fd != -1)
		shutdown(s->fd, SHUT_RDWR);

	return 1;
}

static int session_free(session s)
{
	if (s->dstbuf)
		free(s->dstbuf);

	if (s->stash)
		sl_string_destroy(s->stash);

	if (s->remote)
		free(s->remote);

#if USE_SSL
	if (s->ssl)
		SSL_free((SSL*)s->ssl);
#endif

	free(s);
	return 1;
}

void session_share(session s)
{
	if (!s)
		return;

	atomic_inc(&s->use_cnt);
}

void session_unshare(session s)
{
	if (!s)
		return;

	if (atomic_dec(&s->use_cnt))
		return;

	lock_destroy(s->strand);
	session_free(s);
}

int session_close(session s)
{
	if (!s)
		return 0;

	if (s->fd != -1)
	{
		shutdown(s->fd, SHUT_RDWR);
		close(s->fd);
	}

	session_unshare(s);
	return 1;
}

static int handler_force_drop(void* _h, int fd, void* _s)
{
	session_close((session)_s);
	return 1;
}

static int handler_accept(handler h, server srv, session* v)
{
	if (h->halt)
		return -1;

	int newfd;

	struct sockaddr_in6 addr6 = {0};
	addr6.sin6_family = AF_UNSPEC;
	socklen_t len = sizeof(addr6);

	if ((newfd = accept(srv->fd, (struct sockaddr*)&addr6, &len)) < 0)
	{
		printf("handler_accept: accept6 fd=%d failed: %s\n", srv->fd, strerror(errno));
		return -1;
	}

	int flag = 1;
	setsockopt(newfd, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(flag));
	flag = 1;
	setsockopt(newfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));

	session s = (session)calloc(1, sizeof(struct _session));
	if (!s) return 0;
	s->strand = lock_create();
	session_share(s);
	s->connected = 1;
	s->h = h;
	s->fd = newfd;
	s->port = srv->port;
	s->tcp = 1;
	s->ipv4 = srv->ipv4;
	s->ctx = h->ctx;
	s->src = s->srcbuf;
	s->f = srv->f;
	s->v = srv->v;
	*v = s;

	if (srv->ssl)
	{
		if (!session_enable_tls(s, NULL, 0))
		{
			close(newfd);
			return -1;
		}
	}

	unsigned long flag2 = 1;
	ioctl(newfd, FIONBIO, &flag2);

	sl_int_add(h->fds, newfd, s);
	h->use++;
	return newfd;
}

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__)

static int kqueue_accept(void* data)
{
	session s = (session)data;
	s->f(s, s->v);
	struct kevent ev = {0};
	EV_SET(&ev, s->fd, EVFILT_READ, EV_ADD|EV_CLEAR, 0, 0, s);
	kevent(s->h->fd, &ev, 1, NULL, 0, NULL);
	return 1;
}

static int kqueue_run(void* data)
{
	session s = (session)data;
	lock_lock(s->strand);

	// Running edge-triggered,
	// so drain the source...

	while (s->f(s, s->v))
		;

	lock_unlock(s->strand);
	return 1;
}

int handler_wait_kqueue(handler h)
{
	if (g_debug) printf("USING KQUEUE\n");
	struct kevent ev = {0}, events[MAX_EVENTS];
	int i;

	for (i = 0; i < h->cnt; i++)
	{
		server srv = &h->srvs[i];

		if (!srv->tcp)
			EV_SET(&ev, srv->fd, EVFILT_READ, EV_ADD|EV_CLEAR, 0, 0, (void*)(size_t)i);
		else
			EV_SET(&ev, srv->fd, EVFILT_READ, EV_ADD, 0, 0, (void*)(size_t)i);

		kevent(h->fd, &ev, 1, NULL, 0, NULL);
	}

	while (!h->halt && h->use)
	{
		struct timespec ts = {0, 1000*1000*100};
		int i, n = kevent(h->fd, NULL, 0, (struct kevent*)events, MAX_EVENTS, &ts);

		for (i = 0; i < n; i++)
		{
			session s = 0;

			if ((int)events[i].udata < h->cnt)
			{
				int idx = (int)events[i].udata;
				server srv = &h->srvs[idx];

				if (srv->tcp)
				{
					int newfd = handler_accept(h, srv, &s);

					if (newfd == -1)
						continue;

					tpool_start(h->tp, &kqueue_accept, s);
				}
				else
				{
					struct _session dummy = {0};
					s = &dummy;
					s->h = h;
					s->fd = srv->fd;
					s->port = srv->port;
					s->ipv4 = srv->ipv4;
					s->src = s->srcbuf;
					s->f = srv->f;
					s->v = srv->v;
					tpool_start(h->tp, &kqueue_run, s);
				}

				continue;
			}

			s = (session)events[i].udata;

			if (events[i].flags & EV_EOF)
				s->disconnected = 1;

			if (s->disconnected)
			{
				EV_SET(&ev, s->fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
				kevent(h->fd, &ev, 1, NULL, 0, NULL);
				sl_int_rem(h->fds, s->fd);
				h->use--;
				s->disconnected = 1;
				s->f(s, s->v);
				session_close(s);
				continue;
			}

			tpool_start(h->tp, &kqueue_run, s);
		}
	}

	close(h->fd);
	return 1;
}

#endif

#if defined(__linux__)

static int epoll_accept(void* data)
{
	session s = (session)data;
	s->f(s, s->v);
	struct epoll_event ev = {0};
	ev.events = EPOLLIN|EPOLLET|EPOLLRDHUP;
	ev.data.ptr = s;
	epoll_ctl(s->h->fd, EPOLL_CTL_ADD, s->fd, &ev);
	return 1;
}

static int epoll_run(void* data)
{
	session s = (session)data;
	lock_lock(s->strand);

	// Running edge-triggered,
	// so drain the source...

	while (s->f(s, s->v))
		;

	lock_unlock(s->strand);
	return 1;
}

int handler_wait_epoll(handler h)
{
	if (g_debug) printf("USING EPOLL\n");
	struct epoll_event ev = {0}, events[MAX_EVENTS];
	int i;

	for (i = 0; i < h->cnt; i++)
	{

		server srv = &h->srvs[i];
		ev.events = EPOLLIN;

		if (!srv->tcp)
			ev.events |= EPOLLET;

		ev.data.u64 = i;
		epoll_ctl(h->fd, EPOLL_CTL_ADD, srv->fd, &ev);
	}

	while (!h->halt && h->use)
	{
		int i, n = epoll_wait(h->fd, (struct epoll_event*)events, MAX_EVENTS, 100);

		for (i = 0; i < n; i++)
		{
			session s = 0;

			if (events[i].data.u64 < h->cnt)
			{
				int idx = events[i].data.u64;
				server srv = &h->srvs[idx];

				if (srv->tcp)
				{
					int newfd = handler_accept(h, srv, &s);

					if (newfd == -1)
						continue;

					tpool_start(h->tp, &epoll_accept, s);
				}
				else
				{
					struct _session dummy = {0};
					s = &dummy;
					s->h = h;
					s->fd = srv->fd;
					s->port = srv->port;
					s->ipv4 = srv->ipv4;
					s->src = s->srcbuf;
					s->f = srv->f;
					s->v = srv->v;
					tpool_start(h->tp, &epoll_run, s);
				}

				continue;
			}

			s = (session)events[i].data.ptr;

			if (events[i].events & EPOLLRDHUP)
				s->disconnected = 1;

			if (s->disconnected)
			{
				epoll_ctl(h->fd, EPOLL_CTL_DEL, s->fd, &ev);
				sl_int_rem(h->fds, s->fd);
				h->use--;
				s->disconnected = 1;
				s->f(s, s->v);
				session_close(s);
				continue;
			}

			tpool_start(h->tp, &epoll_run, s);
		}
	}

	close(h->fd);
	return 1;
}

#endif

#if defined(POLLIN) && WANT_POLL

static int poll_accept(void* data)
{
	session s = (session)data;
	s->f(s, s->v);
	h->rpollfds[s->idx].fd = -1;
	h->rpollfds[s->idx].events = POLLIN|POLLRDHUP;
	h->rpollfds[s->idx].revents = 0;
	return 1;
}

static int poll_run(void* data)
{
	session s = (session)data;
	lock_lock(s->strand);

	while (s->f(s, s->v))
		;

	lock_unlock(s->strand);
	s->h->rpollfds[s->idx].fd = s->fd;
	return 1;
}

int handler_wait_poll(handler h)
{
	if (g_debug) printf("USING POLL\n");
	int i;

	for (i = 0; i < h->cnt; i++)
	{
		h->rpollfds[i].fd = h->srvs[i].fd;
		h->rpollfds[i].events = POLLIN;
		h->rpollfds[i].revents = 0;
	}

	int cnt = i;

	while (!h->halt && h->use)
	{
		int i, n = poll(h->rpollfds, cnt, 10);

		for (i = 0; (i < cnt) && n; i++)
		{
			session s = 0;

			int fd = h->rpollfds[i].fd;
			if (fd == -1) continue;

			if ((i < h->cnt) && (h->rpollfds[i].revents & POLLIN))
			{
				server srv = &h->srvs[i];

				if (srv->tcp)
				{
					int newfd = handler_accept(h, srv, &s);

					if (newfd == -1)
						continue;

					s->idx = cnt++;
					tpool_start(h->tp, &poll_accept, s);
				}
				else
				{
					struct _session dummy = {0};
					s = &dummy;
					s->h = h;
					s->fd = srv->fd;
					s->port = srv->port;
					s->ipv4 = srv->ipv4;
					s->src = s->srcbuf;
					s->f = srv->f;
					s->v = srv->v;
					s->idx = i;
					h->rpollfds[i].fd = -1;
					tpool_start(h->tp, &poll_run, s);
				}

				continue;
			}

			if (!sl_int_get(h->fds, fd, &s))
				continue;

#ifdef _WIN32
			// WSAPoll doesn't support POLLRDHUP...

			if (!s->disconnected)
			{
				char buf;
				int rlen = recv(s->fd, &buf, 1, MSG_PEEK);

				if (rlen == 0)
					s->disconnected = 1;

				if ((rlen < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
					s->disconnected = 1;
			}
#else
			if (h->rpollfds[i].revents & POLLRDHUP)
				s->disconnected = 1;
#endif

			if (s->disconnected)
			{
				sl_int_rem(h->fds, s->fd);
				h->use--;
				h->rpollfds[i--] = h->rpollfds[--cnt];

#ifndef _WIN32
				if (!h->threads)
#endif
					s->f(s, s->v);

				session_close(s);
			}
			else if (h->rpollfds[i].revents & POLLIN)
			{
				s->idx = i;
				h->rpollfds[i].fd = -1;
				tpool_start(h->tp, &poll_run, s);
			}

		}
	}

	return 1;
}

#endif

static int handler_select_set(void* _h, int fd, void* _s)
{
	if (fd == -1)
		return 1;

	handler h = (handler)_h;
	FD_SET(fd, &h->rfds);

	if (fd > h->hi)
		h->hi = fd;

	return 1;
}

static int select_accept(void* data)
{
	session s = (session)data;
	s->f(s, s->v);
	return 1;
}

static int select_run(void* data)
{
	session s = (session)data;
	lock_lock(s->strand);

	while (s->f(s, s->v))
		;

	lock_unlock(s->strand);
	handler_select_set(s->h, s->fd, s);
	s->h->srvs[s->idx].fd = s->fd;
	s->busy = 0;
	return 1;
}

static int handler_select_check(void* _h, int fd, void* _s)
{
	handler h = (handler)_h;
	session s = (session)_s;

	if (s->busy != 0)
		return 1;

	if (!s->disconnected)
	{
		char buf;
		int rlen = recv(s->fd, &buf, 1, MSG_PEEK);

		if (rlen == 0)
			s->disconnected = 1;

		if ((rlen < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
			s->disconnected = 1;

		if (s->disconnected)
		{
			sl_int_add(h->badfds, fd, s);
			return 1;
		}
	}

	if (!FD_ISSET(fd, &h->rfds))
		return 1;

	s->busy = 1;
	tpool_start(s->h->tp, &select_run, s);
	return 1;
}

static int handler_select_bads(void* _h, int fd, void* _s)
{
	handler h = (handler)_h;
	session s = (session)_s;

	s->f(s, s->v);
	session_close(s);
	sl_int_rem(h->fds, fd);
	h->use--;
	return 1;
}

int handler_wait_select(handler h)
{
	if (g_debug) printf("USING SELECT\n");
	h->badfds = sl_int_create();

	while (!h->halt && h->use)
	{
		FD_ZERO(&h->rfds);
		h->hi = 0;
		int i;

		for (i = 0; i < h->cnt; i++)
			handler_select_set(h, h->srvs[i].fd, NULL);

		sl_int_iter(h->fds, &handler_select_set, h);

		// When running with threads it would be better to wake up
		// select with a signal rather than use the timeout...

		struct timeval tv = {0, 1000*10};
		int n = select(h->hi+1, &h->rfds, 0, 0, &tv);
		int cnt = h->cnt;

		for (i = 0; (i < h->cnt) && n; i++)
		{
			server srv = &h->srvs[i];
			session s = 0;

			if (!FD_ISSET(srv->fd, &h->rfds))
				continue;

			if (srv->tcp)
			{
				int newfd = handler_accept(h, srv, &s);

				if (newfd == -1)
					continue;

				s->busy = 1;
				s->idx = cnt++;
				tpool_start(h->tp, &select_accept, s);
			}
			else
			{
				struct _session dummy = {0};
				s = &dummy;
				s->h = h;
				s->fd = srv->fd;
				s->port = srv->port;
				s->ipv4 = srv->ipv4;
				s->src = s->srcbuf;
				s->f = srv->f;
				s->v = srv->v;
				s->busy = 1;
				s->idx = i;
				h->srvs[i].fd = -1;
				tpool_start(h->tp, &select_run, s);
			}
		}

		sl_int_iter(h->fds, &handler_select_check, h);

		if (sl_int_count(h->badfds) > 0)
		{
			sl_int_iter(h->badfds, &handler_select_bads, h);
			sl_int_destroy(h->badfds);
			h->badfds = sl_int_create();
		}
	}

	sl_int_destroy(h->badfds);
	return 1;
}

// Use the platform-best option...

int handler_wait(handler h)
{
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__)
	return handler_wait_kqueue(h);
#elif defined(__linux__)
	return handler_wait_epoll(h);
#elif defined(POLLIN) && WANT_POLL
	return handler_wait_poll(h);
#else
	return handler_wait_select(h);
#endif
}

int handler_set_tls(handler h, const char* keyfile)
{
#if USE_SSL
	static int init = 0;

	if (!init)
	{
		SSL_load_error_strings();
		SSLeay_add_all_algorithms();
		SSL_library_init();
		init = 1;
	}

	h->ctx = (SSL_CTX*)SSL_CTX_new(TLSv1_2_server_method());

	if (!h->ctx)
	{
		printf("SSL new server context failed\n");
		ERR_print_errors_fp(stderr);
		return 0;
	}

	tmprsakey512 = RSA_generate_key(512, RSA_F4, NULL, NULL);
	tmprsakey1024 = RSA_generate_key(1024, RSA_F4, NULL, NULL);

	SSL_CTX_set_options((SSL_CTX*)h->ctx, SSL_OP_ALL);
	SSL_CTX_set_cipher_list((SSL_CTX*)h->ctx, SSL_DEFAULT_CIPHER_LIST);
	SSL_CTX_set_options((SSL_CTX*)h->ctx, SSL_OP_SINGLE_DH_USE);
	SSL_CTX_set_tmp_rsa_callback((SSL_CTX*)h->ctx, ssl_callback_TmpRSA);
	SSL_CTX_set_tmp_dh_callback((SSL_CTX*)h->ctx, ssl_callback_TmpDH);

	if (keyfile)
	{
		if (!SSL_CTX_use_RSAPrivateKey_file((SSL_CTX*)h->ctx, (char*)keyfile,  SSL_FILETYPE_PEM))
			printf("SSL load RSA key failed\n");

		if (!SSL_CTX_use_certificate_file((SSL_CTX*)h->ctx, (char*)keyfile, SSL_FILETYPE_PEM))
			printf("SSL load certificate failed\n");

		//if (!SSL_CTX_set_default_verify_paths((SSL_CTX*)h->ctx))
		//	printf("SSL set_default_verify_paths failed\n");

		if (!SSL_CTX_load_verify_locations((SSL_CTX*)h->ctx, (char*)keyfile, (char*)NULL))
		{
			printf("SSL set_load_verify_locations failed\n");

			if (!SSL_CTX_set_default_verify_paths((SSL_CTX*)h->ctx))
				printf("SSL set_default_verify_paths faile\n");
		}
	}

	return 1;
#else
	return 0;
#endif
}

static int join_multicast(int fd6, int fd4, const char* addr)
{
	if (!addr)
		return 0;

	int status = 0;

	if (fd6 >= 0)
	{
		struct sockaddr_in6 addr6 = {0};
		addr6.sin6_family = AF_INET6;

		if (parse_addr6(addr, &addr6))
		{
			struct ipv6_mreq mreq;
			memcpy(&mreq.ipv6mr_multiaddr, &addr6.sin6_addr, sizeof(struct in6_addr));
			mreq.ipv6mr_interface = 0;
			status = setsockopt(fd6, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char*)&mreq, sizeof(mreq));
		}
	}

	if (fd4 >= 0)
	{
		struct sockaddr_in addr4 = {0};
		addr4.sin_family = AF_INET;

		if (parse_addr4(addr, &addr4))
		{
			struct ip_mreq mreq;
			memcpy(&mreq.imr_multiaddr, &addr4.sin_addr, sizeof(struct in_addr));
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);
			status = setsockopt(fd4, IPPROTO_IP, IPV6_JOIN_GROUP, (char*)&mreq, sizeof(mreq));
		}
	}

	return status;
}

#if 0
static int leave_multicast(int fd6, int fd4, const char* addr)
{
	if (!addr)
		return 0;

	int status = 0;

	if (fd6 >= 0)
	{
		struct sockaddr_in6 addr6 = {0};
		addr6.sin6_family = AF_INET6;

		if (parse_addr6(addr, &addr6))
		{
			struct ipv6_mreq mreq;
			memcpy(&mreq.ipv6mr_multiaddr, &addr6.sin6_addr, sizeof(struct in6_addr));
			mreq.ipv6mr_interface = 0;
			status = setsockopt(fd6, IPPROTO_IPV6, IPV6_LEAVE_GROUP, (char*)&mreq, sizeof(mreq));
		}
	}

	if (fd4 >= 0)
	{
		struct sockaddr_in addr4 = {0};
		addr4.sin_family = AF_INET;

		if (parse_addr4(addr, &addr4))
		{
			struct ip_mreq mreq;
			memcpy(&mreq.imr_multiaddr, &addr4.sin_addr, sizeof(struct in_addr));
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);
			status = setsockopt(fd4, IPPROTO_IP, IPV6_LEAVE_GROUP, (char*)&mreq, sizeof(mreq));
		}
	}

	return status;
}
#endif

int handler_add_uncle(handler h, const char* binding, unsigned short port, const char* scope)
{
	extern uncle uncle_create2(handler h, const char* binding, unsigned short port, const char* scope);
	uncle u = uncle_create2(h, binding, port, scope);
	if (!u) return 0;
	h->u[h->uncs++] = u;
	return 1;
}

uncle handler_get_uncle(handler h, const char* scope)
{
	int i;

	for (i = 0; i < h->uncs; i++)
	{
		if (!strcmp(uncle_get_scope(h->u[i]), scope))
			return h->u[i];
	}

	return NULL;
}

static int handler_add_server2(handler h, int (*f)(session, void* v), void* v, const char* binding, unsigned short port, int tcp, int ssl, const char* maddr6, const char* maddr4, const char* name)
{
	int fd6 = socket(AF_INET6, tcp?SOCK_STREAM:SOCK_DGRAM, 0);

	if (fd6 < 0)
	{
		printf("handler_add_server: error socket6 failed: %s\n", strerror(errno));
		return 0;
	}

	int flag = 1;
	setsockopt(fd6, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));

#if !defined(_WIN32)
	flag = 1;
	setsockopt(fd6, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
#endif

#if !defined(_WIN32)
	flag = 1;
	setsockopt(fd6, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&flag, sizeof(flag));
#endif

	struct sockaddr_in6 addr6 = {0};
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(port);
	const struct in6_addr my_in6addr_any = IN6ADDR_ANY_INIT;
	addr6.sin6_addr = my_in6addr_any;

	if (bind(fd6, (struct sockaddr*)&addr6, sizeof(addr6)) != 0)
	{
		printf("handler_add_server: warning bind6 failed: %s\n", strerror(errno));
		close(fd6);
		fd6 = -1;
		return 0;
	}

	if (!tcp)
	{
		unsigned long flag2 = 1;
		ioctl(fd6, FIONBIO, &flag2);
	}

	if (fd6 != -1)
	{
		server srv = &h->srvs[h->cnt++];
		srv->fd = fd6;
		srv->port = port;
		srv->tcp = tcp;
		srv->ssl = ssl && tcp && h->ctx;
		srv->ipv4 = 0;
		srv->f = f;
		srv->v = v;

		if (maddr6)
			join_multicast(fd6, -1, maddr6);
	}

	if (tcp && (fd6 != -1))
	{
		if (listen(fd6, 128) != 0)
		{
			printf("handler_add_server: error listen6 failed: %s\n", strerror(errno));
			close(fd6);
			return 0;
		}
	}

	int fd4 = socket(AF_INET, tcp?SOCK_STREAM:SOCK_DGRAM, 0);

	if (fd4 < 0)
	{
		printf("handler_add_server: error socket4 failed: %s\n", strerror(errno));
		return 0;
	}

	flag = 1;
	setsockopt(fd4, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));
#if !defined(_WIN32)
	flag = 1;
	setsockopt(fd4, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
#endif

	struct sockaddr_in addr4 = {0};
	addr4.sin_family = AF_INET;
	addr4.sin_port = htons(port);
	addr4.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(fd4, (struct sockaddr*)&addr4, sizeof(addr4)) != 0)
	{
		printf("handler_add_server: warning bind4 failed: %s\n", strerror(errno));
		close(fd4);
		fd4 = -1;
		return 0;
	}

	if (!tcp)
	{
		unsigned long flag2 = 1;
		ioctl(fd4, FIONBIO, &flag2);
	}

	if (fd4 != -1)
	{
		server srv = &h->srvs[h->cnt++];
		srv->fd = fd4;
		srv->port = port;
		srv->tcp = tcp;
		srv->ssl = ssl && tcp && h->ctx;
		srv->ipv4 = 1;
		srv->f = f;
		srv->v = v;

		if (maddr4)
			join_multicast(-1, fd4, maddr4);
	}

	if (tcp && (fd4 != -1))
	{
		if (listen(fd4, 128) != 0)
		{
			printf("handler_add_server: error listen4 failed: %s\n", strerror(errno));
			close(fd4);
			return 0;
		}
	}

	if (name && name[0] && h->uncs)
	{
		uncle u = h->u[h->uncs-1];
		uncle_add(u, name, hostname(), port, tcp, ssl);
	}

	h->use++;
	return 1;
}

int handler_add_server(handler h, int (*f)(session, void* v), void* v, const char* binding, unsigned short port, int tcp, int ssl, const char* name)
{
	return handler_add_server2(h, f, v, binding, port, tcp, ssl, NULL, NULL, name);
}

int handler_add_client(handler h, int (*f)(session, void* data), void* data, session s)
{
	session_share(s);
	s->h = h;
	s->f = f;
	s->v = data;
	h->use++;
	sl_int_add(h->fds, s->fd, s);

	unsigned long flag2 = 1;
	ioctl(s->fd, FIONBIO, &flag2);

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__)
	tpool_start(h->tp, &kqueue_accept, s);
#elif defined(__linux__)
	tpool_start(h->tp, &epoll_accept, s);
#elif defined(POLLIN) && WANT_POLL
	tpool_start(h->tp, &poll_accept, s);
#else
	tpool_start(h->tp, &select_accept, s);
#endif

	return 0;
}

handler handler_create(int threads)
{
#ifdef _WIN32
	static int cnt = 0;

	if (!cnt++)
	{
		WORD wVersionRequested = MAKEWORD(2,2);
		WSADATA wsaData;

		if (WSAStartup(wVersionRequested, &wsaData) != 0)
		{
		    WSACleanup();
		    return NULL;
		}
	}
#endif

	handler h = (handler)calloc(1, sizeof(struct _handler));
	if (!h) return NULL;
	h->fds = sl_int_create();
	h->tp = tpool_create(h->threads=threads);
	h->strand = lock_create();

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__)
	h->fd = kqueue();

	if (h->fd < 0)
		return 0;

#elif defined(__linux__)
	h->fd = epoll_create(10);

	if (h->fd < 0)
		return 0;

#elif defined(POLLIN) && WANT_POLL
#else
#endif

	return h;
}

int handler_add_multicast(handler h, int (*f)(session, void* v), void* v, const char* binding, unsigned short port, const char* addr6, const char* addr4, const char* name)
{
	return handler_add_server2(h, f, v, binding, port, 0, 0, addr6, addr4, name);
}

int handler_destroy(handler h)
{
	if (!h)
		return 0;

	h->halt = 1;
	msleep(100);
	int i;

	for (i = 0; i < h->uncs; i++)
		uncle_destroy(h->u[i]);

	if (h->tp)
		tpool_destroy(h->tp);

	lock_destroy(h->strand);

	sl_int_iter(h->fds, &handler_force_drop, h);
	sl_int_destroy(h->fds);

#if USE_SSL
	if (h->ctx)
		SSL_CTX_free((SSL_CTX*)h->ctx);
#endif

	free(h);
	return 1;
}
