
default: build

.DEFAULT:
	$(MAKE) -C tmp/ $@

T=0
#
# "make test T=1" can be used too
#
test:
	$(MAKE) && $(MAKE) -C test$(T) spec
vtest:
	$(MAKE) && $(MAKE) -C test$(T) vspec

TS=$(shell ls -1 | grep test.)
#
clean:
	rm -f src/header.c
	rm -f tmp/*.o tmp/rodzo tmp/pfize
	rm -f bin/rodzo
	$(foreach t, $(TS), $(MAKE) -C $(t) clean;)

# copy updated version of dep libs into src/
#
upgrade:
	cp -v ../flutil/src/flutil.* src/

.PHONY: clean test vtest upgrade

