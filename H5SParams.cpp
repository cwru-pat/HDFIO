#include "H5SParams.h"

H5SParams::H5SParams()
: dims(1), maxdims(1), start(1), stride(1), count(1), block(1), chunk(1) { }

H5SParams::H5SParams(int rank_in)
: dims(rank_in), maxdims(rank_in), start(rank_in), stride(rank_in), count(rank_in), block(rank_in), chunk(rank_in)
{
  rank=rank_in;
}

void H5SParams::setRank( int rank_in )
{
  rank = rank_in;

  dims.setRank(rank);
  maxdims.setRank(rank);
  start.setRank(rank);
  stride.setRank(rank);
  count.setRank(rank);
  block.setRank(rank);
  chunk.setRank(rank);
}

H5SParams::~H5SParams() {}

void H5SParams::setVerbosity(int verbosity_in)
{
  verbosity_level = verbosity_in;
}

void H5SParams::setCount()
{
  for(int i = 0; i < rank; ++i)
  {
    count[i] = (dims[i] - start[i]) / stride[i] + !!((dims[i] - start[i]) % stride[i]);
    if (count[i] < 1) {
      count[i] = 1;
      //output warning statement//Note that I take advantage of this in code
    }
    else if (count[i] > dims[i])
    {
      count[i] = dims[i];
      //output verbose statement
    }
  }
}

void H5SParams::setDefaults(int rank_in, H5SizeArray &dims_in)
{
  setRank(rank_in);
  dims = dims_in;
  maxdims = dims;
  block.setValues(1);
  start.setValues(0);
  stride.setValues(1);
  setCount();
  chunk = dims;
}

int H5SParams::getRank()
{
  return rank;
}

void H5SParams::createSpace()
{
  id = H5Screate_simple(rank, dims.getPtr(), maxdims.getPtr());
}

herr_t H5SParams::closeSpace()
{
  return H5Sclose(id);
}
/**
 * @brief Sets a hyperslab assuming everything is defined (except count)
 * @details This assumes that id, start, stride, and block have been defined.
 * It then calculates the count and sets a new hyperslab.
 */
void H5SParams::setHyperslab()
{
  //set count
  setCount();

  //seting hyperslab
  H5Sselect_hyperslab(id, H5S_SELECT_SET, start.getPtr(), stride.getPtr(), count.getPtr(), block.getPtr());
}
