#include <iostream>
#include <iomanip>
#include <string>

#include "H5IO.h"

using namespace std;

int main()
{
  #define ARRAY_RANK 3
  
  int gridsize=1;
  
  H5SizeArray dims (3, 4, 4, 4); ///< (# dimensions, x-dim, y-dim)
  H5SizeArray start (3, 0, 0, 0); ///< (# dimensions, start x, start y)
  H5SizeArray stride (3, 1, 1, 1); ///< (# dimensions, stride x, stride y)

  for(int i = 0; i<ARRAY_RANK; ++i)
    gridsize *= dims[i];

  float *f = new float[gridsize];
  float *g = new float[gridsize];
  for(int i = 0; i<gridsize; ++i)
  {
    f[i] = i;
  }

  // Create H5IO class for I/O
  H5IO myIO(ARRAY_RANK, dims, H5T_NATIVE_FLOAT);

  // include debugging output
  myIO.setVerbosity(H5IO::debug);

  // Write an ARRAY_RANK array to a file
  myIO.setMemHyperslab(start, stride);
  myIO.writeArrayToFile(f, "test.h5", "dataset0", false);
  for(int i = 0; i<gridsize; ++i)
  {
    f[i] = i / 10.0;
  }

  myIO.writeArrayToFile(f, "test.h5", "dataset1", false);
  myIO.readArrayFromFile(g, "test.h5", "dataset0");
  myIO.writeArrayToFile(g, "test.h5", "dataset0_out", false);
  myIO.readArrayFromFile(g, "test.h5", "dataset1");
  myIO.writeArrayToFile(g, "test.h5", "dataset1_out", false);
  /*// "append" a 1D array to a file (eg, for writing table-like data)
  myIO.setMemHyperslab1D(0, start, 2);
  myIO.writeArrayToFile(f, "test.h5", "/group/dataset1", true);
  // append again:
  myIO.writeArrayToFile(f, "test.h5", "/group/dataset1", true);*/

  return 0;
}
