
CFLAGS=-g -Wall -O3
LDLIBS=
CC=c99


rodzo: rodzo.o
rodzo.o: rodzo.c header.c
	$(CC) $(CFLAGS) -o $@ -c $<

header.c: pfize header_src.c
	./pfize print_header src/header_src.c > src/header.c

pfize: pfize.o


.PHONY: clean

clean:
	rm -f src/*.o src/*.so*
	rm -f r.c

