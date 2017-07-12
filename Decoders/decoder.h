#ifndef _DECODER_H_
#define _DECODER_H_

#include <windows.h>

#include "OVDecoderTrait.h"


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
private:
	int				m_Feature;		// �������� ��������

	unsigned char	*LocalBuff;		// ��������� ����� ��� �������������� ����
	int				SizeBuff;		// � ��� ������

public:

	//Decoder() {};

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
	int					m_Feature;		// �������� ��������

	BMP					m_BitMap;

	char				*m_LocalBuff;		// ��������� ����� ��� �������������� ����
	int					m_SizeBuff;		// � ��� ������

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
class Nvidia: public Decoder
{
private:
	int			m_Feature;		// �������� ��������

public:
				Nvidia();

				~Nvidia();

	int			Feature() override;

	int			Init(int Dev, DecoderInfo *DI) override;

	int			Decode(Frame *Frame, FrameInfo *FI) override;

	int			Destroy();

};



#endif