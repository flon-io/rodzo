
default: build

.DEFAULT:
	cd tmp/ && $(MAKE) $@

V=0
#
# "make test V=1" can be used too
#
test:
	cd test$(V)/ && $(MAKE) spec
vtest:
	cd test$(V)/ && $(MAKE) vspec


.PHONY: clean test vtest

clean:
	rm -f src/header.c
	rm -f tmp/*.o tmp/rodzo tmp/pfize
	rm -f bin/rodzo
	cd test/ && $(MAKE) clean

