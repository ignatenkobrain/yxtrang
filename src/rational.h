#ifndef RATIONAL_H
#define RATIONAL_H

typedef struct { long long n; long long d; } rational;

extern void r_reduce(rational *r);
extern void r_float(rational *r, double v);

inline static void r_int(rational *r, long long v) { r->n=v; r->d=1; }
inline static void r_rat(rational *r, rational * v) { r->n=v->n; r->d=v->d; }
inline static void r_rat2(rational *r, long long n, long long d) { r->n=n; r->d=d; }
#define r_rat3(r,w,n,d) r_rat2(r, (w*d)+n, d)

inline static long long r_get_int(rational *r) { return r->n/r->d; }
inline static double r_get_float(rational *r) { return ((double)r->n)/r->d; }

inline static void r_add(rational *r, rational *v) { r->n=(r->n*v->d)+(v->n*r->d); r->d=v->d*r->d; }
inline static void r_sub(rational *r, rational *v) { r->n=(r->n*v->d)-(v->n*r->d); r->d=v->d*r->d; }
inline static void r_mul(rational *r, rational *v) { r->n*=v->n; r->d*=v->d; }
inline static void r_div(rational *r, rational *v) { r->n*=v->d; r->d*=v->n; }

inline static void r_addi(rational *r, long long v) { r->n=r->n+(v*r->d); r->d=r->d; }
inline static void r_subi(rational *r, long long v) { r->n=r->n-(v*r->d); r->d=r->d; }
inline static void r_muli(rational *r, long long v) { r->n*=v; }
inline static void r_divi(rational *r, long long v) { r->d*=v; }

#define r_eq(r,v) ( ((r)->n*(v)->d)==((v)->n*(r)->d) )
#define r_ne(r,v) ( ((r)->n*(v)->d)!=((v)->n*(r)->d) )
#define r_gt(r,v) ( ((r)->n*(v)->d)>((v)->n*(r)->d) )
#define r_ge(r,v) ( ((r)->n*(v)->d)>=((v)->n*(r)->d) )
#define r_lt(r,v) ( ((r)->n*(v)->d)<((v)->n*(r)->d) )
#define r_le(r,v) ( ((r)->n*(v)->d)<=((v)->n*(r)->d) )

#define r_eqi(r,v) ( (r)->n==((long long)v*(r)->d) )
#define r_nei(r,v) ( (r)->n!=((long long)v*(r)->d) )
#define r_gti(r,v) ( (r)->n>((long long)v*(r)->d) )
#define r_gei(r,v) ( (r)->n>=((long long)v*(r)->d) )
#define r_lti(r,v) ( (r)->n<((long long)v*(r)->d) )
#define r_lei(r,v) ( (r)->n<=((long long)v*(r)->d) )

#endif
