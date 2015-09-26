
default: tests

.DEFAULT:
	$(MAKE) -C tmp/ $@


T=0
#
# "make test T=1" can be used too
#
test: build
	$(MAKE) -C test$(T) spec
vtest: build
	$(MAKE) -C test$(T) vspec

TS=$(shell ls -1 | grep test.)

tests: build
	$(foreach t, $(TS), $(MAKE) -C $(t) spec;)
vtests: build
	$(foreach t, $(TS), $(MAKE) -C $(t) vspec;)

rtest: build
	bin/rodzo --test

clean:
	rm -f src/header.c
	rm -f tmp/*.o tmp/rodzo tmp/pfize
	rm -f bin/rodzo
	$(foreach t, $(TS), $(MAKE) -C $(t) clean;)

# copy up-to-date versions of dep libs into src/
#
upgrade:
	cp -v ../flutil/src/flutil.[ch] src/

.PHONY: clean test vtest upgrade

