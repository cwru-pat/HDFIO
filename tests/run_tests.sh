#!/bin/bash

# Switch to the directory containing this script,
cd "$(dirname "$0")"
# And up a directory should be the main codebase.
cd ..

mkdir -p build
cd build

cmake ..
if [ $? -ne 0 ]; then
    echo "Error: CMake failed!"
    exit 1
fi

make -j4
if [ $? -ne 0 ]; then
    echo "Error: make failed!"
    exit 1
fi

# try to run generated test
pwd
./tests/test
if [ $? -ne 0 ]; then
    echo "Error: run failed!"
    exit 1
fi

echo ""
echo ""
echo "generated content in test.h5"
echo ""
h5dump test.h5
