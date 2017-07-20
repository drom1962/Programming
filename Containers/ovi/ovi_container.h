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

struct Header_OVI								    // Структура заголовка файла
{
    unsigned char		Ver;						// Версия контейнера
    unsigned char		VerOVSML;					// Версия писалки

	unsigned int		Mod;

    FILETIME			Time;						// Время первого кадра

	DWORD		        ExtraData;                  // Позиция ExtaData в файле
    DWORD		        SizeExtraData;				// и ее размер

                                                    // Для видео потока
    unsigned char		VideoCodec;					// Имя кодека
    unsigned int		Width;						// Разрешение
    unsigned int		Height;
    unsigned int		VideoBitRate;				// Битрейт
    unsigned int		GOP;						// GOP
    double      		FPS;						// FPS

    double      		Duration;					// Длина фрагмента
    unsigned long		mDuration;					// Длина фрагмента в МС

    DWORD               CountVideoFrame;			// Количество фреймов

    DWORD				FirstVideoFrame;			// Начало видео потока в файле
    DWORD				EndVideoFrame;				// Конец видео потока в файле
    DWORD				MainVideoIndex;				// Начало основного видео индекса - в конце файла при закрытии

    DWORD				MaxSizeVideoFrame;			// Максимальный размер фрейма
    DWORD				Reserved1;
    DWORD				Reserved2;

    // Для аудио потока
    unsigned char		AudioCodec;					// Код кодека
    unsigned int		BitsPerSample;				// Битрейт
    unsigned int		SamplesPerSec;
    DWORD				FirstAudioFrame;				// Начало аудио потока в файле
    DWORD               CountAudioFrame;			// Количество фреймов

    DWORD				MainAudioIndex;				// Начало основного аудио индекса - в конце файла при закрытии

    DWORD				MaxSizeAudioFrame;			// Максимальный размер фрейма
    DWORD				Reserved3;
    DWORD				Reserved4;

    CameraInfo			CI;							// 


                                                    // Контрольные суммы
    DWORD				CrcFile;					// Контрольная сумма файла
    unsigned char		CrcHeader;					// Контрольная сумма заголовка
};
/*
struct ElementVideoIndex
	{
	uint64_t			TimeFrame;					// Временная метка

	DWORD				Position;					//  
    DWORD				SizeFrame;					// Размер 
    DWORD				UserPosition;				// Пользовательские данные к кадру
    DWORD				SizeUser;					// и их размер 
    DWORD				VideoChank;

	unsigned int		NumberIndex;				// Номер кадра - ???
	uint8_t		        TypeFrame;					// Тип кадра

	};

struct VideoChank
	{
	DWORD					NextChank;				            // Следующий блок в файле
	DWORD					CountFrameIntoChank;	            // Количество фреймов в блоке
	ElementVideoIndex		IndexFrame[MAXFRAMESINTOCHANC];		// Данные кадра для индекса
	// В конце индексов сами кадры
	};
*/	
struct ElementVideoIndex2
	{
	uint64_t			TimeFrame;					// Временная метка
	
    DWORD				Position;					//  
    DWORD				SizeFrame;					// Размер 
    DWORD				SizeUserData;				// Размер пользовательских данных
    DWORD				VideoChank;

	unsigned int		NumberIndex;				// Номер кадра - ???
	uint8_t     		TypeFrame;					// Тип кадра
	};


struct ElementAudioIndex
	{
	uint64_t			TimeFrame;					// Временная метка
    unsigned int		MumberIndex;				// Номер кадра - ???
	DWORD				Position;					//  
    DWORD				SizeFrame;					// Размер 
    DWORD				AudioChank;
	};




struct VideoChank2
	{
    DWORD					NextChank;				            // Следующий блок в файле
    DWORD					CountFrameIntoChank;	            // Количество фреймов в блоке
    ElementVideoIndex2		IndexFrame[MAXFRAMESINTOCHANC];		// Данные кадра для индекса 
                                                                // В конце индексов сами кадры
	};


struct AudioChank
	{
    DWORD					NextChank;				            // Следующий блок в файле
    DWORD					CountFrameIntoChank;	            // Количество фреймов в блоке
    ElementAudioIndex		IndexFrame[MAXFRAMESINTOCHANC];		// Данные кадра для индекса 
                                                                // В конце индексов сами кадры
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


	// Параметры записи
	unsigned char		m_Mod;						// Файл на запись или только на чтение 1- запись  0 - чтение

	int					m_Flush;					// Через какое число блоков делать команду Flush
													// 0 - не делать
													// 1,...
	int					c_Flush;

	HANDLE		 		m_hFile;					// Системный хедер на файл
	DWORD				m_SizeOviFile;			

	unsigned char		m_CurrentPosition;			// Текущая позиция в файле

	DWORD				m_CurentVideoFrame;				
	DWORD				m_CurentAudioFrame;				
	DWORD				m_CurentUserData;				
	
	// Индексы в памяти
	DWORD				m_FreeIndex;				// Первый свободный индекс
	DWORD				m_MaxIndex;					// Размер индекса

	// Заголовок 
	Header_OVI			m_H_OVI;


	// Робочие переменные для видео
	unsigned char		*m_VideoBuff;
	DWORD				m_VideoBufferSize; 
	unsigned char		*m_FrameBuff;
	DWORD				m_FrameBuffSize; 

	DWORD				m_MaxGOP; 
	DWORD				m_SizeVideoFrames; 
	DWORD				m_CurrentVideoFrame;			// Общее число кадров
	DWORD               m_CountVideoFrameIntoChunk;		// Количество фреймов в группе
	unsigned char		*m_VideoFramesIntoBuffers;
	DWORD				m_VideoLastNext;				// Последний записаный юлок
	long				m_LastReadFrame;				// Последний прочитаный фрейм
	DWORD				m_MaxVideoFrame;				// Размер максимального фрейма
	
	VideoChank2			m_VC;

	unsigned char	    *m_VideoIndexs;					// Буфер под ниндексы
	DWORD				m_MaxVideoIndex;
	DWORD				m_LastVideoRefresh;

	DWORD				m_CurrentVideoChunk;			// Текущая группа кадров
	DWORD				m_PosFrames;

	// Робочие переменные для аудио
	unsigned char		*m_AudioBuff;
	DWORD				m_AudioBufferSize; 
	DWORD				m_AudioSizeFrames; 
	DWORD				m_CurrentAudioFrame;
	DWORD               m_CountAudioFrameIntoChunk;		// Количество фреймов в группе
	unsigned char		*m_AudioFramesIntoBuffers;
	DWORD				m_AudioLastNext;				// Последний записаный блок
	AudioChank			*m_AC;
	unsigned char	    *m_AudioIndexs;					// Буфер под ниндексы
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

	// Автоматически разбивает по обьему или по количеству (200 кадров в чанке)
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

	// Автоматически разбивает по ...
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