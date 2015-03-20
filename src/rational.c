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

	r->n /= num;
	r->d /= num;

	if ((r->d) < 0)
	{
		r->n *= -1;
		r->d *= -1;
	}
}

const char *r_tostring(char *tmpbuf, rational *r)
{
	r_reduce(r);

	if (r->d == 1)
		sprintf(tmpbuf, "%lld", r->n);
	else
		sprintf(tmpbuf, "%lld/%lld", r->n, r->d);

	return tmpbuf;
}

