#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "skiplist.h"

typedef struct node* node;

struct node
{
	void*	key;
	void*	value;
	node	forward[1];
};

struct SL
{
	node	header;
	int		dups, level;
	int		(*compare)();
	void	(*freeitem)();
};

#define MaxNumberOfLevels 32
#define MaxLevel (MaxNumberOfLevels-1)
#define NewNodeOfLevel(x) (node)malloc(sizeof(struct node)+((x)*sizeof(node)))

// Allows using integer values as keys...

static int default_compare(void* k1, void* k2)
{
	if (k1 < k2)
		return -1;
	else if (k1 == k2)
		return 0;
	else
		return 1;
}

SL sl_open(int (*compare)(), void (*freeitem)())
{
	SL l;
	int i;

	if (compare == NULL)
		compare = default_compare;

	l = (SL)malloc(sizeof(struct SL));

	if (l == NULL)
		return NULL;

	l->level = 1;
	l->header = NewNodeOfLevel(MaxNumberOfLevels);

	if (l->header == NULL)
	{
		free(l);
		return NULL;
	}

	for (i = 0; i < MaxNumberOfLevels; i++)
		l->header->forward[i] = NULL;

	l->header->key = NULL;
	l->compare = compare;
	l->freeitem = freeitem;
	l->dups = 1;

	return(l);
}


void sl_close(SL l)
{
	node p, q;

	if ((l == NULL) || (l->header == NULL))
		return;

	p = l->header;
	q = p->forward[0];
	free(p);
	p = q;

	while (p != NULL)
	{
		q = p->forward[0];

		if (l->freeitem)
			l->freeitem(p->key);

		free(p);
		p = q;
	}

	free(l);
}

#define frand() ((double)rand() / RAND_MAX)

static int RandomLevelSL()
{
	const double P = 0.5;
    int lvl = (int)(log(frand())/log(1.-P));
    return lvl < MaxLevel ? lvl : MaxLevel;
}

int sl_add(SL l, void *key, void* value)
{
	int i, k;
	node update[MaxNumberOfLevels];
	node p, q;

	p = l->header;

	for (k = l->level-1; k >= 0; k--)
	{
		 while ((q = p->forward[k]) && (l->compare(q->key, key) < 0))
			p = q;

		 update[k] = p;
	}

	if (!l->dups)
	{
		if (q && (l->compare(q->key, key) == 0))
			return -1;
	}

	k = RandomLevelSL();

	if (k >= l->level)
	{
		l->level++;
		k = l->level - 1;
		update[k] = l->header;
	}

	q = NewNodeOfLevel(k+1);

	if (q == NULL)
		return 0;

	q->key = key;
	q->value = value;

	for (i = 0; i < k; i++)
		q->forward[i] = NULL;

	for (; k >= 0; k--)
	{
		p = update[k];
		q->forward[k] = p->forward[k];
		p->forward[k] = q;
	}

	return 1;
}

int sl_del(SL l, void *key)
{
	int k, m;
	node update[MaxNumberOfLevels];
	node p, q;

	p = l->header;

	for (k = l->level-1; k >= 0; k--)
	{
		while ((q = p->forward[k]) && (l->compare(q->key, key) < 0))
			p = q;

		update[k] = p;
	}

	q = p->forward[0];

	if (q && (l->compare(q->key, key) == 0))
	{
		m = l->level - 1;

		for (k = 0; k <= m; k++)
		{
			p = update[k];

			if ((p == NULL) || (p->forward[k] != q))
				break;

			p->forward[k] = q->forward[k];
		}

		if (l->freeitem)
			l->freeitem(q->key);

		free(q);

		m = l->level - 1;

		while ((l->header->forward[m] == NULL) && m > 0)
			m--;

		l->level = m + 1;
		return 1;
	}
	else
		return 0;
}

int sl_get(SL l, void *key, void** value)
{
	int k;
	node p, q;

	p = l->header;

	for (k = l->level-1; k >= 0; k--)
	{
		while ((q = p->forward[k]) && (l->compare(q->key, key) < 0))
			p = q;
	}

	if ((q == NULL) || (l->compare(q->key, key) != 0))
		return 0;

	*value = q->value;
	return 1;
}

void sl_iter(SL l, int (*function)(), void *arg)
{
	node p, q;

	if ((l == NULL) || (l->header == NULL) || (function == NULL))
		return;

	p = l->header;
	p = p->forward[0];

	while (p != NULL)
	{
		q = p->forward[0];

		if (function(arg, p->key, p->value) == 0)
			break;

		p = q;
	}
}

void sl_test(long cnt)
{
	SL sl = sl_open(0, 0, 0);
	long i;

	for (i = 1; i <= cnt; i++)
		sl_add(sl, (void*)(long)i, (void*)i);

	sl_close(sl);
}

