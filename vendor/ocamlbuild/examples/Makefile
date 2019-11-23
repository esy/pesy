.PHONY: all test clean

all: 	
	for d in [0-9]*-*; do $(MAKE) -C $$d all; done

test:
	for d in [0-9]*-*; do $(MAKE) -C $$d test; done

clean:
	for d in [0-9]*-*; do $(MAKE) -C $$d clean; done
