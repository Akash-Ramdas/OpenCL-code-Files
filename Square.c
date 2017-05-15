#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#include <unistd.h>
#else
#include <CL/cl.h>
#endif

#include "err_code.h"
#include "wtime.h"
#include "device_info.h"
//#include "device_picker.h"
//pick up device type from compiler command line or from
//the default type
#ifndef DEVICE
#define DEVICE CL_DEVICE_TYPE_DEFAULT
#endif


//extern double wtime();       // returns time since some fixed past point (wtime.c)
//extern int output_device_info(cl_device_id );


//------------------------------------------------------------------------------

#define TOL    (0.001)   // tolerance used in floating point comparisons
#define LENGTH (1024)    // length of vectors a, b, and c

//------------------------------------------------------------------------------
//
// kernel:  square
//
// Purpose: Compute the elementwise sum output = input * input
//
// input: input float vector of length count
//
// output: output float vector of length count holding the sum input * input
//

const char *KernelSource = "\n" \
"__kernel void square(                                                  \n" \
"   __global float* input,                                              \n" \
"   __global float* output,                                             \n" \
"   const unsigned int count)                                           \n" \
"{                                                                      \n" \
"   int i = get_global_id(0);                                           \n" \
"   if(i < count)                                                       \n" \
"       output[i] = input[i]*input[i];                                  \n" \
"}                                                                      \n" \
"\n";

//------------------------------------------------------------------------------


int main(int argc, char** argv)
{
    int          err;               // error code returned from OpenCL calls

    float*       h_input= (float*) calloc(LENGTH, sizeof(float));       // Input vector
    float*       h_output = (float*) calloc(LENGTH, sizeof(float));     // Output vector = Input^2 returned from the compute device

    unsigned int correct;           // number of correct results

    size_t global;                  // global domain size

    cl_device_id     device_id;     // compute device id
    cl_context       context;       // compute context
    cl_command_queue commands;      // compute command queue
    cl_program       program;       // compute program
    cl_kernel        ko_square;     // compute kernel

    cl_mem d_input;                 // device memory used for the input  a vector
    cl_mem d_output;                // device memory used for the output c vector

    // Fill vectors a and b with random float values
    int i = 0;
    int count = LENGTH;
    for(i = 0; i < count; i++){
        h_input[i] = rand() / (float)RAND_MAX;
    }

    // Set up platform and GPU device

    cl_uint numPlatforms;

    // Find number of platforms
    err = clGetPlatformIDs(0, NULL, &numPlatforms);
    checkError(err, "Finding platforms");
    if (numPlatforms == 0)
    {
        printf("Found 0 platforms!\n");
        return EXIT_FAILURE;
    }

    // Get all platforms
    cl_platform_id Platform[numPlatforms];
    err = clGetPlatformIDs(numPlatforms, Platform, NULL);
    checkError(err, "Getting platforms");

    // Secure a GPU
    for (i = 0; i < numPlatforms; i++)
    {
        err = clGetDeviceIDs(Platform[i], DEVICE, 1, &device_id, NULL);
        if (err == CL_SUCCESS)
        {
            break;
        }
    }

    if (device_id == NULL)
        checkError(err, "Finding a device");

    err = output_device_info(device_id);
    checkError(err, "Printing device output");

    // Create a compute context
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    checkError(err, "Creating context");

    // Create a command queue
    commands = clCreateCommandQueue(context, device_id, 0, &err);
    checkError(err, "Creating command queue");

    // Create the compute program from the source buffer
    program = clCreateProgramWithSource(context, 1, (const char **) & KernelSource, NULL, &err);
    checkError(err, "Creating program");

    // Build the program
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        size_t len;
        char buffer[2048];

        printf("Error: Failed to build program executable!\n%s\n", err_code(err));
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);
        return EXIT_FAILURE;
    }

    // Create the compute kernel from the program
    ko_square = clCreateKernel(program, "square", &err);
    checkError(err, "Creating kernel");

    // Create the input (a, b) and output (c) arrays in device memory
    d_input  = clCreateBuffer(context,  CL_MEM_READ_ONLY,  sizeof(float) * count, NULL, &err);
    checkError(err, "Creating buffer d_input");

    d_output  = clCreateBuffer(context,  CL_MEM_WRITE_ONLY, sizeof(float) * count, NULL, &err);
    checkError(err, "Creating buffer d_output");

    // Write a and b vectors into compute device memory
    err = clEnqueueWriteBuffer(commands, d_input, CL_TRUE, 0, sizeof(float) * count, h_input, 0, NULL, NULL);
    checkError(err, "Copying h_input to device at d_input");

    // Set the arguments to our compute kernel
    err  = clSetKernelArg(ko_square, 0, sizeof(cl_mem), &d_input);
    err |= clSetKernelArg(ko_square, 1, sizeof(cl_mem), &d_output);
    err |= clSetKernelArg(ko_square, 2, sizeof(unsigned int), &count);
    checkError(err, "Setting kernel arguments");

    double rtime = wtime();

    // Execute the kernel over the entire range of our 1d input data set
    // letting the OpenCL runtime choose the work-group size
    global = count;
    err = clEnqueueNDRangeKernel(commands, ko_square, 1, NULL, &global, NULL, 0, NULL, NULL);
    checkError(err, "Enqueueing kernel");

    // Wait for the commands to complete before stopping the timer
    err = clFinish(commands);
    checkError(err, "Waiting for kernel to finish");

    rtime = wtime() - rtime;
    printf("\nThe kernel ran in %lf seconds\n",rtime);

    // Read back the results from the compute device
    err = clEnqueueReadBuffer( commands, d_output, CL_TRUE, 0, sizeof(float) * count, h_output, 0, NULL, NULL );  
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to read output array!\n%s\n", err_code(err));
        exit(1);
    }

    // Test the results
    correct = 0;
    float tmp;

    for(i = 0; i < count; i++)
    {
        tmp = h_input[i]*h_input[i];     // assign element i of a*a to tmp
        tmp -= h_output[i];              // compute deviation of expected and output result
        if(tmp*tmp < TOL*TOL)            // correct if square deviation is less than tolerance squared
            correct++;
        else {
            printf(" tmp %f h_input %f h_output %f \n",tmp, h_input[i], h_output[i]);
        }
    }

    // summarise results
    printf("B = A*A:  %d out of %d results were correct.\n", correct, count);

    // cleanup then shutdown
    clReleaseMemObject(d_input);
    clReleaseMemObject(d_output);
    clReleaseProgram(program);
    clReleaseKernel(ko_square);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);

    free(h_input);
    free(h_output);

    return 0;
}
//Code to Compile: gcc -I /path-to-AMD/include -L/path-to-AMD/lib/x86 Square.c -Wl,-rpath,/path-to-AMD/lib/x86 -lOpenCL -o Square.o