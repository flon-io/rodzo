
CFLAGS=-fPIC -O3 -g -Wall -Werror
LDLIBS=
CC=c99
NAME=mnemo

SRC=../src
VPATH=$(SRC)


build: $(NAME).o
	cp $(SRC)/$(NAME).h .

spec.c:
	../../bin/rodzo ../spec

build-spec: spec.c
	#$(CC) -I. $< $(NAME).o
	#$(CC) $< $(NAME).o

spec: build build-spec
	./a.out

vspec: build build-spec
	valgrind --leak-check=full -v ./a.out

# this target is for the rodzo-development only
#
rvspec: lib
	valgrind --leak-check=full -v ../../bin/rodzo -o spec2.c ../spec

