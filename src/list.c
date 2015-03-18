#include <stdlib.h>

#include "list.h"

struct list_
{
	node *front, *back;
	size_t cnt;
};

int list_init(list *l)
{
	if (!l)
		return 0;

	l->front = l->back = NULL;
	l->cnt = 0;
	return 1;
}

size_t list_count(const list *l)
{
	if (!l)
		return 0;

	return l->cnt;
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

	node *n = l->front;

	while (n)
	{
		node *save = n;
		n = n->next;
		free(save);
	}

	l->front = l->back = NULL;
	l->cnt = 0;
	return 1;
}

node *list_front(const list *l)
{
	if (!l)
		return NULL;

	return l->front;
}

node *list_prevz(node *n)
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

node *list_back(const list *l)
{
	if (!l)
		return NULL;

	return l->back;
}

int list_iter(list *l, int (*f)(node*,void*), void *data)
{
	if (!l)
		return 0;

	node *n = l->front;

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

	if (l->front == n)
		l->front = n->next;
	else
		n->prev->next = n->next;

	if (l->back == n)
		l->back = n->prev;
	else
		n->next->prev = n->prev;

	n->prev = n->next = NULL;
	l->cnt--;
	return 1;
}

int list_push_front(list *l, node *n)
{
	if (!l || !n)
		return 0;

	n->prev = NULL;
	l->cnt++;

	if (!l->front)
	{
		l->front = l->back = n;
		n->next = NULL;
		return 1;
	}

	l->front->prev = n;
	n->next = l->front;
	l->front = n;
	return 1;
}

int list_push_back(list *l, node *n)
{
	if (!l || !n)
		return 0;

	n->next = NULL;
	l->cnt++;

	if (!l->front)
	{
		l->front = l->back = n;
		n->prev = NULL;
		return 1;
	}

	l->back->next = n;
	n->prev = l->back;
	l->back = n;
	return 1;
}

int list_pop_front(list *l, node **n)
{
	if (!l)
		return 0;

	if (!l->front)
		return 0;

	l->cnt--;

	if (n)
	{
		*n = l->front;
		(*n)->prev = NULL;
		(*n)->next = NULL;
	}

	l->front = l->front->next;
	l->front->prev = NULL;
	return 1;
}

int list_pop_back(list *l, node **n)
{
	if (!l)
		return 0;

	if (!l->back)
		return 0;

	l->cnt--;

	if (n)
	{
		*n = l->back;
		(*n)->prev = NULL;
		(*n)->next = NULL;
	}

	l->back = l->back->prev;
	l->back->next = NULL;
	return 1;
}

int list_insert_before(list *l, node *n, node *v)
{
	if (!l || !n || !v)
		return 0;

	l->cnt++;

	if (l->front == n)
		l->front = v;

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

	l->cnt++;

	if (l->back == n)
		l->back = v;

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

	if (!l2->front)
		return;

	if (!l->front)
	{
		l->front = l2->front;
		l->back = l2->back;
		l->cnt = l2->cnt;
		l2->front = l2->back = NULL;
		l2->cnt = 0;
		return;
	}

	l->back->next = l2->front;
	l2->front->prev = l->back;
	l->back = l2->back;
	l->cnt += l2->cnt;
	l2->front = l2->back = NULL;
	l2->cnt = 0;
}

