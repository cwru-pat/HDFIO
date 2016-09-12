/**
 * 
 */
#ifndef H5IO_h
#define H5IO_h

#include <hdf5.h>
#include <iostream>
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

  bool _checkDatasetExists(std::string dset_name, bool read_flag);

  void _setCompressionPList();

  bool _setAppend();

  void _closeFileThings();

public:

  enum verbosity {off, verbose, debug};

  H5IO(int mem_rank_in, H5SizeArray &mem_dims_in, hid_t mem_type_in);
  
  H5IO(int mem_rank_in, hsize_t mem_dim_in, hid_t mem_type_in);
  
  ~H5IO();
  
  void setVerbosity(int verbosity_in);
  
  void setDatasetType(hid_t dataset_type_in);
  
  void setMemHyperslab(H5SizeArray &start_in, H5SizeArray &stride_in);

  void setMemHyperslab1D(int print_dim, H5SizeArray &start_in, hsize_t stride_in);
  
  void setMemHyperslabNm1D(int drop_dim, H5SizeArray &start_in, H5SizeArray &stride_in);
  
  bool writeArrayToFile(void *array, std::string file_name, std::string dset_name, bool append_flag);

  bool readArrayFromFile(void *arry, std::string file_name, std::string dset_name);
};

#endif
