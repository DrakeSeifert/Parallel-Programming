// 1. Program header

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <omp.h>

//#include "CL/cl.h"
#include "cl.h"
//#include "CL/cl_platform.h"
#include "cl_platform.h"


#ifndef NMB
#define	NMB			64
#endif

//#define NUM_ELEMENTS		NMB*1024*1024 //1k to 8M 
int NUM_ELEMENTS; //1k to 8M

//#ifndef LOCAL_SIZE
//#define	LOCAL_SIZE		64 //1 to 1024
//#endif
int LOCAL_SIZE; //1 to 1024

//#define	NUM_WORK_GROUPS		NUM_ELEMENTS/LOCAL_SIZE
int NUM_WORK_GROUPS;

const char *CL_FILE_NAME = { "first3.cl" };
const float TOL = 0.0001f;

void Wait( cl_command_queue );
int LookAtTheBits( float );

//OpenCL error code
void PrintCLError( cl_int, char * = "", FILE * = stderr );

struct errorcode
{
	cl_int		statusCode;
	char *		meaning;
}
ErrorCodes[ ] =
{
	{ CL_SUCCESS,				""					},
	{ CL_DEVICE_NOT_FOUND,			"Device Not Found"			},
	{ CL_DEVICE_NOT_AVAILABLE,		"Device Not Available"			},
	{ CL_COMPILER_NOT_AVAILABLE,		"Compiler Not Available"		},
	{ CL_MEM_OBJECT_ALLOCATION_FAILURE,	"Memory Object Allocation Failure"	},
	{ CL_OUT_OF_RESOURCES,			"Out of resources"			},
	{ CL_OUT_OF_HOST_MEMORY,		"Out of Host Memory"			},
	{ CL_PROFILING_INFO_NOT_AVAILABLE,	"Profiling Information Not Available"	},
	{ CL_MEM_COPY_OVERLAP,			"Memory Copy Overlap"			},
	{ CL_IMAGE_FORMAT_MISMATCH,		"Image Format Mismatch"			},
	{ CL_IMAGE_FORMAT_NOT_SUPPORTED,	"Image Format Not Supported"		},
	{ CL_BUILD_PROGRAM_FAILURE,		"Build Program Failure"			},
	{ CL_MAP_FAILURE,			"Map Failure"				},
	{ CL_INVALID_VALUE,			"Invalid Value"				},
	{ CL_INVALID_DEVICE_TYPE,		"Invalid Device Type"			},
	{ CL_INVALID_PLATFORM,			"Invalid Platform"			},
	{ CL_INVALID_DEVICE,			"Invalid Device"			},
	{ CL_INVALID_CONTEXT,			"Invalid Context"			},
	{ CL_INVALID_QUEUE_PROPERTIES,		"Invalid Queue Properties"		},
	{ CL_INVALID_COMMAND_QUEUE,		"Invalid Command Queue"			},
	{ CL_INVALID_HOST_PTR,			"Invalid Host Pointer"			},
	{ CL_INVALID_MEM_OBJECT,		"Invalid Memory Object"			},
	{ CL_INVALID_IMAGE_FORMAT_DESCRIPTOR,	"Invalid Image Format Descriptor"	},
	{ CL_INVALID_IMAGE_SIZE,		"Invalid Image Size"			},
	{ CL_INVALID_SAMPLER,			"Invalid Sampler"			},
	{ CL_INVALID_BINARY,			"Invalid Binary"			},
	{ CL_INVALID_BUILD_OPTIONS,		"Invalid Build Options"			},
	{ CL_INVALID_PROGRAM,			"Invalid Program"			},
	{ CL_INVALID_PROGRAM_EXECUTABLE,	"Invalid Program Executable"		},
	{ CL_INVALID_KERNEL_NAME,		"Invalid Kernel Name"			},
	{ CL_INVALID_KERNEL_DEFINITION,		"Invalid Kernel Definition"		},
	{ CL_INVALID_KERNEL,			"Invalid Kernel"			},
	{ CL_INVALID_ARG_INDEX,			"Invalid Argument Index"		},
	{ CL_INVALID_ARG_VALUE,			"Invalid Argument Value"		},
	{ CL_INVALID_ARG_SIZE,			"Invalid Argument Size"			},
	{ CL_INVALID_KERNEL_ARGS,		"Invalid Kernel Arguments"		},
	{ CL_INVALID_WORK_DIMENSION,		"Invalid Work Dimension"		},
	{ CL_INVALID_WORK_GROUP_SIZE,		"Invalid Work Group Size"		},
	{ CL_INVALID_WORK_ITEM_SIZE,		"Invalid Work Item Size"		},
	{ CL_INVALID_GLOBAL_OFFSET,		"Invalid Global Offset"			},
	{ CL_INVALID_EVENT_WAIT_LIST,		"Invalid Event Wait List"		},
	{ CL_INVALID_EVENT,			"Invalid Event"				},
	{ CL_INVALID_OPERATION,			"Invalid Operation"			},
	{ CL_INVALID_GL_OBJECT,			"Invalid GL Object"			},
	{ CL_INVALID_BUFFER_SIZE,		"Invalid Buffer Size"			},
	{ CL_INVALID_MIP_LEVEL,			"Invalid MIP Level"			},
	{ CL_INVALID_GLOBAL_WORK_SIZE,		"Invalid Global Work Size"		},
};

void
PrintCLError( cl_int status, char * prefix, FILE *fp )
{
	if( status == CL_SUCCESS )
		return;
	const int numErrorCodes = sizeof( ErrorCodes ) / sizeof( struct errorcode );
	char * meaning = "";
	for( int i = 0; i < numErrorCodes; i++ )
	{
		if( status == ErrorCodes[i].statusCode )
		{
			meaning = ErrorCodes[i].meaning;
			break;
		}
	}

	fprintf( fp, "%s %s\n", prefix, meaning );
}

int
main( int argc, char *argv[ ] )
{
	// see if we can even open the opencl kernel program
	// (no point going on if we can't):

	FILE *fp;
#ifdef WIN32
	errno_t err = fopen_s( &fp, CL_FILE_NAME, "r" );
	if( err != 0 )
#else
	fp = fopen( CL_FILE_NAME, "r" );
	if( fp == NULL )
#endif
	{
		fprintf( stderr, "Cannot open OpenCL source file '%s'\n", CL_FILE_NAME );
		return 1;
	}

	//Check command line args
	if(argc != 3) {
		fprintf(stderr, "USAGE: ./a.out LOCAL_SIZE GLOBAL_SIZE\n");
		exit(2);
	}
	LOCAL_SIZE = atoi(argv[1]);
	NUM_ELEMENTS = atoi(argv[2]);
	NUM_WORK_GROUPS = NUM_ELEMENTS / LOCAL_SIZE;

	cl_int status;		// returned status from opencl calls
				// test against CL_SUCCESS

	// get the platform id:

	cl_platform_id platform;
	status = clGetPlatformIDs( 1, &platform, NULL );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clGetPlatformIDs failed (2)\n" );
	
	// get the device id:

	cl_device_id device;
	status = clGetDeviceIDs( platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clGetDeviceIDs failed (2)\n" );

	//cl_int maxWorkGroupSize = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE,
	
	// 2. allocate the host memory buffers:

	float *hA = new float[ NUM_ELEMENTS ];
	float *hB = new float[ NUM_ELEMENTS ];
	float *hC = new float[ NUM_ELEMENTS ];
	size_t abSize = NUM_ELEMENTS * sizeof(float);
	size_t cSize = NUM_WORK_GROUPS * sizeof(float);

	// fill the host memory buffers:

	for( int i = 0; i < NUM_ELEMENTS; i++ )
	{
		hA[i] = hB[i] = (float) sqrt(  (double)i  );
	}

	//size_t dataSize = NUM_ELEMENTS * sizeof(float);

	// 3. create an opencl context:

	cl_context context = clCreateContext( NULL, 1, &device, NULL, NULL, &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateContext failed\n" );

	// 4. create an opencl command queue:

	cl_command_queue cmdQueue = clCreateCommandQueue( context, device, 0, &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateCommandQueue failed\n" );

	// 5. allocate the device memory buffers:

	cl_mem dA = clCreateBuffer( context, CL_MEM_READ_ONLY,  abSize, NULL, &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateBuffer failed (1)\n" );

	cl_mem dB = clCreateBuffer( context, CL_MEM_READ_ONLY,  abSize, NULL, &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateBuffer failed (2)\n" );

	cl_mem dC = clCreateBuffer( context, CL_MEM_WRITE_ONLY, cSize, NULL, &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateBuffer failed (3)\n" );

	// 6. enqueue the 2 commands to write the data from the host buffers to the device buffers:

	status = clEnqueueWriteBuffer( cmdQueue, dA, CL_FALSE, 0, abSize, hA, 0, NULL, NULL );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clEnqueueWriteBuffer failed (1)\n" );

	status = clEnqueueWriteBuffer( cmdQueue, dB, CL_FALSE, 0, abSize, hB, 0, NULL, NULL );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clEnqueueWriteBuffer failed (2)\n" );

	Wait( cmdQueue );

	// 7. read the kernel code from a file:

	fseek( fp, 0, SEEK_END );
	size_t fileSize = ftell( fp );
	fseek( fp, 0, SEEK_SET );
	char *clProgramText = new char[ fileSize+1 ];		// leave room for '\0'
	size_t n = fread( clProgramText, 1, fileSize, fp );
	clProgramText[fileSize] = '\0';
	fclose( fp );
	if( n != fileSize )
		fprintf( stderr, "Expected to read %d bytes read from '%s' -- actually read %d.\n", fileSize, CL_FILE_NAME, n );

	// create the text for the kernel program:

	char *strings[1];
	strings[0] = clProgramText;
	cl_program program = clCreateProgramWithSource( context, 1, (const char **)strings, NULL, &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateProgramWithSource failed\n" );
	delete [ ] clProgramText;

	// 8. compile and link the kernel code:

	char *options = { "" };
	status = clBuildProgram( program, 1, &device, options, NULL, NULL );
	if( status != CL_SUCCESS )
	{
		size_t size;
		clGetProgramBuildInfo( program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &size );
		cl_char *log = new cl_char[ size ];
		clGetProgramBuildInfo( program, device, CL_PROGRAM_BUILD_LOG, size, log, NULL );
		fprintf( stderr, "clBuildProgram failed:\n%s\n", log );
		delete [ ] log;
	}

	// 9. create the kernel object:

	cl_kernel kernel = clCreateKernel( program, "ArrayMultReduce", &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateKernel failed\n" );

	// 10. setup the arguments to the kernel object:

	status = clSetKernelArg( kernel, 0, sizeof(cl_mem), &dA );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clSetKernelArg failed (1)\n" );

	status = clSetKernelArg( kernel, 1, sizeof(cl_mem), &dB );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clSetKernelArg failed (2)\n" );

	status = clSetKernelArg( kernel, 2, sizeof(float), NULL );

	status = clSetKernelArg( kernel, 3, sizeof(cl_mem), &dC );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clSetKernelArg failed (3)\n" );


	// 11. enqueue the kernel object for execution:

	size_t globalWorkSize[3] = { NUM_ELEMENTS, 1, 1 };
	size_t localWorkSize[3]  = { LOCAL_SIZE,   1, 1 };

	Wait( cmdQueue );
	double time0 = omp_get_wtime( );

	time0 = omp_get_wtime( );

	status = clEnqueueNDRangeKernel( cmdQueue, kernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL );
	if( status != CL_SUCCESS ) {
		fprintf( stderr, "clEnqueueNDRangeKernel failed: %d\n", status );
		PrintCLError(status, "LOG:", stderr);
	}

	Wait( cmdQueue );
	double time1 = omp_get_wtime( );

	// 12. read the results buffer back from the device to the host:

	status = clEnqueueReadBuffer( cmdQueue, dC, CL_TRUE, 0, cSize, hC, 0, NULL, NULL );
	if( status != CL_SUCCESS )
			fprintf( stderr, "clEnqueueReadBuffer failed\n" );

	float sum = 0.;
	for(int i = 0; i < NUM_WORK_GROUPS; i++)
	{
		sum += hC[i];
	}

	// did it work?

	for( int i = 0; i < NUM_ELEMENTS; i++ )
	{
		float expected = hA[i] * hB[i];
		if( fabs( hC[i] - expected ) > TOL )
		{
			//fprintf( stderr, "%4d: %13.6f * %13.6f wrongly produced %13.6f instead of %13.6f (%13.8f)\n",
				//i, hA[i], hB[i], hC[i], expected, fabs(hC[i]-expected) );
			//fprintf( stderr, "%4d:    0x%08x *    0x%08x wrongly produced    0x%08x instead of    0x%08x\n",
				//i, LookAtTheBits(hA[i]), LookAtTheBits(hB[i]), LookAtTheBits(hC[i]), LookAtTheBits(expected) );
		}
	}

	//printf("%8d\t%4d\t%10d\t%10.3lf GigaMultsPerSecond\n",
	//	NMB, LOCAL_SIZE, NUM_WORK_GROUPS, (double)NUM_ELEMENTS/(time1-time0)/1000000000. );
	printf("\n------------------------------------------------------------------------------\n");
	//printf("NMB:     %8d\n", NMB);
	printf("LOCAL_SIZE:  %4d\n", LOCAL_SIZE);
	printf("GLOBAL_SIZE:   %4d\n", NUM_ELEMENTS);
	printf("NUM_WORK_GROUPS:%10d\n", NUM_WORK_GROUPS);
	printf("Time:      %10.3lf GigaMults/Sec\n", (double)NUM_ELEMENTS/(time1-time0)/1000000000.);
	printf("------------------------------------------------------------------------------\n");

#ifdef WIN32
	Sleep( 2000 );
#endif


	// 13. clean everything up:

	clReleaseKernel(        kernel   );
	clReleaseProgram(       program  );
	clReleaseCommandQueue(  cmdQueue );
	clReleaseMemObject(     dA  );
	clReleaseMemObject(     dB  );
	clReleaseMemObject(     dC  );

	delete [ ] hA;
	delete [ ] hB;
	delete [ ] hC;

	return 0;
}


int
LookAtTheBits( float fp )
{
	int *ip = (int *)&fp;
	return *ip;
}


// wait until all queued tasks have taken place:

void
Wait( cl_command_queue queue )
{
      cl_event wait;
      cl_int      status;

      status = clEnqueueMarker( queue, &wait );
      if( status != CL_SUCCESS )
	      fprintf( stderr, "Wait: clEnqueueMarker failed\n" );

      status = clWaitForEvents( 1, &wait );
      if( status != CL_SUCCESS )
	      fprintf( stderr, "Wait: clWaitForEvents failed\n" );
}
