
CFLAGS=-g -Wall -O3
LDLIBS=flutil.o
CC=c99

SRC=../src
VPATH=$(SRC)


build: rodzo
	cp rodzo ../bin/

rodzo.o: rodzo.c header.c flutil.o

header.c: pfize header_src.c
	./pfize print_header $(SRC)/header_src.c > $(SRC)/header.c

# pfize gets its own rule to avoid flutil.o in the $(LDLIBS)
#
pfize: pfize.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: build

