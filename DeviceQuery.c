//Written by Akash Ramdas
//Decleration of header files requred for the code
#include <stdio.h>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#include <unistd.h>
#else
#include <CL/cl.h> // Contains the OpenCL header files
#endif
int main(int argc, char const *argv[])
{
	//Get the Number of OpenCl Platforms
	cl_uint numPlatforms;
	clGetPlatformIDs(0, NULL, &numPlatforms);
	//Find each platforms Platform ID
	cl_platform_id PlatformIDs[numPlatforms];
	clGetPlatformIDs(numPlatforms, PlatformIDs, NULL);
	//Variables to store Platform Information
	cl_char PlatformNames[numPlatforms][1000];
	cl_char PlatformProfile[numPlatforms][1000];
	cl_char PlatformVersion[numPlatforms][1000];
	printf("The platforms are:\n");
	//Loop to find information for each platform
	for (int i = 0; i < numPlatforms; ++i)
	{
		//Find Platform Profile
		clGetPlatformInfo(PlatformIDs[i], CL_PLATFORM_PROFILE, sizeof(PlatformProfile[i]), &PlatformProfile[i], NULL);
		//Find Platform's supported openCL version
		clGetPlatformInfo(PlatformIDs[i], CL_PLATFORM_VERSION, sizeof(PlatformVersion[i]), &PlatformVersion[i], NULL);
		//Find Platform's Name
		clGetPlatformInfo(PlatformIDs[i], CL_PLATFORM_NAME, sizeof(PlatformNames[i]), &PlatformNames[i], NULL);
		//Print the Platform Information
		printf(" %d. %s\n Version Supported: %s \n Profile: %s\n" , (i+1), PlatformNames[i],PlatformVersion[i],PlatformProfile[i]);
		//Find number of devices in each platform 	
		cl_uint numDevices[numPlatforms]
		clGetDeviceIDs(PlatformIDs[i], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices[i]);
		//Find Device IDs of the devices in each platform
		cl_device_id DeviceIDs[numDevices[i]];
		clGetDeviceIDs(PlatformIDs[i], CL_DEVICE_TYPE_ALL, numDevices[i], &DeviceIDs, NULL);
		//Variables to store device information
		cl_char DeviceNames[numDevices[i]][1000];
		for (int j = 0; j < numDevices[i]; ++j)
		{
			//Find device names
			clGetDeviceInfo(DeviceIDs[j], CL_DEVICE_NAME, sizeof(DeviceNames[j]), &DeviceNames[j], NULL);
			//Print device names 
			printf("\tThe devices included on this platform are:\n \t\t%d.%d %s\n" , (i+1), (j+1), DeviceNames[j]);
		}
	}	
	return 0;
}
//Code to compile : gcc -I /path-to-AMD/include -L/path-to-AMD/lib/x86 DeviceQuery.c -Wl,-rpath,/path-to-AMD/lib/x86 -lOpenCL -o DeviceQuery.o
