#ifndef RATIONAL_H
#define RATIONAL_H

typedef struct { long long n, d; } rational;

#define r_copy(r,v) { (r)->n=(v)->n; (r)->d=(v)->d; }
#define r_int(r,v) { (r)->n=v; (r)->d=1; }
#define r_rat(r,wholes,num,den) { (r)->n=(wholes*den)+num; (r)->d=den; }

#define r_get_int(r) (r_reduce(r), (r)->n/(r)->d)
#define r_get_float(r) (r_reduce(r), (double)(r)->n/(double)(r)->d)

#define r_add(r,v) { (r)->n=((r)->n*(v)->d)+((v)->n*(r)->d); (r)->d=(v)->d*(r)->d; }
#define r_sub(r,v) { (r)->n=((r)->n*(v)->d)-((v)->n*(r)->d); (r)->d=(v)->d*(r)->d; }
#define r_mul(r,v) { (r)->n*=(v)->n; (r)->d*=(v)->d; }
#define r_div(r,v) { (r)->n*=(v)->d; (r)->d*=(v)->n; }

#define r_addi(r,i) { (r)->n=(r)->n+((long long)i*(r)->d); (r)->d=(r)->d; }
#define r_subi(r,i) { (r)->n=(r)->n-((long long)i*(r)->d); (r)->d=(r)->d; }
#define r_muli(r,i) { (r)->n*=(long long)i; }
#define r_divi(r,i) { (r)->d*=(long long)i; }

#define r_eq(r,v) (((r)->n*(v)->d)==((v)->n*(r)->d))
#define r_ne(r,v) (((r)->n*(v)->d)!=((v)->n*(r)->d))
#define r_gt(r,v) (((r)->n*(v)->d)>((v)->n*(r)->d))
#define r_ge(r,v) (((r)->n*(v)->d)>=((v)->n*(r)->d))
#define r_lt(r,v) (((r)->n*(v)->d)<((v)->n*(r)->d))
#define r_le(r,v) (((r)->n*(v)->d)<=((v)->n*(r)->d))

#define r_eqi(r,i) ((r)->n==((long long)i*(r)->d))
#define r_nei(r,i) ((r)->n!=((long long)i*(r)->d))
#define r_gti(r,i) ((r)->n>((long long)i*(r)->d))
#define r_gei(r,i) ((r)->n>=((long long)i*(r)->d))
#define r_lti(r,i) ((r)->n<((long long)i*(r)->d))
#define r_lei(r,i) ((r)->n<=((long long)i*(r)->d))

extern const char *r_tostring(char *tmpbuf, rational *r);
extern void r_reduce(rational *r);

#endif
