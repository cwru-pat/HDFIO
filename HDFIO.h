#include <hdf5.h>
#include <hdf5_hl.h>
#include <iostream>
#include <string>

#define H5IO_VERBOSE_OFF 0
#define H5IO_VERBOSE_ON 1
#define H5IO_VERBOSE_DEBUG 2

#define H5IO_VERBOSE_COUT if( verbosity_level >= H5IO_VERBOSE_ON ) std::cout
#define H5IO_DEBUG_COUT if( verbosity_level == H5IO_VERBOSE_DEBUG ) std::cout

class H5SizeArray
{
private:
  int rank;
  hsize_t *array;
public:
  H5SizeArray(int rank_in)
  {
    rank = rank_in;
    array = new hsize_t[rank];
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

};


class H5SParams
{
private:
  int rank;

  void _setCount()
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

  void setDefaults(int rank_in, hsize_t *dims_in)
  {
    setRank(rank_in);
    dims.setValues(dims_in);
    maxdims.setValues(dims.getPtr());
    block.setValues(1);
    start.setValues((hsize_t) 0);
    stride.setValues(1);
    _setCount();
    chunk.setValues(dims.getPtr());
  }
  
  int getRank()
  {
    return rank;
  }

  void setHyperslabAppend(hid_t dset_id)
  {
    herr_t status;

    //checks if dspace for id exists if so closes it
    if(H5Sis_simple(id))
      status = H5Sclose(id); 

    //get id for dspace
    id = H5Dget_space(dset_id); 
    //initialize dspace params 
    setRank( H5Sget_simple_extent_ndims(id) ); 

    //get max dims and dims of dsapce
    status = H5Sget_simple_extent_dims(id, dims.getPtr(), maxdims.getPtr());
    
    block.setValues(1);

    //increase dspace dims by 1 (this is so dataset can be extended)
    dims[0] += 1;//block[0]; if wanted multi line write?
    
    //start at begining of 
    start.setValues((hsize_t) 0);

    //last line to start write
    start[0] = dims[0] - 1;

    //set stride and count
    stride.setValues(1);

    _setCount();
    //appending 1 line so fix count
    count[0] = 1;
    
    //seting hyperslab
    status = H5Sselect_hyperslab(id, H5S_SELECT_SET, start.getPtr(), stride.getPtr(), count.getPtr(), block.getPtr());
  }

  void setUndersampleHyperslab(hsize_t *start_in, hsize_t *stride_in)
  {
    herr_t status;

    block.setValues(1);
    
    //start at begining of 
    start.setValues(start_in);

    //set stride and count
    stride.setValues(stride_in);

    //set count
    _setCount();

    //checks if dspace for id exists if so closes it
    if(H5Sis_simple( id ))
      status = H5Sclose(id); 

    //get id for dspace. Assumes rank and dims set
    id = H5Screate_simple(rank, dims.getPtr(), maxdims.getPtr());

    //seting hyperslab
    status = H5Sselect_hyperslab(id, H5S_SELECT_SET, start.getPtr(), stride.getPtr(), count.getPtr(), block.getPtr());
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
    mem_dspace.setDefaults(mem_rank_in, mem_dims_in);
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

  bool _openOrCreateFile(std::string file_name, bool append_flag)
  {
    H5IO_DEBUG_COUT << "Seeing if file exists..." << std::flush;
    _pauseH5ErrorHandeling();
    file_id = H5Fopen(file_name.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    _resumeH5ErrorHandeling();

    if(file_id < 0)
    {
      if(append_flag)
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
    dset_dspace.setRank(mem_dspace.getRank()+1);
    dset_dspace.dims[0] = 0;
    
    for(int i = 1; i < dset_dspace.getRank(); ++i)
      dset_dspace.dims[i] = mem_dspace.dims[i-1] / mem_dspace.stride[i-1];

    dset_dspace.maxdims.setValues(dset_dspace.dims.getPtr());
    dset_dspace.maxdims[0] = H5S_UNLIMITED;

    dset_dspace.id = H5Screate_simple(dset_dspace.getRank(), dset_dspace.dims.getPtr(), dset_dspace.maxdims.getPtr());
    
    dset_dspace.chunk.setValues(dset_dspace.dims.getPtr());
    _setCompressionPList();

    dset_id = H5Dcreate(file_id, dset_name.c_str(), dset_dspace.type, dset_dspace.id, H5P_DEFAULT, dset_chunk_plist, H5P_DEFAULT);
    status = H5Dclose(dset_id);
    status = H5Sclose(dset_dspace.id);
  }

  void _createOpenDataset(std::string dset_name)
  {
    /* 
    * This shrinks the file dspace to be minimal dimensions i.e. if there is a 
    * mem_dspace.dim that is 1 it will skip over unless all are one then it 
    * sets the dset rank to 1 and dset_dspace.dim[0] = 1.
    */
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
  }

  bool _checkAppend()
  {
    if(dset_dspace.getRank() == 1 + mem_dspace.getRank())
    {
      if(dset_dspace.maxdims[0] == H5S_UNLIMITED || dset_dspace.dims[0] <= dset_dspace.maxdims[0])
      {
        for( int i = 1; i < dset_dspace.getRank(); ++i )
          if( dset_dspace.dims[i] < mem_dspace.dims[i-1]/mem_dspace.stride[i-1] )
            return false;
        return true;
      }
    }
    return false; 
       
  }

  void _setCompressionPList()
  {
    dset_chunk_plist = H5Pcreate(H5P_DATASET_CREATE);
    status = H5Pset_layout(dset_chunk_plist, H5D_CHUNKED);

    status = H5Pset_chunk(dset_chunk_plist, dset_dspace.getRank(), dset_dspace.chunk.getPtr());
    
    if(_checkCompression())
      status = H5Pset_deflate(dset_chunk_plist, compression_level);
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
    hsize_t mem_dims_in[mem_rank_in] = {mem_dim_in};
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
    mem_dspace.setUndersampleHyperslab(start_in, stride_in);
  }

  void setMemHyperslab1D(int print_dim, hsize_t *start_in, hsize_t stride_in)
  {
    H5SizeArray stride_arr(mem_dspace.getRank());

    //set stride too large so that count will be 1
    stride_arr.setValues(mem_dspace.dims.getPtr());
    //fix stride in print dim to be reasonable
    stride_arr[print_dim] = stride_in;
    
    mem_dspace.setUndersampleHyperslab(start_in, stride_arr.getPtr());
  }

  void setMemHyperslabNm1D(int drop_dim, hsize_t *start_in, hsize_t *stride_in)
  {
    H5SizeArray stride_arr(mem_dspace.getRank());
    
    //set stride from input
    stride_arr.setValues(stride_in);
    
    //ensure stride in drop dim is too large so count will be 1
    stride_arr[drop_dim]=mem_dspace.dims[drop_dim];

    mem_dspace.setUndersampleHyperslab(start_in, stride_arr.getPtr());
  }

  bool writeSampledArrayToFile(void *array, std::string file_name, std::string dset_name, bool append_flag)
  { 
    
    //status = H5Sselect_hyperslab(mem_dspace.id, H5S_SELECT_SET, mem_dspace.start.getPtr(), mem_dspace.stride.getPtr(), mem_dspace.count.getPtr(), mem_dspace.block.getPtr()); //selects parts of memory to write
    
    file_id = _openOrCreateFile(file_name,append_flag);
    
    if(append_flag)
    {
      //check if dataset does NOT exists
      if( ! H5LTfind_dataset(file_id, dset_name.c_str()) )
          _createCloseDatasetAppend(dset_name);
          
      dset_id = H5Dopen( file_id, dset_name.c_str() , H5P_DEFAULT);

      dset_dspace.setHyperslabAppend(dset_id);

      if(_checkAppend())
        status = H5Dset_extent(dset_id, dset_dspace.dims.getPtr());
      else
      {
        H5IO_VERBOSE_COUT << "Can't extend dataset for an append. Aborting write." << std::endl;
        return false;
      }
    }
    else //create new file
    {
      if( ! H5LTfind_dataset(file_id, dset_name.c_str()) )
      {
        H5IO_VERBOSE_COUT << "Can't write dataset to one that exists. Aborting write." << std::endl;
        return false;
      }
      else
        _createOpenDataset(dset_name);
    }
    
    status = H5Dwrite(dset_dspace.id, mem_dspace.type, mem_dspace.id, dset_dspace.id, H5P_DEFAULT, array);

    return true;
    _closeAllTheThings();//needs to close the things
  }

};
