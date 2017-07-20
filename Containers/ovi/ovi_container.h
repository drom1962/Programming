#include <windows.h>
#include <stdint.h>


#include "..\container.h"

#include "..\common.h"

using namespace Archive_space;


enum {
	RefreshVideo		=0x01,
	RefreshAudio		=0x02,

	G_O_P				=60,	

	VIDEOBUFFERSIZE		=1024*1024,
	AUDIOBUFFERSIZE		=1*1024,
	FRAMEBUFFERSIZE		=128*1024,

    MAXFRAMESINTOCHANC	= 200,
    MAXSIZEBLOCK		= 1024 * 1024
	};


#pragma pack(push,1)

struct Header_OVI								    // ��������� ��������� �����
{
    unsigned char		Ver;						// ������ ����������
    unsigned char		VerOVSML;					// ������ �������

	unsigned int		Mod;

    FILETIME			Time;						// ����� ������� �����

	DWORD		        ExtraData;                  // ������� ExtaData � �����
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
/*
struct ElementVideoIndex
	{
	uint64_t			TimeFrame;					// ��������� �����

	DWORD				Position;					//  
    DWORD				SizeFrame;					// ������ 
    DWORD				UserPosition;				// ���������������� ������ � �����
    DWORD				SizeUser;					// � �� ������ 
    DWORD				VideoChank;

	unsigned int		NumberIndex;				// ����� ����� - ???
	uint8_t		        TypeFrame;					// ��� �����

	};

struct VideoChank
	{
	DWORD					NextChank;				            // ��������� ���� � �����
	DWORD					CountFrameIntoChank;	            // ���������� ������� � �����
	ElementVideoIndex		IndexFrame[MAXFRAMESINTOCHANC];		// ������ ����� ��� �������
	// � ����� �������� ���� �����
	};
*/	
struct ElementVideoIndex2
	{
	uint64_t			TimeFrame;					// ��������� �����
	
    DWORD				Position;					//  
    DWORD				SizeFrame;					// ������ 
    DWORD				SizeUserData;				// ������ ���������������� ������
    DWORD				VideoChank;

	unsigned int		NumberIndex;				// ����� ����� - ???
	uint8_t     		TypeFrame;					// ��� �����
	};


struct ElementAudioIndex
	{
	uint64_t			TimeFrame;					// ��������� �����
    unsigned int		MumberIndex;				// ����� ����� - ???
	DWORD				Position;					//  
    DWORD				SizeFrame;					// ������ 
    DWORD				AudioChank;
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

#pragma pack(pop)


class   OVI2 : public Archiv
{
private: 
	int					m_VerOVMSML;
	HANDLE		 		m_hLog;						// 

	// 
	uint32_t			m_Flags,
						m_Crypto,
						m_WriteMetaData;

	AES					*AES_HEADER;
	AES					*AES_STREAM;
	WORD				m_CifKeyForHeader[AES_256_Nk];


	// ��������� ������
	unsigned char		m_Mod;						// ���� �� ������ ��� ������ �� ������ 1- ������  0 - ������

	int					m_Flush;					// ����� ����� ����� ������ ������ ������� Flush
													// 0 - �� ������
													// 1,...
	int					c_Flush;

	HANDLE		 		m_hFile;					// ��������� ����� �� ����
	DWORD				m_SizeOviFile;			

	unsigned char		m_CurrentPosition;			// ������� ������� � �����

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

    uint64_t		    m_average_frame_time;

public:
	OVI2(int VerOVSML);
	~OVI2();

	bool		            CheckExtension(wchar_t *);

	Archiv *	            CreateConteiner();

	void 					Version(V_e_r*);

	int Create				(LPCWSTR FileName,FileInfo *FileInfo);

	int Init				(LPCWSTR FileName,FileInfo *FI);

	int InitEx				(LPCWSTR FileName, FileInfo *FI, LPCWSTR Pass);

	int	Open				(LPCWSTR FileName,FileInfo *FI);

	int	OpenEx				(LPCWSTR FileName, FileInfo *FI, LPCWSTR Pass);

	int IsOpen				();

	int						CheckFile();

	DWORD					GetMaxVideoFrame();

	unsigned char *			GetLocalBufer();

	int						SetCurrentFrame(long Index);

	long					GetCurrentFrame();

	int						GetFileInfo			(FileInfo *FileInfo);

	int						GetFileInfo2(LPCWSTR FileName,FileInfo *FI);

	int						GetVideoCodec();
	
	int						SetFileInfo			(FileInfo *FileInfo);

	int						SetExtraData		(unsigned char *ExtraData,DWORD BuffSize);

	int						GetExtraData		(unsigned char *ExtraData,DWORD BuffSize,DWORD *SizeExtraData);

	// ������������� ��������� �� ������ ��� �� ���������� (200 ������ � �����)
	//
	int						WriteVideoFrame(unsigned char *VideoFrame,DWORD SizeFrame,int KeyFlag,uint64_t Time,unsigned char  *UserData,DWORD Size);

	int						WrireGroupVideoFrames();

	int						SizeVideoFrame(long IndexFrame);

	int						GetInfoVideoFrame(DWORD IndexFrame,VideoFrameInfo *FI);

	int						ReadVideoFrame(long IndexFrame,unsigned char *BuffFrame,DWORD BuffSize,VideoFrameInfo *FI);

	int						ReadNextVideoFrame(unsigned char *BuffFrame,DWORD BuffSize,VideoFrameInfo *FI);
	
	int						ReadGroupVideoFrame(DWORD VideoChunk);

	int						SeekVideoFrameByTime(uint64_t Time,DWORD *IndexFrame);

	long					SeekPreviosKeyVideoFrame(long IndexFrame);

	long					SeekNextKeyVideoFrame(long IndexFrame);
	
	int						ReadNextKeyVideoFrame(DWORD IndexFrame,unsigned char *BuffFrame,DWORD BuffSize,VideoFrameInfo *FI);

	int						ReadPreviosKeyFrame(DWORD IndexFrame,unsigned char *BuffFrame,DWORD BuffSize,VideoFrameInfo *FI);

	// ������������� ��������� �� ...
	//
	int						PutAudioFrameIntoBuff(unsigned char *AudioFrame);

	int						WrireGroupAudioFrames();

	int						ReadAudioFrame(DWORD IndexFrame,unsigned char *BuffFrame,DWORD BuffSize);

	int						SeekAudioFrameByTime(uint64_t Time,DWORD *InxedFrame);
	

	int						Refresh(int Count);
	
	int						Flush();
	
	int						Close(FileInfo *FI);

	char *					Errors(int Cod);

	int		          OVI2::CreateBuff(unsigned char **Buff,int SizeAtom,DWORD SizeBuff,DWORD NewSize);

	int						Recovery(LPCWSTR FileName);

	int						GetVideoFrameInfo(DWORD IndexFrame, VideoFrameInfo *VFI);

	void					Debug(void);

private: 
	

	int		WriteHeader(bool Flag);
	
	int		ReadHeader(Header_OVI *HD);

	DWORD	MyRead(unsigned char  *Buff,DWORD SizeBuff,DWORD Position);
	
	DWORD	MyWrite(void *Buff,DWORD SizeBuff,DWORD Position);

	int		Logs(void *Buff,DWORD Size);

	int		Read_Video_Frame(int Index,unsigned char *BuffFrame,DWORD BuffSize);

	int		Encrypt(unsigned char *Buff,DWORD Size);

	int		Decrypt(unsigned char *Buff,DWORD Size);
};