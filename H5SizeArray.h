#ifndef H5SizeArray_h
#define H5SizeArray_h

#include <hdf5.h>

class H5SizeArray
{
private:
  int rank;

  hsize_t *array;

public:
  H5SizeArray(int rank_in, ...);

  ~H5SizeArray();

  hsize_t* getPtr();

  int getRank();

  hsize_t& operator[](int idx);

  bool operator=(H5SizeArray &input);

  void setValues(hsize_t value);

  void setRank(int rank_in);

  void print();

};

#endif