/**
 * 
 */
#ifndef H5IO_h
#define H5IO_h

#include <hdf5.h>
#include <iostream>
#include <typeinfo>
#include <vector>
#include "H5SizeArray.h"
#include "H5SParams.h"

/**
 * @brief Class for easy HDF5 file IO
 * @details [long description]
 * 
 */
class H5IO
{
private:
  H5SParams mem_dspace, //holds memory dataspace things
            dset_dspace; //holds file dataset dataspace things
  
  herr_t status;
  
  hid_t file_id,
        dset_id,
        dset_chunk_plist;
  
  int compression_level,
      verbosity_level;
      
  H5E_auto2_t default_error_func; //stores function for default h5 error out
  
  void *default_error_out; //pointer to default error output

  void _initialize(int mem_rank_in, H5SizeArray &mem_dims_in, hid_t mem_type_in);

  void _pauseH5ErrorHandeling();
  
  void _resumeH5ErrorHandeling();

  bool _openOrCreateFile(std::string file_name, bool read_flag);

  bool _checkCompression();

  void _split(const std::string &s, char delim, std::vector<std::string> &elems);

  bool _createGroups(std::string &dset_name);

  bool _createCloseDatasetAppend(std::string dset_name);

  bool _createOpenDataset(std::string dset_name);

  bool _checkAppend();

  bool _checkDatasetExists(std::string dset_name);

  void _setCompressionPList();

  bool _setAppend();

  void _closeFileThings();

public:

  enum verbosity {off, verbose, debug};

  template<typename T> H5IO(int mem_rank_in, H5SizeArray &mem_dims_in)
  : mem_dspace(), dset_dspace()
  {
    _initialize(mem_rank_in, mem_dims_in, typeForH5<T>());
  }

  template<typename T> H5IO(int mem_rank_in, hsize_t mem_dim_in)
  : mem_dspace(), dset_dspace()
  {

    H5SizeArray mem_dims_in(mem_rank_in);
    mem_dims_in.setValues(mem_dim_in);

    _initialize(mem_rank_in, mem_dims_in, typeForH5<T>());

  }


  H5IO(int mem_rank_in, H5SizeArray &mem_dims_in, hid_t mem_type_in);
  
  H5IO(int mem_rank_in, hsize_t mem_dim_in, hid_t mem_type_in);
  
  ~H5IO();
  
  void setVerbosity(int verbosity_in);
  
  void setDatasetType(hid_t dataset_type_in);
  
  void setMemHyperslab(H5SizeArray &start_in, H5SizeArray &stride_in);

  void setMemHyperslab1D(int print_dim, H5SizeArray &start_in, hsize_t stride_in);
  
  void setMemHyperslabNm1D(int drop_dim, H5SizeArray &start_in, H5SizeArray &stride_in);

  /**
* @brief Returns the HDF5 Type for a given value.
 * @param value A value to use. Can be anything. Just used to get the type info
 * @return The HDF5 native type for the value
 */
template<typename T> hid_t typeForH5()
{
  T value;

    if(typeid(value) == typeid(char))
      return H5T_NATIVE_CHAR;
    if(typeid(value) == typeid(signed char))
      return H5T_NATIVE_SCHAR;
    if(typeid(value) == typeid(unsigned char))
      return H5T_NATIVE_UCHAR;
    if(typeid(value) == typeid(short))
      return H5T_NATIVE_SHORT;
    if(typeid(value) == typeid(unsigned short))
      return H5T_NATIVE_USHORT;
    if(typeid(value) == typeid(int))
      return H5T_NATIVE_INT;
    if(typeid(value) == typeid(unsigned))
      return H5T_NATIVE_UINT;
    if(typeid(value) == typeid(long))
      return H5T_NATIVE_LONG;
    if(typeid(value) == typeid(unsigned long))
      return H5T_NATIVE_ULONG;
    if(typeid(value) == typeid(long long))
      return H5T_NATIVE_LLONG;
    if(typeid(value) == typeid(unsigned long long))
      return H5T_NATIVE_ULLONG;
    if(typeid(value) == typeid(float))
      return H5T_NATIVE_FLOAT;
    if(typeid(value) == typeid(double))
      return H5T_NATIVE_DOUBLE;
    if(typeid(value) == typeid(long double))
      return H5T_NATIVE_LDOUBLE;
    if(typeid(value) == typeid(hsize_t))
      return H5T_NATIVE_HSIZE;
    if(typeid(value) == typeid(hssize_t))
      return H5T_NATIVE_HSSIZE;
    if(typeid(value) == typeid(herr_t))
      return H5T_NATIVE_HERR;
    if(typeid(value) == typeid(hbool_t))
      return H5T_NATIVE_HBOOL;
    
    H5IO_VERBOSE_COUT << "Unknown type: '" << (typeid(value).name()) << "' is not currently supported.";
    return -1;
}
  
  bool writeArrayToFile(void *array, std::string file_name, std::string dset_name, bool append_flag);
};

#endif
