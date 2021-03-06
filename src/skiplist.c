#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "skiplist.h"

struct slnode_
{
	char *key;
	void *value;
	slnode *forward[0];
};

#define max_levels 16
#define max_level (max_levels-1)
#define new_node_of_level(n) (slnode*)malloc(sizeof(slnode)+((n)*sizeof(slnode*)))
#define frand() ((double)rand() / RAND_MAX)

static int random_level(void)
{
	const double P = 0.5;
    int lvl = (int)(log(frand())/log(1.0-P));
    return lvl < max_level ? lvl : max_level;
}

static int defcmp(const char *s1, const char* s2)
{
	if ((long long)s1 < (long long)s2)
		return -1;
	else if ((long long)s1 > (long long)s2)
		return 1;
	else
		return 0;
}

void sl_init(skiplist *d, int dups, int (*compare)(const char*, const char*), void (*deleter)(void*))
{
	if (!d) return;
	d->header = new_node_of_level(max_levels);
	if (d->header == NULL) return;
	d->level = 1;
	d->dups = dups;
	d->compare = compare ? compare : defcmp;
	d->deleter = deleter;
	d->p = NULL;
	int i;

	for (i = 0; i < max_levels; i++)
		d->header->forward[i] = NULL;

	d->header->key = NULL;
}

int sl_set(skiplist *d, const char *key, void *value)
{
	if (!d || !key) return 0;
	slnode *update[max_levels], *p = d->header, *q = NULL;
	int i, k;

	for (k = d->level-1; k >= 0; k--)
	{
		while ((q = p->forward[k]) && (d->compare(q->key, key) < 0))
			p = q;

		update[k] = p;
	}

	if (!d->dups)
		if (q && (d->compare(q->key, key) == 0))
			return 0;

	k = random_level();

	if (k >= d->level)
	{
		d->level++;
		k = d->level - 1;
		update[k] = d->header;
	}

	q = new_node_of_level(k+1);

	if (q == NULL)
		return 0;

	q->key = (char*)key;
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

int sl_del(skiplist *d, const char *key, void **value)
{
	if (!d || !key) return 0;
	slnode *update[max_levels], *p = d->header, *q = NULL;
	int k, m;

	for (k = d->level-1; k >= 0; k--)
	{
		while ((q = p->forward[k]) && (d->compare(q->key, key) < 0))
			p = q;

		update[k] = p;
	}

	q = p->forward[0];

	if (q && (d->compare(q->key, key) == 0))
	{
		if (d->deleter) d->deleter(q->key);
		void *v = q->value;
		m = d->level - 1;

		for (k = 0; k <= m; k++)
		{
			p = update[k];

			if ((p == NULL) || (p->forward[k] != q))
				break;

			p->forward[k] = q->forward[k];
		}

		free(q);
		m = d->level - 1;

		while ((d->header->forward[m] == NULL) && (m > 0))
			m--;

		d->level = m + 1;
		if (value) *value = v;
		return 1;
	}

	return 0;
}

int sl_get(skiplist *d, const char *key, void **value)
{
	if (!d || !key) return 0;
	slnode *p = d->header, *q = NULL;
	int k;

	for (k = d->level-1; k >= 0; k--)
	{
		while ((q = p->forward[k]) && (d->compare(q->key, key) < 0))
			p = q;
	}

	if (q && (d->compare(q->key, key) == 0))
	{
		if (value) *value = q->value;
		return 1;
	}

	return 0;
}

void sl_start(skiplist *d)
{
	if (!d) return;
	d->p = d->header->forward[0];
}

void *sl_next(skiplist *d, const char **key)
{
	if (!d) return NULL;
	if (d->p == NULL) return NULL;
	if (key) *key = d->p->key;
	void *value = d->p->value;
	d->p = d->p->forward[0];
	return value;
}

void sl_done(skiplist *d, void (*deleter)(void*))
{
	if (!d) return;
	slnode *p = d->header, *q;
	q = p->forward[0];
	free(p);
	p = q;

	while (p != NULL)
	{
		q = p->forward[0];
		if (d->deleter) d->deleter(p->key);
		if (deleter) deleter(p->value);
		free(p);
		p = q;
	}

	d->header = NULL;
}

void sl_clear(skiplist *d, void (*deleter)(void*))
{
	sl_done(d,  deleter);
	sl_init(d, d->dups, d->compare, d->deleter);
}
