#!/bin/bash
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

BASE="$(realpath ../Base)"
LIB="$(realpath ../build/lib)"

CC="gcc"
DISASM_CFLAGS="-I$BASE -std=c99 -D_POSIX_C_SOURCE=200112L -Wno-initializer-overrides -Werror -Wimplicit-function-declaration -nostdlib -nostartfiles -g3 -DBUILD_DEBUG"
TEST_CFLAGS="-I$BASE -std=c99 -D_POSIX_C_SOURCE=200112L -nostartfiles -g3"
LDFLAGS="-L$LIB -lbase -lm -Wl,-e,entry_point -lpthread"

echo "Building disasm library..."
$CC -c disasm.c -o disasm.o $DISASM_CFLAGS
ar rcs libdisasm.a disasm.o 
echo -e "=> \033[32mBUILD COMPLETE: libdisasm.a\033[0m"

echo "Building disasm_test..."
$CC disasm_test.c -L. -ldisasm -o disasm_test $TEST_CFLAGS $LDFLAGS

mkdir -p ../build/lib
rm disasm.o 
mv libdisasm.a ../build/lib/libdisasm.a

mkdir -p ../build/tests
mv disasm_test ../build/tests/disasm_test

echo -e "=> \033[32mBUILD COMPLETE: disasm_test\033[0m"
