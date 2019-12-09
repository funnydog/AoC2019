#ifndef PERM_H
#define PERM_H

#include <stdlib.h>
#include <string.h>

struct piterator
{
	int *a;
	int *c;
	int  i;
	int  size;
};

static inline void perm_free(struct piterator *p)
{
	free(p);
}

static inline struct piterator *perm_init(const int *arr, size_t size)
{
	struct piterator *p = calloc(1, sizeof(*p) + 2 * size * sizeof(*arr));
	if (p)
	{
		p->size = size;
		p->a = (int *)((char *)p + sizeof(*p));
		p->c = p->a + size;
		memcpy(p->a, arr, size * sizeof(*arr));
	}
	return p;
}

static inline struct piterator *perm_next(struct piterator *p)
{
	while (p->i < p->size)
	{
		if (p->c[p->i] < p->i)
		{
			if (p->i & 1)
			{
				int t = p->a[p->i];
				p->a[p->i] = p->a[p->c[p->i]];
				p->a[p->c[p->i]] = t;
			}
			else
			{
				int t = p->a[0];
				p->a[0] = p->a[p->i];
				p->a[p->i] = t;
			}
			p->c[p->i]++;
			p->i = 0;
			return p;
		}
		else
		{
			p->c[p->i] = 0;
			p->i++;
		}
	}
	perm_free(p);
	return NULL;
}

#endif
