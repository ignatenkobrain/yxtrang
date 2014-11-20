#ifndef UUID_H
#define UUID_H

#include <stdint.h>

typedef struct _uuid { uint64_t u1, u2; } uuid;

extern const char* uuid_to_string(const uuid* u, char* buf);
extern const uuid* uuid_from_string(const char*, uuid* u);
extern uint64_t uuid_ts(const uuid* u);
extern void uuid_seed(uint64_t v);		// unique 48-bits eg. MAC-address or rand
extern const uuid* uuid_gen(uuid* u);

extern uuid uuid_set(uint64_t, uint64_t);		// used for testing only

static inline int uuid_compare(const uuid* v1, const uuid* v2)
{
	if (v1->u1 < v2->u1)
		return -1;
	else if (v1->u1 == v2->u1)
	{
		if (v1->u2 < v2->u2)
			return -1;
		else if (v1->u2 == v2->u2)
			return 0;
	}

	return 1;
}

#endif
