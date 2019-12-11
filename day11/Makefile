CFLAGS=-Wall -g -ggdb

.PHONY: clean all

all: day11

day11.c: queue.h

day11: day11.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -f *.o day11
