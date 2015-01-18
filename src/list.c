#include <stdlib.h>

#include "list.h"

struct list_
{
	node *head, *tail;
	size_t nodes;
};

int list_init(list *l)
{
	if (!l)
		return 0;

	l->head = l->tail = NULL;
	l->nodes = 0;
	return 1;
}

size_t list_count(const list *l)
{
	if (!l)
		return 0;

	return l->nodes;
}

list *list_create(void)
{
	return (list*)calloc(1, sizeof(list));
}

int list_destroy(list *l)
{
	if (!l)
		return 0;

	list_clear(l);
	free(l);
	return 1;
}

int list_clear(list *l)
{
	if (!l)
		return 0;

	node *n = l->head;

	while (n)
	{
		node *save = n;
		n = n->next;
		free(save);
	}

	l->head = l->tail = NULL;
	l->nodes = 0;
	return 1;
}

node *list_front(list *l)
{
	if (!l)
		return NULL;

	return l->head;
}

node *list_prev(node *n)
{
	if (!n)
		return NULL;

	return n->prev;
}

node *list_next(node *n)
{
	if (!n)
		return NULL;

	return n->next;
}

node *list_back(list *l)
{
	if (!l)
		return NULL;

	return l->tail;
}

int list_iter(list *l, int (*f)(node*,void*), void *data)
{
	if (!l)
		return 0;

	node *n = l->head;

	while (n)
	{
		node *save = n;
		n = n->next;

		if (f)
			if (!f(save, data))
				break;
	}

	return 1;
}

int list_remove(list *l, node *n)
{
	if (!l || !n)
		return 0;

	if (l->head == n)
		l->head = n->next;

	if (l->tail == n)
		l->tail = n->prev;

	if (n->prev)
		n->prev->next = n->next;

	if (n->next)
		n->next->prev = n->prev;

	if (n->prev || n->next)
		l->nodes--;

	n->prev = n->next = NULL;
	return 1;
}

int list_push_front(list *l, node *n)
{
	if (!l || !n)
		return 0;

	n->prev = NULL;
	l->nodes++;

	if (!l->head)
	{
		l->head = l->tail = n;
		n->next = NULL;
		return 1;
	}

	l->head->prev = n;
	n->next = l->head;
	l->head = n;
	return 1;
}

int list_push_back(list *l, node *n)
{
	if (!l || !n)
		return 0;

	n->next = NULL;
	l->nodes++;

	if (!l->head)
	{
		l->head = l->tail = n;
		n->prev = NULL;
		return 1;
	}

	l->tail->next = n;
	n->prev = l->tail;
	l->tail = n;
	return 1;
}

int list_pop_front(list *l, node **n)
{
	if (!l)
		return 0;

	if (!l->head)
		return 0;

	if (n)
	{
		*n = l->head;
		(*n)->prev = NULL;
		(*n)->next = NULL;
	}

	l->head = l->head->next;
	l->head->prev = NULL;
	l->nodes--;
	return 1;
}

int list_pop_back(list *l, node **n)
{
	if (!l)
		return 0;

	if (!l->tail)
		return 0;

	if (n)
	{
		*n = l->tail;
		(*n)->prev = NULL;
		(*n)->next = NULL;
	}

	l->tail = l->tail->prev;
	l->tail->next = NULL;
	l->nodes--;
	return 1;
}

int list_insert_before(list *l, node *n, node *v)
{
	if (!l || !n || !v)
		return 0;

	l->nodes++;

	if (l->head == n)
		l->head = v;

	if (n->prev)
		n->prev->next = v;

	v->prev = n->prev;
	v->next = n;
	n->prev = v;
	return 1;
}

int list_insert_after(list *l, node *n, node *v)
{
	if (!l || !n || !v)
		return 0;

	l->nodes++;

	if (l->tail == n)
		l->tail = v;

	if (n->next)
		n->next->prev = v;

	v->next = n->next;
	v->prev = n;
	n->next = v;
	return 1;
}

void list_concat(list *l, list *l2)
{
	if (!l || !l2)
		return;

	if (!l2->head)
		return;

	if (!l->head)
	{
		l->head = l2->head;
		l->tail = l2->tail;
		l2->head = l2->tail = NULL;
		return;
	}

	l->tail->next = l2->head;
	l2->head->prev = l->tail;
	l->tail = l2->tail;
	l2->head = l2->tail = NULL;
}

