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
	OVI_SmallBuff=0x12,
    OVI_Max = 0x13,
    OVI_Err1 = 0x20,
    OVI_Err2 = 0x21,
    OVI_Err3 = 0x22,
    OVI_Err4 = 0x23,
	OVI_NotSupport = 0xfe

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
	uint32_t			Width;						// ����������
	uint32_t			Height;
	uint32_t			VideoBitRate;				// �������
	uint32_t			GOP;						// GOP
	double				FPS;						// FPS

	double				Duration;					// ����� ���������
	uint32_t            CountVideoFrame;			// ���������� �������
	
	// ��� ����� ������
	uint8_t				AudioCodec;					// ��� ������
	uint32_t			BitsPerSample;				// �������
	uint32_t			SamplesPerSec;

	uint32_t            CountAudioSample;			// ���������� �������

	uint32_t			SizeOviFile;

	// �������� ����
	uint32_t			MaxSizeVideoFrame;			// ������������ ����� ����
	uint32_t			MaxSizeAudioSample;			// ������������ ����� ����
	};



struct VideoFrameInfo
	{
	uint64_t			Time;						// ��������� �����
	uint8_t		        Type;						// ��� �����
	uint32_t			Size;						// ������ 
    uint32_t			SizeUserData;				// ������ ����������

	int					Codec;						// �����
	
    unsigned char		*Frame;  					// ��������� �����
	};

struct AudioSampleInfo
	{
	uint64_t			Time;						// ��������� �����
	uint32_t			Size;						// ������ 
    unsigned char		*Sample;  					// ��������� �����
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

	DWORD               CountMetaDataFrame;			// ���������� �������
	};




struct MetaDataInfo
	{
	uint32_t			Size;						// ������ 
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
		uint32_t			m_Error;
	public:
		virtual				~Archiv(){}

		virtual bool		CheckExtension(wchar_t *)=0;

		virtual Archiv		*CreateConteiner()=0;

        virtual void		Version(V_e_r*)=0;


		// 
		virtual int			Create(const wchar_t *FileName,FileInfo *FI)=0;

		virtual int			CreateEx(const wchar_t *FileName,uint8_t *Pass,FileInfo *FI)=0;

		virtual int			Open(const wchar_t *FileName,FileInfo *FI)=0;

		virtual int			OpenEx(const wchar_t *FileName,uint8_t *Pass,FileInfo *FI)=0;
		
		virtual	int			IsOpen()=0;

		virtual int			Close(FileInfo *FI)=0;

		virtual int			GetFileInfo(FileInfo *FI)=0;

		virtual	int			SetFileInfo(const FileInfo *FileInfo)=0;


		// ����� �����
		virtual int			WriteVideoFrame(const void *VideoFrame,uint32_t SizeFrame,int KeyFlag,uint64_t Time,const void  *UserData,uint32_t Size)=0;

		virtual int			ReadVideoFrame(const long IndexFrame,void *BuffFrame,uint32_t BuffSize,VideoFrameInfo *VFI)=0;

		virtual int			SeekVideoFrameByTime(const uint64_t Time,uint32_t *IndexFrame)=0;

		virtual long		SeekPreviosKeyVideoFrame(long IndexFrame)=0;

		virtual long		SeekNextKeyVideoFrame(long IndexFrame)=0;

		virtual int			SetExtraData(const void *ExtraData,uint32_t BuffSize)=0;

		virtual int			GetExtraData(unsigned char *ExtraData,uint32_t BuffSize,uint32_t *SizeExtraData)=0;

		virtual int			GetInfoVideoFrame(uint32_t IndexFrame,VideoFrameInfo *VFI)=0;


		// �������� �����
		virtual int			WriteAudioSample(const void *Sample,uint32_t SizeSample,uint64_t Time)=0;

		virtual int			ReadAudioSample(const uint32_t IndexSample,const void *Sample,uint32_t Size,AudioSampleInfo *ASI)=0;

		virtual long		SeekAudioSampleByTime(const uint64_t Time,uint32_t *InxedSample)=0;


		// 
		virtual	int			Refresh(const int Count)=0;
	
		virtual int			Flush()=0;

		virtual int			Recovery(const wchar_t *FileName)=0;

		//
		virtual int			GetLastError()=0;
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
		virtual int					WriteMetaData(unsigned char  *MetaData, uint32_t Size, uint64_t Time)=0;

		virtual int					ReadMetaData(long IndexFrame, unsigned char *BuffFrame, uint32_t BuffSize, MetaDataInfo *FI)=0;

		virtual int					SeekMetaDataByTime(uint64_t Time, uint32_t *IndexFrame)=0;


		//
		//
		virtual int					Refresh(int Count)=0;

		virtual int					Flush()=0;

		virtual int					Close(MetaDataFileInfo *FI)=0;

		virtual char *				Errors(int Cod)=0;

		virtual int					Recovery(LPCWSTR FileName)=0;
	
	};

}
#endif