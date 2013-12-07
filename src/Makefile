
CFLAGS=-g -Wall -O3
LDLIBS=
CC=c99

build: rodzo
	mv rodzo ../

rodzo: rodzo.o
rodzo.o: rodzo.c header.c
	$(CC) $(CFLAGS) -o $@ -c $<

header.c: pfize header_src.c
	./pfize print_header header_src.c > header.c

pfize: pfize.o

