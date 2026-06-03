#!/bin/bash
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

BASE="$(realpath ../Base)"
LIB="$(realpath ../build/lib)"

CC="gcc"
GRAPHICS_CFLAGS="-I$BASE -std=c99 -D_POSIX_C_SOURCE=200112L -Wextra -Werror -Wimplicit-function-declaration -nostdlib -nostartfiles -g3 -DBUILD_DEBUG"
TEST_CFLAGS="-I$BASE -std=c99 -D_POSIX_C_SOURCE=200112L -nostartfiles -g3"
#LDFLAGS="-L$LIB -lbase -Wl,-e,entry_point -lpthread -lGL -lEGL $(pkg-config --cflags --libs wayland-egl wayland-client)"
LDFLAGS="-L$LIB -lbase -Wl,-e,entry_point -lpthread -lGL -lEGL -lwayland-client -lwayland-egl"

echo "Building graphics library..."
$CC -c graphics.c -o graphics.o $GRAPHICS_CFLAGS
ar rcs libgraphics.a graphics.o 
echo -e "=> \033[32mBUILD COMPLETE: libgraphics.a\033[0m"

echo "Building graphics_test..."
$CC graphics_test.c -L. -lgraphics -o graphics_test $TEST_CFLAGS $LDFLAGS

mkdir -p ../build/lib
rm graphics.o 
mv libgraphics.a ../build/lib/libgraphics.a

mkdir -p ../build/tests
mv graphics_test ../build/tests/graphics_test

echo -e "=> \033[32mBUILD COMPLETE: graphics_test\033[0m"
