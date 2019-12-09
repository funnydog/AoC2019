CFLAGS=-g -ggdb -Wall

.PHONY: clean all

all: day4

day4: day4.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -f *.o day4
