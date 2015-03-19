#include <string.h>
#include <stdio.h>

#include "rational.h"

static long long gcd(long long num, long long remainder)
{
	if (remainder == 0)
		return num;

	return gcd(remainder, num%remainder);
}

void reduce(rational *r)
{
	long long num = 0;

	if (r->d > r->n)
		num = gcd(r->d, r->n);
	else if (r->d < r->n)
		num = gcd(r->n, r->d);
	else
		num = gcd(r->n, r->d);

	r->n /= num;
	r->d /= num;

	if (r->d < 0)
	{
		r->n *= -1;
		r->d *= -1;
	}
}

void r_add(rational *r, const rational* v)
{
	r->n = (r->n * v->d) + (v->n * r->d);
	r->d = v->d * r->d;
}

void r_sub(rational *r, const rational* v)
{
	r->n = (r->n * v->d) - (v->n * r->d);
	r->d = v->d * r->d;
}

void r_mul(rational *r, const rational* v)
{
	r->n *= v->n;
	r->d *= v->d;
}

void r_div(rational *r, const rational* v)
{
	r->n *= v->d;
	r->d *= v->n;
}

void r_addi(rational *r, long long v)
{
	r->n = r->n + (v * r->d);
	r->d = r->d;
}

void r_subi(rational *r, long long v)
{
	r->n = r->n - (v * r->d);
	r->d = r->d;
}

void r_muli(rational *r, long long v)
{
	r->n *= v;
}

void r_divi(rational *r, long long v)
{
	r->d *= v;
}

int r_eq(const rational *r, const rational *v) { return (r->n * v->d) == (v->n * r->d); }
int r_neq(const rational *r, const rational *v) { return (r->n * v->d) != (v->n * r->d); }
int r_gt(const rational *r, const rational *v) { return (r->n * v->d) > (v->n * r->d); }
int r_gte(const rational *r, const rational *v) { return (r->n * v->d) >= (v->n * r->d); }
int r_lt(const rational *r, const rational *v) { return (r->n * v->d) < (v->n * r->d); }
int r_lte(const rational *r, const rational *v) { return (r->n * v->d) <= (v->n * r->d); }
