#define _LARGEFILE64_SOURCE		// for 32-bit builds

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <dirent.h>
#endif

#ifdef _WIN32
#define fsync _commit
#define mkdir(p1,p2) _mkdir(p1)
#define msleep Sleep
#else
#define msleep(ms) usleep(ms*1000)
#endif

#include "store.h"
#include "tree.h"
#include "thread.h"

static const char TR_BEGIN = '+';		// Begin transaction (start)
static const char TR_END = '-';			// End transaction (commit)
static const char TR_CANCEL = '!';		// Cancel (rollback)

#define MAX_LOGFILE_SIZE (8LL*1024*1024*1024)

#define MAX_LOGFILES 256			// 8 bits +
#define POS_BITS 56					// 56 bits = 64 bits

#define MAKE_FILEPOS(idx,pos) (((uint64_t)idx << POS_BITS) | POS(pos))
#define FILEIDX(fp) (unsigned)((fp) >> POS_BITS)
#define POS(fp) ((fp) & ~(0xffULL<<POS_BITS))

#define FLAG_RM		1

typedef char string[1024];

#define ZEROTH_LOG "0.log"
#define FIRST_LOG "1.log"

struct _store
{
	tree tptr;
	string filename[MAX_LOGFILES], path1, path2;
	void (*f)(void*,const uuid,const void*,int);
	void* p1;
	uint64_t eodpos[MAX_LOGFILES];
	int fd[MAX_LOGFILES], idx;
	int transactions, current;
	lock lk;
};

struct _hstore
{
	store st;
	uint64_t start_pos;
	int wait_for_write, dbsync;
	unsigned nbr;
};

#ifdef _WIN32
static long pread(int fd, void *buf, size_t nbyte, off_t offset)
{
	DWORD len = 0;
	HANDLE h = (void*) _get_osfhandle(fd);
	OVERLAPPED ov = {0};
	ov.Offset = offset & 0xFFFFFFFF;
	ov.OffsetHigh = ((uint64_t)offset >> 32) & 0xFFFFFFFF;
	ReadFile(h, buf, (DWORD)nbyte, &len, &ov);
	return (long)len;
}

static long pwrite(int fd, const void *buf, size_t nbyte, off_t offset)
{
	DWORD len = 0;
	HANDLE h = (void*) _get_osfhandle(fd);
	OVERLAPPED ov = {0};
	ov.Offset = offset & 0xFFFFFFFF;
	ov.OffsetHigh = ((uint64_t)offset >> 32) & 0xFFFFFFFF;
	WriteFile(h, buf, (DWORD)nbyte, &len, &ov);
	return (long)len;
}
#endif

static void dirlist(const char* path, const char* dotted_ext, int (*f)(void*,const char*), void* p1)
{
	if (!path || !dotted_ext || !f)
		return;

	if (strchr(dotted_ext, '\\') || strchr(dotted_ext, '/'))
		return;

	if (dotted_ext[0] != '.')
		return;

#ifdef _WIN32
	if (strlen(path) > 256)
		return;

	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	char filespec[1024];
	sprintf(filespec, "%s\\*%s", path, dotted_ext);

	if ((hFind = FindFirstFile(filespec, &FindFileData)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!f(p1, FindFileData.cFileName))
				break;
		}
		 while(FindNextFile(hFind, &FindFileData));

		FindClose(hFind);
	}
#else
	DIR* dirp = opendir(path);
	if (!dirp) return;
	struct dirent entry;
	struct dirent* result;

	while (!readdir_r(dirp, &entry, &result))
	{
		if (!result)
			break;

		const char* tmpext = strrchr(entry.d_name, '.');
		if (!tmpext) continue;

		if (strcmp(tmpext, dotted_ext))
			continue;

		if (!f(p1, entry.d_name))
			break;
	}

	closedir(dirp);
#endif
}

static int prefix(char* buf, unsigned nbr, const uuid u, unsigned flags, unsigned len)
{
	char tmpbuf[256];
	return sprintf(buf, "* %X %s %X %X ", nbr, uuid_to_string(u, tmpbuf), flags, len);
}

static int parse(const char* buf, unsigned* nbr, uuid u, unsigned* flags, unsigned* len)
{
	char tmpbuf[256];
	tmpbuf[0] = 0;
	sscanf(buf, "%*s %X %s %X %X ", nbr, tmpbuf, flags, len);
	tmpbuf[sizeof(tmpbuf)-1] = 0;
	uuid_from_string(tmpbuf, u);
	const char* src = buf;

	while (*src++ != ' ')		// skip control
		;

	while (*src++ != ' ')		// skip nbr
		;

	while (*src++ != ' ')		// skip uuid_t
		;

	while (*src++ != ' ')		// skip flags
		;

	while (*src++ != ' ')		// skip len
		;

	return src-buf;
}

unsigned long store_count(const store st)
{
	return tree_count(st->tptr);
}

static int store_write2(store st, const void* buf, size_t len, const void* buf2, size_t len2, uint64_t pos)
{
	int wlen;

#ifdef _WIN32
	size_t nbytes = len + len2;
	char tmpbuf[1024*2];

	if (nbytes <= sizeof(tmpbuf))
	{
		memcpy(tmpbuf, buf, len);
		memcpy(tmpbuf+len, buf2, len2);
		wlen = (size_t)pwrite(st->fd[st->idx-1], tmpbuf, (size_t)nbytes, pos);
	}
	else
	{
		wlen = (size_t)pwrite(st->fd[st->idx-1], buf, (size_t)len, pos);
		wlen += pwrite(st->fd[st->idx-1], buf2, len2, pos+len2);
	}
#else
	struct iovec iov[2];
	iov[0].iov_base = (void*)buf;
	iov[0].iov_len = len;
	iov[1].iov_base = (void*)buf2;
	iov[1].iov_len = len2;
	wlen = pwritev(st->fd[st->idx-1], iov, 2, pos);
#endif

	if (wlen != (len+len2))
	{
		printf("store_write2 pwrite fd=%d data failed, pos=%llu\n", st->fd[st->idx-1], (unsigned long long)pos);
		return 0;
	}

	return 1;
}

static int store_write(store st, const void* buf, size_t len, uint64_t pos)
{
	int wlen = pwrite(st->fd[st->idx-1], buf, len, pos);

	if (wlen != len)
	{
		printf("store_write pwrite fd=%d data failed, pos=%llu\n", st->fd[st->idx-1], (unsigned long long)pos);
		return 0;
	}

	return 1;
}

static int store_apply(store st, int n, uint64_t pos)
{
	int fd = st->fd[st->idx-1];
	int cnt = 0;

	if (st->transactions)
		lock_lock(st->lk);

	for (;;)
	{
		char tmpbuf[1024];

		if (pread(fd, tmpbuf, sizeof(tmpbuf), pos) <= 0)
			return 0;

		if (tmpbuf[0] == TR_BEGIN)
		{
			unsigned nbr = 0;
			sscanf(tmpbuf, "%*s %X", &nbr);
			const char* src = tmpbuf;

			while (*src++ != '\n')
				pos++;
		}
		else if (tmpbuf[0] == TR_CANCEL)
		{
			unsigned nbr = 0;
			sscanf(tmpbuf, "%*s %X", &nbr);

			if (nbr == n)
				break;

			const char* src = tmpbuf;

			while (*src++ != '\n')
				pos++;
		}
		else if (tmpbuf[0] == TR_END)
		{
			unsigned nbr = 0;
			sscanf(tmpbuf, "%*s %X", &nbr);

			if (nbr == n)
				break;

			const char* src = tmpbuf;

			while (*src++ != '\n')
				pos++;
		}
		else
		{
			unsigned nbr, flags, nbytes;
			uuid_t u;

			int skip = parse(tmpbuf, &nbr, &u, &flags, &nbytes);

			if (nbr == n)
			{
				if (!(flags & FLAG_RM))
				{
					int idx = st->idx-1;
					uint64_t fp = MAKE_FILEPOS(idx,pos);
					tree_add(st->tptr, &u, fp);
				}
				else
					tree_del(st->tptr, &u);

				if (st->f)
				{
					if (nbytes)
					{
						char* src = tmpbuf+skip;
						int big = 0;

						if ((src+nbytes) >= (tmpbuf+sizeof(tmpbuf)))
						{
							big = 1;
							src = (char*)malloc(nbytes+1);

							if (pread(fd, src, nbytes, pos+skip) <= 0)
								break;

							src[nbytes] = 0;
						}

						st->f(st->p1, &u, src, flags&FLAG_RM?-nbytes:nbytes);
						if (big) free(src);
					}
					else
						st->f(st->p1, &u, NULL, 0);
				}

				cnt++;
			}

			pos += skip;
			pos += nbytes;
		}
	}

	if (st->transactions)
		lock_unlock(st->lk);

	return cnt;
}

int store_add(store st, const uuid u, const void* buf, size_t len)
{
	if (!st || !u || !buf || !len)
		return 0;

	if (len > STORE_MAX_WRITELEN)
		return 0;

	char tmpbuf[256];
	char* dst = tmpbuf;
	unsigned flags = 0;
	int plen = prefix(dst, 0, u, flags, len);
	uint64_t pos = st->eodpos[st->idx-1];

	if (!store_write2(st, tmpbuf, plen, buf, len, pos))
		return 0;

	st->eodpos[st->idx-1] += plen + len;
	int idx = st->idx-1;
	uint64_t fp = MAKE_FILEPOS(idx,pos);
	tree_add(st->tptr, u, fp);
	return 1;
}

int store_rem2(store st, const uuid u, const void* buf, size_t len)
{
	if (!st || !u || !buf || !len)
		return 0;

	if (!tree_del(st->tptr, u))
		return 0;

	char tmpbuf[256];
	unsigned flags = FLAG_RM;
	int plen = prefix(tmpbuf, 0, u, flags, len);
	uint64_t pos = st->eodpos[st->idx-1];

	if (!store_write2(st, tmpbuf, plen, buf, len, pos))
		return 0;

	st->eodpos[st->idx-1] += plen + len;
	return 1;
}

int store_rem(store st, const uuid u)
{
	if (!st || !u)
		return 0;

	if (!tree_del(st->tptr, u))
		return 0;

	char tmpbuf[256];
	unsigned flags = FLAG_RM;
	int plen = prefix(tmpbuf, 0, u, flags, 0);
	tmpbuf[plen++] = '\n';
	uint64_t pos = st->eodpos[st->idx-1];

	if (!store_write(st, tmpbuf, plen, pos))
		return 0;

	st->eodpos[st->idx-1] += plen;
	return 1;
}

int store_get(const store st, const uuid u, void** buf, size_t* len)
{
	if (!st || !u || !buf || !len)
	{
		printf("store_get failed invalid args\n");
		return 0;
	}

	unsigned long long v;

	if (st->transactions)
		lock_lock(st->lk);

	int ok = tree_get(st->tptr, u, &v);

	if (st->transactions)
		lock_unlock(st->lk);

	if (!ok)
		return 0;

	int idx = FILEIDX(v);
	int fd = st->fd[idx];
	uint64_t pos = POS(v);
	char tmpbuf[1024];

	if (pread(fd, tmpbuf, sizeof(tmpbuf)-1, pos) <= 0)
	{
		printf("store_get pread fd=%d prefix failed, pos=%llu\n", fd, (unsigned long long)pos);
		return 0;
	}

	unsigned nbr, flags, nbytes;
	uuid_t tmp_u;
	int skip = parse(tmpbuf, &nbr, &tmp_u, &flags, &nbytes);

	if (uuid_compare(u, &tmp_u) != 0)	// CHECK uuid_t match
	{
		char tmpbuf1[256], tmpbuf2[256];
		printf("store_get failed uuid_t=%s !=%s failed\n", uuid_to_string(u, tmpbuf1), uuid_to_string(&tmp_u, tmpbuf2));
		return 0;
	}

	if (nbytes == 0)					// CHECK has length
	{
		printf("store_get failed invalid length\n");
		return 0;
	}

	if (*buf && (*len < (nbytes-1)))	// CHECK buffer big enough
	{
		free(*buf);
		*buf = 0;
		*len = 0;
	}

	if (!*buf)							// If not, allocate new one
	{
		*buf = (char*)malloc(*len=(nbytes+1));
		if (!*buf) return 0;
	}

	char* bufptr = (char*)*buf;

	// If wholely within tmpbuf then
	// we already have it all!

	if ((skip+nbytes) <= sizeof(tmpbuf))
	{
		memcpy(bufptr, tmpbuf+skip, nbytes);
		bufptr[nbytes] = 0;
		return nbytes;
	}

	if (pread(fd, bufptr, nbytes, pos+skip) != nbytes)
	{
		printf("store_get pread fd=%d data failed, pos=%llu\n", fd, (unsigned long long)pos+skip);
		return 0;
	}

	bufptr[nbytes] = 0;
	return nbytes;
}

hstore store_begin(store st, int dbsync)
{
	if (!st)
		return 0;

	hstore h = (hstore)calloc(1, sizeof(struct _hstore));

	if (!h)
		return 0;

	h->nbr = atomic_inc(&st->current);
	atomic_inc(&st->transactions);
	h->st = st;
	h->wait_for_write = 1;
	h->dbsync = dbsync;
	return h;
}

int store_hget(hstore h, const uuid u, void** buf, size_t* len)
{
	if (!h)
		return 0;

	return store_get(h->st, u, buf, len);
}

int store_hadd(hstore h, const uuid u, const void* buf, size_t len)
{
	if (!h || !u || !buf || !len)
		return 0;

	char tmpbuf[256];
	char* dst = tmpbuf;

	if (h->wait_for_write)
		dst += sprintf(dst, "%c %X\n", TR_BEGIN, h->nbr);

	unsigned flags = 0;
	int plen = dst - tmpbuf;
	plen += prefix(dst, h->nbr, u, flags, len);
	uint64_t pos = atomic_addu64(&h->st->eodpos[h->st->idx-1], plen+len);

	if (h->wait_for_write)
	{
		h->wait_for_write = 0;
		h->start_pos = pos;
	}

	int ok = store_write2(h->st, tmpbuf, plen, buf, len, pos);
	return ok;
}

int store_hrem2(hstore h, const uuid u, const void* buf, size_t len)
{
	if (!h || !u)
		return 0;

	char tmpbuf[256];
	char* dst = tmpbuf;

	if (h->wait_for_write)
		dst += sprintf(dst, "%c %X\n", TR_BEGIN, h->nbr);

	unsigned flags = FLAG_RM;
	int plen = dst - tmpbuf;
	plen += prefix(dst, h->nbr, u, flags, 0);
	uint64_t pos = atomic_addu64(&h->st->eodpos[h->st->idx-1], plen+len);

	if (h->wait_for_write)
	{
		h->wait_for_write = 0;
		h->start_pos = pos;
	}

	int ok = store_write2(h->st, tmpbuf, plen, buf, len, pos);
	return ok;
}

int store_hrem(hstore h, const uuid u)
{
	if (!h || !u)
		return 0;

	char tmpbuf[256];
	char* dst = tmpbuf;

	if (h->wait_for_write)
		dst += sprintf(dst, "%c %X\n", TR_BEGIN, h->nbr);

	unsigned flags = FLAG_RM;
	int plen = dst - tmpbuf;
	plen += prefix(dst, h->nbr, u, flags, 0);
	tmpbuf[plen++] = '\n';
	uint64_t pos = atomic_addu64(&h->st->eodpos[h->st->idx-1], plen);

	if (h->wait_for_write)
	{
		h->wait_for_write = 0;
		h->start_pos = pos;
	}

	int ok = store_write(h->st, tmpbuf, plen, pos);
	return ok;
}

int store_cancel(hstore h)
{
	if (!h)
		return 0;

	if (!h->wait_for_write)
	{
		char tmpbuf[256];
		int len = sprintf(tmpbuf, "%c %X\n", TR_CANCEL, h->nbr);
		uint64_t pos = atomic_addu64(&h->st->eodpos[h->st->idx-1], len);
		int ok2 = store_write(h->st, tmpbuf, len, pos);

		if (!ok2)
		{
			free(h);
			return 0;
		}

		if (h->dbsync)
			fsync(h->st->fd[h->st->idx]);
	}

	atomic_dec_and_zero(&h->st->transactions, &h->st->current);
	free(h);
	return 1;
}

int store_end(hstore h)
{
	if (!h)
		return 0;

	if (!h->wait_for_write)
	{
		char tmpbuf[256];
		int len = sprintf(tmpbuf, "%c %X\n", TR_END, h->nbr);
		uint64_t pos = atomic_addu64(&h->st->eodpos[h->st->idx-1], len);
		int ok2 = store_write(h->st, tmpbuf, len, pos);

		if (!ok2)
		{
			free(h);
			return 0;
		}

		store_apply(h->st, h->nbr, h->start_pos);

		if (h->dbsync)
			fsync(h->st->fd[h->st->idx]);
	}

	atomic_dec_and_zero(&h->st->transactions, &h->st->current);
	free(h);
	return 1;
}

static void store_load_file(store st)
{
	uint64_t pos = 0, save_pos = 0, next_pos = 0;
	unsigned save_nbr = 0, cnt = 0;
	int fd = st->fd[st->idx-1];
	int valid = 1;

	for (;;)
	{
		char tmpbuf[1024];

		if (pread(fd, tmpbuf, sizeof(tmpbuf), pos) <= 0)
			break;

		if (tmpbuf[0] == TR_BEGIN)
		{
			unsigned nbr = 0;
			sscanf(tmpbuf, "%*s %X", &nbr);

			if (!save_nbr)
			{
				save_nbr = nbr;
				save_pos = pos;
				next_pos = pos;
			}
			else
				valid = 0;

			const char* src = tmpbuf;

			while (*src++ != '\n')
				pos++;
		}
		else if (tmpbuf[0] == TR_CANCEL)
		{
			unsigned nbr = 0;
			sscanf(tmpbuf, "%*s %X", &nbr);
			const char* src = tmpbuf;

			while (*src++ != '\n')
				pos++;

			if (nbr == save_nbr)		// drop on rollback
			{
				save_nbr = 0;
				pos = next_pos;
			}
			else
				valid = 0;
		}
		else if (tmpbuf[0] == TR_END)
		{
			unsigned nbr = 0;
			sscanf(tmpbuf, "%*s %X", &nbr);
			const char* src = tmpbuf;

			while (*src++ != '\n')
				pos++;

			if (nbr == save_nbr)		// apply on commit
			{
				cnt += store_apply(st, nbr, save_pos);
				save_nbr = 0;

				// If we didn't encounter any embedded or
				// interleaved transaction elements, then
				// we don't have to go back and restart...

				if (!valid)
					pos = next_pos;
			}
			else
				valid = 0;
		}
		else if (save_nbr != 0)
		{
			unsigned nbr, flags, nbytes;
			uuid_t u;
			int skip = parse(tmpbuf, &nbr, &u, &flags, &nbytes);

			if (nbr != save_nbr)
				valid = 0;

			pos += skip;
			pos += nbytes;
		}
		else
		{
			unsigned nbr, flags, nbytes;
			uuid_t u;
			int skip = parse(tmpbuf, &nbr, &u, &flags, &nbytes);

			if (!(flags & FLAG_RM))
			{
				int idx = st->idx-1;
				uint64_t fp = MAKE_FILEPOS(idx,pos);
				tree_add(st->tptr, &u, fp);
			}
			else
				tree_del(st->tptr, &u);

			if (st->f)
			{
				if (nbytes)
				{
					char* src = tmpbuf+skip;
					int big = 0;

					if ((src+nbytes) >= (tmpbuf+sizeof(tmpbuf)))
					{
						big = 1;
						src = (char*)malloc(nbytes+1);

						if (pread(fd, src, nbytes, pos+skip) <= 0)
							return;
					}

					src[nbytes] = 0;
					st->f(st->p1, &u, src, flags&FLAG_RM?-nbytes:nbytes);
					if (big) free(src);
				}
				else
					st->f(st->p1, &u, NULL, 0);
			}

			cnt++;
			pos += skip;
			pos += nbytes;
		}
	}

	st->eodpos[st->idx-1] = pos;
	printf("store_load_file: '%s' applied=%u, size=%llu MiB\n", st->filename[st->idx-1], cnt, (unsigned long long)pos/1024/1024);
}

static int store_merge_item(void* h, const uuid u, unsigned long long* v)
{
	store st = (store)h;
	int idx = FILEIDX(*v);

	if (idx == 0)
		return 0;

	int fd = st->fd[idx];
	uint64_t pos = POS(*v);
	char tmpbuf[1024];

	if (pread(fd, tmpbuf, sizeof(tmpbuf), pos) <= 0)
	{
		printf("store_merge_item pread fd=%d prefix failed, pos=%llu\n", fd, (unsigned long long)pos);
		return -1;
	}

	unsigned nbr, flags, nbytes;
	uuid_t tmp_u;
	int skip = parse(tmpbuf, &nbr, &tmp_u, &flags, &nbytes);
	tmpbuf[skip] = 0;

	if (uuid_compare(u, &tmp_u) != 0)	// CHECK uuid_t match
	{
		char tmpbuf1[256], tmpbuf2[256];
		printf("store_merge_item failed uuid_t=%s !=%s failed\n", uuid_to_string(u, tmpbuf1), uuid_to_string(&tmp_u, tmpbuf2));
		return -1;
	}

	if (nbytes == 0)					// CHECK has length
	{
		printf("store_merge_item failed invalid length\n");
		return -1;
	}

	int wlen, len = skip+nbytes;

	if (len <= sizeof(tmpbuf))
	{
		wlen = pwrite(st->fd[0], tmpbuf, len, st->eodpos[0]);
	}
	else
	{
		void* buf = malloc(nbytes);

		if (!buf)
			return -1;

		if (pread(fd, buf, nbytes, pos+skip) != nbytes)
		{
			printf("store_merge_item pread fd=%d data failed, pos=%llu\n", fd, (unsigned long long)(pos+skip));
			return -1;
		}

#ifdef _WIN32
		size_t len = skip + nbytes;
		char tmpbuf2[1024*2];

		if (len <= sizeof(tmpbuf2))
		{
			memcpy(tmpbuf2, tmpbuf, skip);
			memcpy(tmpbuf2+skip, buf, nbytes);
			wlen = (size_t)pwrite(st->fd[0], tmpbuf2, (size_t)len, st->eodpos[0]);
		}
		else
		{
			wlen = (size_t)pwrite(st->fd[0], tmpbuf, (size_t)skip, st->eodpos[0]);
			wlen += pwrite(st->fd[0], buf, nbytes, st->eodpos[0]+skip);
		}
#else
		struct iovec iov[2];
		iov[0].iov_base = (void*)tmpbuf;
		iov[0].iov_len = skip;
		iov[1].iov_base = (void*)buf;
		iov[1].iov_len = nbytes;
		wlen = pwritev(st->fd[0], iov, 2, st->eodpos[0]);
#endif
	}

	if (wlen != len)
	{
		printf("store_merge_item write fd=%d data failed, pos=%llu\n", fd, (unsigned long long)st->eodpos[0]);
		return -1;
	}

	*v = st->eodpos[0];
	st->eodpos[0] += len;
	return 1;
}

static int store_open_file(store st, const char* filename, int readonly, int create)
{
	if (!st)
		return 0;

	int fd = st->fd[st->idx] = open(filename, (create?O_CREAT:0)|(readonly?O_RDONLY:O_RDWR), 0666);

	if (fd < 0)
		return 0;

	strcpy(st->filename[st->idx], filename);
	st->idx++;
	return 1;
}

static int store_merge(store st)
{
	if (!st)
		return 0;

	close(st->fd[0]);
	st->fd[0] = open(st->filename[0], O_RDWR, 0666);

	if (st->fd[0] < 0)
		return 0;

	printf("store_merge: begin\n");
	size_t cnt = tree_iter(st->tptr, st, &store_merge_item);
	printf("store_merge: end, %ld items\n", (long)cnt);
	fsync(st->fd[0]);
	close(st->fd[0]);

	while (st->idx-- > 1)
	{
		close(st->fd[st->idx]);
		remove(st->filename[st->idx]);
		st->eodpos[st->idx] = 0;
	}

	st->idx = 0;
	string filename;
	sprintf(filename, "%s/%s", st->path1, ZEROTH_LOG);
	store_open_file(st, filename, 1, 1);
	return 1;
}


static int store_open_handler(void* p1, const char* name)
{
	if (!strcmp(name, ZEROTH_LOG))
		return 1;

	store st = (store)p1;
	string filename;
	sprintf(filename, "%s/%s", st->path2, name);

	if (!store_open_file(st, filename, 1, 0))
		return 1;

	store_load_file(st);
	return 1;
}

store store_open2(const char* path1, const char* path2, int compact, void (*f)(void*,const uuid,const void*,int), void* p1)
{
	store st = (store)calloc(1, sizeof(struct _store));
	if (!st || !path1) return NULL;
	if (!path2) path2 = path1;
	strcpy(st->path1, path1);
	strcpy(st->path2, path2);
	st->f = f;
	st->p1 = p1;
	st->tptr = tree_create();
	st->lk = lock_create();

	if ((mkdir(st->path1, 0777) < 0) && (errno != EEXIST))
	{
		printf("store_open: mkdir '%s' error: %s\n", st->path1, strerror(errno));
		store_close(st);
		return NULL;
	}

	string filename, filename2;
	sprintf(filename, "%s/%s", st->path1, ZEROTH_LOG);
	sprintf(filename2, "%s/%s", st->path1, FIRST_LOG);
	struct stat s = {0};
	stat(filename, &s);
	int do_merge = 0;

	if (compact && (s.st_size >= (MAX_LOGFILE_SIZE*32)))
	{
		printf("store_open: compaction scheduled\n");
		rename(filename, filename2);
		do_merge++;
	}

	if (store_open_file(st, filename, 1, 1))
	{
		printf("store_open_file: '%s'\n", filename);
		store_load_file(st);
	}

	if (store_open_file(st, filename2, 1, 0))
		store_load_file(st);

	if (strcmp(st->path1, st->path2))
	{
		if ((mkdir(st->path2, 0777) < 0) && (errno != EEXIST))
		{
			printf("store_open: mkdir '%s' error: %s\n", st->path2, strerror(errno));
			store_close(st);
			return NULL;
		}
	}

	// Open all timestamped log files.

	dirlist(st->path2, ".log", &store_open_handler, st);

	if (st->idx == (MAX_LOGFILES-1))
		do_merge++;

	if (do_merge)
		store_merge(st);

	// Create an active log.

	sprintf(filename, "%s/%lld.log", st->path2, (long long)time(NULL));
	store_open_file(st, filename, 0, 1);
	printf("store_open_file: '%s'\n", filename);
	return st;
}

static int store_logreader_apply(store st, int n, uint64_t pos, int (*f)(void*,const uuid,const void*,int), void* p1)
{
	int idx = FILEIDX(pos);
	int fd = st->fd[idx];

	for (;;)
	{
		char tmpbuf[1024];

		if (pread(fd, tmpbuf, sizeof(tmpbuf), pos) <= 0)
			return 0;

		if (tmpbuf[0] == TR_BEGIN)
		{
			unsigned nbr = 0;
			sscanf(tmpbuf, "%*s %X", &nbr);
			const char* src = tmpbuf;

			while (*src++ != '\n')
				pos++;
		}
		else if (tmpbuf[0] == TR_CANCEL)
		{
			unsigned nbr = 0;
			sscanf(tmpbuf, "%*s %X", &nbr);

			if (nbr == n)
				break;

			const char* src = tmpbuf;

			while (*src++ != '\n')
				pos++;
		}
		else if (tmpbuf[0] == TR_END)
		{
			unsigned nbr = 0;
			sscanf(tmpbuf, "%*s %X", &nbr);

			if (nbr == n)
				break;

			const char* src = tmpbuf;

			while (*src++ != '\n')
				pos++;
		}
		else
		{
			unsigned nbr, flags, nbytes;
			uuid_t u;

			int skip = parse(tmpbuf, &nbr, &u, &flags, &nbytes);

			if (nbr == n)
			{
				if (nbytes)
				{
					char* src = tmpbuf+skip;
					int big = 0;

					if ((src+nbytes) >= (tmpbuf+sizeof(tmpbuf)))
					{
						big = 1;
						src = (char*)malloc(nbytes+1);

						if (pread(fd, src, nbytes, pos+skip) <= 0)
							return 0;

						src[nbytes] = 0;
					}

					int ok = f(p1, &u, src, flags&FLAG_RM?-nbytes:nbytes);
					if (big) free(src);
					if (!ok) return 0;
				}
				else
					f(p1, &u, NULL, 0);
			}

			pos += skip;
			pos += nbytes;
		}
	}

	return 1;
}

int store_tail(store st, const uuid u, int (*f)(void*,const uuid,const void*,int), void* p1)
{
	if (!st || !u)
		return 0;

	unsigned long long v = 0;

	// A zero UUID is a request for the begining.

	if (!uuid_is_zero(u))
	{
		if (st->transactions)
			lock_lock(st->lk);

		int ok = tree_get(st->tptr, u, &v);

		if (st->transactions)
			lock_unlock(st->lk);

		if (!ok)
			return 0;
	}

	uint64_t pos = POS(v);
	int idx = FILEIDX(pos);
	uint64_t save_pos = 0, next_pos = 0;
	unsigned save_nbr = 0, cnt = 0;
	int valid = 1;

	for (;;)
	{
		int fd = st->fd[idx];
		char tmpbuf[1024];

		if (pread(fd, tmpbuf, sizeof(tmpbuf), pos) <= 0)
		{
			if (++idx < st->idx)
			{
				idx++;			// move on to the
				pos = 0;		// next log file
				valid = 1;
				save_nbr = 0;
			}
			else
			{
				--idx;			// just tail the active log
				msleep(1);		// TO-DO: use a wakeup
			}

			continue;
		}

		if (tmpbuf[0] == TR_BEGIN)
		{
			unsigned nbr = 0;
			sscanf(tmpbuf, "%*s %X", &nbr);

			if (!save_nbr)
			{
				save_nbr = nbr;
				save_pos = pos;
				next_pos = pos;
			}
			else
				valid = 0;

			const char* src = tmpbuf;

			while (*src++ != '\n')
				pos++;
		}
		else if (tmpbuf[0] == TR_CANCEL)
		{
			unsigned nbr = 0;
			sscanf(tmpbuf, "%*s %X", &nbr);
			const char* src = tmpbuf;

			while (*src++ != '\n')
				pos++;

			if (nbr == save_nbr)		// drop on rollback
			{
				save_nbr = 0;
				pos = next_pos;
			}
			else
				valid = 0;
		}
		else if (tmpbuf[0] == TR_END)
		{
			unsigned nbr = 0;
			sscanf(tmpbuf, "%*s %X", &nbr);
			const char* src = tmpbuf;

			while (*src++ != '\n')
				pos++;

			if (nbr == save_nbr)		// apply on commit
			{
				if (!store_logreader_apply(st, nbr, save_pos, f, p1))
					break;

				save_nbr = 0;

				// If we didn't encounter any embedded or
				// interleaved transaction elements, then
				// we don't have to go back and restart...

				if (!valid)
					pos = next_pos;
			}
			else
				valid = 0;
		}
		else if (save_nbr != 0)
		{
			unsigned nbr, flags, nbytes;
			uuid_t u;
			int skip = parse(tmpbuf, &nbr, &u, &flags, &nbytes);

			if (nbr != save_nbr)
				valid = 0;

			pos += skip;
			pos += nbytes;
		}
		else
		{
			unsigned nbr, flags, nbytes;
			uuid_t u;
			int skip = parse(tmpbuf, &nbr, &u, &flags, &nbytes);

			if (nbytes)
			{
				char* src = tmpbuf+skip;
				int big = 0;

				if ((src+nbytes) >= (tmpbuf+sizeof(tmpbuf)))
				{
					big = 1;
					src = (char*)malloc(nbytes+1);

					if (pread(fd, src, nbytes, pos+skip) <= 0)
						return 0;
				}

				src[nbytes] = 0;
				int ok = f(p1, &u, src, flags&FLAG_RM?-nbytes:nbytes);
				if (big) free(src);
				if (!ok) break;
			}
			else
			{
				if (!f(p1, &u, NULL, 0))
					break;
			}

			cnt++;
			pos += skip;
			pos += nbytes;
		}
	}

	return cnt;
}

void store_done(hreader r)
{
	if (!r)
		return;

	free(r);
}

store store_open(const char* path1, const char* path2, int compact)
{
	return store_open2(path1, path2, compact, NULL, NULL);
}

int store_close(store st)
{
	if (!st)
		return 0;

	while (st->idx-- > 0)
	{
		if (st->fd[st->idx] >= 0)
		{
			fsync(st->fd[st->idx]);
			close(st->fd[st->idx]);
		}
	}

	tree_destroy(st->tptr);
	lock_destroy(st->lk);
	free(st);
	return 1;
}

