#!/bin/bash
set -e

BASE="$(realpath ../Base)"

CC="gcc"
MISTY_CFLAGS="-I$BASE -std=c99 -D_POSIX_C_SOURCE=200112L -Wextra -Werror -Wimplicit-function-declaration -nostdlib -nostartfiles -g -DBUILD_DEBUG"
TEST_CFLAGS="-I$BASE -std=c99 -D_POSIX_C_SOURCE=200112L -nostartfiles -g"
LDFLAGS="-L$BASE/build -lbase -Wl,-e,entry_point -lpthread"

mkdir -p build
cd build

echo "Building misty library..."
$CC -c ../misty.c -o misty.o $MISTY_CFLAGS
ar rcs libmisty.a misty.o

echo "Building misty_test..."
$CC ../misty_test.c -L. -lmisty -o misty_test $TEST_CFLAGS $LDFLAGS

echo "Build complete: build/misty_test"
