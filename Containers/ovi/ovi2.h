#ifndef _OVI2_H_
#define _OVI2_H_


#include <windows.h>
#include <stdint.h>

//#include "..\ArchiveData.h"


enum VideoCodecType2
{
    N_O_N_E = 0,
    M_J_P_E_G,
    M_P_E_G_4,
    H_2_6_4,
    H_2_6_5
};

#include "container.h"

using namespace Archive_space;

#define RefreshVideo		0x01
#define RefreshAudio		0x02

#define G_O_P				60	

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


#pragma pack(push,1)

enum video_codec {
    vcodec_undefined = 0,
    vcodec_mjpeg = 1,
    vcodec_mpeg4 = 2,
    vcodec_h264 = 3,
    vcodec_h265 = 4
};

struct Header_OVI								    // ��������� ��������� �����
{
    unsigned char		Ver;						// ������ ����������
    unsigned char		VerOVSML;					// ������ �������

    FILETIME			Time;						// ����� ������� �����

                                                    // ExtraData
    DWORD		        ExtraData;					// ������� ExtaData � �����
    DWORD		        SizeExtraData;				// � �� ������

                                                    // ��� ����� ������
    unsigned char		VideoCodec;					// ��� ������
    unsigned int		Width;						// ����������
    unsigned int		Height;
    unsigned int		VideoBitRate;				// �������
    unsigned int		GOP;						// GOP
    double      		FPS;						// FPS

    double      		Duration;					// ����� ���������
    unsigned long		mDuration;					// ����� ��������� � ��

    DWORD               CountVideoFrame;			// ���������� �������

    DWORD				FirstVideoFrame;			// ������ ����� ������ � �����
    DWORD				EndVideoFrame;				// ����� ����� ������ � �����
    DWORD				MainVideoIndex;				// ������ ��������� ����� ������� - � ����� ����� ��� ��������

    DWORD				MaxSizeVideoFrame;			// ������������ ������ ������
    DWORD				Reserved1;
    DWORD				Reserved2;

    // ��� ����� ������
    unsigned char		AudioCodec;					// ��� ������
    unsigned int		BitsPerSample;				// �������
    unsigned int		SamplesPerSec;
    DWORD				FirstAudioFrame;				// ������ ����� ������ � �����
    DWORD               CountAudioFrame;			// ���������� �������

    DWORD				MainAudioIndex;				// ������ ��������� ����� ������� - � ����� ����� ��� ��������

    DWORD				MaxSizeAudioFrame;			// ������������ ������ ������
    DWORD				Reserved3;
    DWORD				Reserved4;

    CameraInfo			CI;							// 


                                                    // ����������� �����
    DWORD				CrcFile;					// ����������� ����� �����
    unsigned char		CrcHeader;					// ����������� ����� ���������
};

#pragma pack(pop)


struct ElementVideoIndex
{
    uint8_t		        TypeFrame;					// ��� �����
    unsigned int		NumberIndex;				// ����� ����� - ???
    DWORD				Position;					//  
    DWORD				SizeFrame;					// ������ 
    uint64_t			TimeFrame;					// ��������� �����
    DWORD				UserPosition;				// ���������������� ������ � �����
    DWORD				SizeUser;					// � �� ������ 
    DWORD				VideoChank;
};

struct ElementVideoIndex2
{
    uint8_t     		TypeFrame;					// ��� �����
    unsigned int		NumberIndex;				// ����� ����� - ???
    DWORD				Position;					//  
    DWORD				SizeFrame;					// ������ 
    DWORD				SizeUserData;				// ������ ���������������� ������
    uint64_t			TimeFrame;					// ��������� �����
    DWORD				VideoChank;
};


struct ElementAudioIndex
{
    unsigned int		MumberIndex;				// ����� ����� - ???
    DWORD				Position;					//  
    DWORD				SizeFrame;					// ������ 
    uint64_t			TimeFrame;					// ��������� �����
    DWORD				AudioChank;
};


struct VideoChank
{
    DWORD					NextChank;				            // ��������� ���� � �����
    DWORD					CountFrameIntoChank;	            // ���������� ������� � �����
    ElementVideoIndex		IndexFrame[MAXFRAMESINTOCHANC];		// ������ ����� ��� ������� 
                                                                // � ����� �������� ���� �����
};

struct VideoChank2
{
    DWORD					NextChank;				            // ��������� ���� � �����
    DWORD					CountFrameIntoChank;	            // ���������� ������� � �����
    ElementVideoIndex2		IndexFrame[MAXFRAMESINTOCHANC];		// ������ ����� ��� ������� 
                                                                // � ����� �������� ���� �����
};


struct AudioChank
{
    DWORD					NextChank;				            // ��������� ���� � �����
    DWORD					CountFrameIntoChank;	            // ���������� ������� � �����
    ElementAudioIndex		IndexFrame[MAXFRAMESINTOCHANC];		// ������ ����� ��� ������� 
                                                                // � ����� �������� ���� �����
};



class   OVI2 : public Archiv
{
private: 
	int					m_VerOVMSML;
	HANDLE		 		m_hLog;						// 


	HANDLE		 		m_hFile;					// ��������� ����� �� ����
	DWORD				m_SizeOviFile;			

	unsigned char		m_CurrentPosition;			// ������� ������� � �����

	unsigned char		m_Mod;						// ���� �� ������ ��� ������ �� ������ 1- ������  0 - ������

	DWORD				m_CurentVideoFrame;				
	DWORD				m_CurentAudioFrame;				
	DWORD				m_CurentUserData;				
	
	// ������� � ������
	DWORD				m_FreeIndex;				// ������ ��������� ������
	DWORD				m_MaxIndex;					// ������ �������

	// ��������� 
	Header_OVI			m_H_OVI;


	// ������� ���������� ��� �����
	unsigned char		*m_VideoBuff;
	DWORD				m_VideoBufferSize; 
	unsigned char		*m_FrameBuff;
	DWORD				m_FrameBuffSize; 

	DWORD				m_MaxGOP; 
	DWORD				m_SizeVideoFrames; 
	DWORD				m_CurrentVideoFrame;			// ����� ����� ������
	DWORD               m_CountVideoFrameIntoChunk;		// ���������� ������� � ������
	unsigned char		*m_VideoFramesIntoBuffers;
	DWORD				m_VideoLastNext;				// ��������� ��������� ����
	long				m_LastReadFrame;				// ��������� ���������� �����
	DWORD				m_MaxVideoFrame;				// ������ ������������� ������
	
	VideoChank2			m_VC;

	unsigned char	    *m_VideoIndexs;					// ����� ��� ��������
	DWORD				m_MaxVideoIndex;
	DWORD				m_LastVideoRefresh;

	DWORD				m_CurrentVideoChunk;			// ������� ������ ������
	DWORD				m_PosFrames;

	// ������� ���������� ��� �����
	unsigned char		*m_AudioBuff;
	DWORD				m_AudioBufferSize; 
	DWORD				m_AudioSizeFrames; 
	DWORD				m_CurrentAudioFrame;
	DWORD               m_CountAudioFrameIntoChunk;		// ���������� ������� � ������
	unsigned char		*m_AudioFramesIntoBuffers;
	DWORD				m_AudioLastNext;				// ��������� ��������� ����
	AudioChank			*m_AC;
	unsigned char	    *m_AudioIndexs;					// ����� ��� ��������
	DWORD				m_MaxAudioIndex;
	DWORD				m_LastAudioRefresh;

public:
	OVI2(int VerOVSML);
	~OVI2();

    bool    CheckExtension(wchar_t * Ext);

    Archiv  * CreateConteiner();

	void 	Version(struct V_e_r*);

	int		Create				(LPCWSTR FileName,FileInfo *FileInfo);

	int		Init				(LPCWSTR FileName,FileInfo *FI);

	int		Open				(LPCWSTR FileName,FileInfo *FI);

	int		IsOpen				();

	int		CheckFile();

	DWORD	GetMaxVideoFrame();

	unsigned char *	GetLocalBufer();

	int		SetCurrentFrame		(long Index);

	long	GetCurrentFrame		();

	int		GetFileInfo			(FileInfo *FileInfo);

	int		GetFileInfo2		(LPCWSTR FileName, FileInfo *FI);

	int		GetVideoCodec();
	
	int		SetFileInfo			(FileInfo *FileInfo);

	int		SetExtraData		(unsigned char *ExtraData,DWORD BuffSize);

	int		GetExtraData		(unsigned char *ExtraData,DWORD BuffSize,DWORD *SizeExtraData);

	// ������������� ��������� �� ������� ��� H.264 � ��� MJPG �� 
	//
	int		WriteVideoFrame(unsigned char *VideoFrame,DWORD SizeFrame,int KeyFlag,uint64_t Time,unsigned char  *UserData,DWORD Size);

	int		WrireGroupVideoFrames();

	int		SizeVideoFrame(long IndexFrame);

	int		GetInfoVideoFrame(DWORD IndexFrame,VideoFrameInfo *FI);

	int		ReadVideoFrame(long IndexFrame,unsigned char *BuffFrame,DWORD BuffSize,VideoFrameInfo *FI);

	int		ReadNextVideoFrame(unsigned char *BuffFrame,DWORD BuffSize,VideoFrameInfo *FI);
	
	int		ReadGroupVideoFrame(DWORD VideoChunk);

	int		SeekVideoFrameByTime(uint64_t Time,DWORD *IndexFrame);

	long	SeekPreviosKeyVideoFrame(long IndexFrame);

	long	SeekNextKeyVideoFrame(long IndexFrame);
	
	int		ReadNextKeyVideoFrame(DWORD IndexFrame,unsigned char *BuffFrame,DWORD BuffSize,VideoFrameInfo *FI);

	int		ReadPreviosKeyFrame(DWORD IndexFrame,unsigned char *BuffFrame,DWORD BuffSize,VideoFrameInfo *FI);

	// ������������� ��������� �� ...
	//
	int		PutAudioFrameIntoBuff(unsigned char *AudioFrame);
	
	int		WrireGroupAudioFrames();

	int		ReadAudioFrame(DWORD IndexFrame,unsigned char *BuffFrame,DWORD BuffSize);

	int		SeekAudioFrameByTime(uint64_t Time,DWORD *InxedFrame);
	

	int		Refresh(int Count);
	
	int		Flush();
	
	int		Close(FileInfo *FI);

	char *	Errors(int Cod);

	int		OVI2::CreateBuff(unsigned char **Buff,int SizeAtom,DWORD SizeBuff,DWORD NewSize);

	int		Recovery(LPCWSTR FileName);
	
	int		InitEx(LPCWSTR FileName, FileInfo *FI, LPCWSTR Pass);
	int		OpenEx(LPCWSTR FileName, FileInfo *FI, LPCWSTR Pass);

private: 
	

	int		WriteHeader(bool Flag);
	
	int		ReadHeader(Header_OVI *HD);

	DWORD	MyRead(unsigned char  *Buff,DWORD SizeBuff,DWORD Position);
	
	DWORD	MyWrite(void *Buff,DWORD SizeBuff,DWORD Position);

	int		Logs(void *Buff,DWORD Size);

	int		Read_Video_Frame(int Index,unsigned char *BuffFrame,DWORD BuffSize);
};

#endif 