#include <hdf5.h>
#include <iostream>
#include <string>
#include <cstdarg>

#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION "[" __FILE__ ":" S2(__LINE__) "] "

#define H5IO_VERBOSE_OFF 0
#define H5IO_VERBOSE_ON 1
#define H5IO_VERBOSE_DEBUG 2

#define H5IO_VERBOSE_COUT if( verbosity_level >= H5IO_VERBOSE_ON ) std::cout << \
  (verbosity_level == H5IO_VERBOSE_DEBUG ? LOCATION : "")
#define H5IO_DEBUG_COUT if( verbosity_level == H5IO_VERBOSE_DEBUG ) std::cout << LOCATION

class H5SizeArray
{
private:
  int rank;
  hsize_t *array;
public:
  H5SizeArray(int rank_in, ...)
  {
    rank = rank_in;
    array = new hsize_t[rank];
    va_list args;
    va_start(args, rank_in);
    for(int i=0; i<rank; ++i)
    {
      array[i] = (hsize_t) va_arg(args, int);
    }
    va_end(args);
  }

  ~H5SizeArray()
  {
    delete[] array;
  }

  hsize_t* getPtr()
  {
    return array;
  }

  int getRank()
  {
    return rank;
  }

  hsize_t& operator[](int idx)
  {
    return array[idx];
  }

  bool operator=(H5SizeArray &input)
  {
    int tmp_rank;
    (input.getRank()< rank ? tmp_rank = input.getRank() : tmp_rank=rank);
    for( int i = 0; i < tmp_rank; i++)
      array[i] = input[i];

    return true;
  }

  void setValues(hsize_t value)
  {
    for(int i = 0; i < rank; ++i)
      array[i] = value;
  }

  void setRank(int rank_in)
  {
    delete[] array;
    rank=rank_in;
    array = new hsize_t[rank];
  }

  void print()
  {
    std::cout << std::endl;
    for(int i=0;i<rank;i++)
      std::cout << array[i] << " ";

    std::cout << std::endl << std::flush;
  }
};


class H5SParams
{
private:
  int rank;
  int verbosity_level;

public:
  hid_t type;
  hid_t id;
  H5SizeArray dims,
              maxdims,
              start,
              stride,
              count,
              block,
              chunk;

  H5SParams()
  : dims(1), maxdims(1), start(1), stride(1), count(1), block(1), chunk(1) { }

  H5SParams(int rank_in)
  : dims(rank_in), maxdims(rank_in), start(rank_in), stride(rank_in), count(rank_in), block(rank_in), chunk(rank_in)
  {
    rank=rank_in;
  }

  void setRank( int rank_in )
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

  ~H5SParams() {}

  void setVerbosity(int verbosity_in)
  {
    verbosity_level = verbosity_in;
  }

  void setCount()
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

  void setDefaults(int rank_in, H5SizeArray &dims_in)
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

  int getRank()
  {
    return rank;
  }

  void createSpace()
  {
    id = H5Screate_simple(rank, dims.getPtr(), maxdims.getPtr());
  }

  herr_t closeSpace()
  {
    return H5Sclose(id);
  }
  /**
   * @brief Sets a hyperslab assuming everything is defined (except count)
   * @details This assumes that id, start, stride, and block have been defined.
   * It then calculates the count and sets a new hyperslab.
   */
  void setHyperslab()
  {
    //set count
    setCount();

    //seting hyperslab
    H5Sselect_hyperslab(id, H5S_SELECT_SET, start.getPtr(), stride.getPtr(), count.getPtr(), block.getPtr());
  }
};
