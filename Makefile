
default: build

.DEFAULT:
	cd src && $(MAKE) $@


.PHONY: clean

clean:
	rm -f src/*.o
	rm -f src/header.c
	rm -f src/pfize
	rm -f rodzo
	rm -f a.out
	rm -f z.c

