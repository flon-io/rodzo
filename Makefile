
CC = clang

SRCS = src/flutil.c
OBJS := $(SRCS:.c=.o)

#.DEFAULT = bin/rodzo

$(OBJS):
	$(CC) $(CFLAGS) -c $< -o $@

all: bin/rodzo

tmp/pfize: src/pfize.c
	$(CC) -std=c11 -Wall -Wextra -O3 src/pfize.c -o tmp/pfize

tmp/header.c: tmp/pfize src/header_src.c
	./tmp/pfize print_header src/header_src.c > tmp/header.c

tmp/rodzo.c: src/rodzo.c tmp/header.c
	cat src/rodzo.c tmp/header.c > tmp/rodzo.c

bin/rodzo: tmp/rodzo.c $(OBJS)
	$(CC) \
      -std=c11 -Wall -Wextra -O0 -g -fno-omit-frame-pointer \
      -Isrc \
      $(OBJS) tmp/rodzo.c \
        -o bin/rodzo

clean:
	rm -f src/*.o
	rm -f tmp/pfize
	rm -f tmp/header.c
	rm -f bin/rodzo


.PHONY: clean

