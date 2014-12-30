#include "list.h"

struct node_
{
	node prev, next;
};

struct list_
{
	node first, last;
	size_t cnt;
};

void list_init(list l)
{
	if (!l)
		return;

	l->first = l->last = NULL;
	l->cnt = 0;
}

list list_create(void)
{
	list l = (list)calloc(1, sizeof(struct list_));
	return l;
}

void list_destroy(list l)
{
	if (!l)
		return;

	list_clear(l);
	free(l);
}

size_t list_count(list l)
{
	if (!l)
		return 0;

	return l->cnt;
}

void list_clear(list l)
{
	if (!l)
		return;

	node n = l->first;

	while (n)
	{
		node save = n->next;
		free(n);
		n = save;
	}

	l->cnt = 0;
}

void list_iter(list l, int (*f)(node,void*), void* data)
{
	if (!l)
		return;

	node n = l->first;

	while (n)
	{
		node save = n->next;

		if (f)
			if (!f(n, data))
				break;

		n = save;
	}
}

void list_push_front(list l, node n)
{
	if (!l)
		return;

	l->cnt++;

	if (!l->first)
	{
		l->first = l->last = n;
		return;
	}

	l->first->prev = n;
	n->next = l->first;
	l->first = n;
}

void list_push_back(list l, node n)
{
	if (!l)
		return;

	l->cnt++;

	if (!l->first)
	{
		l->first = l->last = n;
		return;
	}

	l->last->next = n;
	n->prev = l->last;
	l->last = n;
}

int list_pop_front(list l, node* n)
{
	if (!l)
		return 0;

	if (!l->first)
		return 0;

	*n = l->first;
	l->first = l->first->next;
	l->first->prev = NULL;
	l->cnt--;
	return 1;
}

int list_pop_back(list l, node* n)
{
	if (!l)
		return 0;

	if (!l->last)
		return 0;

	*n = l->last;
	l->last = l->last->prev;
	l->last->next = NULL;
	l->cnt--;
	return 1;
}
