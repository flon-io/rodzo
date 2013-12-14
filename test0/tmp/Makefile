
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
	../../bin/rodzo ../spec

build-spec: spec.c
	$(CC) -L. -lmnemo $<

spec: build build-spec
	LD_LIBRARY_PATH=. time ./a.out

vspec: build build-spec
	LD_LIBRARY_PATH=. valgrind --leak-check=full -v ./a.out

# this target is for the rodzo-development only
#
rvspec: lib
	valgrind --leak-check=full -v ../../bin/rodzo -o spec2.c ../spec

