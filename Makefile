all: base misty

base:
	./Base/build.sh

misty:
	./MistyMountainParser/build.sh

clean:
	rm -rf build
