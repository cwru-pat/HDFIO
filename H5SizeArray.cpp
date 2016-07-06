#include <hdf5.h>
#include <iostream>
#include <cstdarg>
#include "H5SizeArray.h"

H5SizeArray::H5SizeArray(int rank_in, ...)
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

H5SizeArray::~H5SizeArray()
{
  delete[] array;
}

hsize_t* H5SizeArray::getPtr()
{
  return array;
}

int H5SizeArray::getRank()
{
  return rank;
}

hsize_t& H5SizeArray::operator[](int idx)
{
  return array[idx];
}

bool H5SizeArray::operator=(H5SizeArray &input)
{
  int tmp_rank;
  (input.getRank()< rank ? tmp_rank = input.getRank() : tmp_rank=rank);
  for( int i = 0; i < tmp_rank; i++)
    array[i] = input[i];

  return true;
}

void H5SizeArray::setValues(hsize_t value)
{
  for(int i = 0; i < rank; ++i)
    array[i] = value;
}

void H5SizeArray::setRank(int rank_in)
{
  delete[] array;
  rank=rank_in;
  array = new hsize_t[rank];
}

void H5SizeArray::print()
{
  std::cout << std::endl;
  for(int i=0;i<rank;i++)
    std::cout << array[i] << " ";

  std::cout << std::endl << std::flush;
}

hsize_t H5SizeArray::mag()
{
  hsize_t prod = 1.;
  for (int i = 0; i < rank; ++i)
    prod *= array[i]
  return prod
}