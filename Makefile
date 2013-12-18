
default: build

.DEFAULT:
	cd tmp/ && $(MAKE) $@

T=0
#
# "make test T=1" can be used too
#
test:
	$(MAKE) && $(MAKE) -C test$(T) spec
vtest:
	$(MAKE) -C test$(T) vspec

TS=$(shell ls -1 | grep test.)
#
clean:
	rm -f src/header.c
	rm -f tmp/*.o tmp/rodzo tmp/pfize
	rm -f bin/rodzo
	$(foreach t, $(TS), $(MAKE) -C $(t) clean;)

.PHONY: clean test vtest

