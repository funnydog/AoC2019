CFLAGS=-Wall -g -ggdb -O0

.PHONY: clean all

all: day6 day6b

day6: day6.o
	$(CC) $(CFLAGS) -o $@ $^

day6b: day6b.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -f *.o day6 day6b
