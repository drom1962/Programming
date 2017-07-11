#include <cuda.h>
#include <cuda_runtime_api.h>

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "nvcuvid.h"


#include <windows.h>

#include "MyCuda.h"

#include <stdio.h>


#include "..\..\container\ovi\Ovi_container.h"

typedef CUresult  (CALLBACK*  cu_Init)(unsigned int Flags);
typedef CUresult  (CALLBACK*  cu_CtxCreate)(CUcontext *pctx, unsigned int flags, CUdevice dev);


int main()
{
	int ret,ret1;

	// Откроем контейнер
    OVI2    *F1=new OVI2(1);
    FileInfo FI;
	
	ret=F1->Open(L"C:\\Video\\xx.ovi",&FI);
	if(ret!=0)
		{
		printf("File not opened\n");	
		return 0;
		}

	CUresult result;


	
	// Загрузим библиотеку
	HINSTANCE		 dll=LoadLibrary("nvcuda.dll");
	if(dll==nullptr) return 0;

	// Возьмем функцию cuInit и выполним ее
	CUresult resInit;
	cu_Init my_cuInit;
	my_cuInit=(cu_Init)GetProcAddress(dll,"cuInit");
	resInit=my_cuInit(0);
	if(resInit!=CUDA_SUCCESS)  return 0;
	printf("CUDA Init success\n");
	
	cu_CtxCreate	my_cuCtxCreate;
	my_cuCtxCreate=(cu_CtxCreate)GetProcAddress(dll,"cuCtxCreate");

	CUcontext   	ctx;
	CUdevice  		dev=0;
	resInit=my_cuCtxCreate(&ctx,CU_CTX_SCHED_AUTO,dev);
	if(resInit!=CUDA_SUCCESS)  return 0;
	printf("CUDA Create context success\n");
	


	//// cuInit
	//result=cuInit(0);
	//if(result!=CUDA_SUCCESS)  return 0;
	//printf("CUDA Init success\n");
	//
	//// Создадим контекс
	//CUcontext   	ctx;
	//CUdevice  		dev=0;
	//result=cuCtxCreate(&ctx,CU_CTX_SCHED_AUTO,dev);
	//if(result!=CUDA_SUCCESS)  return 0;
	//printf("CUDA Create context success\n");
	
	MyCuda *m1=new MyCuda();
    
    // Распарсим I кадр
    CUvideoparser           obj;
    CUVIDPARSERPARAMS       Parsparam;
    CUVIDSOURCEDATAPACKET   Sdp;

    VideoFrameInfo VFI;

    memset(&Parsparam,0,sizeof(CUVIDPARSERPARAMS));
    memset(&Sdp,0,sizeof(CUVIDSOURCEDATAPACKET));

    //   Заполним CUVIDPARSERPARAMS
    //
    switch(FI.VideoCodec)
		{
		case H264:
				Parsparam.CodecType= cudaVideoCodec_H264;
				break;

		case MJPEG:
				Parsparam.CodecType= cudaVideoCodec_JPEG;
				break;

		default:
				return false;
		}

	// Создадим декодер для данного файла
	cuDecoderInfo DI;

    DI.Codec		=Parsparam.CodecType;
    DI.Width		=FI.Width;
    DI.Height		=FI.Height;

    DI.NewWidth		=FI.Width;
    DI.NewHeight	=FI.Height;
	DI.Flag			=VP;
	
	ret=m1->CreateDecoder(&DI);
	if(ret) 
		{
		return 0;
		}

    Parsparam.ulMaxNumDecodeSurfaces=1;
    Parsparam.ulMaxDisplayDelay=1;

	Parsparam.pfnSequenceCallback	=(PFNVIDSEQUENCECALLBACK)SequenceCallback;
	Parsparam.pfnDecodePicture		=(PFNVIDDECODECALLBACK)DecodePicture;
	Parsparam.pfnDisplayPicture		=(PFNVIDDISPLAYCALLBACK)DisplayPicture;

    result=cuvidCreateVideoParser(&obj,&Parsparam);

    F1->ReadVideoFrame(0,nullptr,0,&VFI);
    Sdp.flags           =CUVID_PKT_ENDOFSTREAM;
    Sdp.payload         =VFI.Data;
    Sdp.payload_size    =VFI.SizeFrame;

    result=cuvidParseVideoData(obj,&Sdp);
    
    result=cuvidDestroyVideoParser(&obj);
	
    
    void *Mdev1;
    ret=m1->AllocateMemory(&Mdev1,F1->GetMaxVideoFrame());
    
	cuVideoFrameInfo cuVFI;

    for(int i=0;i<FI.CountVideoFrame;i++)
        {
        ret=F1->ReadVideoFrame(i,nullptr,0,&VFI);

		cuVFI.Size=VFI.SizeFrame;
		cuVFI.Frame=VFI.Data;

		m1->DecoderFrame(DI.Decoder,cuVFI);


        ret1=m1->CopyMemoryHostToDevice(VFI.Data,Mdev1,VFI.SizeFrame);

		printf("%5d - %d  %d \n",i,ret,ret1);

        }

	getchar();

	return 0;
}