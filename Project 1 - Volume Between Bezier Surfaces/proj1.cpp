#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define XMIN	0.
#define XMAX	3.
#define YMIN	0.
#define YMAX	3.

#define TOPZ00  0.
#define TOPZ10  1.
#define TOPZ20  0.
#define TOPZ30  0.

#define TOPZ01  1.
#define TOPZ11  6.
#define TOPZ21  1.
#define TOPZ31  0.

#define TOPZ02  0.
#define TOPZ12  1.
#define TOPZ22  0.
#define TOPZ32  4.

#define TOPZ03  3.
#define TOPZ13  2.
#define TOPZ23  3.
#define TOPZ33  3.

#define BOTZ00  0.
#define BOTZ10  -3.
#define BOTZ20  0.
#define BOTZ30  0.

#define BOTZ01  -2.
#define BOTZ11  10.
#define BOTZ21  -2.
#define BOTZ31  0.

#define BOTZ02  0.
#define BOTZ12  -5.
#define BOTZ22  0.
#define BOTZ32  -6.

#define BOTZ03  -3.
#define BOTZ13   2.
#define BOTZ23  -8.
#define BOTZ33  -3.

int NUMNODES; 	//Defined in command line argument
int NUMT;		//Defined in command line argument

double Height( int iu, int iv )	// iu,iv = 0 .. NUMNODES-1
{
	double u = (double)iu / (double)(NUMNODES-1);
	double v = (double)iv / (double)(NUMNODES-1);

	// the basis functions:

	double bu0 = (1.-u) * (1.-u) * (1.-u);
	double bu1 = 3. * u * (1.-u) * (1.-u);
	double bu2 = 3. * u * u * (1.-u);
	double bu3 = u * u * u;

	double bv0 = (1.-v) * (1.-v) * (1.-v);
	double bv1 = 3. * v * (1.-v) * (1.-v);
	double bv2 = 3. * v * v * (1.-v);
	double bv3 = v * v * v;

	// finally, we get to compute something:


    double top =       bu0 * ( bv0*TOPZ00 + bv1*TOPZ01 + bv2*TOPZ02 + bv3*TOPZ03 )
                    + bu1 * ( bv0*TOPZ10 + bv1*TOPZ11 + bv2*TOPZ12 + bv3*TOPZ13 )
                    + bu2 * ( bv0*TOPZ20 + bv1*TOPZ21 + bv2*TOPZ22 + bv3*TOPZ23 )
                    + bu3 * ( bv0*TOPZ30 + bv1*TOPZ31 + bv2*TOPZ32 + bv3*TOPZ33 );

    double bot =       bu0 * ( bv0*BOTZ00 + bv1*BOTZ01 + bv2*BOTZ02 + bv3*BOTZ03 )
                    + bu1 * ( bv0*BOTZ10 + bv1*BOTZ11 + bv2*BOTZ12 + bv3*BOTZ13 )
                    + bu2 * ( bv0*BOTZ20 + bv1*BOTZ21 + bv2*BOTZ22 + bv3*BOTZ23 )
                    + bu3 * ( bv0*BOTZ30 + bv1*BOTZ31 + bv2*BOTZ32 + bv3*BOTZ33 );

    return top - bot;	// if the bottom surface sticks out above the top surface
			// then that contribution to the overall volume is negative
}

double calcVolume(int me, int total, int num, double fullTileArea, double halfTileArea, double quarterTileArea)
{
	int first = (num / total) * me;
	int last = ((num / total) * (me + 1)) - 1;
	double volume = 0.;
	//for(int i = numNodesSquared * omp_get_thread_num() / NUMT; i <= (numNodesSquared * (omp_get_thread_num() + 1) / NUMT) - 1; i++)
	for(int i = first; i <= last; i++)
	{
		//printf("Thread #%d\ti = %d\n", omp_get_thread_num(), i);

		int iu = i % NUMNODES;
		int iv = i / NUMNODES;

		double tileMultiplier = 0.;

		if( iu == 0 ||
			iv == 0 ||
			iu == NUMNODES - 1 ||
			iv == NUMNODES - 1 )
		{
			if( (iu == 0 && iv == 0) ||
				(iu == NUMNODES - 1 && iv == 0) ||
				(iu == 0 && iv == NUMNODES - 1) ||
				(iu == NUMNODES - 1 && iv == NUMNODES - 1))
			{
				tileMultiplier = quarterTileArea;
			} else {
				tileMultiplier = halfTileArea;
			}
		} else {
			tileMultiplier = fullTileArea;
		}

		volume = volume + Height(iu, iv) * tileMultiplier;
	}
	return volume;
}

int main( int argc, char *argv[ ] )
{
	#ifndef _OPENMP
		fprintf( stderr, "OpenMP is not supported here -- sorry.\n" );
       	return 1;
	#endif

	if(argc != 4) {
		printf("USAGE: ./a.out numTries numNodes numThreads\n");
		exit(1);
	}
	int NUMTRIES = atoi(argv[1]);
	NUMNODES = atoi(argv[2]);
	NUMT = atoi(argv[3]);

	printf("\nNUMTRIES: %d\n", NUMTRIES);
	printf("NUMNODES: %d\n", NUMNODES);
	printf("NUMT: %d\n\n", NUMT);

	// the area of a single full-sized tile:

	double fullTileArea = (  ( ( XMAX - XMIN )/(double)(NUMNODES-1) )  *
				( ( YMAX - YMIN )/(double)(NUMNODES-1) )  );

	double halfTileArea = fullTileArea / 2.;
	double quarterTileArea = fullTileArea / 4.;

	// sum up the weighted heights into the variable "volume"
	// using an OpenMP for loop and a reduction:

	double volume = 0.;

	omp_set_num_threads( NUMT );

	double maxMegaHeightsPerSecond = 0.;
    double sumMegaHeightsPerSecond = 0.;
	int numNodesSquared = NUMNODES*NUMNODES;

	int total = NUMT;
    for(int t = 0; t < NUMTRIES; t++)
    {

		double time0 = omp_get_wtime( );
		#pragma omp parallel reduction(+:volume), shared(total), shared(numNodesSquared), shared(fullTileArea), shared(halfTileArea), shared(quarterTileArea)
		{
			int me = omp_get_thread_num();
			volume = volume + calcVolume(me, total, numNodesSquared, fullTileArea, halfTileArea, quarterTileArea);
		}
		double time1 = omp_get_wtime( );

		double megaMults = numNodesSquared/(time1-time0)/1000000.;
		sumMegaHeightsPerSecond += megaMults;
		if(megaMults > maxMegaHeightsPerSecond)
			maxMegaHeightsPerSecond = megaMults;

		if(t != NUMTRIES - 1) //If not the last thread pool
			volume = 0.; //Reset volume
	}

	double avgMegaMults = sumMegaHeightsPerSecond/(double)NUMTRIES;
	printf( "   Peak Performance = %8.2lf MegaMults/Sec\n", maxMegaHeightsPerSecond );
	printf( "Average Performance = %8.2lf MegaMults/Sec\n", avgMegaMults );
	printf( "             Volume = %8.2f Units\n\n", volume);


	return 0;
}
