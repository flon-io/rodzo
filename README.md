
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

tspec: s
	strace -r ./s

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

      //ensure(strcmp("brown fox", s) == 0);
      ensure(s === "brown fox");
    }

    it "doesn't trim when not necessary"
    {
      s = flu_strrtrim("");

      //ensure(strcmp("", s) == 0);
      ensure(s === "");
    }
  }
}
```
(note: before each and after each get inlined in the examples).

Running ```make spec``` from test4/ should yield something like:

<src="doc/output0.png" />

### specifying lines with L=

When one wants to only run one example, it's OK to add ```L={lnumber}``` when running the specs:

```
$ make spec L=45
```

It's OK to pass multiple line numbers, separated by commas:

```
$ make spec L=45,72
```

The line numbers are the ones given by the coloured output of rodzo. If one has more than 1 file, line numbers for the second file will start at line_count(first file)...

Pointing to the line number directly to a ```describe``` or to a ```context``` will run all the examples in that branch.

### specifying a pattern with E=

The E command line argument is used to run any branch/example that contains the given string.

For example:
```
$ make spec E=string
```
will run all the ```describe```, ```context``` or ```it``` whose text contains the string "string".

The pattern is a plain pattern, no fancy regular expression matching.

If a ```describe``` or a ```context``` matches, all the examples in its branch will be run.

### specifying a set of spec files to run with F=

By default, rodzo is run against a whole ../spec dir. The F makefile env variable can be used to declare what spec files to take into account.
```
$ make spec F=strings_spec.c
```

It's OK to pass multiple files, thanks to double quotes:
```
$ make spec F="strings_spec.c int_*_spec.c"
```

(Yes, classical file globbing is OK).

### specifying an example (it) to run with I=

Sometimes, one gets stuck with a segfault in some piece of code. Running with Valgrind (see below) indicates that, the error occurs in `it_6 (s.c:640)`. That points to an example automatically numbered `6`. There is no easy way to infer a line number or an example text to run just that example, so rodzo lets one ask for it directly:

```
$ make spec I=6
```

and only that example will get run.


### running with Valgrind (vspec)

As seen in [test4/tmp/Makefile](test4/tmp/Makefile) there is a ```vspec``` target. It's meant for running the specs with Valgrind as the host.

Extra care has been taken for rodzo to generate a spec infrastructure which Valgrind flags as "0 leaks 0 errors" so that one can focus on cleaning leaks and errors from his code.

Beware leaks introduced by the spec themselves, generating a string, comparing it with some string literal, then not freeing the generated string... ```before each``` and ```after each``` can help in those cases, to prevent cluttering the specs with free() calls.

Another tool for dealing with memory leaks is [===f / ===F](#f-and-f). When comparing strings it tells rodzo to free the left side (f) or both (F) after comparison. That may spare a few lines.

### running with -d

When running rodzo with `-d`, two files are emitted along the spec source file and its compiled executable, those two files are `spec_tree.txt` and `spec_pseudo.txt`. They both represent the tree of spec as seen by rodzo. The tree one is very detailed, with line numbers and levels, while the second one is a rendition of the spec in pseudo rodzo spec idiom.

These `-d` files are used for debugging / developping rodzo.


## Writing specs

Since rodzo follows [rspec](http://rspec.info) for many things, a person used to write rspec specs should easily grasp rodzo specs.

A single test is encapsulated in an "example", it's a block of C code introduced by an "it" and a description:

```c
  it "trims on the right"
  {
    ensure(flu_strrtrim("brown fox \n\t") === "brown fox");
  }
```

One could chain the "its" and be done, testing every relevant aspect of his piece of software, but introducing hierarchy does help the author and the readers (of the specs and of their output).

As seen, spec leaves are introduced by "it". Branches are created thanks to "describe" and "context". From the point of view of rodzo, they are equivalent. What matters is how it is read, by humans.

In the following piece of spec, the str functions are gathered in a "context" and each function gets a "describe". Key aspects of each functions are challenged via an "it" example:
```c
#include "flutil.h"

context "str functions"
{
  describe "flu_strrtrim(char *s)"
  {
    it "trims on the right"
    {
      ensure(flu_strrtrim("brown fox \n\t") === "brown fox");
    }

    it "doesn't trim when not necessary"
    {
      ensure(flu_strrtrim("") === "");
    }
  }
}
```

This hierachy is followed when the specs are run and their output is presented:
<img src="doc/output0.png" />

### pending

An example (`it`) without a body will be considered pending.
Replacing `it` with `pending` will mark the example as pending.
Inserting a `pending();` in the body of an example will flag the example as pending.

Pending example are not run, they're simply reported as "pending" (in yellow).

These are all pending:
```c
  it "accepts pending stuff" // not yet implemented

  pending "accepts pending nodes" // no reason given
  {
    printf("whatever...\n");
  }

  pending "accepts pending nodes without body" // no reason given

  it "accepts explicitely pending stuff" // no reason given
  {
    pending();
  }

  it "accepts explicitely pending stuff with text" // no time yet
  {
    pending("no time yet");
  }
```
In comment are the pending texts derived in each situation.

### ensure

"ensure" is a pseudo-function that wraps a boolean condition. When the spec is run, the condition must return a true value for the ensure to be considered green.

```c
  it "checks various things"
  {
    ensure(1 + 1 == 2);
    ensure(1 + 3 != 2);
    ensure(strlen("petit bateau") == 12);
    // ...
  }
```

### ensure and ===

To compare two strings, it's easy to write:
```c
  it "picks the right animal"
  {
    char *s = pick_animal("grey", "africa");

    ensure(strcmp(s, "elephant") == 0);
  }
```

There is a "===" string-friendly construct available within "ensure":
```c
    ensure(s === "elephant");
```

This construct considers its left-side element as the "computed" value and the right-side one as the "expected" value. It'll deliver a nicely formatted error message in case of mismatch.

Ensure understands "!==" as well:
```c
    ensure(s !== "parrot");
```

### ===i

Same as `===` but the case is ignored. `===if` and `===iF` are OK.


### ~==

Match with a regular expression:

```c
    ensure(s ~== "^[Pp]arrots?$");
    ensure(s ~==i "^parrots?$"); // case insensitive
```

### ^== and $==

"starts with" and "ends with" respectively.

```c
    ensure(s ^== "this is "); // s should start with "this is"
    ensure(s $== ". That's it."); // s should end with ". That's it"
```

```^==i``` and ```$==i``` work as expected, case insensitively.

### >==

Means "contains". The text will be succesful if the string on the left contains the one on the right.

```c
    expect(s >== "Parrot");
    expect(s >==i "parrot"); // case insensitive
```

### ===f and ===F

When testing functions that return newly allocated strings, it's advantageous to immediately free the returned value.

```c
  describe "smult(char *text, size_t count)"
  {
    it "multiplies the text"
    {
      char *s = smult("ab", 3);
      ensure(s === "ababab");
      free(s);

      s = smult("", 3);
      ensure(s === "");
      free(s);

      s = smult("ab", 0);
      ensure(s === "");
      free(s);

      s = smult("ab", -1);
      ensure(s === "");
      free(s);
    }
  }
```

```===f``` says roughly "compare and free the computed value (left) before returning". It lets us shrink the above to:

```c
  describe "smult(char *text, size_t count)"
  {
    it "multiplies the text"
    {
      ensure(smult("ab", 3) ===f "ababab");
      ensure(smult("", 3) ===f "");
      ensure(smult("ab", 0) ===f "");
      ensure(smult("ab", -1) ===f "");
    }
  }
```

```===F``` like ```===f``` frees the left char array, but also frees the right one.

```~==f```, ```^==f``` and ```$==f``` work as expected.

### ensure (expect) and {printf-format}===

Usually, one writes an expectation like:

```
  expect(add_one(2) == 3);
```

It works fine if the expectation is fulfilled, but in case of failure, it just says "failed".

If one needs to see the expected value and the resulting value, one can "help" rodzo and pass it a printf format before the `===` (or `!==`).

```
  expect(add_one(2) i== 3);
```

Accepted "formats" are `c`, `d`, `e`, `f`, `o`, `i`, `li`, `lli`, `u`, `zu`, `zd`, `lu` and `llu`.

Beware `d` isn't "double" but "decimal".

### string comparisons / checks and RDZ_HEXDUMP

Sometimes, string comparisons seem to go wrong. It's OK to turn RDZ_HEXDUMP on to go a char by char comparison of the two strings.

```
RDZ_HEXDUMP=1 make spec
```

would yield something ressembling:

<img src="doc/hexdump.png" />

### before and after

Those two block markers let one run code before or after examples (```it``` blocks). It is very important to understand the difference between the "all" and the "each" (and "each offline") flavours.

Those blocks can be placed at any level. It is common to place more generic ones near the root of the spec tree and ones specifics to some examples along with them wrapped in a describe/context.

The code placed in an offline block ("{before|after} each offline" or "{before|after} all") is wrapped in a function that is called at the appropriate moment, it thus runs in its own function scope. The code is an inline block ("{before|after} each") runs in the example's function scope.

### before all / after all

```before all``` and ```after all``` are turned into functions by rodzo and those functions are called before

```c
  describe "smult(char *text, size_t count)"
  {
    before all
    {
      printf("enter smult...");
    }
    after all
    {
      printf("exeunt.");
    }
    it "multiplies the text (0)"
    {
      ensure(smult("ab", 3) ===f "ababab");
    }
    it "multiplies the text (1)"
    {
      ensure(smult("", 3) ===f "");
    }
  }
```

From the code above, the resulting spec will call the ```before all``` function before running the examples and then will call the ```after all``` after the two examples.

### before each / after each

Whereas ```before all``` and ```after all``` wrap a whole set of examples, ```before each``` and ```after each``` are repeated before and after each of the examples at the level they are set (or at the below levels).

Whereas ```{before|after} all``` is turned into a function and called, ```{before|after} each``` is inlined at the beginning or the end respectively of each example.

Thus
```c
  before each
  {
    printf("before each\n");
  }
  after each
  {
    printf("after each\n");
  }
  describe "the flux capacitator"
  {
    it "flips burgers"
    {
      ensure(1 == 2);
    }
  }
```

becomes
```c
  // describe "the flux capacitator" li39

    // it "flips burgers" li41
    //
    int it_10()
    {
      // before each li23
    printf("before each\n");

      char *msg43 = NULL;
      int r43 = (1 == 2);
        rdz_record(r43, msg43, 10, 43, 43); if ( ! r43) goto _over;

    _over:

      // after each li27
    printf("after each\n");

      return 1;
    } // it_10()
```

### before each / after each offline

```before each``` and ```after each``` are inlined within the examples (the ```it```) they wrap. When flagged with ```offline```, the block instead becomes a function that is called before/after the example.

As a consequence, all the matching ```before each offline``` are called before the (inlined) ```before each``` are run. Same for the ```after each offline```, tey get called after any ```after each```.

Remember, offline scope is not the same as inline scope.

## How it works

Rodzo is an executable (single-file) that reads the _spec.c files it gets pointed at and generates a single .c file that is (hopefully) compilable.

Thus something like
```c
  it "compares strings 1"
  {
    ensure(mne_tos(1) === "i");
  }
```
gets turned in the generated file to
```c
  // it "compares strings 1" li45
  //
  int it_15()
  {
    char *msg47 = NULL;
    char *result47 = (mne_tos(1));
    char *expected47 = "i";
    msg47 = rdz_compare_strings(result47, expected47);
    int r47 = (msg47 == NULL);
      rdz_record(r47, msg47, 15, 47, 77); if ( ! r47) goto _over;

  _over:

    return 1;
  } // it_15()
```

Rodzo takes care to place on top of the generated spec file all the rdz_ methods necessary for tracking the spec run.

The rodzo executable only does that. The rest of the work is done thanks to the Makefile.


## License

MIT (see [LICENSE.txt](LICENSE.txt))

