
CFLAGS=-I../src -g -Wall -O3
LDLIBS=
CC=c99
VPATH=../src

RODZO=$(shell which rodzo)
ifeq ($(RODZO),)
  RODZO=../../bin/rodzo
endif


s.c: ../spec/*_spec.c
	$(RODZO) -d $(SPECS) -o s.c

s: $(NAME).o

spec: s
	time ./s
	@echo "[31m"
	-diff -u expected_pseudo.txt spec_pseudo.txt
	@echo "[0m"

vspec: s
	valgrind --leak-check=full -v ./s

clean:
	rm -f *.o *.so *.c s

.PHONY: spec vspec clean

