
default: build

.DEFAULT:
	cd tmp/ && $(MAKE) $@

T=0
#
# "make test T=1" can be used too
#
test:
	cd test$(T)/ && $(MAKE) spec
vtest:
	cd test$(T)/ && $(MAKE) vspec

clean:
	rm -f src/header.c
	rm -f tmp/*.o tmp/rodzo tmp/pfize
	rm -f bin/rodzo
	cd test0/ && $(MAKE) clean
	cd test1/ && $(MAKE) clean

.PHONY: clean test vtest

