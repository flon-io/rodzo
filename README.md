
# rodzo

Where I come from "rodzo" means "red". Rodzo is a [rspec](http://rspec.info) inspired tool for, well, C.

Only tested on Debian GNU/Linux (7). I'll ready it for OSX later on (have to upgrade my 2008 macbook somehow).

Rodzo is mostly a pre-preprocessor (bin/rodzo) that turns a set of _spec.c files into a single .c file ready to compile. It's probably best to use rodzo in conjunction with GNU Make to have a very short spec / run (red) / implement / run (green) loop.


## similar projects

* [cspec](https://github.com/arnaudbrejeon/cspec)
* [cspec](https://github.com/nebularis/cspec)


## usage

Under [test4/](test4) is a vanilla project with rodzo included in its scaffold.

```
$ tree test4/
test4
├── Makefile
├── README.md
├── spec
│   └── str_spec.c
├── src
│   ├── flutil.c
│   └── flutil.h
└── tmp
    └── Makefile

    3 directories, 6 files
```

test4/Makefile:

```make
NAME=flutil

default: $(NAME).o

.DEFAULT spec clean:
        $(MAKE) -C tmp/ $@ NAME=$(NAME)

        .PHONY: spec clean
```

test4/tmp/Makefile:

```make
CFLAGS=-I../src -g -Wall -O3
LDLIBS=
CC=c99
VPATH=../src

RODZO=$(shell which rodzo)
ifeq ($(RODZO),)
  RODZO=../../bin/rodzo
endif


s.c: ../spec/*_spec.c
	$(RODZO) ../spec -o s.c

s: $(NAME).o

spec: s
	time ./s

vspec: s
	valgrind --leak-check=full -v ./s

clean:
	rm -f *.o *.so *.c s

.PHONY: spec vspec clean
```

Running ```make spec``` from test4/ should yield something like:

<img src="doc/output0.png" />

### specifying lines with L=

TODO

### specifying a pattern with E=

TODO


## License

MIT (see [LICENSE.txt](LICENSE.txt))

