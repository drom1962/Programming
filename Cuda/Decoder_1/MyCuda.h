#ifndef _MYCUDA_
#define _MYCUDA_
#include <windows.h>

#include "cuda_runtime.h"
#include "cuda_runtime_api.h"
#include "device_launch_parameters.h"

#include "nvcuvid.h"


int CUDAAPI SequenceCallback(void *UserData, CUVIDEOFORMAT *VIDEOFORMAT);
int CUDAAPI DecodePicture	(void *UserData, CUVIDPICPARAMS *PICPARAMS);
int CUDAAPI DisplayPicture	(void *UserData, CUVIDPARSERDISPINFO *PARSERDISPINFO);

enum VideoCodecType
	{
		NONE = 0,
		MJPEG,
		MPEG4,
		H264,
		H265
	};


struct TreadParm
	{
	int			ID;
	int			TypeDecoder;
	};



#include "cuviddec.h"
#include  "nvcuvid.h"

//  Для поля Flag
//
enum DecoderFlags
    {
    CUDA		=0,
    VP			=1,
    DXVA        =2,

    Color		=0,
    BlackWhite	=1
    };

struct cuVideoFrameInfo
	{
	int					Size;
	unsigned char		*Frame;
	};


struct cuDecoderInfo
	{
	// Входные
	unsigned int	Codec;
	unsigned int	Height;
	unsigned int	Width;
	unsigned int	WorkPict;

	// Выходные
	unsigned int	OutPict;
	unsigned int	NewHeight;
	unsigned int	NewWidth;
	unsigned int	Flag;

	unsigned int	Pitch;
	};

struct CallBacks
	{
	int						Step;
	// Callback
	CUVIDEOFORMAT			VIDEO_FORMAT;
	CUVIDPICPARAMS			PIC_PARAMS;
	CUVIDPARSERDISPINFO		PARSER_DISP_INFO;
	};


// Берм файл в пямяти
struct Frames
	{
	DWORD			Size;
	int				Type;
	unsigned char	*Data;
	};

enum
	{
	DecodResize		=0x00000010,
	PostResize		=0x00000020,
	CopyDevToHost	=0x00000100,
	Surface			=0x00001000
	};

class MyCuda
{
private:

	unsigned int	m_Opportunities;

	bool			m_Device;               // Устройство

	cudaError_t		m_LastError;            // Типа ошибки

	CallBacks	    m_CallBacks;            // 

	CUvideoparser   m_Parser;               // Парсер

	CUvideodecoder	m_Decoder;              // Декодер

	cuDecoderInfo	m_DI;

    // Локальный буфер
	unsigned int	m_DecodFrameSize;       // Размер внутреннего буфера
	unsigned char	*m_BuffDecodFrame;      // Сам буфер

public:

	MyCuda();

    MyCuda(int Device);

	~MyCuda();

	int	GetVersion();

	unsigned int GetOpportunities();

    // Парсинг
	int CreateParser(unsigned char VideoCodec);

   	int ParserFrame(Frames);

    int DestroyParser(CUvideoparser);

	static int CUDAAPI SequenceCallback (void *UserData, CUVIDEOFORMAT *VIDEOFORMAT);
	static int CUDAAPI DecodePicture    (void *UserData, CUVIDPICPARAMS *PICPARAMS);
	static int CUDAAPI DisplayPicture   (void *UserData, CUVIDPARSERDISPINFO *PARSERDISPINFO);

    // Декодирование
    int CreateDecoder(cuDecoderInfo *);

    int DecoderFrame(int PictIdx);

	int GetDecodeFrame(int PictIdx,unsigned char *Buff,int Size,int Flag);

   	int DestroyDecoder(CUvideodecoder );

	// DirectX

	int PostProcessFrame();
	
	int GetD3D9Surface(int PictIdx,void *Surface);


    // Работа с памятью (зачем не знаю)
    int AllocateMemory(void **,int);

	int FreeMemory(void *);

	int CopyMemoryHostToDevice(const void *,void *,int Size);

	int CopyMemoryDeviceToHost(const void *,void *,int Size);
    
	unsigned char * GetBuff();
};
#endif