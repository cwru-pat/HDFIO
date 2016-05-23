
#include <iostream>
#include <hdf5.h>


// Constants
const char saveFilePath[] = "test.h5";
const hsize_t ndims = 2;
const hsize_t ncols = 3;

int main()
{
    // Create a hdf5 file
    hid_t file = H5Fcreate(saveFilePath, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    std::cout << "- File created" << std::endl;

    // Create a 2D dataspace
    // The size of the first dimension is unlimited (initially 0)
    // The size of the second dimension is fixed
    hsize_t dims[ndims] = {0, ncols};
    hsize_t max_dims[ndims] = {H5S_UNLIMITED, ncols};
    hid_t file_space = H5Screate_simple(ndims, dims, max_dims);
    std::cout << "- Dataspace created" << std::endl;

    // Create a dataset creation property list
    // The layout of the dataset have to be chunked when using unlimited dimensions
    hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
    //H5Pset_layout(plist, H5D_CHUNKED);
    // The choice of the chunk size affects performances
    // This is a toy example so we will choose one line
    hsize_t chunk_dims[ndims] = {5, ncols};
    H5Pset_chunk(plist, ndims, chunk_dims);
    H5Pset_deflate(plist, 9);
    std::cout << "- Property list created" << std::endl;

    // Create the dataset 'dset1'
    hid_t dset = H5Dcreate(file, "dset1", H5T_NATIVE_FLOAT, file_space, H5P_DEFAULT, plist, H5P_DEFAULT);
    std::cout << "- Dataset 'dset1' created" << std::endl;

    // Close resources
    H5Pclose(plist);
    H5Sclose(file_space);
    // We don't need the file dataspace anymore because when the dataset will be extended,
    // we will have to grab the updated file dataspace anyway

    // We will now append two buffers to the end of the datset
    // The first one will be two lines long
    // The second one will be three lines long

    // ## First buffer

    // Create 2D buffer (contigous in memory, row major order)
    // We will allocate enough memory to store 3 lines, so we can reuse the buffer
    hsize_t nlines = 3;
    float *buffer = new float[nlines * ncols];
    // Let us create an array of pointers so we can use the b[i][j] notation
    // instead of buffer[i * ncols + j]
    float **b = new float*[nlines];
    for (hsize_t i = 0; i < nlines; ++i){
        b[i] = &buffer[i * ncols];
    }

    // Initial values in buffer to be written in the dataset
    b[0][0] = 0.1;
    b[0][1] = 0.2;
    b[0][2] = 0.3;
    b[1][0] = 0.4;
    b[1][1] = 0.5;
    b[1][2] = 0.6;

    // Create a memory dataspace to indicate the size of our buffer
    // Remember the first buffer is only two lines long
    dims[0] = 2;
    dims[1] = ncols;
    hid_t mem_space = H5Screate_simple(ndims, dims, NULL);
    std::cout << "- Memory dataspace created" << std::endl;

    // Extend dataset
    // We set the initial size of the dataset to 0x3, we thus need to extend it first
    // Note that we extend the dataset itself, not its dataspace
    // Remember the first buffer is only two lines long
    dims[0] = 2;
    dims[1] = ncols;
    H5Dset_extent(dset, dims);
    std::cout << "- Dataset extended" << std::endl;

    // Select hyperslab on file dataset
    file_space = H5Dget_space(dset);
    hsize_t start[2] = {0, 0};
    hsize_t count[2] = {2, ncols};
    H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);
    std::cout << "- First hyperslab selected" << std::endl;

    // Write buffer to dataset
    // mem_space and file_space should now have the same number of elements selected
    // note that buffer and &b[0][0] are equivalent
    H5Dwrite(dset, H5T_NATIVE_FLOAT, mem_space, file_space, H5P_DEFAULT, buffer);
    std::cout << "- First buffer written" << std::endl;
    
    // We can now close the file dataspace
    // We could close the memory dataspace and create a new one,
    // but we will simply update its size
    H5Sclose(file_space);

    // ## Second buffer

    // New values in buffer to be appended to the dataset
    b[0][0] = 1.1;
    b[0][1] = 1.2;
    b[0][2] = 1.3;
    b[1][0] = 1.4;
    b[1][1] = 1.5;
    b[1][2] = 1.6;
    b[2][0] = 1.7;
    b[2][1] = 1.8;
    b[2][2] = 1.9;

    // Resize the memory dataspace to indicate the new size of our buffer
    // The second buffer is three lines long
    dims[0] = 3;
    dims[1] = ncols;
    H5Sset_extent_simple(mem_space, ndims, dims, NULL);
    std::cout << "- Memory dataspace resized" << std::endl;

    // Extend dataset
    // Note that in this simple example, we know that 2 + 3 = 5
    // In general, you could read the current extent from the file dataspace
    // and add the desired number of lines to it
    dims[0] = 5;
    dims[1] = ncols;
    H5Dset_extent(dset, dims);
    std::cout << "- Dataset extended" << std::endl;

    // Select hyperslab on file dataset
    // Again in this simple example, we know that 0 + 2 = 2
    // In general, you could read the current extent from the file dataspace
    // The second buffer is three lines long
    file_space = H5Dget_space(dset);
    start[0] = 2;
    start[1] = 0;
    count[0] = 3;
    count[1] = ncols;
    H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);
    std::cout << "- Second hyperslab selected" << std::endl;

    // Append buffer to dataset
    H5Dwrite(dset, H5T_NATIVE_FLOAT, mem_space, file_space, H5P_DEFAULT, buffer);
    std::cout << "- Second buffer written" << std::endl;

    // Close resources
    delete[] b;
    delete[] buffer;
    H5Sclose(file_space);
    H5Sclose(mem_space);
    H5Dclose(dset);
    H5Fclose(file);
    std::cout << "- Resources released" << std::endl;
}