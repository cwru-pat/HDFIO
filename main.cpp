#include "HDFIO.h"
#include <iostream>

int main()
{
	float f[2000*1000];
	for(int i = 0; i<2000*1000; ++i)
		f[i]=rand()/((float) RAND_MAX);
	std::cout << f[0] <<std::endl << std::flush;
	hsize_t dims[2] = {2000,1000};
	HDFIO<float,int> myIO(dims, 2, HDFIO_VERBOSE_DEBUG);
	myIO.setHyperslabParameters(1);
	myIO.writeArrayToFile(f,"test.h5.gz","data1");
	return 0;
}