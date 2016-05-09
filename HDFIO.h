#ifndef HDFIO_HEADER 
#define HDFIO_HEADER

#include <hdf5.h>
#include <iostream>
#include <fstream>
#include <string>

#define HDFIO_VERBOSE_OFF 0
#define HDFIO_VERBOSE_ON 1
#define HDFIO_VERBOSE_DEBUG 2

#define HDFIO_OUT_TYPE_FLOAT 0
#define HDFIO_OUT_TYPE_DOUBLE 1
#define HDFIO_OUT_TYPE_LDOUBLE 2

#define HDFIO_DEFAULT_CHUNK_DIMS 8
#define HDFIO_DEFAULT_BLOCK_DIMS 1
#define HDFIO_DEFAULT_START_POINT 0
#define HDFIO_DEFAULT_STRIDE 1

#define HDFIO_VERBOSE_COUT if(verbosity_level == HDFIO_VERBOSE_ON) std::cout
#define HDFIO_DEBUG_COUT if(verbosity_level >= HDFIO_VERBOSE_ON) std::cout


struct HDFIOFileInfo
{
  hid_t   file_id, // File identifier
          file_dspace_id, // file dataspace identifiers
          dataset_id, // File dataset identifier
          data_compression_plist;
};

//template <typename RT, typename IT>
class HDFIO
{
private:
  int rank;

  hsize_t *mem_dims,
          *stride,
          *file_dims,
          *count_dims,
          *block_dims,
          *start_point,
          *chunk_dims;

  hid_t dataset_type,
        memory_type,
        mem_dspace_id; // mem dataspace identifier
  
  herr_t  status;

  int verbosity_level,
      compression_level;


public:
  HDFIO(hsize_t *mem_dims_in, int rank_in, hid_t memory_type_in)
  {
    initialize(mem_dims_in, rank_in, memory_type_in, HDFIO_VERBOSE_OFF);
  }

  HDFIO(hsize_t *mem_dims_in, int rank_in, hid_t memory_type_in, int verbosity_level_in)
  {
    initialize(mem_dims_in, rank_in, memory_type_in, verbosity_level_in);
  }

  void initialize(hsize_t *mem_dims_in, int rank_in, hid_t memory_type_in, int verbosity_level_in)
  {

    verbosity_level = verbosity_level_in;
    compression_level = 9;
    memory_type = memory_type_in;
    dataset_type = memory_type;

    rank = rank_in;

    HDFIO_VERBOSE_COUT << "Creating HDFIO with rank = " << rank << std::endl << std::flush;

    HDFIO_DEBUG_COUT << "Allocating internal variables..." << std::flush;
    mem_dims = new hsize_t[rank];
    stride = new hsize_t[rank];
    file_dims = new hsize_t[rank];
    count_dims = new hsize_t[rank];
    block_dims = new hsize_t[rank];
    chunk_dims = new hsize_t[rank];
    start_point = new hsize_t[rank];
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    HDFIO_DEBUG_COUT << "Initializing mem_dims..." << std::flush;
    for(int i=0; i<rank; ++i)
    {
      mem_dims[i] = mem_dims_in[i];
    }
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    /* Create dataspace for the memory array */
    HDFIO_DEBUG_COUT << "Creating mem_dspace_id..." << std::flush;
    mem_dspace_id = H5Screate_simple(rank, mem_dims, NULL);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    setHyperslabParameters(HDFIO_DEFAULT_STRIDE);
  }

  ~HDFIO()
  {
    HDFIO_DEBUG_COUT << "Closing HDFIO..." << std::flush;
    status = H5Sclose(mem_dspace_id);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;
  }

  /**
   * @brief Sets Debug mode
   * @details Can choose between 
   * HDFIO_VERBOSE_OFF 0
   * HDFIO_VERBOSE_ON 1
   * HDFIO_VERBOSE_DEBUG 2
   * 
   * @param verbosity_in verbosity level
   */
  void setVerbosity(int verbosity_in)
  {
    verbosity_level = verbosity_in;
  }

  void setDatasetType(hid_t dataset_type_in)
  {
    dataset_type = dataset_type_in;
  }

  void setMemoryType(hid_t memory_type_in)
  {
    memory_type = memory_type_in;
  }

  void setHyperslabParameters(hsize_t stride_in)
  {
    HDFIO_DEBUG_COUT << "Setting stride..." << std::flush;
    for(int i=0; i<rank; ++i)
      stride[i] = stride_in;
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;
    setHyperslabParameters(stride);
  }

  void setHyperslabParameters(hsize_t *stride_in)
  {
    HDFIO_DEBUG_COUT << "Setting additional hyperslab parameters..." << std::flush;
    for(int i=0; i<rank; ++i)
    {
      stride[i] = stride_in[i]; // stride of hyperslab
      file_dims[i] = mem_dims[i]/stride[i];
      chunk_dims[i] = (file_dims[i]<HDFIO_DEFAULT_CHUNK_DIMS ? file_dims[i] : HDFIO_DEFAULT_CHUNK_DIMS);
      count_dims[i] = file_dims[i];  // number of blocks in each direction 
      block_dims[i] = HDFIO_DEFAULT_BLOCK_DIMS; // size of blocks (chosen to be the size of the file's dataset)
      start_point[i] = HDFIO_DEFAULT_START_POINT; // start of hyperslab
    }
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    HDFIO_DEBUG_COUT << "Selecting hyperslab..." << std::flush;
        /* Select hyperslab for memory array to be output */
    status = H5Sselect_hyperslab(mem_dspace_id, H5S_SELECT_SET, start_point, stride, count_dims, block_dims);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;
    
  }

  HDFIOFileInfo getFileInfo(std::string file_name, std::string dataset_name)
  {
    HDFIOFileInfo file_info;

      /* Create a file */
    if(std::ifstream(file_name))
    {
      HDFIO_DEBUG_COUT << "File exists; opening it..." << std::flush;
      file_info.file_id = H5Fopen(file_name.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);  
    }
    else
    {
      HDFIO_DEBUG_COUT << "Creating file..." << std::flush;
      file_info.file_id = H5Fcreate(file_name.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
    }
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    /* Create dataspace for the files dataset */
    HDFIO_DEBUG_COUT << "Creating datspace..." << std::flush;
    file_info.file_dspace_id = H5Screate_simple(rank, file_dims, NULL);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    //test to see if gzip will work
    //todo: tidy this
    unsigned int filter_info;
    htri_t avail = H5Zfilter_avail(H5Z_FILTER_DEFLATE);
    if (!avail) {
         HDFIO_DEBUG_COUT << "gzip filter not available." << std::endl << std::flush;
    }
    status = H5Zget_filter_info (H5Z_FILTER_DEFLATE, &filter_info);
    if ( !(filter_info & H5Z_FILTER_CONFIG_ENCODE_ENABLED) ||
                !(filter_info & H5Z_FILTER_CONFIG_DECODE_ENABLED) ) {
         HDFIO_DEBUG_COUT << "gzip filter not available for encoding and decoding." <<std::endl << std::flush;
    }

    HDFIO_DEBUG_COUT << "Creating compression plist..." << std::flush;
    file_info.data_compression_plist = H5Pcreate (H5P_DATASET_CREATE);
    HDFIO_DEBUG_COUT << "Initializing..." << std::flush;
    status = H5Pset_deflate (file_info.data_compression_plist, compression_level);
    status = H5Pset_chunk (file_info.data_compression_plist, rank, chunk_dims);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    /* Create dataset and write it into the file */
    HDFIO_DEBUG_COUT << "Creating dataset..." << std::flush;
    file_info.dataset_id = H5Dcreate(file_info.file_id, dataset_name.c_str(), dataset_type, file_info.file_dspace_id, H5P_DEFAULT, file_info.data_compression_plist, H5P_DEFAULT);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;
      
    return file_info;
  }

  void closeFileInfo(HDFIOFileInfo *file_info)
  {
    status = H5Dclose(file_info->dataset_id); 
    status = H5Sclose(file_info->file_dspace_id); 
    status = H5Fclose(file_info->file_id);
    status = H5Pclose(file_info->data_compression_plist);
  }

  void writeArrayToFile(void *array, std::string file_name, std::string dataset_name)
  {  
    HDFIOFileInfo file_info = getFileInfo(file_name,dataset_name);

    /* Write the data */
    HDFIO_DEBUG_COUT << "Writing dataset..." << std::flush;
    status = H5Dwrite(file_info.dataset_id, memory_type, mem_dspace_id, file_info.file_dspace_id, H5P_DEFAULT, array);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;
   
    closeFileInfo(&file_info);
  }

  void readFileToArray(void *array, std::string file_name, std::string dataset_name)
  {  
    HDFIOFileInfo file_info = getFileInfo(file_name,dataset_name);

    /* Write the data */
    HDFIO_DEBUG_COUT << "Writing dataset..." << std::flush;
    status = H5Dread(file_info.dataset_id, memory_type, mem_dspace_id, file_info.file_dspace_id, H5P_DEFAULT, array);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;
   
    closeFileInfo(&file_info);
  }

};

#endif
