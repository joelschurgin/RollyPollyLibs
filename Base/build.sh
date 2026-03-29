#!/bin/bash
set -e

CC="gcc"
BASE_CFLAGS="-std=c99 -D_POSIX_C_SOURCE=200112L -Wextra -Werror -Wimplicit-function-declaration -nostdlib -nostartfiles -g -DBUILD_DEBUG"
TEST_CFLAGS="-std=c99 -D_POSIX_C_SOURCE=200112L -nostartfiles -g"
LDFLAGS="-Wl,-e,entry_point -lpthread"

mkdir -p build
cd build

echo "Building base library..."
$CC -c ../base.c -o base.o $BASE_CFLAGS
ar rcs libbase.a base.o

echo "Building base_test..."
$CC ../base_test.c -L. -lbase -o base_test $TEST_CFLAGS $LDFLAGS

echo "Build complete: build/base_test"
