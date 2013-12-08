
default: build

.DEFAULT:
	cd tmp/ && $(MAKE) $@

#test:
#	cd test/ && $(MAKE) test


.PHONY: clean

clean:
	rm -f src/header.c
	rm -f tmp/*.o tmp/rodzo tmp/pfize
	rm -f bin/rodzo

