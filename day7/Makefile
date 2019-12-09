CFLAGS=-Wall -g -ggdb

.PHONY: clean all

all: day7

day7: day7.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -f *.o day7
