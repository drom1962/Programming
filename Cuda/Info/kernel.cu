#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>


int main()
{

cudaDeviceProp Prop;

cudaError_t e=cudaGetDeviceProperties (&Prop,0);



}
