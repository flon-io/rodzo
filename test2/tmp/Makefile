
CFLAGS=-I../src
LDLIBS=
CC=c99
VPATH=../src
NAME=aabro

RODZO=../../bin/rodzo


s.c: ../spec/*_spec.c
	$(RODZO) ../spec -o s.c

s: $(NAME).o

spec: s
	time ./s

vspec: s
	valgrind --leak-check=full -v ./s

clean:
	rm -f *.o
	rm -f *.so
	rm -f *.c
	rm -f s

.PHONY: spec vspec clean

