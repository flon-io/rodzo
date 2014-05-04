
default: build

.DEFAULT:
	$(MAKE) -C tmp/ $@

spec:
	$(MAKE) -C tmp/ spec

.PHONY: clean spec

clean:
	rm -f tmp/*.o tmp/*.so tmp/*.c tmp/mnemo.h tmp/a.out

