#ifndef RATIONAL_H
#define RATIONAL_H

typedef struct
{
	long long n, d;
}
 rational;

#define r_copy(r,v) { (r)->n = (v)->n; (r)->d = (v)->d; }
#define r_int(r,v) { (r)->n = v; (r)->d = 1; }

// Useful for expressing currency, eg. $3.21 = r_rat(3,21,100)
#define r_rat(r,w,num,den) { (r)->n = (w*den)+num; (r)->d = den; }

#define r_get_int(r) (r_reduce(r), (r)->n / (r)->d)
#define r_get_real(r) (r_reduce(r), (double)(r)->n / (double)(r)->d)

extern int r_eq(const rational *r, const rational *v);
extern int r_neq(const rational *r, const rational *v);
extern int r_gt(const rational *r, const rational *v);
extern int r_gte(const rational *r, const rational *v);
extern int r_lt(const rational *r, const rational *v);
extern int r_lte(const rational *r, const rational *v);

extern void r_add(rational *r, const rational *v);
extern void r_sub(rational *r, const rational *v);
extern void r_mul(rational *r, const rational *v);
extern void r_div(rational *r, const rational *v);

extern void r_addi(rational *r, long long v);
extern void r_subi(rational *r, long long v);
extern void r_muli(rational *r, long long v);
extern void r_divi(rational *r, long long v);

extern void r_reduce(rational *r);

#endif
