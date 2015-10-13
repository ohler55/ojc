
all:
	make -C src

clean:
	make -C src clean
	make -C test clean

test: all
	make -C test test
