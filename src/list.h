#ifndef LIST_H
#define LIST_H

typedef struct node_* node;
typedef struct list_* list;

typedef struct list_ list_t;
typedef struct node_ node_t;

extern void push_front(list l, node t);
extern void push_back(list l, node t);

#endif
