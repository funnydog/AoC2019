CFLAGS=-Wall -g -ggdb

.PHONY: clean all

all: day9

day9: day9.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -f *.o day9
