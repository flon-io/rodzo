
default: build

.DEFAULT:
	cd tmp/ && $(MAKE) $@

spec:
	cd tmp/ && $(MAKE) spec

.PHONY: clean spec

clean:
	rm -f tmp/*.o tmp/*.so tmp/z.c

