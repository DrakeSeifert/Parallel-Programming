#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <math.h>

int main(int argc, char *argv[])
{
    #ifndef _OPENMP
        fprintf( stderr, "OpenMP is not available\n" );
        return 1;
    #endif

    if(argc != 4) {
        printf("USAGE: ./a.out numThreads DataSetSize NumTests\n");
        exit(2);
    }
    int NUMT = atoi(argv[1]); //Tested NUMT 1, 2, and 4
    int NUMS = atoi(argv[2]); //Test NUMS 1000, 100000, 30000000, 64000000
    int NUMTESTS = atoi(argv[3]);

    printf("---------------------------------------\n");
    printf("NumThreads:  %d\n", NUMT);
    printf("DataSetSize: %d\n", NUMS);
    printf("NumTests:    %d\n", NUMTESTS);

    float *A;
    A = new float [NUMS];
    float *B;
    B = new float [NUMS];

    srand(time(NULL));
    for(int i = 0; i < NUMS; i++) {
    	A[i] = rand() % 2147483647;
    }

    omp_set_num_threads(NUMT);
	double maxMegaMults = 0.;
    double sumMegaMults = 0.;

	for(int t = 0; t < NUMTESTS; t++)
    {
        double time0 = omp_get_wtime();

        #pragma omp parallel for
        for( int i = 0; i < NUMS; i++ )
        {
        	B[i] = sqrt(A[i]);
        }

		double time1 = omp_get_wtime();

		double megaMults = (double)NUMS / (time1-time0) / 1000000.;
        sumMegaMults += megaMults;
        if(megaMults > maxMegaMults)
        	maxMegaMults = megaMults;
    }

	double avgMegaMults = sumMegaMults/(double)NUMTESTS;
	printf( "   Peak Performance = %8.2lf MegaMults/Sec\n", maxMegaMults);
	printf( "Average Performance = %8.2lf MegaMults/Sec\n", avgMegaMults);

	delete [] A;
	delete [] B;

    return 0;
}