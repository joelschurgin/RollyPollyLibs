.PHONY: all base disasm misty ladybugger moonfruit graphics gooey_tui test

all: base disasm misty ladybugger gooey_tui moonfruit graphics test

base:
	./Base/build.sh

disasm: base
	./Disasm/build.sh

misty: base
	./MistyMountainParser/build.sh

ladybugger: base disasm misty
	./Ladybugger/build.sh

moonfruit: base gooey_tui
	./MoonFruitMacros/build.sh

graphics: base
	./Graphics/build.sh

gooey_tui: base
	./GooeyTui/build.sh

test:
	./test/build.sh all

clean:
	rm -rf build
