#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

int rule2(const char *str)
{
	int p = 0;
	for (const char *s = str; *s; s++)
	{
		if (*s == p)
		{
			return 1;
		}
		p = *s;
	}
	return 0;
}

int rule3(const char *str)
{
	int p = 0;
	for (const char *s = str; *s; s++)
	{
		if (*s < p)
		{
			return 0;
		}
		p = *s;
	}
	return 1;
}

int rule4(const char *str)
{
	int t = INT_MAX;
	int p = 0;
	int c = 0;
	for (const char *s = str; *s; s++)
	{
		if (*s == p)
		{
			c++;
		}
		else if (c > 0 && c < t)
		{
			t = c;
			c = 0;
		}
		else
		{
			c = 0;
		}
		p = *s;
	}
	if (c > 0 && c < t)
	{
		t = c;
	}
	return t == 1;
}

int main(int argc, char *argv[])
{
	assert(rule2("111111") && rule3("111111"));
	assert(rule2("223450") && !rule3("223450"));
	assert(!rule2("123789") && rule3("123789"));
	assert(rule4("112233"));
	assert(!rule4("123444"));
	assert(rule4("111122"));

	char buf[10];
	int p1, p2;
	p1 = p2 = 0;
	for (int i = 109165; i < 576723+1; i++)
	{
		snprintf(buf, sizeof(buf), "%d", i);
		int r3 = rule3(buf);
		if (rule2(buf) && r3)
		{
			p1++;
		}
		if (r3 && rule4(buf))
		{
			p2++;
		}
	}
	printf("part1: %d\npart2: %d\n", p1, p2);
	return 0;
}
