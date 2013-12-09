
CFLAGS=-fPIC -O3 -g -Wall -Werror
LDLIBS=
CC=c99
NAME=mnemo

SRC=../src
VPATH=$(SRC)


build: lib
	cp $(SRC)/$(NAME).h .

lib: lib$(NAME).so

lib$(NAME).so: $(NAME).o
	$(CC) -shared -Wl,-soname,lib$(NAME).so $^ -o $@

spec: lib
    #
	# TODO: stop using -r
    #
	../../bin/rodzo -r ../spec

