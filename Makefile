
default: build

.DEFAULT:
	cd tmp/ && $(MAKE) $@

test:
	cd test/ && $(MAKE) spec
vtest:
	cd test/ && $(MAKE) vspec


.PHONY: clean test vtest

clean:
	rm -f src/header.c
	rm -f tmp/*.o tmp/rodzo tmp/pfize
	rm -f bin/rodzo
	cd test/ && $(MAKE) clean

