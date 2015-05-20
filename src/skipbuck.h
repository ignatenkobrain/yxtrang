#ifndef SKIPBUCK_H
#define SKIPBUCK_H

#include <string.h>

typedef struct skipbuck_ skipbuck;

extern skipbuck *sb_create(int (*compare)(const void*, const void*), void *(*copykey)(const void*), void (*freekey)(void*));
extern skipbuck *sb_create2(int (*compare)(const void*, const void*), void *(*copykey)(const void*), void (*freekey)(void*), void *(*copyval)(const void*), void (*freeval)(void*));

extern int sb_add(skipbuck *s, const void *key, const void *value);
extern int sb_get(const skipbuck *s, const void *key, const void **value);
extern int sb_rem(skipbuck *s, const void *key);

extern int sb_erase(skipbuck *s, const void *key, const void *value, int (*compare)(const void*,const void*));
extern int sb_efface(skipbuck *s, const void *value, int (*compare)(const void*,const void*));
extern void sb_iter(const skipbuck *s, int (*)(void*,void*,void*), void *p1);
extern void sb_find(const skipbuck *s, const void *key, int (*)(void*,void*,void*), void *p1);
extern unsigned long sb_count(const skipbuck *s);

extern void sb_destroy(skipbuck *s);


#endif
