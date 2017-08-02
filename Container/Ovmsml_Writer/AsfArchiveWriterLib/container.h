#ifndef _CONTANER_H_
#define _CONTANER_H_

#include <stdint.h>

// ������ ������
#define OVI_S_OK			0x00
#define OVI_File			0x01
#define OVI_NotOpen			0x02
#define OVI_NotReadHeder	0x22
#define OVI_NotClose		0x03
#define OVI_ReadOnly		0x04
#define OVI_CreateIndex		0x05
#define OVI_ReadFrame		0x06
#define OVI_ExtraNone		0x07
#define OVI_FrameNone		0x08
#define OVI_NotAlloc		0x09
#define OVI_NotWrite		0x0A
#define OVI_NotRead			0x7A
#define OVI_NextRefresh		0x0B
#define OVI_InvalidIndex	0x0C
#define OVI_InvalidBuffer	0x0D
#define OVI_InvalidParam	0x0E
#define OVI_E_FAIL			0x0F
#define OVI_CrcFile_FAIL	0x10
#define OVI_CrcHeader_FAIL	0x11
#define OVI_Max				0x12
#define OVI_Err1			0x20
#define OVI_Err2			0x21
#define OVI_Err3			0x22
#define OVI_Err4			0x23


#define MAXFRAMESINTOCHANC  200
#define MAXSIZEBLOCK        1024*1024

struct FileInfo
	{
	unsigned char		Ver;						// ������ ����������
	unsigned char		VerOVSML;					// ������ �������

	int					Mod;						// ����� �����

	FILETIME			Time;						// ����� ������� �����

	// ��� ����� ������
	unsigned char		VideoCodec;					// ��� ������
	unsigned int		Width;						// ����������
	unsigned int		Height;
	unsigned int		VideoBitRate;				// �������
	unsigned int		GOP;						// GOP
	double				FPS;						// FPS
	double				Duration;					// ����� ���������
	DWORD               CountVideoFrame;			// ���������� �������
	
	// ��� ����� ������
	unsigned char		AudioCodec;					// ��� ������
	unsigned int		BitsPerSample;				// �������
	unsigned int		SamplesPerSec;
	DWORD               CountAudioFrame;			// ���������� �������

	DWORD				SizeOviFile;
	};


struct VideoFrameInfo
	{
	int					Codec;						// �����
	uint8_t		        TypeFrame;					// ��� �����
	DWORD				SizeFrame;					// ������ 
    DWORD				SizeUserData;				// ������ ����������
	uint64_t			TimeFrame;					// ��������� �����
    unsigned char		*Data;  					// ��������� �����
	};

struct CameraInfo
	{
	char Brend[8];
	char Model[8];
	};


struct Ver
	{
	int		Major;
	int		Minor;
    int     sub1;
    int     sub2;
	};




//
//
// ����������� ����� ��� ����������
//
//
namespace Archive_space
{

const int base_time = 1000000;

class  Archiv
	{
	public:
		virtual ~Archiv(){}

		virtual bool	CheckExtension(wchar_t *)=0;

		virtual Archiv *CreateConteiner()=0;

		virtual int		Init(LPCWSTR FileName,FileInfo *FI)=0;

		virtual int		Open(LPCWSTR FileName,FileInfo *FI)=0;

		virtual	int		IsOpen()=0;

		virtual int		GetFileInfo(FileInfo *FI)=0;

		virtual int		GetFileInfo2(LPCWSTR FileName,FileInfo *FI)=0;

		virtual	int		SetFileInfo(FileInfo *FileInfo)=0;

		virtual int		SetExtraData(unsigned char *ExtraData,DWORD BuffSize)=0;

		virtual int		GetExtraData(unsigned char *ExtraData,DWORD BuffSize,DWORD *SizeExtraData)=0;

		virtual	DWORD	GetMaxVideoFrame()=0;

		virtual int		ReadVideoFrame(long IndexFrame,unsigned char *BuffFrame,DWORD BuffSize,VideoFrameInfo *VFI)=0;

		virtual unsigned char *			GetLocalBufer()=0;

		virtual int		SeekVideoFrameByTime(uint64_t Time,DWORD *IndexFrame)=0;

		virtual long	SeekPreviosKeyVideoFrame(long IndexFrame)=0;

		virtual long	SeekNextKeyVideoFrame(long IndexFrame)=0;

		virtual int		WriteVideoFrame(unsigned char *VideoFrame,DWORD SizeFrame,int KeyFlag,uint64_t Time,unsigned char  *UserData,DWORD Size)=0;

		virtual	int		Refresh(int Count)=0;
	
		virtual int		Flush()=0;
	
		virtual int		Close(FileInfo *FI)=0;

		virtual int		Recovery(LPCWSTR FileName)=0;

	};
 
} 

unsigned char Crc8(unsigned char *pcBlock, unsigned int len,unsigned char Old_crc);

#endif