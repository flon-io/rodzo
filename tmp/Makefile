
CFLAGS=-g -Wall -O3
LDLIBS=flutil.o aabro.o
CC=c99

SRC=../src
VPATH=$(SRC)


build: rodzo r2
	cp rodzo r2 ../bin/

rodzo.o: rodzo.c header.c flutil.o
r2.o: r2.c flutil.o aabro.o

header.c: pfize header_src.c
	./pfize print_header $(SRC)/header_src.c > $(SRC)/header.c

# pfize gets its own rule to avoid flutil.o in the $(LDLIBS)
#
pfize: pfize.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: build

