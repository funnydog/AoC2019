#include <assert.h>
#include <regex.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

enum
{
	CMD_NEW,
	CMD_CUT,
	CMD_INC,
};

struct command
{
	int type;
	int64_t val;

	struct command *next;
};

void command_free(struct command *lst)
{
	while (lst)
	{
		struct command *t = lst;
		lst = lst->next;
		free(t);
	}
}

struct command *command_new(int type, const char *value)
{
	struct command *c = malloc(sizeof(*c));
	if (c)
	{
		c->type = type;
		c->val = strtoll(value, NULL, 10);
		c->next = NULL;
	}
	return c;
}

struct command *command_load(FILE *input)
{
	/* compile the regular expressions */
	const char *cut_pattern = "cut (-?[0-9]+)";
	const char *new_pattern = "deal into new stack";
	const char *inc_pattern = "deal with increment (-?[0-9]+)";
	regex_t cut_regex, new_regex, inc_regex;
	if (regcomp(&cut_regex, cut_pattern, REG_EXTENDED))
	{
		return NULL;
	}
	if (regcomp(&new_regex, new_pattern, REG_EXTENDED))
	{
		regfree(&cut_regex);
		return NULL;
	}
	if (regcomp(&inc_regex, inc_pattern, REG_EXTENDED))
	{
		regfree(&new_regex);
		regfree(&cut_regex);
		return NULL;
	}

	struct command *head = NULL;
	struct command **tail = &head;
	char *line = NULL;
	size_t pline = 0;
	while (getline(&line, &pline, input) != -1)
	{
		struct command *c;
		regmatch_t match[2];
		if (!regexec(&cut_regex, line, 2, match, 0))
		{
			c = command_new(CMD_CUT, line+match[1].rm_so);
			*tail = c;
			tail = &c->next;
		}
		else if (!regexec(&new_regex, line, 2, match, 0))
		{
			c = command_new(CMD_NEW, "0");
			*tail = c;
			tail = &c->next;
		}
		else if (!regexec(&inc_regex, line, 2, match, 0))
		{
			c =  command_new(CMD_INC, line+match[1].rm_so);
			*tail = c;
			tail = &c->next;
		}
	}
	free(line);

	regfree(&inc_regex);
	regfree(&new_regex);
	regfree(&cut_regex);
	return head;
}

static uint64_t mulmod(uint64_t a, uint64_t b, uint64_t m)
{
#ifndef SLOW
	long double x;
	uint64_t c;
	int64_t r;
	a %= m;
	b %= m;
	x = a;
	c = x * b / m;
	r = (int64_t)(a * b - c * m) % m;
	return r < 0 ? r + m : r;
#else
	assert((a & 0x8000000000000000) == 0);
	assert((b & 0x8000000000000000) == 0);;
	a %= m;
	b %= m;
	int64_t result = 0;
	while (b > 0)
	{
		if (b & 1)
		{
			result = (result + a) % m;
		}
		b >>= 1;
		a = (a * 2) % m;
	}
	return result;
#endif
}

static uint64_t powmod(uint64_t base, uint64_t exponent, uint64_t modulus)
{
	assert((base     & 0x8000000000000000) == 0);
	assert((exponent & 0x8000000000000000) == 0);;
	if (modulus == 1)
	{
		return 0;
	}
	base %= modulus;
	int64_t result = 1;
	while (exponent > 0)
	{
		if (exponent & 1)
		{
			result = mulmod(result, base, modulus);
		}
		exponent >>= 1;
		base = mulmod(base, base, modulus);
	}
	return result;
}

struct deck
{
	uint64_t start;
	uint64_t period;
	uint64_t size;
};

static void deck_make(struct deck *deck, uint64_t size)
{
	deck->start = 0;
	deck->period = 1;
	deck->size = size;
}

static void deck_shuffle(struct deck *d, const struct command *cmd, uint64_t times)
{
	uint64_t s = d->start;
	uint64_t p = d->period;
	uint64_t size = d->size;
	for (; cmd; cmd = cmd->next)
	{
		switch (cmd->type)
		{
		case CMD_CUT:
			s = s + mulmod(size + cmd->val, p, size);
			s %= size;
			break;

		case CMD_NEW:
			p = size - p;
			s = (s + p) % size;
			break;

		case CMD_INC:
			p = mulmod(
				p,
				powmod(cmd->val, size-2, size),
				size
				);
			break;
		}
	}

	/* could skip if times == 1 but for simplicity we throw away
	 * some CPU cycles */
	s = mulmod(
		mulmod(
			s,
			(size + 1 - powmod(p, times, size)),
			size),
		powmod(size + 1 - p, size-2, size),
		size);
	p = powmod(p, times, size);

	d->start = s;
	d->period = p;
}

static uint64_t index_of(struct deck *deck, uint64_t value)
{
	uint64_t i;
	uint64_t start = deck->start;
	for (i = 0; start != value; i++)
	{
		start = (start + deck->period)  % deck->size;
	}
	return i;
}

static uint64_t value_of(struct deck *deck, uint64_t index)
{
	uint64_t start = deck->start;
	for (uint64_t i = 0; i < index; i++)
	{
		start = (start + deck->period) % deck->size;
	}
	return start;
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
	struct command *cmd_list = command_load(input);
	fclose(input);

	struct deck d;
	deck_make(&d, 10007);
	deck_shuffle(&d, cmd_list, 1);
	printf("part1: %lu\n", index_of(&d, 2019));

	deck_make(&d, 119315717514047);
	deck_shuffle(&d, cmd_list, 101741582076661);
	printf("part2: %lu\n", value_of(&d, 2020));

	command_free(cmd_list);
	return 0;
}
