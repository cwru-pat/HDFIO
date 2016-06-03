#include "HDFIO.h"
#include <iostream>

int main()
{
  #define ARRAY_RANK 2
  
  int gridsize=1;
  
  H5SizeArray dims (2, 10, 10);
  H5SizeArray start (2, 1, 1);
  H5SizeArray stride (2, 2, 2);

  for(int i = 0; i<ARRAY_RANK; ++i)
    gridsize *= dims[i];

  float *f = new float[gridsize];
  
  for(int i = 0; i<gridsize; ++i)
  {
    f[i] = i;
  }

  H5IO myIO(ARRAY_RANK, dims, H5T_NATIVE_FLOAT);
  myIO.setVerbosity(H5IO_VERBOSE_DEBUG);
  myIO.setMemHyperslab(start, stride);
  myIO.writeArrayToFile(f, "test.h5", "dataset1", true);
  myIO.writeArrayToFile(f, "test.h5", "dataset1", true);
  start.setValues(0);
  stride.setValues(41);
  myIO.setMemHyperslab(start, stride);
  myIO.writeArrayToFile(f, "test.h5", "dataset2", false);
  stride.setValues(1);
  myIO.setMemHyperslab(start, stride);
  myIO.writeArrayToFile(f, "test.h5", "dataset0", false);
  std::cout << 10/4 << std::endl;
  exit(EXIT_SUCCESS);
}
