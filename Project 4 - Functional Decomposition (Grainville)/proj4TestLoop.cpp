#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#define NUMTESTS 1000

const float GRAIN_GROWS_PER_MONTH =             8.0;
const float ONE_DEER_EATS_PER_MONTH =           0.5;

const float AVG_PRECIP_PER_MONTH =              6.0;
const float AMP_PRECIP_PER_MONTH =              6.0;
const float RANDOM_PRECIP =                     2.0;

const float AVG_TEMP =                          50.0;
const float AMP_TEMP =                          20.0;
const float RANDOM_TEMP =                       10.0;

const float MIDTEMP =                           40.0;
const float MIDPRECIP =                         10.0;

int  NowYear;           // 2014 - 2019
int  NowMonth;          // 0 - 11

float NowPrecip;        // Inches of rain per month
float NowTemp;          // Temperature this month
float NowHeight;        // Grain height in inches
int  NowNumDeer;        // Current deer population

int SimulationStep;		// Number of simulations ran

int unsigned seed = 0;

float Ranf(float low, float high, unsigned int* seed)
{
    float r = (float) rand_r(seed); // 0 - RAND_MAX
    return(low + r * (high - low) / (float)RAND_MAX);
}

float farenheightToCelsius (int temp) {
	return (5./9.) * (temp - 32);
}

float inchesToCentimeters(float length) {
	return length * 2.54;
}

float factorCondition(float var) {
	return pow(M_E, pow(-((var - MIDTEMP) / 10), 2));
}

void initGlobalVariables() {
	NowNumDeer = 1;
	NowHeight = 1.;
	NowMonth = 0;
	NowYear = 2014;
	SimulationStep = 1;
}

void printSimulation() {
	printf("----------------------------------------------------------\n");
	printf("Simulation Step: %d\n", SimulationStep);
	printf("Year:            %d\n", NowYear);
	printf("Month:           %d\n\n", NowMonth);
	printf("Precipitation:   %f\n", NowPrecip);
	printf("Temperature:     %f\n", farenheightToCelsius(NowTemp));
	printf("Grain Height:    %f\n", inchesToCentimeters(NowHeight));
	printf("GrainDeer:       %d\n", NowNumDeer);
}

void incrementTime() {
	if(NowMonth == 11) {
		NowMonth = 0;
		NowYear++;
	} else {
		NowMonth++;
	}
}

void calcTempAndPrecip() {
	float ang = (30. * (float)NowMonth + 15.) * (M_PI / 180.);
	float temp = AVG_TEMP - AMP_TEMP * cos(ang);
	NowTemp = temp + Ranf(-RANDOM_TEMP, RANDOM_TEMP, &seed);
	float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
	NowPrecip = precip + Ranf(-RANDOM_PRECIP, RANDOM_PRECIP, &seed);
	if(NowPrecip < 0.)
		NowPrecip = 0.;
}

void Watcher() {

	while(NowYear != 2020) {
	//Compute
	#pragma omp critical (DoneComputing)

	//Assign
	#pragma omp critical (DoneAssigning)

	//Print
	printSimulation();
	incrementTime();
	calcTempAndPrecip();
	SimulationStep++;

	#pragma omp critical (DonePrinting)
	;
	}
}

void Grain() {

	while(NowYear != 2020) {
	//Compute
	float tempNowHeight = NowHeight;
	tempNowHeight += factorCondition(NowTemp) * factorCondition(NowPrecip) * GRAIN_GROWS_PER_MONTH;
	tempNowHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
	#pragma omp critical (DoneComputing)

	//Assign
	NowHeight = tempNowHeight;
	#pragma omp critical (DoneAssigning)
	;

	//Print
	#pragma omp critical (DonePrinting)
	;
	}
}

void GrainDeer() {

	while(NowYear != 2020) {
	//Compute
	int tempNowNumDeer;
	if(NowNumDeer > NowHeight && NowNumDeer > 0)
		tempNowNumDeer = NowNumDeer - 1;
	else if(NowNumDeer < NowHeight)
		tempNowNumDeer = NowNumDeer + 1;
	else
		tempNowNumDeer = NowNumDeer;
	#pragma omp critical (DoneComputing)

	//Assign
	NowNumDeer = tempNowNumDeer;
	#pragma omp critical (DoneAssigning)
	;

	//Print
	#pragma omp critical (DonePrinting)
	;
	}
}

int main(int argc, char *argv[]) {

	#ifndef _OPENMP
        fprintf(stderr, "OpenMP is not available\n");
        return 1;
    #endif

    initGlobalVariables();
	calcTempAndPrecip();

	omp_set_num_threads(3);

	double sumTime = 0.;
	double slowestTime = -1;
	for(int i = 0; i < NUMTESTS; i++) {
		double time0 = omp_get_wtime( );

		//while(NowYear != 2020) {
			#pragma omp parallel sections
			{
				#pragma omp section
			    {
			    	Watcher();
			    }
			    #pragma omp section
			    {
			    	Grain();
			    }
				#pragma omp section
			    {
			    	GrainDeer();
			    }
			    //Implied barrier
			}
			//SimulationStep++;
		//}
		double time1 = omp_get_wtime( );
		double totalTime = time1 - time0;
		sumTime += totalTime;
		if(totalTime < slowestTime || slowestTime == -1) {
			slowestTime = totalTime;
		}

		initGlobalVariables();
		calcTempAndPrecip();
	}

	double avgTime = sumTime / NUMTESTS;
	printf("Quickest time: %lf\n", slowestTime);
	printf(" Average time: %lf\n", avgTime);

	return 0;
}