
# Compile tests
file(GLOB HDFIO_TESTS ./*.cpp)

foreach( testsourcefile ${HDFIO_TESTS} )
  get_filename_component(TEST_EXE_NAME ${testsourcefile} NAME_WE)
  add_executable( ${TEST_EXE_NAME} ${testsourcefile} )
  target_link_libraries( ${TEST_EXE_NAME} LINK_PUBLIC HDFIOLib ${HDF5_LIBRARY} )
  unset(TEST_EXE_NAME)
endforeach( testsourcefile ${HDFIO_TESTS} )
