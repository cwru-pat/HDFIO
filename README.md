# HDFIO
C++ wrapper class to read and write simple arrays and hyperslabs

[![Build Status](https://travis-ci.org/cwru-pat/HDFIO.svg?branch=master)](https://travis-ci.org/cwru-pat/HDFIO)

## Dependencies

 - 1) CMake, `cmake`
 - 1) HDF5 Library, `libhdf5-dev`
 - 2) HDF5 Tools, `hdf5-tools`

Install these using a command like `sudo apt-get install libhdf5-dev hdf5-tools cmake`

## Building & Compiling

Build & compile tests using cmake then make. For example:

```
mkdir build
cd build
cmake ..
make
```

Or, just run the tests:

```
./tests/run_tests.sh
```
