#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int pattern[] = { 0, 1, 0, -1 };

static int get_sum(const char *signal, size_t slen, int i)
{
	int v = 0;
	size_t start = 0;
	while (start < slen)
	{
		int value = pattern[(start+1)/(i+1) % 4];
		size_t size = (i+1) - (start+1)%(i+1);
		if (value == 0)
		{
			/* nothing to do */
		}
		else if (value == 1)
		{
			for (size_t j = start; j < start+size && j < slen; j++)
			{
				v += signal[j] - '0';
			}
		}
		else if (value == -1)
		{
			for (size_t j = start; j < start+size && j < slen; j++)
			{
				v -= signal[j] - '0';
			}
		}
		start += size;
	}
	return (v > 0 ? v : -v) % 10;
}

static char *part1(char *txt, size_t len)
{
	for (int phase = 0; phase < 100; phase++)
	{
		for (size_t i = 0; i < len; i++)
		{
			/* we can reuse the same array since the
			 * stored value is never reused for the
			 * computation of the following element */
			txt[i] = '0' + get_sum(txt, len, i);
		}
	}
	/* add a terminator */
	txt[9] = 0;
	return txt;
}

static char *part2(char *txt, size_t len)
{
	size_t offset = 0;
	for (int i = 0; i < 7; i++)
	{
		offset = offset * 10 + txt[i] - '0';
	}
	/* this computation holds only for values at the second half
	 * of the signal */
	assert(offset > len / 2);

	size_t longlen = len * 10000 - offset;
	char *longtxt = malloc(longlen);
	for (size_t i = 0; i < longlen; i++)
	{
		longtxt[i] = txt[(offset + i) % len];
	}

	for (int phase = 0; phase < 100; phase++)
	{
		int sum = 0;
		for (int j = longlen-1; j >= 0; j--)
		{
			sum += longtxt[j] - '0';
			longtxt[j] = (sum > 0 ? sum : -sum) % 10 + '0';
		}
	}
	memcpy(txt, longtxt, 8);
	txt[9] = 0;
	free(longtxt);
	return txt;
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

	char *line = NULL;
	size_t sline = 0;
	if (getline(&line, &sline, input) == -1)
	{
		fprintf(stderr, "No input available\n");
		fclose(input);
		return -1;
	}

	size_t len = strlen(line)-1;
	/* chop the \n */
	line[len] = 0;
	printf("part1: %s\n", part1(line, len));

	rewind(input);
	getline(&line, &sline, input);
	line[len] = 0;

	printf("part2: %s\n", part2(line, len));
	free(line);
	fclose(input);
	return 0;
}
