project(HDFIO)
cmake_minimum_required(VERSION 2.6.0)

# Make sure compiler supports c++11 standards
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  message(WARNING "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support, but has c++0x support. Attempting to build anyways; compile may fail.")
else()
  message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different compiler.")
endif()


# Compiler options
# add -pg for gprof
# add -g for valgrind
SET(HDFIO_DEBUG FALSE CACHE STRING "Turn on debug mode (turn off optimizations, turn on valgrind")
SET(HDFIO_PROFILE FALSE CACHE STRING "Turn on profiling (gprof flag)")
if(HDFIO_DEBUG)
  set(PROFILING     "-g")
  set(OPT_LEVEL     "-O1")
  set(CC_OPTS       "")
else()
  set(PROFILING     "")
  set(OPT_LEVEL     "-O3 -ffast-math")
  set(CC_OPTS       "-march=native")
endif()
if(HDFIO_PROFILE)
  set(PROFILING "${PROFILING} -pg")
endif()
set(WARNINGS          "-pedantic -Wall -Wno-unused-variable -Wno-unused-function")
set(CMAKE_CXX_FLAGS   "${CC_OPTS} ${OPT_LEVEL} ${WARNINGS} ${PROFILING}")
set(CMAKE_EXE_LINKER_FLAGS  "${PROFILING}")


# HDF5 libraries
find_library(HDF5_LIBRARY
     NAMES hdf5 libhdf5
     PATHS /home/*/hdf5/build/lib /usr/lib)
set(HDF5_LIBRARIES "${HDF5_LIBRARY}")
message(STATUS " HDF5_LIBRARY: ${HDF5_LIBRARY}")

add_subdirectory(src)
add_subdirectory(tests)
