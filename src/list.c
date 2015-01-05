#include <stdlib.h>

#include "list.h"

struct node_
{
	node prev, next;
};

struct list_
{
	node first, last;
	size_t count;
};

int list_init(list l)
{
	if (!l)
		return 0;

	l->first = l->last = NULL;
	l->count = 0;
	return 1;
}

list list_create(void)
{
	list l = (list)calloc(1, sizeof(struct list_));
	return l;
}

int list_destroy(list l)
{
	if (!l)
		return 0;

	list_clear(l);
	free(l);
	return 1;
}

size_t list_count(list l)
{
	if (!l)
		return 0;

	return l->count;
}

int list_clear(list l)
{
	if (!l)
		return 0;

	node n = l->first;

	while (n)
	{
		node save = n->next;
		free(n);
		n = save;
	}

	l->count = 0;
	return 1;
}

int list_remove(list l, node n)
{
	if (!l || !n)
		return 0;

	if (l->first == n)
		l->first = n->next;

	if (l->last == n)
		l->last = n->prev;

	if (n->prev)
		n->prev->next = n->next;

	if (n->next)
		n->next->prev = n->prev;

	return 1;
}

int list_iter(list l, int (*f)(node,void*), void* data)
{
	if (!l)
		return 0;

	node n = l->first;

	while (n)
	{
		node save = n->next;

		if (f)
			if (!f(n, data))
				break;

		n = save;
	}

	return 1;
}

int list_push_front(list l, node n)
{
	if (!l || !n)
		return 0;

	l->count++;
	n->prev = n->next = NULL;

	if (!l->first)
	{
		l->first = l->last = n;
		return 1;
	}

	l->first->prev = n;
	n->next = l->first;
	l->first = n;
	return 1;
}

int list_push_back(list l, node n)
{
	if (!l || !n)
		return 0;

	l->count++;
	n->prev = n->next = NULL;

	if (!l->first)
	{
		l->first = l->last = n;
		return 1;
	}

	l->last->next = n;
	n->prev = l->last;
	l->last = n;
	return 1;
}

int list_pop_front(list l, node* n)
{
	if (!l)
		return 0;

	if (!l->first)
		return 0;

	if (n)
		*n = l->first;

	l->first = l->first->next;
	l->first->prev = NULL;
	l->count--;
	return 1;
}

int list_pop_back(list l, node* n)
{
	if (!l)
		return 0;

	if (!l->last)
		return 0;

	if (n)
		*n = l->last;

	l->last = l->last->prev;
	l->last->next = NULL;
	l->count--;
	return 1;
}

int list_insert_before(list l, node n, node v)
{
	if (!l || !n || !v)
		return 0;

	if (l->first == n)
		l->first = v;

	if (n->prev)
		n->prev->next = v;

	v->next = n;
	n->prev = v;
	return 1;
}

int list_insert_after(list l, node n, node v)
{
	if (!l || !n || !v)
		return 0;

	if (l->last == n)
		l->last = v;

	if (n->next)
		n->next->prev = v;

	v->next = n->next;
	n->next = v;
	return 1;
}
