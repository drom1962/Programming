#ifndef _DECODER_H_
#define _DECODER_H_

#include <windows.h>

#include "OVDecoderTrait.h"

#include "cuda_runtime.h"
#include "cuda_runtime_api.h"
#include "device_launch_parameters.h"

#include "nvcuvid.h"

enum VideoCodecType
	{
	NONE = 0,
	MJPEG,
	MPEG4,
	H264,
	H265
	};


struct DecoderInfo
	{
	// �������
	unsigned int	Codec;
	unsigned int	Height;
	unsigned int	Width;
	unsigned int	WorkPict;

	// ��������
	unsigned int	OutPict;
	unsigned int	NewHeight;
	unsigned int	NewWidth;
	unsigned int	Flag;

	unsigned int	Pitch;
	};




// ���� ���� � ������
struct Frame
	{
	DWORD			Size;
	int				Type;
	unsigned char	*Data;
	};

// ���� ���� � ������
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

//  ��� ���� Flag
//
enum DecoderFlags
	{
	CUDA = 0,
	VP = 1,
	DXVA = 2,

	Color = 0,
	BlackWhite = 1
	};

#pragma pack(push,1)
struct	BMP 
		{
		BITMAPFILEHEADER	BM_Heade;
		BITMAPINFOHEADER	BM_Info;
		};
#pragma pack(pop)

//
//		��������� ��������
//
class Decoder
{
public:
	int				DecoderFeature;	// �������� ��������

	unsigned char	*LocalBuff;		// ��������� ����� ��� �������������� ����
	int				SizeBuff;		// � ��� ������

	int				LastError;

	DecoderInfo		DI;

public:

	virtual				~Decoder() =0;

	virtual int			Feature() = 0;

	virtual int			Init(int Dev,DecoderInfo *DI) = 0;

	virtual int			Decode(Frame *Frame, FrameInfo *FI) = 0;

};

using namespace ov_decoder;

//
//     ������� ����� FFmpeg
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
//     ������� ����� NVIDIA
//

struct CallBacks
	{
	int						Step;
	// Callback
	CUVIDEOFORMAT			VIDEO_FORMAT;
	CUVIDPICPARAMS			PIC_PARAMS;
	CUVIDPARSERDISPINFO		PARSER_DISP_INFO;
	};


class Nvidia: public Decoder
{
private:

	CUcontext   	m_ctx;

	CUvideoparser   m_Parser;		// ������

	CallBacks		m_CallBacks;	//

	CUvideodecoder	m_Decoder;
	
public:
				Nvidia();

				~Nvidia();

	int			Feature() override;

	int			Init(int Dev, DecoderInfo *DI) override;

	int			Decode(Frame *Frame, FrameInfo *FI) override;

	int			Destroy();

private:
	int			ParserFrame(Frame *Fr);

	static int CUDAAPI SequenceCallback(void *UserData, CUVIDEOFORMAT *VIDEOFORMAT);
	static int CUDAAPI DecodePicture(void *UserData, CUVIDPICPARAMS *PICPARAMS);
	static int CUDAAPI DisplayPicture(void *UserData, CUVIDPARSERDISPINFO *PARSERDISPINFO);
};


#endif