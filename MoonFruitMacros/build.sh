#!/bin/bash
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

BASE="$(realpath ../Base)"
GOOEY_TUI="$(realpath ../GooeyTui)"
LIB="$(realpath ../build/lib)"

CC="gcc"
MOONFRUIT_CFLAGS="-I$BASE -I$GOOEY_TUI -std=c99 -D_POSIX_C_SOURCE=200112L -Wno-initializer-overrides -Werror -Wimplicit-function-declaration -nostdlib -nostartfiles -g3 -DBUILD_DEBUG"
MOONFRUIT_MAIN_CFLAGS="-I$BASE -I$GOOEY_TUI -std=c99 -D_POSIX_C_SOURCE=200112L -nostartfiles -g3"
LDFLAGS="-L$LIB -lbase -lgooey_tui -Wl,-e,entry_point -lpthread"

echo "Building moonfruit library..."
$CC -c moonfruit.c -o moonfruit.o $MOONFRUIT_CFLAGS
ar rcs libmoonfruit.a moonfruit.o

echo -e "=> \033[32mBUILD COMPLETE: libmoonfruit.a\033[0m"

echo "Building moonfruit..."
$CC moonfruit_main.c -L. -lmoonfruit -o moonfruit $MOONFRUIT_MAIN_CFLAGS $LDFLAGS

mkdir -p ../build/lib
rm moonfruit.o
mv libmoonfruit.a ../build/lib/libmoonfruit.a

mkdir -p ../build
mv moonfruit ../build/moonfruit

echo -e "=> \033[32mBUILD COMPLETE: moonfruit\033[0m"
