#ifndef RATIONAL_H
#define RATIONAL_H

typedef struct
{
	long long n, d;
}
 rational;

inline void r_copy(rational *r, const rational *v) { r->n = v->n; r->d = v->d; }
inline void r_int(rational *r, long long v) { r->n = v; r->d = 1; };

// Useful for expressing currency, eg. $3.21 = r_rat(3,21,100)

inline void r_rat(rational *r, long long w, long long n, long long d) { r->n = (w*d)+n; r->d = d; };

inline long long r_get_int(const rational *r) { return r->n / r->d; }
inline double r_get_real(const rational *r) { return (double)r->n / (double)r->d; }

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

extern void reduce(rational *r);

#endif
