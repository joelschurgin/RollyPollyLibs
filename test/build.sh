#!/bin/bash
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

for arg in "$@"; do
    case "$arg" in
    all)
        echo "building all..."
        ./build.sh m32 dwarf2
        ./build.sh m32 dwarf3
        ./build.sh m32 dwarf4
        ./build.sh m32 dwarf5

        ./build.sh m64 dwarf2
        ./build.sh m64 dwarf3
        ./build.sh m64 dwarf4
        ./build.sh m64 dwarf5

        exit
        ;;
    m32)
        ARCH=-m32
        ARCH_NAME=32
        ;;
    m64)
        ARCH=-m64
        ARCH_NAME=64
        ;;
    dwarf2)
        DWARF_VERSION=-gdwarf-2
        DWARF_NAME=dwarf2
        ;;
    dwarf3)
        DWARF_VERSION=-gdwarf-3
        DWARF_NAME=dwarf3
        ;;
    dwarf4)
        DWARF_VERSION=-gdwarf-4
        DWARF_NAME=dwarf4
        ;;
    dwarf5)
        DWARF_VERSION=-gdwarf-5
        DWARF_NAME=dwarf5
        ;;
    esac
done

EXEC_NAME=test
if [[ "$ARCH_NAME" ]]; then
    EXEC_NAME+="$ARCH_NAME"
fi
if [[ "$DWARF_NAME" ]]; then
    EXEC_NAME+="_$DWARF_NAME"
fi

BUILD_DIR=../build/tests/dwarf_tests
mkdir -p $BUILD_DIR
gcc main.c -o $BUILD_DIR/$EXEC_NAME -g $ARCH $DWARF_VERSION

echo -e "=> \033[32mBUILD COMPLETE: $EXEC_NAME\033[0m"
