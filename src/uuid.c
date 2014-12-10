#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/time.h>
#endif

#include "uuid.h"

#define MASK_48 0x0000FFFFFFFFFFFF
#define BITS_48 48

static uint64_t s_seed = 0;

uuid_t uuid_set(uint64_t v1, uint64_t v2)
{
	uuid_t tmp = {0};
	tmp.u1 = v1;
	tmp.u2 = v2;
	return tmp;
}

uint64_t uuid_ts(const uuid_t* u)
{
	return u->u1;
}

const char* uuid_to_string(const uuid_t* u, char* buf)
{
	sprintf(buf, "%016llX:%04llX:%012llX",
		(unsigned long long)u->u1, (unsigned long long)(u->u2 >> BITS_48),
		(unsigned long long)(u->u2 & MASK_48));

	return buf;
}

const uuid_t* uuid_from_string(const char* s, uuid_t* u)
{
	if (!s) return u;
	unsigned long long p1 = 0, p2 = 0, p3 = 0;
	sscanf(s, "%llX:%llX:%llX", &p1, &p2, &p3);
	u->u1 = p1;
	u->u2 = p2 << BITS_48;
	u->u2 |= p3 & MASK_48;
	return u;
}

void uuid_seed(uint64_t v)
{
	s_seed = v & MASK_48;
}

#ifdef _WIN32
static int gettimeofday(struct timeval* tv, struct timezone*)
{
	static const uint64_t epoch = 116444736000000000ULL;

    FILETIME    file_time;
    SYSTEMTIME  system_time;
    ULARGE_INTEGER ularge;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    ularge.LowPart = file_time.dwLowDateTime;
    ularge.HighPart = file_time.dwHighDateTime;

    tv->tv_sec = (time_t) ((ularge.QuadPart - epoch) / 10000000L);
    tv->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}
#endif

const uuid_t* uuid_gen(uuid_t* u)
{
	static uint64_t s_last = 0;
	static uint64_t s_cnt = 0;

	if (!s_seed)
		uuid_seed(time(0));

	struct timeval tp;
	gettimeofday(&tp, 0);
	uint64_t now = tp.tv_sec;
	now *= 1000L * 1000L;
	now += tp.tv_usec;

	if (now != s_last)
	{
		s_last = now;
		s_cnt = 1;
	}

	u->u1 = now;
	u->u2 = s_cnt++ << BITS_48;
	u->u2 |= s_seed;
	return u;
}

uuid_t* uuid_copy(const uuid_t* v1)
{
	uuid_t* v2 = (uuid_t*)malloc(sizeof(struct _uuid));
	v2->u1 = v1->u1;
	v2->u2 = v1->u2;
	return v2;
}

int uuid_compare(const uuid_t* v1, const uuid_t* v2)
{
	if (v1->u1 < v2->u1)
		return -1;
	else if (v1->u1 == v2->u1)
	{
		if (v1->u2 < v2->u2)
			return -1;
		else if (v1->u2 == v2->u2)
			return 0;
	}

	return 1;
}
