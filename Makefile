
default: build

.DEFAULT:
	cd tmp/ && $(MAKE) $@

test:
	cd test/ && $(MAKE) spec


.PHONY: clean test

clean:
	rm -f src/header.c
	rm -f tmp/*.o tmp/rodzo tmp/pfize
	rm -f bin/rodzo
	cd test/ && $(MAKE) clean

