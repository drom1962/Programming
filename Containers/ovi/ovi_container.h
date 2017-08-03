#include "..\common.h"

#include "..\container.h"
using namespace Archive_space;

enum {
	RefreshVideo				= 0x01,
	RefreshAudio				= 0x02,

	VIDEOMAXSIZEBLOCK			= 1024*1024,	// ������ ������ ��� ������ ����� ������
	FRAMEBUFFERSIZE				= 128*1024,		// ������ �����
	VIDEOMAXFRAMESINTOCHANC		= 200,			// ������ � �����
    
    AUDIOMAXSIZEBLOCK			= 200 * 1024,	// ������ ������ ��� ������ ����� ������
	AUDIOBUFFERSIZE				= 1*1024,		// ������ ���������� ����� �����
	AUDIOMAXFRAMESINTOCHANC		= 200			// ������ � �����
	};


#pragma pack(push,1)
//-------------------------------------------------------------------------------------
//		� � � � � � � � �   � � � � � � � � �   � � � � � � � � � �
//-------------------------------------------------------------------------------------
struct Header_OVI									    
	{
    uint8_t				Ver;						// ������ ����������
    uint32_t			VerOVSML;					// ������ �������

	uint32_t			Mod;

    FILETIME			Time;						// ����� ������� �����

	// ��� ����� ������
    //------------------------------------------------ 
    uint8_t				VideoCodec;					// ��� ������
    uint32_t			Width;						// ����������
    uint32_t			Height;
    uint32_t			VideoBitRate;				// �������
    uint32_t			GOP;						// GOP
    double      		FPS;						// FPS

    double      		Duration;					// ����� ���������

    uint32_t            CountVideoFrame;			// ���������� �������

	uint32_t	        ExtraData;                  // ������� ExtaData � �����
    uint32_t	        SizeExtraData;				// � �� ������


    uint32_t			FirstVideoFrame;			// ������ ����� ������ � �����
    uint32_t			MainVideoIndex;				// ������ ��������� ����� ������� - � ����� ����� ��� ��������

    uint32_t			MaxSizeVideoFrame;			// ������������ ������ ������
    uint32_t			VideoReserved1;
    uint32_t			VideoReserved2;

    // ��� ����� ������
	//------------------------------------------------ 
    uint8_t				AudioCodec;					// ��� ������
    uint32_t			BitsPerSample;				// �������
	uint32_t			SamplesPerSec;

    uint32_t            CountAudioSample;			// ���������� �������

    uint32_t			FirstAudioSample;			// ������ ����� ������ � �����
    uint32_t			MainAudioIndex;				// ������ ��������� ����� ������� - � ����� ����� ��� ��������

    uint16_t			MaxSizeAudioSample;			// ������������ ������ ������
    uint32_t			AudioReserved1;
    uint32_t			AudioReserved2;

    // ����������� �����

    uint32_t			CrcFile;					// ����������� ����� �����
    uint8_t				CrcHeader;					// ����������� ����� ���������
	};

//
//		��������� �������� ����� �������
//
struct ElementVideoIndex2
	{
	uint64_t			TimeFrame;					// ��������� �����
	
    uint32_t			Position;					//  
    uint32_t			SizeFrame;					// ������ 
    uint32_t			SizeUserData;				// ������ ���������������� ������
    uint32_t			VideoChank;

	uint8_t     		TypeFrame;					// ��� �����
	};


//
//		��������� �������� ����� �������
//
struct ElementAudioIndex
	{
	uint64_t			TimeSample;					// ��������� �����
	uint32_t			Position;					//  
    uint16_t			SizeSample;					// ������ 
    uint32_t			AudioChank;
	};


//
//		��������� 
//
struct VideoChank2
	{
    uint32_t				NextChank;								// ��������� ���� � �����
    uint32_t				CountFrameIntoChank;					// ���������� ������� � �����
	uint32_t				SizeAllFrames;
    ElementVideoIndex2		IndexFrame[VIDEOMAXFRAMESINTOCHANC];	// ������ ����� ��� ������� 
																	// � ����� �������� ���� �����
	};


//
//		��������� 
//
struct AudioChank
	{
    uint32_t				NextChank;								// ��������� ���� � �����
    uint32_t				CountFrameIntoChank;					// ���������� ������� � �����
	uint32_t				SizeAllSample;
    ElementAudioIndex		IndexSample[AUDIOMAXFRAMESINTOCHANC];	// ������ ����� ��� ������� 
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
	uint32_t			m_SizeOviFile;			

	unsigned char		m_CurrentPosition;			// ������� ������� � �����
	
	// ������� � ������
	uint32_t			m_FreeIndex;				// ������ ��������� ������
	uint32_t			m_MaxIndex;					// ������ �������

	// ��������� 
	Header_OVI			m_H_OVI;

	// ������� ���������� ��� �����
	VideoChank2			m_VC;							// ��������� ����� �������
	uint32_t            m_CountVideoFrameIntoChunk;		// ���������� ������� � ������
	uint32_t			m_VideoLastNext;				// ��������� ��������� ����

	uint32_t			m_LastVideoChunk;				// ��������� ���������� ����
	uint32_t			m_LastVideoRefresh;
	uint32_t			m_CurrentVideoChunk;			// ������� ������ ������

	// ������ ��� �����������
	uint8_t				*m_VideoBuff;					// ����� ��� �����
	uint32_t			m_VideoBufferSize;				// � ��� ������
	uint8_t				*m_VideoFramesIntoBuffers;

	uint8_t				*m_LocalFrame;					// ����� ��� ����
	uint32_t			m_SizeLocalFrame;				// � ��� ������
	
	uint32_t			m_SizeVideoFrames;				// ������� ������ ������	

	// �������
	uint8_t			    *m_VideoIndexs;					// ����� ��� ��������
	uint32_t			m_MaxVideoIndex;				// � ��� ������

	uint32_t			m_MaxGOP;						// GOP
	
	uint32_t			m_CurrentVideoFrame;			// ����� ����� ������
	
	uint32_t			m_MaxVideoFrame;				// ������ ������������� ������

	uint32_t			m_VideoPosFrames;

	// ������� ���������� ��� �����
	AudioChank			m_AC;							// ��������� ����� �������
	uint32_t			m_AudioLastNext;				// ��������� ��������� ����
	uint32_t			m_LastAudioRefresh;				// ��������� ���������� ����
	uint32_t			m_CurrentAudioChunk;			// ������� ����� ����

	// ������
	uint8_t				*m_AudioBuff;					// ��������� ����� ��� ����� �����
	uint32_t			m_AudioBufferSize;				// � ��� ������
	uint8_t				*m_AudioSamplesIntoBuffers;		// ��������� ������� � ������

	uint32_t			m_SizeAudioSamples;				// ������ ����� ������ � ������

	uint8_t				*m_LocalSample;					// ����� ��� �����
	uint32_t			m_SizeLocalSample;				// � ��� ������
	

	uint32_t			m_CurrentAudioSample;
	uint32_t             m_CountAudioSampleIntoChunk;	// ���������� ������� � ������
	
	uint8_t			    *m_AudioIndexs;					// ����� ��� ��������
	uint32_t			m_MaxAudioIndex;				// ������������ ���������� ��������� � �������

	uint32_t			m_AudioPosSamples;

	uint32_t			m_LastAudioChunk;				// ��������� ���������� ����


    uint64_t		    m_average_frame_time;

public:
	OVI2(int VerOVSML);
	~OVI2();

	bool		            CheckExtension(wchar_t *);

	Archiv *	            CreateConteiner();

	void 					Version(V_e_r*);

	//
	//------------------------
	int Create				(LPCWSTR FileName,FileInfo *FileInfo);

	int CreateEx			(LPCWSTR FileName,LPCWSTR Pass,FileInfo *FileInfo);

	int	Open				(LPCWSTR FileName,FileInfo *FI);

	int	OpenEx				(LPCWSTR FileName, LPCWSTR Pass,FileInfo *FI);

	int IsOpen				();
	
	int GetFileInfo			(FileInfo *FileInfo);

	int	SetFileInfo			(FileInfo *FileInfo);

	int						Close(FileInfo *FI);
	
	// ������������� ��������� �� ������ ��� �� ���������� (200 ������ � �����)
	//
	int						WriteVideoFrame(unsigned char *VideoFrame,uint32_t SizeFrame,int KeyFlag,uint64_t Time,unsigned char  *UserData,uint32_t Size);

	int						ReadVideoFrame(long IndexFrame,unsigned char *BuffFrame,uint32_t BuffSize,VideoFrameInfo *FI);

	int						SeekVideoFrameByTime(uint64_t Time,uint32_t *IndexFrame);

	long					SeekPreviosKeyVideoFrame(long IndexFrame);

	long					SeekNextKeyVideoFrame(long IndexFrame);
	
	int						SetExtraData(unsigned char *ExtraData,uint32_t BuffSize);

	int						GetExtraData(unsigned char *ExtraData,uint32_t BuffSize,uint32_t *SizeExtraData);

	int						GetInfoVideoFrame(uint32_t IndexFrame,VideoFrameInfo *FI);


	// ������������� ��������� �� ...
	//
	int						WriteAudioSample(unsigned char *Sample,uint32_t Size,uint64_t Time);
	
	int						ReadAudioSample(uint32_t IndexSample,unsigned char *Sample,uint32_t Size,AudioSampleInfo *ASI);

	long					SeekAudioSampleByTime(uint64_t Time,uint32_t *InxedSample);

	//
	//
	int						Refresh(int Count);
	
	int						Flush();
	
	int						Recovery(LPCWSTR FileName);

	char *					Errors(int Cod);

	int						CreateBuff(unsigned char **Buff,int SizeAtom,DWORD SizeBuff,DWORD NewSize);



//  ��������� �������

	int						CheckFile();


	void					Debug(void);

private: 

	int		ReadGroupVideoFrame(uint32_t VideoChunk);

	int		ReadGroupAudioSamples(uint32_t AudioChunk);
	
	int		WrireGroupVideoFrames();

	int		WrireGroupAudioSamples();

	int		WriteHeader(bool Flag);
	
	int		ReadHeader(Header_OVI *HD);

	DWORD	MyRead(unsigned char  *Buff,DWORD SizeBuff,DWORD Position);
	
	DWORD	MyWrite(void *Buff,DWORD SizeBuff,DWORD Position);

	int		Read_Video_Frame(int Index,unsigned char *BuffFrame,DWORD BuffSize);

	int		Encrypt(unsigned char *Buff,DWORD Size);

	int		Decrypt(unsigned char *Buff,DWORD Size);

};