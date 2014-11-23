#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jsonq.h"

const char* json_quick(const char* s, const char* name)
{
	return NULL;
}

uint64_t json_quick_int(const char* s, const char* name)
{
	return 0;
}

double json_quick_real(const char* s, const char* name)
{
	return 0.0;
}

int json_quick_bool(const char* s, const char* name)
{
	return 0;
}

int json_quick_null(const char* s, const char* name)
{
	return 0;
}

extern const char* json_quick_array(const char* s, int n)
{
	return NULL;
}

extern uint64_t json_quick_array_int(const char* s, int n)
{
	return 0;
}
extern double json_quick_array_real(const char* s, int n)
{
	return 0.0;
}
extern int json_quick_array_bool(const char* s, int n)
{
	return 0;
}
extern int json_quick_array_null(const char* s, int n)
{
	return 0;
}


