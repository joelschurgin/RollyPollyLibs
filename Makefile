.PHONY: all base disasm misty ladybugger moonfruit graphics gooey_tui test

all: base disasm misty ladybugger moonfruit graphics gooey_tui test

base:
	./Base/build.sh

disasm:
	./Disasm/build.sh

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
