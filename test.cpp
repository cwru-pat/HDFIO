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
    f[i] = std::rand() / ((float) RAND_MAX);
    g[i]=0.;
  }

  HDFIO myIO(dims, ARRAY_RANK, H5T_NATIVE_FLOAT, HDFIO_VERBOSE_DEBUG);
  myIO.setDatasetType(H5T_NATIVE_DOUBLE);
  myIO.setHyperslabParameters(1);
  myIO.writeArrayToFile(f, "test.h5.gz", "dataset1");
  myIO.readFileToArray(g,"test.h5.gz","dataset1");
  for(hsize_t i = 0; i<gridsize; ++i)
  {
    diff+=f[i]-g[i];
  }
  std::cout << "f - g = " << diff << std::endl;
  return 0;

  exit(EXIT_SUCCESS);
}
