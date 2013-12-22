
NAME=aabro

default: $(NAME).o

.DEFAULT:
	$(MAKE) -C tmp/ $@

spec:
	$(MAKE) -C tmp/ spec
vspec:
	$(MAKE) -C tmp/ vspec

clean:
	$(MAKE) -C tmp/ clean

.PHONY: spec vspec clean

