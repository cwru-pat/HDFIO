#include <hdf5.h>
#include <iostream>
#include <string>

#define H5IO_VERBOSE_OFF 0
#define H5IO_VERBOSE_ON 1
#define H5IO_VERBOSE_DEBUG 2

#define H5IO_VERBOSE_COUT if(verbosity_level >= H5IO_VERBOSE_ON) std::cout
#define H5IO_DEBUG_COUT if(verbosity_level == H5IO_VERBOSE_DEBUG) std::cout

class H5SizeArray
{
private:
  hsize_t *array;
public:
  H5SizeArray(int rank)
  {
    array = new hsize_t[rank];
  }

  ~H5SizeArray()
  {
    delete[] array;
  }

  hszie_t& operator[](int idx)
  {
    return array[i];
  }

  void setValues(hsize_t value)
  {
    for(int i = 0; i < rank; ++i)
      array[i] = value;
  }

  void setValues(hsize_t values[])
  {
    for(int i = 0; i < rank; ++i)
      array[i] = values[i];
  }

};


class H5SParams
{
private:
  int rank

  _delete()
  {
    delete dims;
    delete maxdims;
    delete start;
    delete stride;
    delete count;
    delete block;
    delete chunk; 
  }

  _newSize(int rank)
  {
    _delete();
    dims = new H5SizeArray(rank);
    maxdims = new H5SizeArray(rank);
    start = new H5SizeArray(rank);
    stride = new H5SizeArray(rank);
    count = new H5SizeArray(rank);
    block = new H5SizeArray(rank);
    chunk = new H5SizeArray(rank);
  }

public:
  hid_t type;
  hid_t id;
  H5SizeArray *dims, //set by user
              *maxdims, //set to NULL (completely unused)
              *start,
              *stride,
              *count,
              *block,
              *chunk;

  H5SParams()
  {
  }

  H5SParams(int rank_in)
  {
    setRank(rank_in);
  }

  void setRank( rank_in )
  {
    rank = rank_in;
    
    _newSize(rank);

  }

  ~H5SParams()
  {
    _delete();
  }

  void setDefaults(int rank_in, hsize_t *dims_in)
  {
    setRank(rank_in);
    dims.setValues(dims_in);
    maxdims.setValues(dims);
    start.setValues(0);
    setStride(1);
    //stride.setValues(1);
    //count.setValues(dims);
    block.setValues(1);
    chunk.setValues(dims);
  }

  void setStride(hsize_t *stride_in)
  {
    stride.setValues(stride)
    for(int i = 0; i < rank; ++i)
      count[i] = dims[i] / stride[i];
  }

  void setStride(hsize_t stride_in)
  {
    stride.setValues(stride)
    for(int i = 0; i < rank; ++i)
      count[i] = dims[i] / stride[i];
  }

  int rank()
  {
    return rank;
  }

  void setHyperslabAppend(hid_t dset_id)
  {
    herr_t status;
    //get id for dspace
    id = H5Dget_space(dset_id); 
    //initialize dspace params 
    setRank( H5Sget_simple_extent_ndims(id) ); 

    //get max dims and dims of dsapce
    status = H5Sget_simple_extent_dims(id, dims[], maxdims[]);
    
    block.setValues(1);

    //increase dspace dims by 1 (this is so dataset can be extended)
    dims[0] += 1;//block[0]; if wanted multi line write?
    
    //set stride and count
    setStride(1);
    //appending 1 line so fix count
    count[0] = 1;

    //start at begining of 
    start.setValues(0);
    //last line to start write
    start[0] = dims[0] - 1;
    
    //seting hyperslab
    status = H5Sselect_hyperslab(id, H5S_SELECT_SET, start[], stride[], count[], block[]);
  }

  void setNm1DHyperslab(int drop_dim, hsize_t start_drop_dim, hsize_t stride_in)
  {
    herr_t status;
    //get id for dspace. Assumes rank and dims set
    id = H5Screate_simple(rank, dims[], maxdims[]);
    
    block.setValues(1);
    
    //set stride and count
    setStride(stride_in);
    
    //write slab in cut dim
    count[drop_dim] = 1;

    //start at begining of 
    start.setValues(0);
    //last line to start write
    start[drop_dim] = start_drop_dim;
    
    //seting hyperslab
    status = H5Sselect_hyperslab(id, H5S_SELECT_SET, start[], stride[], count[], block[]);
  }

  void set1DHyperslab(int print_dim, hsize_t * start_in, hsize_t stride_in)
  {
    herr_t status;
    //get id for dspace. Assumes rank and dims set
    id = H5Screate_simple(rank, dims[], maxdims[]);
    
    block.setValues(1);
    
    //start at begining of 
    start.setValues(start_in);

    //set stride
    stride.setValues(stride_in);
    //set count
    count.setValues(1);
    //out dim
    count[print_dim] = (dims[print_dim] - start[print_dim]) / strid[print_dim];

    
    //rod dimm;
    start[drop_dim] = 0;
    
    //seting hyperslab
    status = H5Sselect_hyperslab(id, H5S_SELECT_SET, start[], stride[], count[], block[]);
  }
  void setNDUndersampleHyperslab(hsize_t * start_in, hsize_t * stride_in)
  {
    start.setValues(start_in);
    herr_t status;
    //get id for dspace. Assumes rank and dims set
    id = H5Screate_simple(rank, dims[], maxdims[]);
    
    block.setValues(1);
    
    //set stride and count
    stride.setValues(stride_in);
    //output rod in

    //out dim
    count[print_dim] = dims[print_dim] / strid[print_dim];

    //start at begining of 
    start.setValues(start_in);
    //rod dimm;
    start[drop_dim] = 0;
    
    //seting hyperslab
    status = H5Sselect_hyperslab(id, H5S_SELECT_SET, start[], stride[], count[], block[]);
  }

};

class H5IO
{
private:
  H5SParams mem_dspace(); //holds memory dataspace things
  H5SParams dset_dspace(); //holds file dataset dataspace things
  herr_t status;
  hid_t file_id;
  hid_t dset_id;
  hid_t dset_chunk_plist;
  int compression_level;
  int verbosity_level;

  void _initialize(int mem_rank_in, hsize_t *mem_dims_in, hid_t mem_type_in)
  {
    compression_level = 9;
    mem_dspace.type=mem_type_in;
    mem_dspace.setDefaults(mem_rank_in, hsize_t mem_dims_in);
    mem_dspace.id = H5Screate_simple(mem_dspace.rank(), mem_dspace.dims[], mem_dspace.maxdims[]);
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

  bool _openOrCreateFile(std::sting file_name, bool read_flag)
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
    HDFIO_VERBOSE_COUT << "Cant compress with gzip. Truning off compression." << std::endl;
    return false;
  }

  void _createCloseDatasetAppend(std::string dset_name)
  {
    dset_dspace.setRank(mem_dspace.rank()+1);
    dset_dspace.dims[0] = 0;
    
    for(int i = 1; i < dset_dspace_rank; ++i)
      dset_dspace.dims[i] = mem_dspace.dims[i-1] / mem_dspace.stride[i-1];

    dset_dspace.maxdims.setValues(dset_dspace_dims[]);
    dset_dspace.maxdims[0] = H5S_UNLIMITED;

    dset_dspace.id = H5Screate_simple(dset_dspace.rank(), dset_dspace.dims[], dset_dspace.maxdims[]);
    
    dset_dspace.chunk.setValues(dset_dspace.dims[]);
    _setCompressionPList();

    dset_id = H5Dcreate(file_id, dset_name, dset_dspace.type, dset_dspace.id, H5P_DEFAULT, dset_chunk_plist, H5P_DEFAULT);
    status = H5Dclose(dset_id);
    status = H5Sclose(dset_dspace.id);
  }

  void _createOpenDataset(std::string dset_name)
  {
    dset_dspace.setRank(mem_dspace.rank());
    
    for(int i = 0; i < dset_dspace_rank; ++i)
      dset_dspace.dims[i] = mem_dspace.dims[i] / mem_dspace.stride[i];

    dset_dspace.maxdims.setValues(dset_dspace_dims[]);


    dset_dspace.id = H5Screate_simple(dset_dspace.rank(), dset_dspace.dims[], dset_dspace.maxdims[]);
    
    dset_dspace.chunk.setValues(dset_dspace.dims[]);
    _setCompressionPList();

    dset_id = H5Dcreate(file_id, dset_name.c_str(), dset_dspace.type, dset_dspace.id, H5P_DEFAULT, dset_chunk_plist, H5P_DEFAULT);
  }

  bool _checkAppend()
  {
    if(dset_dspace.rank() == 1 + mem_dspace.rank())
      if(dset_dspace.maxdims[0] == H5S_UNLIMITED || dset_dspace.dims[0] <= dset_dspace.maxdims[0])
      {
        for( int i = 1; i < file_dspace_rank; ++i )
          if( dset_dspace.dims[i] < mem_dspace.dims[i-1]/mem_dspace.stride[i-1] )
            return false;
        return true;
      }
    return false; 
       
  }

  void _setCompressionPList()
  {
    dset_chunk_plist = H5Pcreate(H5P_DATASET_CREATE);
    status = H5Pset_layout(dset_chunk_plist, H5D_CHUNKED);

    status = H5Pset_chunk(dset_chunk_plist, dset_dspace.rank(), dset_dspace.chunk[]);
    
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
  {
    _initialize(mem_rank_in, mem_dims_in, mem_type_in);
  }

  H5IO(int mem_rank_in, hsize_t mem_dim_in, hid_t mem_type_in)
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
    dset_dspace.type = dataset_type_in;
  }

  void setMemStride(hsize_t *stride_in)
  {
    mem_dspace.setStride(stride_in);
  }

  void setMemStride(hsize_t stride_in)
  {
    mem_dspace.setStride(stride_in);
  }

  bool writeSampledArrayToFile(void *array, std::string file_name, std::string dset_name, bool append_flag)
  { 
    
    status = H5Sselect_hyperslab(mem_dspace.id, H5S_SELECT_SET, mem_dspace.start[], mem_dspace.stride[], mem_dspace.count[], mem_dspace.block[]); //selects parts of memory to write
    
    file_id = _openOrCreateFile(file_name,read_flag);
    
    if(append_flag)
    {
      //check if dataset does NOT exists
      if( ! H5LTfind_dataset(file_id, dset_name.c_str()) )
          _createCloseDatasetAppend(dset_name);
          
      dset_id = H5Dopen( file_id, dataset_name.c_str() );

      dset_dspace.setHyperslabAppend(dset_id);

      if(_checkAppend())
        status = H5Dset_extent(dset_id, dset_dspace.dims[]);
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

    _closeAllTheThings();//needs to close the things
  }

}