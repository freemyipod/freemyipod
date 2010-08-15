CC= gcc
OPTS= -Wall -g

all: extract2g

extract2g: extract2g.c extract2g.h
	$(CC) $(OPTS) $< -o $@

.PHONY: clean
clean:
	rm -f extract2g
