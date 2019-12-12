#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <regex.h>

struct moon
{
	int64_t pos[3];
	int64_t vel[3];
};

struct moon *load_moons(FILE *input, size_t *count)
{
	regex_t regex;
	const char *pattern = "<x=(-?[0-9]+), y=(-?[0-9]+), z=(-?[0-9]+)>";
	if (regcomp(&regex, pattern, REG_EXTENDED))
	{
		return NULL;
	}
	struct moon *m = NULL;
	size_t msize = 0;
	size_t mcount = 0;

	char *line = NULL;
	size_t lsize = 0;
	while (getline(&line, &lsize, input) != -1)
	{
		regmatch_t match[4];
		if (regexec(&regex, line, 4, match, 0))
		{
			continue;
		}
		if (mcount == msize)
		{
			size_t nsize = msize ? msize * 2 : 4;
			struct moon *nm = realloc(m, nsize * sizeof(*m));
			if (!nm)
			{
				regfree(&regex);
				free(m);
				return NULL;
			}
			msize = nsize;
			m = nm;
		}
		for (int i = 0; i < 3; i++)
		{
			m[mcount].pos[i] = strtol(line+match[i+1].rm_so, NULL, 10);
			m[mcount].vel[i] = 0;
		}
		mcount++;
	}
	free(line);
	regfree(&regex);

	*count = mcount;
	return m;
}

void step_axis(struct moon *m, size_t count, int axis)
{
	for (size_t i = 0; i < count; i++)
	{
		for (size_t j = i + 1; j < count; j++)
		{
			if (m[i].pos[axis] < m[j].pos[axis])
			{
				m[i].vel[axis]++;
				m[j].vel[axis]--;
			}
			else if (m[i].pos[axis] > m[j].pos[axis])
			{
				m[i].vel[axis]--;
				m[j].vel[axis]++;
			}
		}
	}
	for (size_t i = 0; i < count; i++)
	{
		m[i].pos[axis] += m[i].vel[axis];
	}
}

int64_t energy(struct moon *m, size_t count, int steps)
{
	for (int a = 0; a < 3; a++)
	{
		for (int j = 0; j < steps; j++)
		{
			step_axis(m, count, a);
		}
	}
	int64_t energy = 0;
	for (size_t i = 0; i < count; i++)
	{
		int64_t pot = 0;
		int64_t kin = 0;
		for (int a = 0; a < 3; a++)
		{
			pot += m[i].pos[a] > 0 ? m[i].pos[a] : -m[i].pos[a];
			kin += m[i].vel[a] > 0 ? m[i].vel[a] : -m[i].vel[a];
		}
		energy += pot * kin;
	}
	return energy;
}

int64_t repeat_axis(struct moon *m, size_t count, int axis)
{
	int64_t *pos = malloc(2 * count *sizeof(*pos));
	int64_t *vel = pos + count;
	for (size_t i = 0; i < count; i++)
	{
		pos[i] = m[i].pos[axis];
		vel[i] = m[i].vel[axis];
	}

	int64_t rep = 0;
	for (;;)
	{
		step_axis(m, count, axis);
		rep++;
		size_t i;
		for (i = 0; i < count; i++)
		{
			if (m[i].pos[axis] != pos[i] || m[i].vel[axis] != vel[i])
			{
				break;
			}
		}
		if (i == count)
		{
			break;
		}
	}
	free(pos);
	return rep;
}

int64_t gcd(int64_t a, int64_t b)
{
	while (b)
	{
		int t = b;
		b = a % b;
		a = t;
	}
	return a;
}

int64_t lcm(int64_t a, int64_t b)
{
	return a*b / gcd(a, b);
}

int64_t repeat(struct moon *m, size_t count)
{
	int64_t x = repeat_axis(m, count, 0);
	int64_t y = repeat_axis(m, count, 1);
	int64_t z = repeat_axis(m, count, 2);
	return lcm(lcm(x, y), z);
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

	size_t count;
	struct moon *moons = load_moons(input, &count);
	fclose(input);
	printf("part1: %ld\n", energy(moons, count, 1000));
	printf("part2: %ld\n", repeat(moons, count));
	free(moons);
	return 0;
}
