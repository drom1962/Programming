#include "..\decoder.h"

Decoder::~Decoder() {};

//
//
//
Nvidia::Nvidia()
	{
	if (cudaSetDevice(0) == cudaSuccess) 		DecoderFeature = DecodResize || CopyDevToHost || Surface;
	else
		{
		//CUresult ret=cuInit(0);
		DecoderFeature = 0;
		}

	SizeBuff = 0;
	LocalBuff = nullptr;
	}

Nvidia::~Nvidia()
	{
	}


int Nvidia::Feature()
	{
	return DecoderFeature;
	}

//
//    Создание декодера
//
int	Nvidia::Init(int Dev, DecoderInfo *DI)
	{

	CUVIDPARSERPARAMS       Parsparam;

	if (m_Parser != nullptr) return 1;

	// cuInit
	CUresult result = cuInit(0);
	if (result != CUDA_SUCCESS)  return -1;

	// Создадим контекс
	result = cuCtxCreate(&m_ctx, CU_CTX_SCHED_AUTO, (CUdevice)Dev);
	if (result != CUDA_SUCCESS)  return -2;

	memset(&Parsparam, 0, sizeof(CUVIDPARSERPARAMS));

	switch (DI->Codec)
		{
		case H264:
			Parsparam.CodecType = cudaVideoCodec_H264;
			break;

		case MJPEG:
			Parsparam.CodecType = cudaVideoCodec_JPEG;
			break;

		default:
			return 2;
		}

	Parsparam.ulMaxNumDecodeSurfaces	= 1;
	Parsparam.ulMaxDisplayDelay			= 0;

	Parsparam.pfnSequenceCallback		= (PFNVIDSEQUENCECALLBACK)SequenceCallback;
	Parsparam.pfnDecodePicture			= (PFNVIDDECODECALLBACK)DecodePicture;
	Parsparam.pfnDisplayPicture			= (PFNVIDDISPLAYCALLBACK)DisplayPicture;
	Parsparam.pUserData					= &m_CallBacks;
	Parsparam.ulErrorThreshold			= 100;

	if (cuvidCreateVideoParser(&m_Parser, &Parsparam) != CUDA_SUCCESS) 	return 3;

	CUVIDDECODECREATEINFO	DCI;
	memset(&DCI, 0, sizeof(DCI));

	// Input format
	//-------------------------------------------------------
	DCI.ulWidth		= DI->Width;
	DCI.ulHeight	= DI->Height;
	DCI.CodecType	= cudaVideoCodec_H264;

	DCI.ulNumDecodeSurfaces = DI->WorkPict;

	while (DCI.ulNumDecodeSurfaces * DCI.ulWidth * DCI.ulHeight > 16 * 1024 * 1024)
		{
		DCI.ulNumDecodeSurfaces--;
		}

	DCI.ChromaFormat = cudaVideoChromaFormat_420;  // (only 4:2:0 is currently supported)

	switch (DI->Flag)
		{
		case VP:
			DCI.ulCreationFlags = cudaVideoCreate_PreferCUVID;
			break;
		case CUDA:
			DCI.ulCreationFlags = cudaVideoCreate_PreferCUDA;
			break;
		case DXVA:
			DCI.ulCreationFlags = cudaVideoCreate_PreferDXVA;
			break;

		default:
			DCI.ulCreationFlags = cudaVideoCreate_Default;
		}

	//DCI.display_area.left   =DI->VIDEO_FORMAT.display_area.left;
	//DI.display_area.bottom =DI->VIDEO_FORMAT.display_area.bottom;

	//DCI.display_area.top    =DI->VIDEO_FORMAT.display_area.top;
	//DCI.display_area.right  =DI->VIDEO_FORMAT.display_area.right;

	// Output format
	//-------------------------------------------------------
	DCI.OutputFormat		= cudaVideoSurfaceFormat_NV12;   // NV12 (currently the only supported output format)
	DCI.DeinterlaceMode		= cudaVideoDeinterlaceMode_Adaptive;

	DCI.ulTargetWidth		= DI->NewWidth;
	DCI.ulTargetHeight		= DI->NewHeight;
	DCI.ulNumOutputSurfaces = DI->OutPict;
	DCI.vidLock				= nullptr;

	//DCI.target_rect.left	=0;
	//DCI.target_rect.top		=0;
	//DCI.target_rect.right	=DCI.ulTargetWidth;
	//DCI.target_rect.bottom	=DCI.ulTargetHeight;

	CUresult ret = cuvidCreateDecoder(&m_Decoder, &DCI);
	if (ret == CUDA_ERROR_INVALID_DEVICE)
		{
		LastError = cudaGetLastError();
		return 4;
		}

	memcpy(&DI, DI, sizeof(DecoderInfo));

	// Заполним битмап хедер

	return S_OK;
	}


//
//		Декодирование кадра
//
int	Nvidia::Decode(Frame *Frame, FrameInfo *FI)
	{
	if (m_Decoder == nullptr) return 1;

	int PictIdx = 0;

	m_CallBacks.PIC_PARAMS.CurrPicIdx = PictIdx;

	if (Frame->Type == 1)
		{
		if (ParserFrame(Frame)) return 1;
		}

	if (cuvidDecodePicture(m_Decoder, &m_CallBacks.PIC_PARAMS) != CUDA_SUCCESS)  return 2;








	return S_OK;
	}


//
//		Удалим декодер
//
int	Nvidia::Destroy()
	{
	if (m_Decoder == nullptr)							return 1;

	if (cuvidDestroyDecoder(m_Decoder) != CUDA_SUCCESS)	return 2;

	// очистим
	m_Decoder = nullptr;

	return S_OK;
	}



//
//	Отпарсим кадр
//
int Nvidia::ParserFrame(Frame *Fr)
{
	CUVIDSOURCEDATAPACKET Sdp;

	if (m_Parser == nullptr)	return 1;

	Sdp.flags			= CUVID_PKT_ENDOFSTREAM;
	Sdp.payload			= Fr->Data;
	Sdp.payload_size	= Fr->Size;

	m_CallBacks.Step = 0;

	if (cuvidParseVideoData(m_Parser, &Sdp) != CUDA_SUCCESS) 		return 2;

	// Должно сработать три callback
	if (m_CallBacks.Step != 7)										return 3;

	// Проверим смену разрешения
	//if( m_DI.Width  != m_CallBacks.VIDEO_FORMAT.coded_width &&
	//	m_DI.Height != m_CallBacks.VIDEO_FORMAT.coded_height)	return 4;

	return 0;
}



// Called before decoding frames and/or whenever there is a format change
//
int CUDAAPI Nvidia::SequenceCallback(void *UserData, CUVIDEOFORMAT *VIDEOFORMAT)
	{
	memcpy(&((CallBacks *)UserData)->VIDEO_FORMAT, VIDEOFORMAT, sizeof(CUVIDEOFORMAT));

	((CallBacks *)UserData)->Step = 1;

	return 1;
	}


// Called when a picture is ready to be decoded (decode order)
//
int CUDAAPI Nvidia::DecodePicture(void *UserData, CUVIDPICPARAMS *PICPARAMS)
	{
	memcpy(&((CallBacks *)UserData)->PIC_PARAMS, PICPARAMS, sizeof(CUVIDPICPARAMS));

	((CallBacks *)UserData)->Step += 2;

	return 1;
	}


// Called whenever a picture is ready to be displayed (display order)
//
int CUDAAPI Nvidia::DisplayPicture(void *UserData, CUVIDPARSERDISPINFO *PARSERDISPINFO)
	{
	memcpy(&((CallBacks *)UserData)->PARSER_DISP_INFO, PARSERDISPINFO, sizeof(CUVIDPARSERDISPINFO));

	((CallBacks *)UserData)->Step += 4;

	return 1;
	}