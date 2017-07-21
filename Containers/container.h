#ifndef _CONTANER_H_
#define _CONTANER_H_

#include <stdint.h>

// ������ ������
enum
{
    OVI_S_OK = 0x00,
    OVI_File = 0x01,
    OVI_NotOpen = 0x02,
    OVI_NotReadHeder = 0x22,
    OVI_NotClose = 0x03,
    OVI_ReadOnly = 0x04,
    OVI_CreateIndex = 0x05,
    OVI_ReadFrame = 0x06,
    OVI_ExtraNone = 0x07,
    OVI_FrameNone = 0x08,
    OVI_NotAlloc = 0x09,
    OVI_NotWrite = 0x0A,
    OVI_NotRead = 0x7A,
    OVI_NextRefresh = 0x0B,
    OVI_InvalidIndex = 0x0C,
    OVI_InvalidBuffer = 0x0D,
    OVI_InvalidParam = 0x0E,
    OVI_E_FAIL = 0x0F,
    OVI_CrcFile_FAIL = 0x10,
    OVI_CrcHeader_FAIL = 0x11,
    OVI_Max = 0x12,
    OVI_Err1 = 0x20,
    OVI_Err2 = 0x21,
    OVI_Err3 = 0x22,
    OVI_Err4 = 0x23
};

enum
{
	N_O_N_E =0,
	M_J_P_G=1,
	H_2_6_4=3
};

struct FileInfo
	{
	unsigned char		Ver;						// ������ ����������
	unsigned char		VerOVSML;					// ������ �������

	unsigned int		Mod;						// ����� �����
													// ����� ����� ����� ������ ������ ������� Flush
													// 0 - �� ������ 0x0000ffff
													// 1,...												

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


struct V_e_r
	{
	int		Major;
	int		Minor;
    int     sub1;
    int     sub2;
	};


enum Flags : unsigned int
	{
	WriteMetaData	= 0x01000000,	    // ������ MetaData � ��� �� ����
	WriteMetaData2	= 0x02000000,		// ������ MetaData � ���������  ���� (�� �������)
	Crypto			= 0x10000000,   	// ������� ��������
	Crypto_AES		= 0x20000000,   	// AES ����������
	CountFlush		= 0x0000FFFF		// �������� ������ flush
	};

struct MetaDataFileInfo
{
	unsigned char		Ver;						// ������ ����������
	unsigned char		VerOVSML;					// ������ �������

	unsigned int		Mod;						// ����� �����
													// ����� ����� ����� ������ ������ ������� Flush
													// 0 - �� ������ 0x0000ffff
													// 1,...												
													// ��� ����� ������
	int					Height;
	int					Width;
	int					MinObject;

	DWORD               CountMetadataFrame;			// ���������� �������
};




struct MetaDataInfo
	{
	DWORD				Size;						// ������ 
	uint64_t			Time;						// ��������� �����
	unsigned char		*Data;  					// ��������� �����
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
		virtual			~Archiv(){}

		virtual bool	CheckExtension(wchar_t *)=0;

		virtual Archiv *CreateConteiner()=0;

        virtual void	Version(V_e_r*)=0;

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

// ���������� 

		virtual int		InitEx(LPCWSTR FileName, FileInfo *FI, LPCWSTR Pass) = 0;

		virtual int		OpenEx(LPCWSTR FileName, FileInfo *FI, LPCWSTR Pass) = 0;
	};
 
} 


//
//
// ����������� ����� ��� ����������
//
//
namespace Meta_Data
{

	class  MetaData
	{
	public:
		virtual						~MetaData() {}

		virtual void 				Version(V_e_r*)=0;

		virtual int					Create(LPCWSTR FileName, MetaDataFileInfo *FileInfo)=0;

		virtual int					CreateEx(LPCWSTR FileName, MetaDataFileInfo *FI, LPCWSTR Pass)=0;

		virtual int					Open(LPCWSTR FileName, MetaDataFileInfo *FI)=0;

		virtual int					OpenEx(LPCWSTR FileName, MetaDataFileInfo *FI, LPCWSTR Pass)=0;

		virtual int					IsOpen()=0;

		virtual unsigned char *		GetLocalBufer()=0;


		// ������������� ��������� �� ������ ��� �� ���������� (200 ������ � �����)
		//
		virtual int					WriteMetaData(unsigned char  *MetaData, DWORD Size, uint64_t Time)=0;

		virtual int					ReadMetaData(long IndexFrame, unsigned char *BuffFrame, DWORD BuffSize, MetaDataInfo *FI)=0;

		virtual int					SeekMetaDataByTime(uint64_t Time, DWORD *IndexFrame)=0;

		virtual int					Refresh(int Count)=0;

		virtual int					Flush()=0;

		virtual int					Close(MetaDataFileInfo *FI)=0;

		virtual char *				Errors(int Cod)=0;

		virtual int					Recovery(LPCWSTR FileName)=0;
	};

}
#endif