novec: proj5.cpp
	icpc -o novec proj5.cpp -lm -openmp -align -qopt-report=3 -qopt-report-phase=vec -no-vec

withvec: proj5.cpp
	icpc -o withvec proj5.cpp -lm -openmp -align -qopt-report=3 -qopt-report-phase=vec

clean:
	rm novec withvec
