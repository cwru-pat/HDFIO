#include "HDFIO.h"
#include <iostream>

int main()
{
  #define ARRAY_RANK 2
  hsize_t dims[ARRAY_RANK] = {10, 10};
  hsize_t gridsize=1;
  
  for(int i = 0; i<ARRAY_RANK; ++i)
    gridsize *= dims[i];

  float f[gridsize];
  
  for(hsize_t i = 0; i<gridsize; ++i)
  {
    f[i] = i;
  }

  H5IO myIO(ARRAY_RANK, dims, H5T_NATIVE_FLOAT);
  myIO.setVerbosity(H5IO_VERBOSE_DEBUG);
  myIO.writeSampledArrayToFile(f, "test.h5", "dataset1",false);
  
  exit(EXIT_SUCCESS);
}
