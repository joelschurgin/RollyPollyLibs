#!/bin/bash
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

CC="gcc"
BASE_CFLAGS="-std=c99 -D_POSIX_C_SOURCE=200112L -Wextra -Werror -Wimplicit-function-declaration -nostdlib -nostartfiles -g -DBUILD_DEBUG"
TEST_CFLAGS="-std=c99 -D_POSIX_C_SOURCE=200112L -nostartfiles -g"
LDFLAGS="-Wl,-e,entry_point -lpthread"

echo "Building base library..."
$CC -c base.c -o base.o $BASE_CFLAGS
ar rcs libbase.a base.o
rm base.o

echo "Building base_test..."
$CC base_test.c -L. -lbase -o base_test $TEST_CFLAGS $LDFLAGS

mkdir -p ../build/lib
mv libbase.a ../build/lib/libbase.a

mkdir -p ../build/tests
mv base_test ../build/tests/base_test

echo "Build complete: build/base_test"
