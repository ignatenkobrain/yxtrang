#ifndef LIST_H
#define LIST_H

#include <string.h>

// Usage:
//
//          typedef struct mynode_ {
//            node_t n;     // Must come first
//            int val;
//          } *mynode;
//
//
// Stack-based:
//
//          list_t l;
//          list_init(&l);
//			mynode my = (mynode)malloc(sizeof(struct mynode_));
//          my->val = 123456;
//          list_push_back(&l, (node)my);
//			list_pop_front(&l, (node*)&my);
//          int val = my->val;
//          free(my);
//
// Heap-based:
//
//          list l = list_create();
//			mynode my = (mynode)malloc(sizeof(struct mynode_));
//          my->val = 123456;
//          list_push_back(l, (node)my);
//			list_pop_front(l, (node*)&my);
//          int val = my->val;
//          free(my);
//          list_destroy(l);

typedef struct node_* node;
typedef struct list_* list;

typedef struct list_ list_t;
typedef struct node_ node_t;

extern void list_init(list l);

extern list list_create(void);
extern void list_destroy(list l);

extern void list_push_front(list l, node n);
extern void list_push_back(list l, node n);

extern int list_pop_front(list l, node* n);
extern int list_pop_back(list l, node* n);

extern size_t list_count(list l);

extern void list_iter(list l, int (*)(node,void*), void* data);
extern void list_clear(list l);

#endif
