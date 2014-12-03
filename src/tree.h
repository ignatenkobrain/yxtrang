#ifndef TREE_H
#define TREE_H

#ifndef TREE_KEY_USERDEFINED
#include "uuid.h"
#define TREE_KEY uuid
#define TREE_KEY_compare uuid_compare
#endif

typedef struct _tree* tree;

extern tree tree_create(void);

extern int tree_add(tree tptr, const TREE_KEY* key, unsigned long long value);
extern int tree_get(const tree tptr, const TREE_KEY* key, unsigned long long* value);
extern int tree_set(const tree tptr, const TREE_KEY* key, unsigned long long value);
extern int tree_del(tree tptr, const TREE_KEY* key);

extern unsigned long tree_count(const tree tptr);
extern int tree_stats(const tree tptr, unsigned long* trunks, unsigned long* branches, unsigned long* leafs);
extern unsigned long tree_iter(const tree tptr, void* h, int (*f)(void* h, const TREE_KEY* key, unsigned long long* value));

extern void tree_destroy(tree tptr);

#endif
