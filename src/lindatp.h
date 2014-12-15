#ifndef LINDATP_H
#define LINDATP_H

#include "network.h"

typedef struct _lindatp* lindatp;

extern lindatp lindatp_create(const char* path1, const char* path2);
extern int lindatp_request(session s, void* param);
extern void lindatp_destroy(lindatp l);

#endif
