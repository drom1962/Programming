#ifndef _DECODER_H_
#define _DECODER_H_

#include <windows.h>

#include "OVDecoderTrait.h"

#include "cuda_runtime.h"
#include "cuda_runtime_api.h"
#include "device_launch_parameters.h"

#include "nvcuvid.h"



struct DecoderInfo
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




// Берм файл в пямяти
struct Frame
	{
	DWORD			Size;
	int				Type;
	unsigned char	*Data;
	};

// Берм файл в пямяти
struct FrameInfo
{
	DWORD			Size;
	unsigned char	*Data;
};



enum
{
	Decod			= 0x00000001,
	DecodResize		= 0x00000010,
	PostResize		= 0x00000020,
	CopyDevToHost	= 0x00000100,
	Surface			= 0x00001000
};



#pragma pack(push,1)
struct	BMP 
		{
		BITMAPFILEHEADER	BM_Heade;
		BITMAPINFOHEADER	BM_Info;
		};
#pragma pack(pop)

//
//		Интерфейс декодера
//
class Decoder
{
public:
	int				DecoderFeature;	// Свойства декодера

	unsigned char	*LocalBuff;		// Локальный буфер под декодированный кадр
	int				SizeBuff;		// и его размер

	DecoderInfo		DI;

public:

	virtual				~Decoder() =0;

	virtual int			Feature() = 0;

	virtual int			Init(int Dev,DecoderInfo *DI) = 0;

	virtual int			Decode(Frame *Frame, FrameInfo *FI) = 0;

};

using namespace ov_decoder;

//
//     Декодер через FFmpeg
//
class Ffmpeg: public Decoder
{
private:
	BMP					m_BitMap;

	//
	video_decoder		*m_dec;
	video_codec			videoCodec;


public:
				Ffmpeg();

				~Ffmpeg();

	int			Feature() override;

	int			Init(int Dev, DecoderInfo *DI) override;

	int			Decode(Frame *Frame, FrameInfo *FI) override;

	int			Destroy();

private:
static void * MyAlloc(size_t cb, void *oldptr, void *arg);
};

//
//     Декодер через NVIDIA
//
class Nvidia: public Decoder
{
private:
	CUvideoparser   m_Parser;   // Парсер

public:
				Nvidia();

				~Nvidia();

	int			Feature() override;

	int			Init(int Dev, DecoderInfo *DI) override;

	int			Decode(Frame *Frame, FrameInfo *FI) override;

	int			Destroy();

};



#endif