#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

long fuel(long mass)
{
	return mass / 3 - 2;
}

long recursive_fuel(long f)
{
	long total = 0;
	while ((f = fuel(f)) > 0)
	{
		total += f;
	}
	return total;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: %s <filename>\n", argv[0]);
		return -1;
	}

	/* load the input */
	FILE *input = fopen(argv[1], "rb");
	if (!input)
	{
		fprintf(stderr, "file %s not found\n", argv[1]);
		return -1;
	}

	long *array = NULL;
	size_t acount = 0;
	size_t asize = 0;

	char *buffer = NULL;
	size_t bsize = 0;
	while (getline(&buffer, &bsize, input) >= 0)
	{
		if (acount == asize)
		{
			size_t newsize = asize ? asize * 2 : 32;
			long *newarray = realloc(array, newsize * sizeof(*newarray));
			if (!newarray)
			{
				fprintf(stderr, "Warning, OOM\n");
				break;
			}
			asize = newsize;
			array = newarray;
		}

		char *end;
		errno = 0;
		array[acount] = strtol(buffer, &end, 10);
		if (errno == 0 && buffer != end)
		{
			acount++;
		}
	}
	free(buffer);

	/* part 1 */
	assert(fuel(12) == 2);
	assert(fuel(14) == 2);
	assert(fuel(1969) == 654);
	assert(fuel(100756) == 33583);

	long total_fuel = 0;
	for (size_t i = 0; i < acount; i++)
	{
		total_fuel += fuel(array[i]);
	}
	printf("part 1: %ld\n", total_fuel);

	/* part 2 */
	assert(recursive_fuel(14) == 2);
	assert(recursive_fuel(1969) == 966);
	assert(recursive_fuel(100756) == 50346);

	total_fuel = 0;
	for (size_t i = 0; i < acount; i++)
	{
		total_fuel += recursive_fuel(array[i]);
	}
	printf("part 2: %ld\n", total_fuel);

	free(array);
	fclose(input);
	return 0;
}
