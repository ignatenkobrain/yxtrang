#ifndef RATIONAL_H
#define RATIONAL_H

typedef struct
{
	long long n, d;
}
 rational;

inline void r_copy(rational *r, const rational *v) { r->n = v->n; r->d = v->d; }
inline void r_set_int(rational *r, long long v) { r->n = v; r->d = 1; };
inline void r_set_rat(rational *r, long long w, long long n, long long d) { r->n = (w*d)+n; r->d = d; };

inline long long r_get_int(const rational *r) { return r->n / r->d; }
inline double r_get_real(const rational *r) { return (double)r->n / (double)r->d; }

// Assume unreduced form...

inline int r_eq(const rational *r, const rational *v) { return (r->n * v->d) == (v->n * r->d); }
inline int r_neq(const rational *r, const rational *v) { return (r->n * v->d) != (v->n * r->d); }
inline int r_gt(const rational *r, const rational *v) { return (r->n * v->d) > (v->n * r->d); }
inline int r_gte(const rational *r, const rational *v) { return (r->n * v->d) >= (v->n * r->d); }
inline int r_lt(const rational *r, const rational *v) { return (r->n * v->d) < (v->n * r->d); }
inline int r_lte(const rational *r, const rational *v) { return (r->n * v->d) <= (v->n * r->d); }

extern void r_add(rational *r, const rational *v);
extern void r_sub(rational *r, const rational *v);
extern void r_mul(rational *r, const rational *v);
extern void r_div(rational *r, const rational *v);

// After a sequence of operations reduce to
// lowest common denominator...

extern void reduce(rational *r);

#endif
