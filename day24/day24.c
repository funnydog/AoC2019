#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned bug_load(FILE *input)
{
	unsigned bug = 0;
	unsigned bit = 1;
	int c;
	while ((c = getc(input)) != EOF)
	{
		switch (c)
		{
		case '?':
		case '.': bit <<= 1; break;
		case '#': bug |= bit; bit <<= 1; break;
		default: break;
		}
	}
	return bug;
}

static unsigned evolve(unsigned bug)
{
	unsigned new = 0;
	const int dx[] = {0, 1, 0, -1};
	const int dy[] = {-1, 0, 1, 0};
	int bit = 1;
	for (int y = 0; y < 5; y++)
	{
		for (int x = 0; x < 5; x++, bit <<= 1)
		{
			unsigned count = 0;
			for (int i = 0; i < 4; i++)
			{
				int nx = x+dx[i];
				int ny = y+dy[i];
				if (0 <= nx && nx < 5 &&
				    0 <= ny && ny < 5)
				{
					count += (bug & (1<<(ny*5+nx))) != 0;
				}
			}
			if (count == 1 || (count == 2 && !(bug & bit)))
			{
				new |= bit;
			}
		}
	}
	return new;
}

static unsigned first_of_cycle(unsigned original)
{
	/* detect the cycle */
	unsigned bug = original;
	unsigned stack[100];
	unsigned count = 0;
	for (;;)
	{
		while (count && stack[count-1] > bug)
		{
			count--;
		}
		if (count && stack[count-1] == bug)
		{
			/* cycle detected */
			break;
		}
		assert(count != 100);
		stack[count++] = bug;
		bug = evolve(bug);
	}

	/* determine the period of the cycle */
	size_t period = 0;
	unsigned old = bug;
	do
	{
		bug = evolve(bug);
		period++;
	} while(bug != old);

	/* store the bugs in the cycle */
	unsigned *cycle_set = malloc(sizeof(*cycle_set) * period);
	for (size_t i = 0; i < period; i++)
	{
		bug = evolve(bug);
		cycle_set[i] = bug;
	}

	/* find the first bug that belongs to the cycle */
	bug = original;
	for (;;)
	{
		size_t i;
		for (i = 0; i < period; i++)
		{
			if (bug == cycle_set[i])
			{
				break;
			}
		}
		if (i < period)
		{
			break;
		}
		bug = evolve(bug);
	}
	free(cycle_set);

	return bug;
}

struct bugmap
{
	unsigned bug[1000];
	size_t lcount;
	int minlevel;
	int maxlevel;
};

unsigned bugmap_get(struct bugmap *map, int level)
{
	if (level < map->minlevel || level > map->maxlevel)
	{
		return 0;
	}
	return map->bug[level - map->minlevel];
}

static unsigned bugmap_count_adj(struct bugmap *map, int level, int x, int y)
{
	if (x == 2 && y == 2)
	{
		return 0;
	}

	const int dx[] = {0, 1, 0, -1};
	const int dy[] = {-1, 0, 1, 0};

	unsigned count = 0;
	for (int i = 0; i < 4; i++)
	{
		int nx = x + dx[i];
		int ny = y + dy[i];

		if (nx < 0)
		{
			count += !!(bugmap_get(map, level-1) & 1<<11);
		}
		else if (nx >= 5)
		{
			count += !!(bugmap_get(map, level-1) & 1<<13);
		}
		else if (ny < 0)
		{
			count += !!(bugmap_get(map, level-1) & 1<<7);
		}
		else if (ny >= 5)
		{
			count += !!(bugmap_get(map, level-1) & 1<<17);
		}
		else if (nx == 2 && ny == 2)
		{
			unsigned bug = bugmap_get(map, level + 1);
			switch(i)
			{
			case 0: count += __builtin_popcount(bug & 0x1f00000); break;
			case 1: count += __builtin_popcount(bug & 0x108421); break;
			case 2: count += __builtin_popcount(bug & 0x1f); break;
			case 3: count += __builtin_popcount(bug & 0x1084210); break;
			}
		}
		else
		{
			count += !!(bugmap_get(map, level) & 1<<(nx + ny * 5));
		}
	}
	return count;
}

static void bugmap_evolve(struct bugmap *map)
{
	struct bugmap new = *map;
	new.lcount = 0;

	/* create a new minimum level if needed. */
	unsigned bug = map->bug[0];
	unsigned newbug = 0;
	int count;
	count = __builtin_popcount(bug & 0x1f);
	if (count == 1 || count == 2) newbug |= 1<<7;
	count = __builtin_popcount(bug & 0x108421);
	if (count == 1 || count == 2) newbug |= 1<<11;
	count = __builtin_popcount(bug & 0x1084210);
	if (count == 1 || count == 2) newbug |= 1<<13;
	count = __builtin_popcount(bug & 0x1f00000);
	if (count == 1 || count == 2) newbug |= 1<<17;
	if (newbug)
	{
		new.bug[new.lcount++] = newbug;
		new.minlevel--;
	}

	/* process the existing inner levels */
	for (size_t i = 0; i < map->lcount; i++)
	{
		int level = i + map->minlevel;
		bug = map->bug[i];
		newbug = 0;
		unsigned bit = 1;
		for (int y = 0; y < 5; y++)
		{
			for (int x = 0; x < 5; x++, bit <<= 1)
			{
				count = bugmap_count_adj(map, level, x, y);
				if (count == 1 || (count == 2 && !(bug & bit)))
				{
					newbug |= bit;
				}
			}
		}
		assert(new.lcount < 1000);
		new.bug[new.lcount++] = newbug;

	}

	/*
	 * create a new maximum level if needed.
	 *
	 * NOTE: bug is the last bug in the list
	 */
	newbug = 0;
	if (bug & 1<<7)  newbug |= 0x1f;
	if (bug & 1<<11) newbug |= 0x108421;
	if (bug & 1<<13) newbug |= 0x1084210;
	if (bug & 1<<17) newbug |= 0x1f00000;
	if (newbug)
	{
		assert(new.lcount < 1000);
		new.bug[new.lcount++] = newbug;
		new.maxlevel++;
	}

	*map = new;
}

static size_t bugmap_count(struct bugmap *map)
{
	size_t count = 0;
	for (size_t i = 0; i < map->lcount; i++)
	{
		count += __builtin_popcount(map->bug[i]);
	}
	return count;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <input>\n", argv[0]);
		return -1;
	}

	FILE *input = fopen(argv[1], "rb");
	if (!input)
	{
		fprintf(stderr, "Cannot open %s for reading\n", argv[1]);
		return -1;
	}
	unsigned bug = bug_load(input);
	fclose(input);
	printf("part1: %u\n", first_of_cycle(bug));

	struct bugmap bm = {};
	bm.bug[bm.lcount++] = bug;
	for (int i = 0; i < 200; i++)
	{
		bugmap_evolve(&bm);
	}

	printf("part2: %zu\n", bugmap_count(&bm));
	return 0;
}
