#!/bin/bash
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

BASE="$(realpath ../Base)"
MISTY="$(realpath ../MistyMountainParser)"
INCLUDES="-I$BASE -I$MISTY"

LIB="$(realpath ../build/lib)"

CC="gcc"
LADYBUGGER_CFLAGS="$INCLUDES -std=c99 -D_POSIX_C_SOURCE=200112L -Wextra -Werror -Wimplicit-function-declaration -nostartfiles -g3 -DBUILD_DEBUG"
LDFLAGS="-L$LIB -lbase -L$LIB -lmisty -Wl,-e,entry_point -lpthread"

echo "Building ladybugger..."
$CC ladybugger.c -o ladybugger $LADYBUGGER_CFLAGS $LDFLAGS

mkdir -p ../build
mv ladybugger ../build/ladybugger

echo -e "=> \033[32mBUILD COMPLETE: ladybugger\033[0m"
