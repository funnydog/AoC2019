#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
	BLACK = '0',
	WHITE = '1',
	TRANS = '2',

	WIDTH = 25,
	HEIGHT = 6,
};

static unsigned layer_count(size_t s)
{
	return s / WIDTH / HEIGHT;
}

static unsigned count_digits(const char *seq, unsigned layer, int digit)
{
	unsigned count = 0;
	for (unsigned j = layer * WIDTH * HEIGHT, n = j + WIDTH * HEIGHT;
	     j < n;
	     j++)
	{
		if (seq[j] == digit)
		{
			count++;
		}
	}
	return count;
}

static unsigned min_layer(const char *seq, size_t s, int digit)
{
	unsigned minc = WIDTH * HEIGHT;
	unsigned minj = UINT_MAX;
	for (unsigned j = 0, n = layer_count(s); j < n; j++)
	{
		unsigned c = count_digits(seq, j, digit);
		if (minc > c)
		{
			minc = c;
			minj = j;
		}
	}
	return minj;
}

static void flatten(char *seq, size_t s)
{
	char *dst = seq;
	char *src = seq + WIDTH * HEIGHT;
	s -= WIDTH * HEIGHT;
	while (s-->0)
	{
		if (dst == seq + WIDTH * HEIGHT)
		{
			dst = seq;
		}
		if (*dst == TRANS)
		{
			*dst = *src;
		}
		dst++;
		src++;
	}
}

static void print(const char *seq)
{
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			switch(*seq++)
			{
			case BLACK: printf(" "); break;
			case WHITE: printf("#"); break;
			default: printf("."); break;
			}
		}
		printf("\n");
	}
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

	char *buf = NULL;
	size_t bsize = 0;
	while (getline(&buf, &bsize, input) != -1)
	{
	}
	fclose(input);

	size_t size = strlen(buf);
	unsigned minj = min_layer(buf, size, BLACK);
	if (minj != UINT_MAX)
	{
		printf("part1: %u\n",
		       count_digits(buf, minj, WHITE) *
		       count_digits(buf, minj, TRANS));
	}

	flatten(buf, size);
	printf("part2:\n");
	print(buf);

	free(buf);
	return 0;
}
