#ifndef H5SParams_h
#define H5SParams_h

#include <hdf5.h>
#include "H5SizeArray.h"

class H5SParams
{
private:
  int rank,
      verbosity_level;

public:
  hid_t type,
        id;

  H5SizeArray dims,
              maxdims,
              start,
              stride,
              count,
              block,
              chunk;

  H5SParams();

  H5SParams(int rank_in);

  void setRank( int rank_in );

  ~H5SParams(); 

  void setVerbosity(int verbosity_in);

  void setCount();

  void setDefaults(int rank_in, H5SizeArray &dims_in);

  int getRank();

  void createSpace();

  herr_t closeSpace();

  void setHyperslab();

};

#endif