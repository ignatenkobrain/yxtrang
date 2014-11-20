#ifndef TREE_H
#define TREE_H

#include <stdlib.h>
#include <stdint.h>

#ifndef TREE_KEY_USERDEFINED
#include "uuid.h"
#define TREE_KEY uuid
#define TREE_KEY_compare uuid_compare
#endif

typedef struct _tree* tree;

extern tree tree_open(void);

extern int tree_add(tree tptr, const TREE_KEY* key, uint64_t value);
extern int tree_get(const tree tptr, const TREE_KEY* key, uint64_t* value);
extern int tree_set(const tree tptr, const TREE_KEY* key, uint64_t value);
extern int tree_del(tree tptr, const TREE_KEY* key);

extern size_t tree_count(const tree tptr);
extern int tree_stats(const tree tptr, long* trunks, long* branches, long* leafs);
extern size_t tree_iter(const tree tptr, void* h, int (*f)(void* h, const TREE_KEY* key, uint64_t* value));

extern void tree_close(tree tptr);

#endif
