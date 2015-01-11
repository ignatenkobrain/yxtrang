#ifndef LIST_H
#define LIST_H

// Usage:
//
//          typedef struct {
//            node n;     		// Must come first
//            int val;
//          } mynode;
//
//
// Static stack-based:
//
//          list l;
//          list_init(&l);
//			..
//			mynode *my = (mynode*)malloc(sizeof(mynode));
//          my->val = 123456;
//          list_push_back(&l, (node*)my);
//			list_pop_front(&l, (node**)&my);
//          int val = my->val;
//          free(my);
//			..
//			list_clear(&l);
//
// Dynamic heap-based:
//
//          list *l = list_create();
//			..
//			mynode *my = (mynode*)malloc(sizeof(mynode));
//          my->val = 123456;
//          list_push_back(l, (node*)my);
//			list_pop_front(l, (node**)&my);
//          int val = my->val;
//          free(my);
//			..
//          list_destroy(l);

#include <string.h>

typedef struct node_ node;
struct node_ { node *prev, *next; };

typedef struct list_ list;

extern int list_init(list *l);		// static list

extern list *list_create(void);		// dynamic list
extern int list_destroy(list *l);	// ..

extern int list_push_front(list *l, node *n);
extern int list_push_back(list *l, node *n);
extern int list_insert_before(list *l, node *n, node *v);
extern int list_insert_after(list *l, node *n, node *v);

extern int list_pop_front(list *l, node **n);	// Returns 'n' if not NULL
extern int list_pop_back(list *l, node **n);	// Returns 'n' if not NULL
extern int list_remove(list *l, node *n);

extern node *list_front(const list *l);		// Returns NULL if empty
extern node *list_back(const list *l);		// Returns NULL if empty

extern size_t list_count(const list *l);		// Returns internal count

extern int list_iter(list *l, int (*)(node*,void*), void *data);
extern int list_clear(list *l);

extern void list_concat(list *l, list *l2);

#endif
