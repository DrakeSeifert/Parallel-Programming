#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define ARRAYSIZE 4
#define NUMTESTS 6 //Number of times to simulate test
#define LOAD_DIVIDER 2 //How often to update user of test status
#define NUMPAD 16 //Padding to be applied to check cache line performance
                  //Tested NUMPAD 0-16

struct s
{
    float value;
    //int pad[NUMPAD];
} Array[ARRAYSIZE];

int main(int argc, char *argv[])
{
    #ifndef _OPENMP
        fprintf( stderr, "OpenMP is not available\n" );
        return 1;
    #endif

    if(argc != 2) {
        printf("USAGE: ./a.out numThreads\n");
        exit(2);
    }
    int NUMT = atoi(argv[1]); //Tested NUMT 1, 2, and 4

    printf("NumPadding: %d\n", NUMPAD);
    printf("NumThreads: %d\n", NUMT);

    omp_set_num_threads(NUMT);
    int someBigNumber = 1000000000;

    double maxMegaAdds = 0.;
    double sumMegaAdds = 0.;

    for(int t = 0; t < NUMTESTS; t++)
    {
        //Update user of how far along testing is going
        int loadingVal = NUMTESTS / LOAD_DIVIDER;
        if(!(t % loadingVal))
            printf("Test #%d\n", t);

        double time0 = omp_get_wtime();

        #pragma omp parallel for
        for(int i = 0; i < 4; i++)
        {
            float tmp = Array[i].value;
            for(int j = 0; j < someBigNumber; j++)
            {
                tmp += 2;
                //Array[i].value = Array[i].value + 2.;
            }
            Array[i].value = tmp;
        }

        double time1 = omp_get_wtime();

        double megaAdds = (double)ARRAYSIZE * someBigNumber / (time1 - time0) / 1000000.;
        sumMegaAdds += megaAdds;
        if(megaAdds > maxMegaAdds)
            maxMegaAdds = megaAdds;
    }

    double avgMegaAdds = sumMegaAdds / (double)NUMTESTS;

    //printf("Performance = %8.2lf MegaAdds/Sec\n", megaAdds);
    printf( "   Peak Performance = %8.2lf MegaAdds/Sec\n", maxMegaAdds);
    printf( "Average Performance = %8.2lf MegaAdds/Sec\n", avgMegaAdds);

    return 0;
}
