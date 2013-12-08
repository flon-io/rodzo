
CFLAGS=-g -Wall -O3
LDLIBS=
CC=c99

SRC=../src
VPATH=$(SRC)

build: rodzo
	cp rodzo ../bin/

rodzo: rodzo.o
rodzo.o: rodzo.c header.c

header.c: pfize header_src.c
	./pfize print_header $(SRC)/header_src.c > $(SRC)/header.c

pfize: pfize.o

