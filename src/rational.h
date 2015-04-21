#ifndef RATIONAL_H
#define RATIONAL_H

typedef struct { long long n, d; } rational;

extern void r_reduce(rational *r);
extern void r_float(rational *r, double v);

inline static void r_int(rational *r, long long v) { r->n=v; r->d=1; }
inline static void r_rat(rational *r, rational *v) { r->n=v->n; r->d=v->d; }
inline static void r_rat2(rational *r, long long n, long long d) { r->n=n; r->d=d; }
#define r_rat3(r,w,n,d) r_rat2(r, (w*d)+n, d)

inline static long long r_get_int(rational *r) { return r->n/r->d; }
inline static double r_get_float(rational *r) { return ((double)r->n)/r->d; }

inline static void r_add(rational *r, rational *v) { r->n=(r->n*v->d)+(v->n*r->d); r->d=v->d*r->d; }
inline static void r_sub(rational *r, rational *v) { r->n=(r->n*v->d)-(v->n*r->d); r->d=v->d*r->d; }
inline static void r_mul(rational *r, rational *v) { r->n*=v->n; r->d*=v->d; }
inline static void r_div(rational *r, rational *v) { r->n*=v->d; r->d*=v->n; }

inline static void r_addi(rational *r, long long v) { r->n=r->n+(v*r->d); }
inline static void r_subi(rational *r, long long v) { r->n=r->n-(v*r->d); }
inline static void r_muli(rational *r, long long v) { r->n*=v; }
inline static void r_divi(rational *r, long long v) { r->d*=v; }

inline static int r_eq(rational *r, rational *v) { return (r->n*v->d) == (r->d*v->n); }
inline static int r_ne(rational *r, rational *v) { return (r->n*v->d) != (r->d*v->n); }
inline static int r_gt(rational *r, rational *v) { return (r->n*v->d) > (r->d*v->n); }
inline static int r_ge(rational *r, rational *v) { return (r->n*v->d) >= (r->d*v->n); }
inline static int r_lt(rational *r, rational *v) { return (r->n*v->d) < (r->d*v->n); }
inline static int r_le(rational *r, rational *v) { return (r->n*v->d) <= (r->d*v->n); }

inline static int r_eqi(rational *r, long long v) { return r->n == (r->d*v); }
inline static int r_nei(rational *r, long long v) { return r->n != (r->d*v); }
inline static int r_gti(rational *r, long long v) { return r->n > (r->d*v); }
inline static int r_gei(rational *r, long long v) { return r->n >= (r->d*v); }
inline static int r_lti(rational *r, long long v) { return r->n < (r->d*v); }
inline static int r_lei(rational *r, long long v) { return r->n <= (r->d*v); }

#endif
