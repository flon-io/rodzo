
CFLAGS=-fPIC -O3 -g -Wall -Werror
LDLIBS=
CC=c99
NAME=mnemo

SRC=../src
VPATH=$(SRC)


build: lib$(NAME).so
	cp $(SRC)/$(NAME).h .

lib$(NAME).so: $(NAME).o
	$(CC) -shared -Wl,-soname,lib$(NAME).so $^ -o $@

spec.c:
	../../bin/rodzo -d ../spec

build-spec: spec.c
	$(CC) -L. -lmnemo $<

spec: build build-spec
	LD_LIBRARY_PATH=. time ./a.out
	LD_LIBRARY_PATH=. ./a.out > spec_out.txt
	@echo "[31m"
	-diff -u expected_out.txt spec_out.txt
	-diff -u expected_pseudo.txt spec_pseudo.txt
	@echo "[0m"

vspec: build build-spec
	LD_LIBRARY_PATH=. valgrind --leak-check=full -v ./a.out

# this target is for the rodzo-development only
#
rvspec: lib$(NAME).so
	valgrind --leak-check=full -v ../../bin/rodzo ../spec

