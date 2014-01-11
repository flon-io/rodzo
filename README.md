
# rodzo

Where I come from "rodzo" means "red". Rodzo is a [rspec](http://rspec.info) inspired tool for, well, C.

Only tested on Debian GNU/Linux (7). I'll ready it for OSX later on (have to upgrade my 2008 macbook somehow).

Rodzo is mostly a pre-preprocessor (bin/rodzo) that turns a set of _spec.c files into a single .c file ready to compile. It's probably best to use rodzo in conjunction with GNU Make to have a very short spec / run (red) / implement / run (green) loop.

This readme is split between [Usage](#usage), [Writing specs](#writing-specs), and [How it works](#how-it-works).

### Similar projects

* [https://github.com/arnaudbrejeon/cspec](https://github.com/arnaudbrejeon/cspec)
* [https://github.com/nebularis/cspec](https://github.com/nebularis/cspec)
* [https://github.com/ViliusLuneckas/CSpec](https://github.com/ViliusLuneckas/CSpec)


## Usage

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

A [spec file](test4/spec/str_spec.c) looks like:

```c
#include "flutil.h"

context "str functions"
{
  before each
  {
    char *s = NULL;
  }
  after each
  {
    if (s != NULL) free(s);
  }

  describe "flu_strrtrim(char *s)"
  {
    it "trims on the right"
    {
      s = flu_strrtrim("brown fox \n\t");

      ensure(strcmp("brown fox", s) == 0);
    }

    it "doesn't trim when not necessary"
    {
      s = flu_strrtrim("");

      ensure(strcmp("", s) == 0);
    }
  }
}
```
(note: before each and after each get inlined in the examples).

Running ```make spec``` from test4/ should yield something like:

<img src="doc/output0.png" />

### specifying lines with L=

TODO

### specifying a pattern with E=

TODO

### running with Valgrind (vspec)

As seen in [test4/tmp/Makefile](test4/tmp/Makefile) there is a ```vspec``` target. It's meant for running the specs with Valgrind as the host.

Extra care has been taken for rodzo to generate a spec infrastructure which Valgrind flags as "0 leaks 0 errors" so that one can focus on cleaning leaks and errors from his code.

Beware leaks introduced by the spec themselves, generating a string, comparing it with some string literal, then not freeing the generated string... ```before each``` and ```after each``` can help in those cases, to prevent cluttering the specs with free() calls.


## Writing specs

TODO

### ensure
### ensure and ===
### before all / after all
### before each / after each
### before each / after each offline


## How it works

TODO


## License

MIT (see [LICENSE.txt](LICENSE.txt))

