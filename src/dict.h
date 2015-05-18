#ifndef DICT_H
#define DICT_H

typedef struct slnode_ slnode;
typedef struct dict_ { slnode *header, *p; int dups, level; } dict;

extern void dict_init(dict *d, int dups);
extern int dict_set(dict *d, const char *key, void *value);
extern void *dict_rem(dict *d, const char *key);
extern void *dict_get(dict *d, const char *key);

extern void dict_start(dict *d);
extern void *dict_iter(dict *d);

extern void dict_done(dict *d, int delkey, void (*delval)(void *value));

#endif
