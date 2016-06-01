#include "HDFIO.h"
#include <iostream>

int main()
{
  #define ARRAY_RANK 2
  hsize_t dims[ARRAY_RANK] = {200, 100};
  hsize_t gridsize=1;
  
  for(int i = 0; i<ARRAY_RANK; ++i)
    gridsize *= dims[i];

  float diff=0.,
        f[gridsize],
        g[gridsize];
  
  for(hsize_t i = 0; i<gridsize; ++i)
  {
    f[i] = i;
  }

  H5IO myIO(ARRAY_RANK, dims, H5T_NATIVE_DOUBLE);
  myIO.writeSampledArrayToFile(f, "test.h5.gz", "dataset1",false);
  
  exit(EXIT_SUCCESS);
}
