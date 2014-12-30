#include "list.h"

struct node_
{
	node prev, next;
};

struct list_
{
	node first, last;
	unsigned cnt;
};

void push_front(list l, node t)
{
	l->cnt++;

	if (!l->first)
	{
		l->first = l->last = t;
		return;
	}

	l->first->prev = t;
	t->next = l->first;
	l->first = t;
}

void push_back(list l, node t)
{
	l->cnt++;

	if (!l->first)
	{
		l->first = l->last = t;
		return;
	}

	l->last->next = t;
	t->prev = l->last;
	l->last = t;
}
