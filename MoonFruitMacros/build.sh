#!/bin/bash
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

BASE="$(realpath ../Base)"
LIB="$(realpath ../build/lib)"

CC="gcc"
MOONFRUIT_CFLAGS="-I$BASE -std=c99 -D_POSIX_C_SOURCE=200112L -Wextra -Werror -Wimplicit-function-declaration -nostdlib -nostartfiles -g3 -DBUILD_DEBUG"
TEST_CFLAGS="-I$BASE -std=c99 -D_POSIX_C_SOURCE=200112L -nostartfiles -g3"
LDFLAGS="-L$LIB -lbase -Wl,-e,entry_point -lpthread"

echo "Building moonfruit library..."
$CC -c moonfruit.c -o moonfruit.o $MOONFRUIT_CFLAGS
ar rcs libmoonfruit.a moonfruit.o

echo -e "=> \033[32mBUILD COMPLETE: libmoonfruit.a\033[0m"

echo "Building moonfruit..."
$CC moonfruit_main.c -L. -lmoonfruit -o moonfruit $TEST_CFLAGS $LDFLAGS

mkdir -p ../build/lib
rm moonfruit.o
mv libmoonfruit.a ../build/lib/libmoonfruit.a

mkdir -p ../build
mv moonfruit ../build/moonfruit

echo -e "=> \033[32mBUILD COMPLETE: moonfruit\033[0m"
