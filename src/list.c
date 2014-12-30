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

size_t list_count(list l)
{
	return l->cnt;
}

void list_clear(list l)
{
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
	if (!l->last)
		return 0;

	*n = l->last;
	l->last = l->last->prev;
	l->last->next = NULL;
	l->cnt--;
	return 1;
}

