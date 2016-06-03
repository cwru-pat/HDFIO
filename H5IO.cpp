#include <hdf5.h>
#include <iostream>
#include <string>

#include "H5SizeArray.h"
#include "H5SParams.h"
#include "H5IO.h"

#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION "[" __FILE__ ":" S2(__LINE__) "] "

#define H5IO_VERBOSE_COUT if( verbosity_level >= verbose ) std::cout << \
  (verbosity_level == debug ? LOCATION : "")
#define H5IO_DEBUG_COUT if( verbosity_level == debug ) std::cout << LOCATION

/**
 * @brief Private initialization function
 * @details Initializes the member elements of the H5IO class.
 * 
 * @param mem_rank_in Integer for rank of array in memory
 * @param mem_dims_in H5SizeArray containing the dimensions of the array in memory
 * @param mem_type_in H5 memory type for the array in memory. 
 * 	See Table 5 and 6 in https://www.hdfgroup.org/HDF5/doc1.6/UG/11_Datatypes.html
 */
void H5IO::_initialize(int mem_rank_in, H5SizeArray &mem_dims_in, hid_t mem_type_in)
{
  H5IO_DEBUG_COUT << "Stashing H5 error handeling parameters..." << std::flush;
  H5IO_DEBUG_COUT << "Done!" << std::endl;
  compression_level = 9;
  mem_dspace.type=mem_type_in;
  dset_dspace.type=mem_type_in;
  mem_dspace.setDefaults(mem_rank_in, mem_dims_in);
  dset_dspace.setDefaults(mem_rank_in, mem_dims_in);
  mem_dspace.createSpace();
}

/**
 * @brief Pause H5's default error handeling
 * @details Enables you to run checks on H5 comands you expect to fail. 
 * 		Saves defaults to member variables default_error_func and default_error_out.
 *
 */
void H5IO::_pauseH5ErrorHandeling()
{
	H5Eget_auto(H5E_DEFAULT, &default_error_func, &default_error_out);
  H5Eset_auto(H5E_DEFAULT,NULL,NULL);
}

/**
 * @brief Resume H5's default error handeling
 * @details Restores H5's error handeling using the saved defaults 
 * 		in member variables default_error_func and default_error_out.
 */
void H5IO::_resumeH5ErrorHandeling()
{
  H5Eset_auto(H5E_DEFAULT,default_error_func,default_error_out);
}

bool H5IO::_openOrCreateFile(std::string file_name, bool read_flag)
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
    H5IO_DEBUG_COUT << "No such file." << std::endl << std::flush;
    H5IO_DEBUG_COUT << "Creating file instead..." << std::flush;
    file_id = H5Fcreate(file_name.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
  }
  else
  {
    H5Fclose(file_id);
    H5IO_DEBUG_COUT << "File exists." << std::endl << std::flush;
    H5IO_DEBUG_COUT << "Opening file..." << std::flush;
    file_id = H5Fopen(file_name.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
  }
  H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;
  return true;
}

bool H5IO::_checkCompression()
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
  H5IO_VERBOSE_COUT << "Cant compress with gzip. Turning off compression." << std::endl;
  return false;
}

void H5IO::_createCloseDatasetAppend(std::string dset_name)
{
  H5IO_DEBUG_COUT << "Creating dataset for appending...." << std::endl << std::flush;

  H5IO_DEBUG_COUT << "  Formating dataspace..." << std::flush;
  dset_dspace.setRank(mem_dspace.getRank()+1);

  dset_dspace.dims[0] = 0;

  for(int i = 1; i < dset_dspace.getRank(); ++i)
    dset_dspace.dims[i] = mem_dspace.count[i-1] * mem_dspace.block[i-1];

  dset_dspace.maxdims = dset_dspace.dims;
  dset_dspace.maxdims[0] = H5S_UNLIMITED;
  H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

  H5IO_DEBUG_COUT << "  Creating dataspace..." << std::flush;
  dset_dspace.createSpace();
  H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

  dset_dspace.chunk = dset_dspace.dims;
  dset_dspace.chunk[0] = 1; // so that chunk has positive values
  _setCompressionPList();

  H5IO_DEBUG_COUT << "  Creating dataset..." << std::flush;
  dset_id = H5Dcreate(file_id, dset_name.c_str(), dset_dspace.type, dset_dspace.id, H5P_DEFAULT, dset_chunk_plist, H5P_DEFAULT);
  H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

  H5IO_DEBUG_COUT << "  Closing... " << std::flush;
  status = H5Dclose(dset_id);
  H5IO_DEBUG_COUT << "Dataset... " << std::flush;
  status = dset_dspace.closeSpace();
  H5IO_DEBUG_COUT << "Dataspace... " << std::flush;
  status = H5Pclose(dset_chunk_plist);
  H5IO_DEBUG_COUT << "Compression plist... Done!" << std::flush;

  H5IO_DEBUG_COUT << "Finisehd creating dataset!" << std::endl << std::flush;
}

void H5IO::_createOpenDataset(std::string dset_name)
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
    int j = 0;
    for(int i = 0; i < mem_dspace.getRank(); ++i)
      if( mem_dspace.block[i] * mem_dspace.count[i] > 1 )
      {
        dset_dspace.dims[j] = mem_dspace.block[i] * mem_dspace.count[i];
        j++;
      }
  }

  dset_dspace.maxdims = dset_dspace.dims;


  dset_dspace.createSpace();

  dset_dspace.chunk = dset_dspace.dims;
  _setCompressionPList();

  dset_id = H5Dcreate(file_id, dset_name.c_str(), dset_dspace.type, dset_dspace.id, H5P_DEFAULT, dset_chunk_plist, H5P_DEFAULT);
  H5Pclose(dset_chunk_plist);
}

bool H5IO::_checkAppend()
{
  if(dset_dspace.getRank() == 1 + mem_dspace.getRank())
  {
    if(dset_dspace.maxdims[0] == H5S_UNLIMITED || dset_dspace.dims[0] <= dset_dspace.maxdims[0])
    {
      for( int i = 1; i < dset_dspace.getRank(); ++i )
        if( dset_dspace.dims[i] != mem_dspace.count[i-1]*mem_dspace.block[i-1]) {
          H5IO_DEBUG_COUT << "Here..." << i << dset_dspace.dims[i] << mem_dspace.count[i-1] << mem_dspace.block[i-1] << std::flush;
          return false;
        }
      return true;
    }
  }
  return false;

}

bool H5IO::_checkDatasetExists(std::string dset_name)
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


void H5IO::_setCompressionPList()
{
  dset_chunk_plist = H5Pcreate(H5P_DATASET_CREATE);
  status = H5Pset_layout(dset_chunk_plist, H5D_CHUNKED);

  status = H5Pset_chunk(dset_chunk_plist, dset_dspace.getRank(), dset_dspace.chunk.getPtr());

  if(_checkCompression())
    status = H5Pset_deflate(dset_chunk_plist, compression_level);
}

bool H5IO::_setAppend()
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
  H5IO_DEBUG_COUT << "  Getting dataspace parameters..." << std::flush;
  status = H5Sget_simple_extent_dims(dset_dspace.id, dset_dspace.dims.getPtr(), dset_dspace.maxdims.getPtr());
  H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

  H5IO_DEBUG_COUT << "  Closing dataspace..." << std::flush;
  status = dset_dspace.closeSpace();
  H5IO_DEBUG_COUT << "Done!" << std::endl << std::flush;

  H5IO_DEBUG_COUT << "  Formating datspace for next row..." << std::flush;
  dset_dspace.stride.setValues(1);
  dset_dspace.block.setValues(1);
  dset_dspace.start.setValues(0);

  //start at end of current last line begining of
  dset_dspace.start[0] = dset_dspace.dims[0];

  //increase dspace dims by 1 (this is so dataset can be extended)
  dset_dspace.dims[0] += 1;//block[0]; if wanted multi line write?
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

void H5IO::_closeFileThings()
{
  H5Fclose(file_id);
  H5Dclose(dset_id);
}

H5IO::H5IO(int mem_rank_in, H5SizeArray &mem_dims_in, hid_t mem_type_in)
: mem_dspace(), dset_dspace()
{
  _initialize(mem_rank_in, mem_dims_in, mem_type_in);
}

H5IO::H5IO(int mem_rank_in, hsize_t mem_dim_in, hid_t mem_type_in)
: mem_dspace(), dset_dspace()
{

  H5SizeArray mem_dims_in(mem_rank_in);
  mem_dims_in.setValues(mem_dim_in);

  _initialize(mem_rank_in, mem_dims_in, mem_type_in);

}

H5IO::~H5IO()
{
  mem_dspace.closeSpace();
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
void H5IO::setVerbosity(int verbosity_in)
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
void H5IO::setDatasetType(hid_t dataset_type_in)
{
  dset_dspace.type = dataset_type_in;
}

void H5IO::setMemHyperslab(H5SizeArray &start_in, H5SizeArray &stride_in)
{
  mem_dspace.start = start_in;
  mem_dspace.stride = stride_in;

  mem_dspace.setHyperslab();
}

void H5IO::setMemHyperslab1D(int print_dim, H5SizeArray &start_in, hsize_t stride_in)
{
  mem_dspace.start = start_in;
  //set stride too large so that count will be 1
  mem_dspace.stride = mem_dspace.dims;
  //fix stride in print dim to be reasonable
  mem_dspace.stride[print_dim] = stride_in;

  mem_dspace.setHyperslab();
}

void H5IO::setMemHyperslabNm1D(int drop_dim, H5SizeArray &start_in, H5SizeArray &stride_in)
{
  mem_dspace.start = start_in;
  mem_dspace.stride = stride_in;

  //ensure stride in drop dim is too large so count will be 1
  mem_dspace.stride[drop_dim] = mem_dspace.dims[drop_dim];

  mem_dspace.setHyperslab();
}

bool H5IO::writeArrayToFile(void *array, std::string file_name, std::string dset_name, bool append_flag)
{
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


  status = dset_dspace.closeSpace();
  _closeFileThings();

  return true;
}
