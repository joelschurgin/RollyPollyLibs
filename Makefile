.PHONY: all base misty test clean

#all: base misty moonfruit test
all: base moonfruit

base:
	./Base/build.sh

misty:
	./MistyMountainParser/build.sh

moonfruit:
	./MoonFruitMacros/build.sh

graphics:
	./Graphics/build.sh

test:
	./test/build.sh all

clean:
	rm -rf build
