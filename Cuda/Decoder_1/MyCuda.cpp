#include <windows.h>

#include "MyCuda.h"

//
//	*
//
MyCuda::MyCuda()
	{
	if(cudaSetDevice(0)== cudaSuccess) 		m_Device= true;
	else
		{
		//CUresult ret=cuInit(0);
		m_Device= false;
		}
	m_Decoder=0;

	m_Parser=nullptr;

	m_DecodFrameSize=0;
	m_BuffDecodFrame=nullptr;
	}


MyCuda::MyCuda(int Device)
	{
	if(cudaSetDevice(Device)== cudaSuccess) m_Device= true;
	else
		m_Device= false;
	}

MyCuda::~MyCuda()
	{
    if(m_Decoder!=0)    MyCuda::DestroyDecoder(m_Decoder);

    if(m_Parser!=0)     MyCuda::DestroyParser(m_Parser);

    if(m_BuffDecodFrame!=nullptr) free(m_BuffDecodFrame);
    }

//
//	Версия
//
int MyCuda::GetVersion()
	{
	int Ver=0;

	if(!m_Device)  return 0;

	//if(cuDriverGetVersion(&Ver)!= cudaSuccess) return 0;

	return Ver;
	}

//
//	Возможности
//
unsigned MyCuda::GetOpportunities()
	{
	return 	DecodResize	&& PostResize && CopyDevToHost && Surface;
	}


//
// Создадим парсер
//
int MyCuda::CreateParser(unsigned char VideoCodec)
	{
	CUVIDPARSERPARAMS       Parsparam;

	if(m_Parser!=nullptr) return 1;

	memset(&Parsparam,0,sizeof(CUVIDPARSERPARAMS));

	switch(VideoCodec)
		{
		case H264:
				Parsparam.CodecType= cudaVideoCodec_H264;
				break;

		case MJPEG:
				Parsparam.CodecType= cudaVideoCodec_JPEG;
				break;

		default:
				return 2;
		}
	
	Parsparam.ulMaxNumDecodeSurfaces=1;
    Parsparam.ulMaxDisplayDelay=0;

	Parsparam.pfnSequenceCallback	=(PFNVIDSEQUENCECALLBACK)SequenceCallback;
	Parsparam.pfnDecodePicture		=(PFNVIDDECODECALLBACK)DecodePicture;
	Parsparam.pfnDisplayPicture		=(PFNVIDDISPLAYCALLBACK)DisplayPicture;
	Parsparam.pUserData=&m_CallBacks;
	Parsparam.ulErrorThreshold=100;
	
	if(cuvidCreateVideoParser(&m_Parser,&Parsparam)!=CUDA_SUCCESS) 	return 3;

	return 0;
	}


//
//	Удалим парсер
//
int MyCuda::DestroyParser(CUvideoparser Parser)
    {
	if(Parser==nullptr) Parser=m_Parser;

    if(cuvidDestroyVideoParser!=0) 
        {
        if(cuvidDestroyVideoParser(m_Parser)!=CUDA_SUCCESS) return 1;
        }

    return 0;
    }

//
//	Отпарсим кадр
//
int MyCuda::ParserFrame(Frames Fr)
	{
	CUVIDSOURCEDATAPACKET Sdp;

	if(m_Parser==nullptr)	return 1;

    Sdp.flags           =CUVID_PKT_ENDOFSTREAM;
	Sdp.payload         =Fr.Data;
	Sdp.payload_size    =Fr.Size;

	m_CallBacks.Step=0;

	if(cuvidParseVideoData(m_Parser,&Sdp)!=CUDA_SUCCESS) 		return 2;

	// Должно сработать три callback
	if(m_CallBacks.Step!=7)										return 3;

	// Проверим смену разрешения
	//if( m_DI.Width  != m_CallBacks.VIDEO_FORMAT.coded_width &&
	//	m_DI.Height != m_CallBacks.VIDEO_FORMAT.coded_height)	return 4;
	
	return 0;
	}


//
//	Создание декодера
//
int MyCuda::CreateDecoder(cuDecoderInfo *DI)
	{
	if(!m_Device)			return 1;

	if(m_Parser==nullptr)	return 2;
	
	if(m_Decoder!=nullptr)	return 3;

   	CUVIDDECODECREATEINFO	DCI;
	memset(&DCI,0,sizeof(DCI));

	// Input format
	//-------------------------------------------------------
	DCI.ulWidth				=DI->Width;
	DCI.ulHeight			=DI->Height;
	DCI.CodecType			=cudaVideoCodec_H264;

   	DCI.ulNumDecodeSurfaces=DI->WorkPict;

	while (DCI.ulNumDecodeSurfaces * DCI.ulWidth * DCI.ulHeight > 16*1024*1024)
		{
        DCI.ulNumDecodeSurfaces--;
		}

	DCI.ChromaFormat= cudaVideoChromaFormat_420;  // (only 4:2:0 is currently supported)

    switch(DI->Flag)
        {
        case VP:
                DCI.ulCreationFlags=cudaVideoCreate_PreferCUVID;
                break;
        case CUDA:
                DCI.ulCreationFlags=cudaVideoCreate_PreferCUDA;
                break;
        case DXVA:
                DCI.ulCreationFlags=cudaVideoCreate_PreferDXVA;
                break;

        default:
                DCI.ulCreationFlags=cudaVideoCreate_Default;
        }

    //DCI.display_area.left   =DI->VIDEO_FORMAT.display_area.left;
    //DI.display_area.bottom =DI->VIDEO_FORMAT.display_area.bottom;

    //DCI.display_area.top    =DI->VIDEO_FORMAT.display_area.top;
    //DCI.display_area.right  =DI->VIDEO_FORMAT.display_area.right;
	
    // Output format
	//-------------------------------------------------------
	DCI.OutputFormat=cudaVideoSurfaceFormat_NV12;   // NV12 (currently the only supported output format)
	DCI.DeinterlaceMode=cudaVideoDeinterlaceMode_Adaptive;

	DCI.ulTargetWidth		=DI->NewWidth;
	DCI.ulTargetHeight		=DI->NewHeight;
	DCI.ulNumOutputSurfaces	=DI->OutPict;
	DCI.vidLock				=nullptr;

    //DCI.target_rect.left	=0;
    //DCI.target_rect.top		=0;
    //DCI.target_rect.right	=DCI.ulTargetWidth;
    //DCI.target_rect.bottom	=DCI.ulTargetHeight;

    CUresult ret=cuvidCreateDecoder(&m_Decoder,&DCI);
	if(ret==CUDA_ERROR_INVALID_DEVICE)
		{
		m_LastError=cudaGetLastError();
		return 4;
		}

	memcpy(&m_DI,DI,sizeof(cuDecoderInfo));

	return 0;
	}

//
//	Удалим декодер
//
int MyCuda::DestroyDecoder(CUvideodecoder Decoder)
	{
	if(m_Decoder==nullptr)								return 1;

	if(cuvidDestroyDecoder(m_Decoder)!=CUDA_SUCCESS)	return 2;
	
	// очистим
	m_Decoder=nullptr;

	return 0;
	}

 int MyCuda::PostProcessFrame()
	{
	return 0;
	}

 
//
//	Переносим декодированный кадр из Device в HOST
//
int MyCuda::GetD3D9Surface(int PictIdx,void *Surface)
	{
	
	CUresult ret=cuvidGetVideoFrameSurface(m_Decoder,PictIdx,&Surface);
	if(ret!=cudaSuccess) return 1;

	return 0;
	}
//
//	Переносим декодированный кадр из Device в HOST
//
int MyCuda::GetDecodeFrame(int PictIdx,unsigned char *Buff,int Size,int Flag)
	{
	unsigned int		DevPtr,
                        Pitch;
	CUVIDPROCPARAMS		VPP;
	unsigned char		*Map;
	unsigned int		DecodFrameSize;

	if(m_Decoder==nullptr)	return 1;
			
	VPP.progressive_frame	=m_CallBacks.PARSER_DISP_INFO.progressive_frame;
	VPP.top_field_first		=m_CallBacks.PARSER_DISP_INFO.top_field_first;

	if(cuvidMapVideoFrame(m_Decoder,PictIdx,&DevPtr,&Pitch,&VPP)!=cudaSuccess) return 2;

	if(Flag==Color)	            DecodFrameSize= (m_DI.NewHeight+m_DI.NewHeight/2)*Pitch;	// Color
	if(Flag==BlackWhite)		DecodFrameSize= m_DI.NewHeight*Pitch;					// Black

	
	if(Buff==nullptr)
		{  // Используем локальный буер
		Map=nullptr;
		if(m_BuffDecodFrame!=nullptr)
			{
			// Уже выделен
			if(m_DecodFrameSize < DecodFrameSize) 	free(m_BuffDecodFrame);
			else									Map=m_BuffDecodFrame;
			}
		if(Map==nullptr)
			{
			m_BuffDecodFrame	=(unsigned char*)malloc(DecodFrameSize);
			m_DecodFrameSize	=DecodFrameSize;
			Map					=m_BuffDecodFrame;
			}
		}
	else
		{
		// Запишем в буфер пользователя
		if(Size<DecodFrameSize)  return (-1)*DecodFrameSize;
		Map=Buff;
		}

	if(cuMemcpyDtoH(Map, DevPtr, m_DecodFrameSize)!=cudaSuccess) return 3;

	if(cuvidUnmapVideoFrame(m_Decoder, DevPtr)!=cudaSuccess)	 return 4;

	m_DI.Pitch=Pitch;  // Сохраним Pitch

	return 0;
	}

//
//	*
//
unsigned char * MyCuda::GetBuff()
	{
	if(m_Decoder==nullptr)	return nullptr;

	return m_BuffDecodFrame;
	}

//
//	Декодируем фрейм
//
int MyCuda::DecoderFrame(int PictIdx)
	{
	if(m_Decoder==nullptr) return 1;

	m_CallBacks.PIC_PARAMS.CurrPicIdx=PictIdx;

	if(cuvidDecodePicture(m_Decoder,&m_CallBacks.PIC_PARAMS)!=CUDA_SUCCESS)  return 2;

	return 0;
	}


//
//	Выделим память на GPU
//
int MyCuda::AllocateMemory(void **Mem,int Size)
	{
	if(!m_Device)  return 1;

	if(cudaMalloc(Mem, Size)!= cudaSuccess) return 2;
		
	return 0;
	}


//
//	Удалим память с GPU
//
int MyCuda::FreeMemory(void *Mem)
	{
	if(!m_Device)  return 1;
    
	return 0;
	}


//
//	*
//
int MyCuda::CopyMemoryHostToDevice(const void *MemHost,void *MemDevice,int Size)
	{
    if(!m_Device)  return 1;

	if(cudaMemcpy(MemDevice,MemHost, Size, cudaMemcpyHostToDevice)!= cudaSuccess) 
		{
		m_LastError=cudaGetLastError();
		return 2;
		}

	return 0;
	}

//
//	*
//
int MyCuda::CopyMemoryDeviceToHost(const void *MemDevice,void *MemHost,int Size)
	{
	if(cudaMemcpy(MemHost,MemDevice,Size, cudaMemcpyDeviceToHost)!= cudaSuccess) 
		{
		m_LastError=cudaGetLastError();
		return 1;
		}

	return 0;
	}


// Called before decoding frames and/or whenever there is a format change
//
int CUDAAPI MyCuda::SequenceCallback(void *UserData, CUVIDEOFORMAT *VIDEOFORMAT)
    {
    memcpy(&((CallBacks *)UserData)->VIDEO_FORMAT,VIDEOFORMAT,sizeof(CUVIDEOFORMAT));

	((CallBacks *)UserData)->Step=1;

    return 1;
    }


// Called when a picture is ready to be decoded (decode order)
//
int CUDAAPI MyCuda::DecodePicture(void *UserData, CUVIDPICPARAMS *PICPARAMS)
    {
	memcpy(&((CallBacks *)UserData)->PIC_PARAMS,PICPARAMS,sizeof(CUVIDPICPARAMS));

	((CallBacks *)UserData)->Step+=2;

    return 1;
    }


// Called whenever a picture is ready to be displayed (display order)
//
int CUDAAPI MyCuda::DisplayPicture(void *UserData, CUVIDPARSERDISPINFO *PARSERDISPINFO)
    {
	memcpy(&((CallBacks *)UserData)->PARSER_DISP_INFO,PARSERDISPINFO,sizeof(CUVIDPARSERDISPINFO));

	((CallBacks *)UserData)->Step+=4;

	return 1;
    }