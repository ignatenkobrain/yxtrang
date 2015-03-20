#include <stdio.h>

#include "rational.h"

static long long gcd(long long num, long long remainder)
{
	if (remainder == 0)
		return num;

	return gcd(remainder, num%remainder);
}

void r_reduce(rational *r)
{
	long long num = 0;

	if ((r->d) > (r->n))
		num = gcd(r->d, r->n);
	else if ((r->d) < (r->n))
		num = gcd(r->n, r->d);
	else
		num = gcd(r->n, r->d);

	if (num < 0)
		num = -num;

	r->n /= num;
	r->d /= num;
}

void r_float(rational *r, double v)
{
	r->n = (long long)(v * 1000000000000000000LL);
	r->d = 1000000000000000000ULL;
	r_reduce(r);
}

const char *r_tostring(rational *r, char *tmpbuf)
{
	r_reduce(r);

	if (r->d == 1)
		sprintf(tmpbuf, "%lld", r->n);
	else
		sprintf(tmpbuf, "%lld rdiv %llu", r->n, r->d);

	return tmpbuf;
}

