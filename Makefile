.PHONY: all base misty test clean

all: base misty test

base:
	./Base/build.sh

misty:
	./MistyMountainParser/build.sh

test:
	./test/build.sh all

clean:
	rm -rf build
