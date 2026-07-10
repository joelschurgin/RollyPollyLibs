#!/bin/bash
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

BASE="$(realpath ../Base)"
LIB="$(realpath ../build/lib)"

CC="gcc"
GOOEY_TUI_CFLAGS="-I$BASE -std=c99 -D_POSIX_C_SOURCE=200112L -Wno-initializer-overrides -Werror -Wimplicit-function-declaration -nostdlib -nostartfiles -g3 -DBUILD_DEBUG"
TEST_CFLAGS="-I$BASE -std=c99 -D_POSIX_C_SOURCE=200112L -nostartfiles -g3"
LDFLAGS="-L$LIB -lbase -lm -Wl,-e,entry_point -lpthread"

echo "Building gooey_tui library..."
$CC -c gooey_tui.c -o gooey_tui.o $GOOEY_TUI_CFLAGS
ar rcs libgooey_tui.a gooey_tui.o 
echo -e "=> \033[32mBUILD COMPLETE: libgooey_tui.a\033[0m"

echo "Building gooey_tui_test..."
$CC gooey_tui_test.c -L. -lgooey_tui -o gooey_tui_test $TEST_CFLAGS $LDFLAGS

mkdir -p ../build/lib
rm gooey_tui.o 
mv libgooey_tui.a ../build/lib/libgooey_tui.a

mkdir -p ../build/tests
mv gooey_tui_test ../build/tests/gooey_tui_test

echo -e "=> \033[32mBUILD COMPLETE: gooey_tui_test\033[0m"
