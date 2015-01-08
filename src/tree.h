#ifndef TREE_H
#define TREE_H

#include "uuid.h"

typedef struct tree_ *tree;

extern tree tree_create(void);

extern int tree_add(tree tptr, const uuid key, unsigned long long value);
extern int tree_get(const tree tptr, const uuid key, unsigned long long *value);
extern int tree_set(const tree tptr, const uuid key, unsigned long long value);
extern int tree_del(tree tptr, const uuid key);

extern size_t tree_count(const tree tptr);
extern int tree_stats(const tree tptr, size_t *trunks, size_t *branches, size_t *leafs);
extern size_t tree_iter(const tree tptr, void *h, int (*)(void*,const uuid,unsigned long long*));

extern void tree_destroy(tree tptr);

#endif
