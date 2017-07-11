#include "MyCuda.h"

#include "cudaD3D9.h"

int CreateD3D()
	{
	IDXGIAdapter Adapter;
	cuD3D9GetDevice(m_Device,&Adapter);
	return 0;
	}


