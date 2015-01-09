
/*
  *Like a b-tree, but optimized for indexing timestamped logfiles.
  *A key value of zero is not allowed.
  *In-order adds are extremely quick (effectively an append)
  *Out-of-order inserts not so much.
 *
  *Thread-safe for a single writer and readers.
 *
  *Future enhancement: if nbr of nodes in a branch *drops below half,
  *then reallocate the branch *at half the size. But only if
  *the number is above a certain minimum. This way we can  garbage
  *collect wasted space.
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "tree.h"

#ifndef TREE_NODES
#define TREE_NODES 64
#endif

typedef struct _trunk trunk;
typedef struct _branch branch;
typedef struct _node node;

struct _node
{
	uuid_t k;

	union
	{
		unsigned long long v;	// if a leaf: stores value
		branch *b;				// otherwise: points to branch
	};
};

struct _branch
{
	unsigned max_nodes, nodes, leaf;
	node n[0];
};

struct _trunk
{
	branch *active;
	trunk *next;
};

struct tree_
{
	trunk *first, *last;
	size_t trunks, branches, leafs;
	uuid_t last_key;
};

static uuid_t kzero = {0};

static int binary_search(node *n, const uuid k, int imin, int imax)
{
	while (imax >= imin)
	{
		int imid = (imax + imin) / 2;
		int x = uuid_compare(&n[imid].k, k);

		if (x == 0)
			return imid;
		else if (x < 0)
			imin = imid + 1;
		else
			imax = imid - 1;
	}

	return -1;
}

// Modified binary search: return position where it is or ought to be

static int binary_search2(node *n, const uuid k, int imin, int imax)
{
	int imid = 0;

	while (imax >= imin)
	{
		imid = (imax + imin) / 2;
		int x = uuid_compare(&n[imid].k, k);

		if (x == 0)
			return -1;
		else if (x < 0)
			imin = imid + 1;
		else
			imax = imid - 1;
	}

	// The binary search can come up either side of the
	// actual insertion point, depending upon odd-even
	// number of points. Make sure we get it right...

	if (uuid_compare(&n[imid].k, k) < 0)
		imid++;

	return imid;
}

tree *tree_create()
{
	tree *tptr = (tree*)calloc(1, sizeof(struct tree_));
	if (!tptr) return NULL;
	tptr->trunks++;
	tptr->last = tptr->first = (trunk*)calloc(1, sizeof(struct _trunk));
	if (!tptr->last) return tptr;

	size_t block_size = TREE_NODES  *sizeof(struct _node);
	block_size += sizeof(struct _branch);

	tptr->branches++;
	trunk *t = tptr->first;
	t->active = (branch*)calloc(1, block_size);
	if (!t->active) return tptr;
	t->active->max_nodes = TREE_NODES;
	t->active->leaf = 1;

	// Add in a dummy (illegal) zero key,
	// to aid/simplify inserts...

	t->active->n[t->active->nodes].k = kzero;
	t->active->n[t->active->nodes].v = 0;
	t->active->nodes++;
	return tptr;
}

size_t tree_count(const tree *tptr)
{
	return tptr->leafs;
}

static int branch_insert(tree *tptr, branch **b2, const uuid k, unsigned long long v)
{
	branch *b = *b2;

	if (b->leaf)
	{
		int imid = binary_search2(b->n, k, 0, b->nodes-1);

		if (imid < 0)
			return 0;

		// If full, reallocate a bigger one...

		if (b->nodes == b->max_nodes)
		{
			b->max_nodes += 1;
			size_t block_size = b->max_nodes  *sizeof(struct _node);
			block_size += sizeof(struct _branch);
			b = (branch*)realloc(b, block_size);
			if (!b) return 0;

			if (*b2 == tptr->first->active)
				tptr->first->active = b;

			*b2 = b;
		}

		// Slide things up and in-fill...

		int i;

		for (i = b->nodes; i > imid; i--)
			b->n[i] = b->n[i-1];

		b->n[i].k = *k;
		b->n[i].v = v;
		b->nodes++;
		tptr->leafs++;
		return 1;
	}

	branch **last = 0;
	node *n = b->n;
	int i;

	for (i = 0; i < b->nodes; i++, n++)
	{
		int x = uuid_compare(k, &n->k);

		if (x < 0)
			break;

		last = &n->b;

		if (x == 0)
			break;
	}

	if (!last)
		return 0;

	return branch_insert(tptr, last, k, v);
}

int tree_insert(tree *tptr, const uuid k, unsigned long long v)
{
	if (!tptr)
		return 0;

	return branch_insert(tptr, &tptr->last->active, k, v);
}

static void trunk_add(tree *tptr, trunk *t, branch *save, branch *b, const uuid k)
{
	if (!t->next)
	{
		tptr->trunks++;
		tptr->last = t->next = (trunk*)calloc(1, sizeof(struct _trunk));
		if (!tptr->last) return;

		size_t block_size = TREE_NODES  *sizeof(struct _node);
		block_size += sizeof(struct _branch);

		tptr->branches++;
		t->next->active = (branch*)calloc(1, block_size);
		if (!t->next->active) return;
		t->next->active->max_nodes = TREE_NODES;
		t->next->active->n[0].k = save->n[0].k;
		t->next->active->n[0].b = save;
		t->next->active->nodes++;
	}

	t = t->next;

	if (t->active->nodes == t->active->max_nodes)
	{
		branch *save2 = t->active;

		size_t block_size = TREE_NODES  *sizeof(struct _node);
		block_size += sizeof(struct _branch);

		tptr->branches++;
		t->active = (branch*)calloc(1, block_size);
		if (!t->active) return;
		t->active->max_nodes = TREE_NODES;
		trunk_add(tptr, t, save2, t->active, k);
	}

	t->active->n[t->active->nodes].k = *k;
	t->active->n[t->active->nodes].b = b;
	t->active->nodes++;
}

int tree_add(tree *tptr, const uuid k, unsigned long long v)
{
	if (!tptr || !k)
		return 0;

	if (uuid_compare(k, &tptr->last_key) < 0)
		return tree_insert(tptr, k, v);

	trunk *t = tptr->first;

	if (t->active->nodes == t->active->max_nodes)
	{
		branch *save2 = t->active;

		size_t block_size = TREE_NODES  *sizeof(struct _node);
		block_size += sizeof(struct _branch);

		tptr->branches++;
		t->active = (branch*)calloc(1, block_size);
		if (!t->active) return 0;
		t->active->max_nodes = TREE_NODES;
		t->active->leaf = 1;
		trunk_add(tptr, t, save2, t->active, k);
	}

	t->active->n[t->active->nodes].k = *k;
	t->active->n[t->active->nodes].v = v;
	t->active->nodes++;
	tptr->leafs++;
	tptr->last_key = *k;
	return 1;
}

static int is_active(const tree *tptr, const branch *b)
{
	trunk *t;

	for (t = tptr->first; t; t = t->next)
	{
		if (b == t->active)
			return 1;
	}

	return 0;
}

static int branch_del(tree *tptr, branch *b, const uuid k)
{
	if (b->leaf)
	{
		int idx = binary_search(b->n, k, 0, b->nodes-1);
		if (idx < 0) return 0;

		while (idx < b->nodes)
		{
			b->n[idx] = b->n[idx+1];
			idx++;
		}

		b->nodes--;
		tptr->leafs--;

		// TO-DO: if 'nodes' drops below half full
		// re-allocate a smaller branch.

		return 1;
	}

	node *last = 0;
	node *n = b->n;
	int i;

	for (i = 0; i < b->nodes; i++, n++)
	{
		int x = uuid_compare(k, &n->k);

		if (x < 0)
			break;

		last = n;

		if (x == 0)
			break;
	}

	if (!last)
		return 0;

	int del = branch_del(tptr, last->b, k);

	if (!last->b->nodes)
	{
		if (!is_active(tptr, last->b))
		{
			free(last->b);
			tptr->branches--;
		}

		while (i < b->nodes)
		{
			last[0] = last[1];
			last++;
			i++;
		}

		b->nodes--;
	}

	return del;
}

int tree_del(tree *tptr, const uuid k)
{
	if (!tptr || !k)
		return 0;

	return branch_del(tptr, tptr->last->active, k);
}

static int branch_get(const tree *tptr, branch *b, const uuid k, unsigned long long *v)
{
	if (b->leaf)
	{
		int idx = binary_search(b->n, k, 0, b->nodes-1);
		if (idx < 0) return 0;
		*v = b->n[idx].v;
		return 1;
	}

	branch *last = 0;
	node *n = b->n;
	int i;

	for (i = 0; i < b->nodes; i++, n++)
	{
		int x = uuid_compare(k, &n->k);

		if (x < 0)
			break;

		last = n->b;

		if (x == 0)
			break;
	}

	if (!last)
		return 0;

	return branch_get(tptr, last, k, v);
}

int tree_get(const tree *tptr, const uuid k, unsigned long long *v)
{
	if (!tptr || !k)
		return 0;

	return branch_get(tptr, tptr->last->active, k, v);
}

static int branch_set(const tree *tptr, branch *b, const uuid k, unsigned long long v)
{
	if (b->leaf)
	{
		int idx = binary_search(b->n, k, 0, b->nodes-1);
		if (idx < 0) return 0;
		b->n[idx].v = v;
		return 1;
	}

	branch *last = 0;
	node *n = b->n;
	int i;

	for (i = 0; i < b->nodes; i++, n++)
	{
		int x = uuid_compare(k, &n->k);

		if (x < 0)
			break;

		last = n->b;

		if (x == 0)
			break;
	}

	if (!last)
		return 0;

	return branch_set(tptr, last, k, v);
}

int tree_set(const tree *tptr, const uuid k, unsigned long long v)
{
	if (!tptr || !k)
		return 0;

	return branch_set(tptr, tptr->last->active, k, v);
}

static int branch_iter(const tree *tptr, size_t *cnt, branch *b, void *h, int (*f)(void*,const uuid,unsigned long long*))
{
	int i;

	for (i = 0; i < b->nodes; i++)
	{
		if (b->leaf)
		{
			if (uuid_compare(&b->n[i].k, &kzero) == 0)
				continue;

			int ok = f(h, &b->n[i].k, &b->n[i].v);

			if (ok < 0)
				return 0;

			*cnt += ok;
			continue;
		}

		if (branch_iter(tptr, cnt, b->n[i].b, h, f) < 0)
			return 0;
	}

	return 1;
}

size_t tree_iter(const tree *tptr, void *h, int (*f)(void*,const uuid,unsigned long long*))
{
	if (!tptr)
		return 0;

	const trunk *t = tptr->last;
	size_t cnt = 0;
	branch_iter(tptr, &cnt, t->active, h, f);
	return cnt;
}

int tree_stats(const tree *tptr, size_t *trunks, size_t *branches, size_t *leafs)
{
	if (!tptr)
		return 0;

	*trunks = tptr->trunks;
	*branches = tptr->branches;
	*leafs = tptr->leafs;
	return 1;
}

static void branch_close(branch *b)
{
	int i;

	for (i = 0; i < b->nodes; i++)
	{
		if (b->leaf)
			continue;

		branch_close(b->n[i].b);
	}

	free(b);
}

static void trunk_close(trunk *t)
{
	if (t->next)
		trunk_close(t->next);

	free(t);
}

void tree_destroy(tree *tptr)
{
	if (!tptr)
		return;

	branch_close(tptr->last->active);
	trunk_close(tptr->first);
	free(tptr);
}
