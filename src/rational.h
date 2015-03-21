#ifndef RATIONAL_H
#define RATIONAL_H

typedef struct { long long n; long long d; } rational;

#define r_copy(r,v) { (r)->n=(v)->n; (r)->d=(v)->d; }
#define r_int(r,v) { (r)->n=(long long)v; (r)->d=1; }
#define r_rat3(r,wholes,num,den) { (r)->n=(long long)(wholes*den)+num; (r)->d=(long long)den; }
#define r_rat2(r,num,den) { (r)->n=(long long)num; (r)->d=(long long)den; }
extern void r_float(rational *r, double v);

#define r_get_float(r) (r_reduce(r), (double)(r)->n/(double)(r)->d)

#define r_add(r,v) { (r)->n=((r)->n*(v)->d)+((v)->n*(r)->d); (r)->d=(v)->d*(r)->d; }
#define r_sub(r,v) { (r)->n=((r)->n*(v)->d)-((v)->n*(r)->d); (r)->d=(v)->d*(r)->d; }
#define r_mul(r,v) { (r)->n*=(v)->n; (r)->d*=(v)->d; }
#define r_div(r,v) { (r)->n*=(v)->d; (r)->d*=(v)->n; }

#define r_addi(r,v) { (r)->n=(r)->n+((long long)v*(r)->d); (r)->d=(r)->d; }
#define r_subi(r,v) { (r)->n=(r)->n-((long long)v*(r)->d); (r)->d=(r)->d; }
#define r_muli(r,v) { (r)->n*=(long long)v; }
#define r_divi(r,v) { (r)->d*=(long long)v; }

#define r_eq(r,v) (((r)->n*(v)->d)==((v)->n*(r)->d))
#define r_ne(r,v) (((r)->n*(v)->d)!=((v)->n*(r)->d))
#define r_gt(r,v) (((r)->n*(v)->d)>((v)->n*(r)->d))
#define r_ge(r,v) (((r)->n*(v)->d)>=((v)->n*(r)->d))
#define r_lt(r,v) (((r)->n*(v)->d)<((v)->n*(r)->d))
#define r_le(r,v) (((r)->n*(v)->d)<=((v)->n*(r)->d))

#define r_eqi(r,v) ((r)->n==((long long)v*(r)->d))
#define r_nei(r,v) ((r)->n!=((long long)v*(r)->d))
#define r_gti(r,v) ((r)->n>((long long)v*(r)->d))
#define r_gei(r,v) ((r)->n>=((long long)v*(r)->d))
#define r_lti(r,v) ((r)->n<((long long)v*(r)->d))
#define r_lei(r,v) ((r)->n<=((long long)v*(r)->d))

extern void r_reduce(rational *r);

#endif
