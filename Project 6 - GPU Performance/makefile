proj: first.cpp first.cl
	g++ first.cpp -Wall /scratch/cuda-7.0/lib64/libOpenCL.so -lm -fopenmp

addMult: first2.cpp first2.cl
	g++ first2.cpp -o addMult -Wall /scratch/cuda-7.0/lib64/libOpenCL.so -lm -fopenmp

reduction:
	g++ first3.cpp -o reduction -Wall /scratch/cuda-7.0/lib64/libOpenCL.so -lm -fopenmp

clean:
	rm a.out addMult reduction
