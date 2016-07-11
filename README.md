# HDFIO
C++ wrapper class to read and write simple arrays and hyperslabs

[![Build Status](https://travis-ci.org/cwru-pat/HDFIO.svg?branch=master)](https://travis-ci.org/cwru-pat/HDFIO)

## Dependencies

 - 1) HDF5 Library, `libhdf5-dev`
 - 2) HDF5 Tools, `hdf5-tools`

Install these using a command like `sudo apt-get install libhdf5-dev hdf5-tools`

## Compiling

For now, compile using someting like
```
g++ test.cpp -o test -lhdf5 -Wall --std=c++11
```
