.PHONY: all base misty test clean

#all: base misty moonfruit graphics gooey_tui test
all: base misty ladybugger

base:
	./Base/build.sh

misty:
	./MistyMountainParser/build.sh

ladybugger:
	./Ladybugger/build.sh

moonfruit:
	./MoonFruitMacros/build.sh

graphics:
	./Graphics/build.sh

gooey_tui:
	./GooeyTui/build.sh

test:
	./test/build.sh all

clean:
	rm -rf build
