#include "HDFIO.h"
#include <iostream>

int main()
{
	#define ARRAY_RANK 2
	hsize_t dims[ARRAY_RANK] = {2000,1000};
	hsize_t gridsize=1;
	for(int i = 0; i<ARRAY_RANK; ++i)
		gridsize*=dims[i];
	float f[gridsize];
	for(hsize_t i = 0; i<gridsize; ++i)
		f[i]=rand()/((float) RAND_MAX);
	std::cout << f[0] <<std::endl << std::flush;
	HDFIO myIO(dims, ARRAY_RANK, H5T_NATIVE_FLOAT, HDFIO_VERBOSE_DEBUG);
	myIO.setDatasetType(H5T_NATIVE_DOUBLE);
	myIO.setHyperslabParameters(1);
	myIO.writeArrayToFile(f,"test.h5.gz","data1");
	return 0;
}