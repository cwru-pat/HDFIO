#ifndef HDFIO_HEADER 
#define HDFIO_HEADER

#include <hdf5.h>
#include <iostream>
#include <string>

#define HDFIO_VERBOSE_OFF 0
#define HDFIO_VERBOSE_ON 1
#define HDFIO_VERBOSE_DEBUG 2

#define HDFIO_OUT_TYPE_FLOAT 0
#define HDFIO_OUT_TYPE_DOUBLE 1
#define HDFIO_OUT_TYPE_LDOUBLE 2

#define HDFIO_DEFAULT_CHUNK_DIM 8
#define HDFIO_DEFAULT_BLOCK_DIM 1
#define HDFIO_DEFAULT_START_POINT 0
#define HDFIO_DEFAULT_STRIDE 1

#define HDFIO_VERBOSE_COUT if(verbosity_level == HDFIO_VERBOSE_ON) std::cout
#define HDFIO_DEBUG_COUT if(verbosity_level >= HDFIO_VERBOSE_ON) std::cout

/**
 * @brief Struct for storing information about HDF5 files
 */
struct HDFIOFileInfo
{
  // File identifier
  hid_t file_id;

  // File dataspace identifier
  hid_t file_dspace_id;

  // File dataset identifier
  hid_t dataset_id;

  // Plist for compression
  hid_t data_compression_plist;
};

class HDFIO
{
protected:
  int rank; // Number of dimensions

  hsize_t *mem_dims,    // dimensions of array in memory
          *stride,      // interval to sample at ("stride" in hyperslab)
          *file_dims,   // dimensions of array in file
          *count_dims,  // number of blocks in each direction
          *block_dims,  // size of blocks (chosen to be the size of the file's dataset)
          *start_point, // start of hyperslab
          *chunk_dims;  // Size of chunk for compressing

  hid_t dataset_type,  // Type of data in dataset (in file)
        memory_type,   // Type of data in memory
        mem_dspace_id; // mem dataspace identifier
  
  herr_t status;

  H5E_auto2_t default_error_func; //stores function for default h5 error out
  void *default_error_out; //pointer to default error output

  int verbosity_level,
      compression_level;

  /**
   * @brief Initialize variables with some useful defaults
   * 
   * @param mem_dims_in
   * @param rank_in
   * @param memory_type_in
   * @param verbosity_level_in
   * @see HDFIO
   */
  void _initialize(hsize_t *mem_dims_in, int rank_in, hid_t memory_type_in,
    int verbosity_level_in)
  {
    HDFIO_VERBOSE_COUT << "Creating HDFIO with rank = " << rank << std::endl;

    verbosity_level = verbosity_level_in;
    compression_level = 9;
    memory_type = memory_type_in;
    dataset_type = memory_type;

    rank = rank_in;
    
    HDFIO_DEBUG_COUT << "Stashing H5 error handeling parameters..." << std::flush;
    status = H5Eget_auto(H5E_DEFAULT, &default_error_func, &default_error_out);
    HDFIO_DEBUG_COUT << "Done!" << std::endl;

    HDFIO_DEBUG_COUT << "Allocating internal variables..." << std::flush;
    mem_dims = new hsize_t[rank];
    stride = new hsize_t[rank];
    file_dims = new hsize_t[rank];
    count_dims = new hsize_t[rank];
    block_dims = new hsize_t[rank];
    chunk_dims = new hsize_t[rank];
    start_point = new hsize_t[rank];
    HDFIO_DEBUG_COUT << "Done!" << std::endl;

    HDFIO_DEBUG_COUT << "Initializing mem_dims..." << std::flush;
    for(int i=0; i<rank; ++i)
    {
      mem_dims[i] = mem_dims_in[i];
    }
    HDFIO_DEBUG_COUT << "Done!" << std::endl;

    /* Create dataspace for the memory array */
    HDFIO_DEBUG_COUT << "Creating mem_dspace_id..." << std::flush;
    mem_dspace_id = H5Screate_simple(rank, mem_dims, NULL);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    setHyperslabParameters(HDFIO_DEFAULT_STRIDE);

    return;
  }
  /**
   * @brief Pause H5's default error handeling
   * @details Enables you to run checks on H5 comands you expect to fail.
   * 
   */
  void _pauseH5ErrorHandeling()
  {
    H5Eset_auto(H5E_DEFAULT,NULL,NULL);
  }

  /**
   * @brief Resume H5's default error handeling
   * 
   */
  void _resumeH5ErrorHandeling()
  {
    H5Eset_auto(H5E_DEFAULT,default_error_func,default_error_out);
  }
  /**
   * @brief initialize variables associated with a file_info instance
   * @details Checks for gzip support; behavior unclear if not supported.
   * 
   * @param file_name Name of file to open/create
   * @param dataset_name Dataset inside file to open/create
   * 
   * @return HDFIOFileInfo instance
   */
  HDFIOFileInfo _getFileInfo(std::string file_name, std::string dataset_name)
  {
    HDFIOFileInfo file_info;

    // Create a file
    HDFIO_DEBUG_COUT << "Seeing if file exists..." << std::flush;
    _pauseH5ErrorHandeling();
    file_info.file_id = H5Fopen(file_name.c_str(), H5F_ACC_RDWR,
      H5P_DEFAULT);
    _resumeH5ErrorHandeling();

    if(file_info.file_id<0)
    {
      HDFIO_DEBUG_COUT << "No such file." << std::endl << "Creating file instead..." << std::flush;
      file_info.file_id = H5Fcreate(file_name.c_str(), H5F_ACC_EXCL,
        H5P_DEFAULT, H5P_DEFAULT);
    }
    else
    {
      H5Fclose(file_info.file_id);
      HDFIO_DEBUG_COUT << "File exists." << std::endl << "Opening file..." << std::flush;
      file_info.file_id = H5Fopen(file_name.c_str(), H5F_ACC_RDWR,
        H5P_DEFAULT);
    }
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    // Create dataspace for the files dataset
    HDFIO_DEBUG_COUT << "Creating datspace..." << std::flush;
    file_info.file_dspace_id = H5Screate_simple(rank, file_dims, NULL);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    // Check to see if gzip will work
    unsigned int filter_info;
    htri_t avail = H5Zfilter_avail(H5Z_FILTER_DEFLATE);
    if(!avail)
      HDFIO_DEBUG_COUT << "gzip filter not available." << std::endl;
    status = H5Zget_filter_info(H5Z_FILTER_DEFLATE, &filter_info);
    if( !(filter_info & H5Z_FILTER_CONFIG_ENCODE_ENABLED)
      || !(filter_info & H5Z_FILTER_CONFIG_DECODE_ENABLED) )
      HDFIO_DEBUG_COUT << "gzip filter not available for encoding and decoding." << std::endl;

    HDFIO_DEBUG_COUT << "Creating compression plist..." << std::flush;
    file_info.data_compression_plist = H5Pcreate(H5P_DATASET_CREATE);
    HDFIO_DEBUG_COUT << "Initializing..." << std::flush;
    status = H5Pset_deflate(file_info.data_compression_plist, compression_level);
    status = H5Pset_chunk(file_info.data_compression_plist, rank, chunk_dims);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    /* Create dataset and write it into the file */
    HDFIO_DEBUG_COUT << "Seeing if dataset exists..." << std::flush;
    _pauseH5ErrorHandeling();
    file_info.dataset_id = H5Dopen(file_info.file_id, dataset_name.c_str(),
     H5P_DEFAULT);
    _resumeH5ErrorHandeling();

    if(file_info.dataset_id<0)
    {
      HDFIO_DEBUG_COUT << "No such dataset." << std::endl << "Creating dataset..." << std::flush;
      file_info.dataset_id = H5Dcreate(file_info.file_id, dataset_name.c_str(),
        dataset_type, file_info.file_dspace_id, H5P_DEFAULT,
        file_info.data_compression_plist, H5P_DEFAULT);
    }
    else
    {
      H5Dclose(file_info.dataset_id);
      HDFIO_DEBUG_COUT << "Dataset exists." << std::endl << "Opening dataset..." << std::flush;
      file_info.dataset_id = H5Dopen(file_info.file_id, dataset_name.c_str(),
        H5P_DEFAULT); 
    }
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    
    return file_info;
  }

  /**
   * @brief close variables associated with a file_info instance
   * 
   * @param file_info Reference to a HDFIOFileInfo instance
   */
  void _closeFileInfo(HDFIOFileInfo *file_info)
  {
    status = H5Dclose(file_info->dataset_id); 
    status = H5Sclose(file_info->file_dspace_id); 
    status = H5Fclose(file_info->file_id);
    status = H5Pclose(file_info->data_compression_plist);

    return;
  }

public:

  /**
   * @brief Constructor for HDFIO class
   * 
   * @param size array of size^rank
   * @param memory_type_in Type for 
   * @see HDFIO
   */
  HDFIO(hsize_t size, int rank_in, hid_t memory_type_in)
  {
    hsize_t mem_dims_in[rank_in];
    for(int i=0; i<rank_in; ++i)
      mem_dims_in[i] = size;

    _initialize(mem_dims_in, rank_in, memory_type_in, HDFIO_VERBOSE_OFF);
  }

  /**
   * @brief Constructor for HDFIO class
   * 
   * @param mem_dims_in Dimensions of array stored in memory
   * @param rank_in Rank of array (# dimensions)
   * @param memory_type_in Type for 
   * @see HDFIO
   */
  HDFIO(hsize_t *mem_dims_in, int rank_in, hid_t memory_type_in)
  {
    _initialize(mem_dims_in, rank_in, memory_type_in, HDFIO_VERBOSE_OFF);
  }

  /**
   * @brief Constructor for HDFIO class
   * 
   * @param mem_dims_in [description]
   * @param rank_in [description]
   * @param memory_type_in [description]
   * @param verbosity_level_in [description]
   */
  HDFIO(hsize_t *mem_dims_in, int rank_in, hid_t memory_type_in, int verbosity_level_in)
  {
    _initialize(mem_dims_in, rank_in, memory_type_in, verbosity_level_in);
  }

  /**
   * @brief Destructor: clean up anything?
   */
  ~HDFIO()
  {
    HDFIO_DEBUG_COUT << "Closing HDFIO..." << std::flush;
    status = H5Sclose(mem_dspace_id);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;
  }

  /**
   * @brief Sets level of verbosity
   * @details Can choose between:
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

  /**
   * @brief Set type of dataset to be written/read in file
   * @details Probably one of these:
   * https://www.hdfgroup.org/HDF5/doc/RM/PredefDTypes.html
   * 
   * @param dataset_type_in dataset type
   */
  void setDatasetType(hid_t dataset_type_in)
  {
    dataset_type = dataset_type_in;
  }

  /**
   * @brief Set type of dataset in memory
   * @details Probably one of these:
   * https://www.hdfgroup.org/HDF5/doc/RM/PredefDTypes.html
   * 
   * @param memory_type_in type in memory
   */
  void setMemoryType(hid_t memory_type_in)
  {
    memory_type = memory_type_in;
  }

  /**
   * @brief Initialize hyperslab parameters
   * @see setHyperslabParameters
   */
  void setHyperslabParameters(hsize_t stride_in)
  {
    HDFIO_DEBUG_COUT << "Setting stride..." << std::flush;
    for(int i=0; i<rank; ++i)
      stride[i] = stride_in;
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;
    setHyperslabParameters(stride);

    return;
  }

  /**
   * @brief Initialize hyperslab parameters
   * @details If you want to set other parameters, check out other methods.
   * If no setter method exists, you may need to modify this class.
   * Or write your own code, you lazy bum.
   * 
   * @param stride_in array: stride in each dimension
   */
  void setHyperslabParameters(hsize_t *stride_in)
  {
    HDFIO_DEBUG_COUT << "Setting additional hyperslab parameters..."
      << std::flush;
    for(int i=0; i<rank; ++i)
    {
      stride[i] = stride_in[i];
      file_dims[i] = mem_dims[i]/stride[i];
      chunk_dims[i] = ( file_dims[i] < HDFIO_DEFAULT_CHUNK_DIM ?
        file_dims[i] : HDFIO_DEFAULT_CHUNK_DIM );
      count_dims[i] = file_dims[i];
      block_dims[i] = HDFIO_DEFAULT_BLOCK_DIM;
      start_point[i] = HDFIO_DEFAULT_START_POINT;
    }
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    HDFIO_DEBUG_COUT << "Selecting hyperslab..." << std::flush;
    // Select hyperslab for memory array to be output
    status = H5Sselect_hyperslab(mem_dspace_id, H5S_SELECT_SET,
      start_point, stride, count_dims, block_dims);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;
    
    return;
  }

  /**
   * @brief THATS RIGHT
   */
  void writeArrayToFile(void *array, std::string file_name,
    std::string dataset_name)
  {  
    HDFIOFileInfo file_info = _getFileInfo(file_name,dataset_name);

    /* Write the data */
    HDFIO_DEBUG_COUT << "Writing dataset..." << std::flush;
    status = H5Dwrite(file_info.dataset_id, memory_type, mem_dspace_id,
      file_info.file_dspace_id, H5P_DEFAULT, array);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;
   
    _closeFileInfo(&file_info);
  }
  /**
   * @brief Reads data from file to array
   * @details 
   * 
   * @param array memory Location of start of memory chunk to be read to.
   * @param file_name File name containg datset to be read
   * @param dataset_name Dataset name which will be read to array
   */
  void readFileToArray(void *array, std::string file_name, std::string dataset_name)
  {  
    HDFIOFileInfo file_info = _getFileInfo(file_name,dataset_name);

    /* Write the data */
    HDFIO_DEBUG_COUT << "Writing dataset to memory..." << std::flush;
    status = H5Dread(file_info.dataset_id, memory_type, mem_dspace_id,
      file_info.file_dspace_id, H5P_DEFAULT, array);
    HDFIO_DEBUG_COUT << "Done!" << std::endl << std::flush;
   
    _closeFileInfo(&file_info);
  }

};

#endif
