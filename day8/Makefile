CFLAGS=-Wall -g -ggdb

.PHONY: clean all

all: day8

day8: day8.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -f *.o day8
