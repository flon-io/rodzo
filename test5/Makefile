
NAME=flutil

default: $(NAME).o

SPECS=../spec

.DEFAULT spec clean:
	$(MAKE) -C tmp/ $@ NAME=$(NAME) SPECS="$(SPECS)"

.PHONY: spec clean

