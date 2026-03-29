#!/bin/bash
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

BASE="$(realpath ../Base)"
LIB="$(realpath ../build/lib)"

CC="gcc"
MISTY_CFLAGS="-I$BASE -std=c99 -D_POSIX_C_SOURCE=200112L -Wextra -Werror -Wimplicit-function-declaration -nostdlib -nostartfiles -g -DBUILD_DEBUG"
TEST_CFLAGS="-I$BASE -std=c99 -D_POSIX_C_SOURCE=200112L -nostartfiles -g"
LDFLAGS="-L$LIB -lbase -Wl,-e,entry_point -lpthread"

echo "Building misty library..."
$CC -c misty.c -o misty.o $MISTY_CFLAGS
ar rcs libmisty.a misty.o

echo "Building misty_test..."
$CC misty_test.c -L. -lmisty -o misty_test $TEST_CFLAGS $LDFLAGS

mkdir -p ../build/lib
rm misty.o
mv libmisty.a ../build/lib/libmisty.a

mkdir -p ../build/tests
mv misty_test ../build/tests/misty_test

echo "Build complete: build/misty_test"
