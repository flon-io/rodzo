
CFLAGS=-I../src
LDLIBS=
CC=c99
VPATH=../src
NAME=aabro

RODZO=../../bin/rodzo


s.c: ../spec/*_spec.c
	$(RODZO) -d ../spec -o s.c

s: $(NAME).o

spec: s
	time ./s
	@echo "[31m"
	-diff -u expected_pseudo.txt spec_pseudo.txt
	@echo "[0m"

vspec: s
	valgrind --leak-check=full -v ./s

clean:
	rm -f *.o
	rm -f *.so
	rm -f *.c
	rm -f s

.PHONY: spec vspec clean

