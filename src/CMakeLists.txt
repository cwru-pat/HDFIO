
# Use HDFIO source files as a library
file( GLOB HDFIO_LIB_SOURCES ./*.cpp )
file( GLOB HDFIO_LIB_HEADERS ./*.h )
add_library( HDFIOLib ${HDFIO_LIB_SOURCES} ${HDFIO_LIB_HEADERS} )
target_include_directories(HDFIOLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
