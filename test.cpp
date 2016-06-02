#include "HDFIO.h"
#include <iostream>

int main()
{
  #define ARRAY_RANK 2
  
  int gridsize=1;
  
  H5SizeArray dims (2, 10, 10);
  H5SizeArray start (2, 0, 0);
  H5SizeArray stride (2, 2, 2);

  for(int i = 0; i<ARRAY_RANK; ++i)
    gridsize *= dims[i];

  float *f = new float[gridsize];
  
  for(int i = 0; i<gridsize; ++i)
  {
    f[i] = i;
  }

  H5IO myIO(ARRAY_RANK, dims.getPtr(), H5T_NATIVE_FLOAT);
  myIO.setVerbosity(H5IO_VERBOSE_DEBUG);
  myIO.setMemHyperslabNm1D(1, start.getPtr(), stride.getPtr());
  myIO.writeArrayToFile(f, "test.h5", "dataset1", true);
  myIO.writeArrayToFile(f, "test.h5", "dataset1", true);
  
  exit(EXIT_SUCCESS);
}
