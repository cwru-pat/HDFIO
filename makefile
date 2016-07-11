COMPILER = g++
OFLAG = 
FLAGS = -Wall -pedantic --std=c++11 $(OFLAG)
LINKS = -lhdf5
eNAME = test

do: compile 

compile: H5IO.o H5SizeArray.o H5SParams.o test.o
	$(COMPILER) $(FLAGS) H5IO.o H5SizeArray.o H5SParams.o test.o $(LINKS) -o $(eNAME)

clean:
	rm -f *.o

test: do
	./test
	h5dump test.h5
	rm test.h5 

H5SizeArray.o: H5SizeArray.cpp H5SizeArray.h
	$(COMPILER) -c $(FLAGS) H5SizeArray.cpp

H5SParams.o: H5SParams.cpp H5SParams.h H5SizeArray.h
	$(COMPILER) -c $(FLAGS) H5SParams.cpp

H5IO.o: H5IO.cpp H5IO.h H5SParams.h H5SizeArray.h
	$(COMPILER) -c $(FLAGS) H5IO.cpp

test.o: test.cpp H5IO.o H5SParams.o H5SizeArray.o H5IO.h H5SParams.h H5SizeArray.h
	$(COMPILER) -c $(FLAGS) test.cpp
