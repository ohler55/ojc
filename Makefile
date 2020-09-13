
all:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
	$(MAKE) -C test clean

test: all
	$(MAKE) -C test test
