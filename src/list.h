#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

// Usage:
//
//          struct mynode
//          {
//            node_t n;     // Must come first
//            int val;
//          };
//
//          list_t l;
//			mynode* my = malloc(sizeof(struct mynode));
//          my->val = 123456;
//          push_back(&l, (node)my);
//             ...
//			pop_front(&l, (node*)&my);
//          int val = my->val;
//          free(my);

typedef struct node_* node;
typedef struct list_* list;

typedef struct list_ list_t;
typedef struct node_ node_t;

extern void push_front(list l, node n);
extern void push_back(list l, node n);

extern int pop_front(list l, node* n);
extern int pop_back(list l, node* n);

extern size_t list_count(list l);

extern void list_iter(list l, int (*)(node,void*), void* data);
extern void list_clear(list l);

#endif
