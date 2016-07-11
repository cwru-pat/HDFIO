#!/bin/bash

make test
if [ $? -ne 0 ]; then
    echo "Error: make and run failed!"
    exit 1
fi
