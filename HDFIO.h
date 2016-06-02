#include <hdf5.h>
#include <iostream>
#include <string>
#include <cstdarg>


#define H5IO_VERBOSE_OFF 0
#define H5IO_VERBOSE_ON 1
#define H5IO_VERBOSE_DEBUG 2

#define H5IO_VERBOSE_COUT if( verbosity_level >= H5IO_VERBOSE_ON ) std::cout << \
  (verbosity_level == H5IO_VERBOSE_DEBUG ? ("[" + std::to_string(__LINE__) + "] ") : "")
#define H5IO_DEBUG_COUT if( verbosity_level == H5IO_VERBOSE_DEBUG ) std::cout << "[" << __LINE__ << "] "

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

  hsize_t& operator[](int idx)
  {
    return array[idx];
  }

  void setValues(hsize_t value)
  {
    for(int i = 0; i < rank; ++i)
      array[i] = value;
  }

  void setValues(hsize_t* values)
  {
    for(int i = 0; i < rank; ++i)
      array[i] = values[i];
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
  : dims(1), maxdims(1), start(1), stride(1), count(1), block(1), chunk(1)
  {
  }

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
      count[i] = (dims[i] - start[i]) / stride[i];
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

  void setDefaults(int rank_in, hsize_t *dims_in)
  {
    setRank(rank_in);
    dims.setValues(dims_in);
    maxdims.setValues(dims.getPtr());
    block.setValues(1);
    start.setValues((hsize_t) 0);
    stride.setValues(1);
    setCount();
    chunk.setValues(dims.getPtr());
  }

  int getRank()
  {
    return rank;
  }

  herr_t setHyperslab(hsize_t *start_in, hsize_t *stride_in)
  {
    herr_t status;

    block.setValues(1);

    //start at begining of
    start.setValues(start_in);

    //set stride and count
    stride.setValues(stride_in);

    //set count
    setCount();

    //get id for dspace. Assumes rank and dims set
    id = H5Screate_simple(rank, dims.getPtr(), maxdims.getPtr());

    //seting hyperslab
    status = H5Sselect_hyperslab(id, H5S_SELECT_SET, start.getPtr(), stride.getPtr(), count.getPtr(), block.getPtr());

    return status;
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

class H5IO
{
private:
  H5SParams mem_dspace, //holds memory dataspace things
            dset_dspace; //holds file dataset dataspace things
  herr_t status;
  hid_t file_id;
  hid_t dset_id;
  hid_t dset_chunk_plist;
  int compression_level;
  int verbosity_level;
  H5E_auto2_t default_error_func; //stores function for default h5 error out
  void *default_error_out; //pointer to default error output

  void _initialize(int mem_rank_in, hsize_t *mem_dims_in, hid_t mem_type_in)
  {
    H5IO_DEBUG_COUT << "Stashing H5 error handeling parameters..." << std::flush;
    status = H5Eget_auto(H5E_DEFAULT, &default_error_func, &default_error_out);
    H5IO_DEBUG_COUT << "Done!" << std::endl;
    compression_level = 9;
    mem_dspace.type=mem_type_in;
    dset_dspace.type=mem_type_in;
    mem_dspace.setDefaults(mem_rank_in, mem_dims_in);
    dset_dspace.setDefaults(mem_rank_in, mem_dims_in);
    mem_dspace.id = H5Screate_simple(mem_dspace.getRank(), mem_dspace.dims.getPtr(), mem_dspace.maxdims.getPtr());
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

  bool _openOrCreateFile(std::string file_name, bool read_flag)
  {
    H5IO_DEBUG_COUT << "Seeing if file exists..." << std::flush;
    _pauseH5ErrorHandeling();
    file_id = H5Fopen(file_name.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    _resumeH5ErrorHandeling();

    if(file_id < 0)
    {
      if(read_flag)
      {
        H5IO_DEBUG_COUT << "No such file." << std::endl;
        H5IO_VERBOSE_COUT << "No data to read: aborting read." << std::endl << std::flush;
        return false;
      }
      H5IO_DEBUG_COUT << "No such file." << std::endl << "Creating file instead..." << std::flush;
      file_id = H5Fcreate(file_name.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
    }
    else
    {
      H5Fclose(file_id);
      H5IO_DEBUG_COUT << "File exists." << std::endl << "Opening file..." << std::flush;
      file_id = H5Fopen(file_name.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    }
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;
    return true;
  }

  bool _checkCompression()
  {
    unsigned int filter_info;
    htri_t avail = H5Zfilter_avail(H5Z_FILTER_DEFLATE);
    if(avail)
    {
      status = H5Zget_filter_info(H5Z_FILTER_DEFLATE, &filter_info);
      if( (filter_info & H5Z_FILTER_CONFIG_ENCODE_ENABLED) && (filter_info & H5Z_FILTER_CONFIG_DECODE_ENABLED) )
      {
        return true;
      }
    }
    H5IO_VERBOSE_COUT << "Cant compress with gzip. Truning off compression." << std::endl;
    return false;
  }

  void _createCloseDatasetAppend(std::string dset_name)
  {
    H5IO_DEBUG_COUT << "Creating dataset for appending...." << std::endl << std::flush;

    H5IO_DEBUG_COUT << "  Setting dataspace rank..." << std::flush;
    dset_dspace.setRank(mem_dspace.getRank()+1);
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    H5IO_DEBUG_COUT << "  Formating dataspace..." << std::flush;
    dset_dspace.dims[0] = 0;

    for(int i = 1; i < dset_dspace.getRank(); ++i)
      dset_dspace.dims[i] = mem_dspace.dims[i-1] / mem_dspace.stride[i-1];

    dset_dspace.maxdims.setValues(dset_dspace.dims.getPtr());
    dset_dspace.maxdims[0] = H5S_UNLIMITED;
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    H5IO_DEBUG_COUT << "  Creating dataspace..." << std::flush;
    dset_dspace.id = H5Screate_simple(dset_dspace.getRank(), dset_dspace.dims.getPtr(), dset_dspace.maxdims.getPtr());
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    dset_dspace.chunk.setValues(dset_dspace.dims.getPtr());
    dset_dspace.chunk[0] = 1; // so that chunk has positive values
    _setCompressionPList();

    H5IO_DEBUG_COUT << "  Creating dataset..." << std::flush;
    dset_id = H5Dcreate(file_id, dset_name.c_str(), dset_dspace.type, dset_dspace.id, H5P_DEFAULT, dset_chunk_plist, H5P_DEFAULT);
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    H5IO_DEBUG_COUT << "  Closing dataset..." << std::flush;
    status = H5Dclose(dset_id);
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;
    H5IO_DEBUG_COUT << "  Closing dataspace..." << std::flush;
    status = H5Sclose(dset_dspace.id);
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    H5IO_DEBUG_COUT << "  Closing Compression plist..." << std::flush;
    status = H5Pclose(dset_chunk_plist);
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    H5IO_DEBUG_COUT << "Finisehd creating dataset!" << std::endl << std::flush;
  }

  void _createOpenDataset(std::string dset_name)
  {
    // This shrinks the file dspace to be minimal dimensions i.e. if there is a
    // mem_dspace.dim that is 1 it will skip over unless all are one then it
    // sets the dset rank to 1 and dset_dspace.dim[0] = 1.
    int new_rank = 0;
    for(int i = 0; i < mem_dspace.getRank(); ++i)
      if( mem_dspace.block[i] * mem_dspace.count[i] > 1 )
        new_rank++;

    if(new_rank == 0)
    {
      dset_dspace.setRank(1);
      dset_dspace.dims[0] = 1;
    }
    else
    {
      dset_dspace.setRank(new_rank);
      int j;
      for(int i = 0; i < mem_dspace.getRank(); ++i)
        if( mem_dspace.block[i] * mem_dspace.count[i] > 1 )
        {
          dset_dspace.dims[j] = mem_dspace.block[i] * mem_dspace.count[i];
          j++;
        }
    }

    dset_dspace.maxdims.setValues(dset_dspace.dims.getPtr());


    dset_dspace.id = H5Screate_simple(dset_dspace.getRank(), dset_dspace.dims.getPtr(), dset_dspace.maxdims.getPtr());

    dset_dspace.chunk.setValues(dset_dspace.dims.getPtr());
    _setCompressionPList();

    dset_id = H5Dcreate(file_id, dset_name.c_str(), dset_dspace.type, dset_dspace.id, H5P_DEFAULT, dset_chunk_plist, H5P_DEFAULT);
    H5Pclose(dset_chunk_plist);
  }

  bool _checkAppend()
  {
    if(dset_dspace.getRank() == 1 + mem_dspace.getRank())
    {
      if(dset_dspace.maxdims[0] == H5S_UNLIMITED || dset_dspace.dims[0] <= dset_dspace.maxdims[0])
      {
        for( int i = 1; i < dset_dspace.getRank(); ++i )
          if( dset_dspace.dims[i] != mem_dspace.count[i-1]*mem_dspace.block[i-1])
            return false;
        return true;
      }
    }
    return false;

  }

  bool _checkDatasetExists(std::string dset_name)
  {
    hid_t temp;
     H5IO_DEBUG_COUT << "Seeing if dataset exists..." << std::flush;
    _pauseH5ErrorHandeling();
    temp = H5Dopen(file_id, dset_name.c_str(), H5P_DEFAULT);
    _resumeH5ErrorHandeling();

    if( temp < 0 ) {
      H5IO_DEBUG_COUT << "Dataset does not exist." << std::endl;
      return false;
    }

    H5Dclose(temp);
    H5IO_DEBUG_COUT << "Dataset exists!" << std::endl << std::flush;
    return true;
  }


  void _setCompressionPList()
  {
    dset_chunk_plist = H5Pcreate(H5P_DATASET_CREATE);
    status = H5Pset_layout(dset_chunk_plist, H5D_CHUNKED);

    status = H5Pset_chunk(dset_chunk_plist, dset_dspace.getRank(), dset_dspace.chunk.getPtr());

    if(_checkCompression())
      status = H5Pset_deflate(dset_chunk_plist, compression_level);
  }

  bool _setAppend()
  {
    herr_t status;
    int tmp_rank;
    H5IO_DEBUG_COUT << "Selecting hyperslab to append to..." << std::endl << std::flush;

    //get id for dspace
    H5IO_DEBUG_COUT << "  Getting dataspace ID..." << std::flush;
    dset_dspace.id = H5Dget_space(dset_id);
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    //initialize dspace params
    H5IO_DEBUG_COUT << "  Getting rank of dataspace..." << std::flush;
    tmp_rank = H5Sget_simple_extent_ndims(dset_dspace.id);
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    dset_dspace.setRank(tmp_rank);

    //get max dims and dims of dsapce
    H5IO_DEBUG_COUT << "  Getting dims and maxdims of dataspace..." << std::flush;
    status = H5Sget_simple_extent_dims(dset_dspace.id, dset_dspace.dims.getPtr(), dset_dspace.maxdims.getPtr());
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    H5IO_DEBUG_COUT << "  Formating datspace for next row..." << std::flush;

    dset_dspace.stride.setValues(1);
    dset_dspace.block.setValues(1);
    dset_dspace.start.setValues((hsize_t) 0);

    //start at end of current last line begining of
    dset_dspace.start[0]=dset_dspace.dims[0];

    //increase dspace dims by 1 (this is so dataset can be extended)
    dset_dspace.dims[0] += 1;//block[0]; if wanted multi line write?
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    H5IO_DEBUG_COUT << "  Closing dataspace..." << std::flush;
    status = H5Sclose(dset_dspace.id);
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    if(_checkAppend()){
        H5IO_DEBUG_COUT << "  Extending dataset..." << std::flush;
        status = H5Dset_extent(dset_id, dset_dspace.dims.getPtr());
        H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;
    } else {
        H5IO_VERBOSE_COUT << "Can't extend dataset for an append. Aborting write." << std::endl;
        return false;
    }

    H5IO_DEBUG_COUT << "  Reopening dataspace..." << std::flush;
    dset_dspace.id = H5Dget_space(dset_id);
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    //seting hyperslab
    H5IO_DEBUG_COUT << "  Selecting hyperslab for append..." << std::flush;
    dset_dspace.setHyperslab();
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

    H5IO_DEBUG_COUT << "Hyperslab selected!" << std::endl << std::flush;

    if(status < 0)
    {
      H5IO_VERBOSE_COUT << "Could not append; aborting write."
        << std::endl << std::flush;
    }
    return true;
  }

  void _closeAllTheThings()
  {
    H5Sclose(dset_dspace.id);
    H5Fclose(file_id);
    H5Dclose(dset_id);
    H5Pclose(dset_chunk_plist);
  }

public:

  H5IO(int mem_rank_in, hsize_t *mem_dims_in, hid_t mem_type_in)
  : mem_dspace(), dset_dspace()
  {
    _initialize(mem_rank_in, mem_dims_in, mem_type_in);
  }

  H5IO(int mem_rank_in, hsize_t mem_dim_in, hid_t mem_type_in)
  : mem_dspace(), dset_dspace()
  {

    hsize_t * mem_dims_in = new hsize_t[mem_rank_in];
    for(int i=0; i<mem_rank_in; i++)
      mem_dims_in[i] = mem_dim_in;

    _initialize(mem_rank_in, mem_dims_in, mem_type_in);

  }

  ~H5IO()
  {
    H5Sclose(mem_dspace.id);
    //do i need to close the classes i created? in particular the arrays?
  }

  /**
   * @brief Sets level of verbosity
   * @details Can choose between:
   * H5IO_VERBOSE_OFF 0
   * H5IO_VERBOSE_ON 1
   * H5IO_VERBOSE_DEBUG 2
   *
   * @param verbosity_in verbosity level
   */
  void setVerbosity(int verbosity_in)
  {
    verbosity_level = verbosity_in;
    mem_dspace.setVerbosity(verbosity_level);
    dset_dspace.setVerbosity(verbosity_level);
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
    dset_dspace.type = dataset_type_in;
  }

  void setMemHyperslab(hsize_t *start_in, hsize_t *stride_in)
  {
    mem_dspace.start.setValues(start_in);
    mem_dspace.stride.setValues(stride_in);

    mem_dspace.setHyperslab();
  }

  void setMemHyperslab1D(int print_dim, hsize_t *start_in, hsize_t stride_in)
  {
    mem_dspace.start.setValues(start_in);
    //set stride too large so that count will be 1
    mem_dspace.stride.setValues(mem_dspace.dims.getPtr());
    //fix stride in print dim to be reasonable
    mem_dspace.stride[print_dim]=stride_in;

    mem_dspace.setHyperslab();
  }

  void setMemHyperslabNm1D(int drop_dim, hsize_t *start_in, hsize_t *stride_in)
  {
    mem_dspace.start.setValues(start_in);
    mem_dspace.stride.setValues(stride_in);

    //ensure stride in drop dim is too large so count will be 1
    mem_dspace.stride[drop_dim]=mem_dspace.dims[drop_dim];

    mem_dspace.setHyperslab();
  }

  bool writeArrayToFile(void *array, std::string file_name, std::string dset_name, bool append_flag)
  {

    //status = H5Sselect_hyperslab(mem_dspace.id, H5S_SELECT_SET, mem_dspace.start.getPtr(), mem_dspace.stride.getPtr(), mem_dspace.count.getPtr(), mem_dspace.block.getPtr()); //selects parts of memory to write

    _openOrCreateFile(file_name,false);

    if(append_flag)
    {
      //check if dataset does NOT exists
      if( ! _checkDatasetExists(dset_name) )
          _createCloseDatasetAppend(dset_name);

      H5IO_DEBUG_COUT << "Opening dataset..." << std::flush;
      dset_id = H5Dopen( file_id, dset_name.c_str() , H5P_DEFAULT);
      H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

      if (! _setAppend())
        return false;
    } else { //create new file
      if( _checkDatasetExists(dset_name) ) {
        H5IO_DEBUG_COUT << "Can't write dataset to one that exists. Aborting write." << std::endl;
        return false;
      }
      else
        _createOpenDataset(dset_name);
    }
    H5IO_DEBUG_COUT << "Writing data..." << std::flush;
    status = H5Dwrite(dset_id, mem_dspace.type, mem_dspace.id, dset_dspace.id, H5P_DEFAULT, array);
    H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;


    H5Sclose(dset_dspace.id);
    H5Fclose(file_id);
    H5Dclose(dset_id);

    return true;
  }
};
