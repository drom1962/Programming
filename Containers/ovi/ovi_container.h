#include "..\common.h"

#include "..\container.h"
using namespace Archive_space;

enum {
	RefreshVideo				= 0x01,
	RefreshAudio				= 0x02,

	VIDEOMAXSIZEBLOCK			= 1024*1024,	// Размер буфера под группу видео кадров
	FRAMEBUFFERSIZE				= 128*1024,		// Размер кадра
	VIDEOMAXFRAMESINTOCHANC		= 200,			// Кадров в блоке
    
    AUDIOMAXSIZEBLOCK			= 200 * 1024,	// Размер буфера под группу аудио кадров
	AUDIOBUFFERSIZE				= 1*1024,		// Размер одиночного аудио кадра
	AUDIOMAXFRAMESINTOCHANC		= 200			// кадров в блоке
	};


#pragma pack(push,1)
//-------------------------------------------------------------------------------------
//		С Т Р У К Т У Р А   З А Г О Л О В К А   К О Н Т Е Й Н Е Р А
//-------------------------------------------------------------------------------------
struct Header_OVI									    
	{
    uint8_t				Ver;						// Версия контейнера
    uint32_t			VerOVSML;					// Версия писалки

	uint32_t			Mod;

    FILETIME			Time;						// Время первого кадра

	// Для видео потока
    //------------------------------------------------ 
    uint8_t				VideoCodec;					// Имя кодека
    uint32_t			Width;						// Разрешение
    uint32_t			Height;
    uint32_t			VideoBitRate;				// Битрейт
    uint32_t			GOP;						// GOP
    double      		FPS;						// FPS

    double      		Duration;					// Длина фрагмента

    uint32_t            CountVideoFrame;			// Количество фреймов

	uint32_t	        ExtraData;                  // Позиция ExtaData в файле
    uint32_t	        SizeExtraData;				// и ее размер


    uint32_t			FirstVideoFrame;			// Начало видео потока в файле
    uint32_t			MainVideoIndex;				// Начало основного видео индекса - в конце файла при закрытии

    uint32_t			MaxSizeVideoFrame;			// Максимальный размер фрейма
    uint32_t			VideoReserved1;
    uint32_t			VideoReserved2;

    // Для аудио потока
	//------------------------------------------------ 
    uint8_t				AudioCodec;					// Код кодека
    uint32_t			BitsPerSample;				// Битрейт
	uint32_t			SamplesPerSec;

    uint32_t            CountAudioSample;			// Количество фреймов

    uint32_t			FirstAudioSample;			// Начало аудио потока в файле
    uint32_t			MainAudioIndex;				// Начало основного аудио индекса - в конце файла при закрытии

    uint16_t			MaxSizeAudioSample;			// Максимальный размер фрейма
    uint32_t			AudioReserved1;
    uint32_t			AudioReserved2;

    // Контрольные суммы

    uint32_t			CrcFile;					// Контрольная сумма файла
    uint8_t				CrcHeader;					// Контрольная сумма заголовка
	};

//
//		Структура элемента видео индекса
//
struct ElementVideoIndex
	{
	uint64_t			Time;						// Временная метка
	
    uint32_t			Offset;						//  
    uint32_t			Size;						// Размер 
    uint32_t			SizeUserData;				// Размер пользовательских данных
    uint32_t			VideoChank;

	uint8_t     		Type;						// Тип кадра
	};


//
//		Структура элемента аудио индекса
//
struct ElementAudioIndex
	{
	uint64_t			Time;						// Временная метка
	uint32_t			Offset;						//  
    uint16_t			Size;						// Размер 
    uint32_t			AudioChank;
	};


//
//		Структура 
//
struct VideoChank
	{
    uint32_t				NextChank;								// Следующий блок в файле
    uint32_t				CountFrameIntoChank;					// Количество фреймов в блоке
	uint32_t				SizeAllFrames;
    ElementVideoIndex		IndexFrame[VIDEOMAXFRAMESINTOCHANC];	// Данные кадра для индекса 
																	// В конце индексов сами кадры
	};


//
//		Структура 
//
struct AudioChank
	{
    uint32_t				NextChank;								// Следующий блок в файле
    uint32_t				CountFrameIntoChank;					// Количество фреймов в блоке
	uint32_t				SizeAllSample;
    ElementAudioIndex		IndexSample[AUDIOMAXFRAMESINTOCHANC];	// Данные кадра для индекса 
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
	uint32_t			m_SizeOviFile;			

	unsigned char		m_CurrentPosition;			// Текущая позиция в файле
	
	// Индексы в памяти
	uint32_t			m_FreeIndex;				// Первый свободный индекс
	uint32_t			m_MaxIndex;					// Размер индекса

	// Заголовок 
	Header_OVI			m_H_OVI;

	// Робочие переменные для видео
	VideoChank			m_VC;							// Локальные видео индексы
	uint32_t            m_CountVideoFrameIntoChunk;		// Количество фреймов в группе
	uint32_t			m_VideoLastNext;				// Последний записаный блок

	uint32_t			m_LastVideoChunk;				// Последний записанный блок
	uint32_t			m_LastVideoRefresh;
	uint32_t			m_CurrentVideoChunk;			// Текущая группа кадров

	// Буфера для видеокадров
	uint8_t				*m_VideoBuff;					// Буфер под кадры
	uint32_t			m_VideoBufferSize;				// и его размер
	uint8_t				*m_VideoFramesIntoBuffers;

	uint8_t				*m_LocalFrame;					// Буфер под кадр
	uint32_t			m_SizeLocalFrame;				// и его размер
	
	uint32_t			m_SizeVideoFrames;				// Текущий размер кадров	

	// Индексы
	uint8_t			    *m_VideoIndexs;					// Буфер под ниндексы
	uint32_t			m_MaxVideoIndex;				// и его размер

	uint32_t			m_MaxGOP;						// GOP
	
	uint32_t			m_CurrentVideoFrame;			// Общее число кадров
	
	uint32_t			m_MaxVideoFrame;				// Размер максимального фрейма

	uint32_t			m_VideoPosFrames;

	// Робочие переменные для аудио
	AudioChank			m_AC;							// Локальные аудио индексы
	uint32_t			m_AudioLastNext;				// Последний записаный блок
	uint32_t			m_LastAudioRefresh;				// Последний прочитаный блок
	uint32_t			m_CurrentAudioChunk;			// Текущий аудио блок

	// Буфера
	uint8_t				*m_AudioBuff;					// Групповой буфер под аудио кадры
	uint32_t			m_AudioBufferSize;				// и его размер
	uint8_t				*m_AudioSamplesIntoBuffers;		// Свободная позиция в буфере

	uint32_t			m_SizeAudioSamples;				// Размер аудио кадров в буфере

	uint8_t				*m_LocalSample;					// Буфер под семпл
	uint32_t			m_SizeLocalSample;				// и его размер
	

	uint32_t			m_CurrentAudioSample;
	uint32_t             m_CountAudioSampleIntoChunk;	// Количество фреймов в группе
	
	uint8_t			    *m_AudioIndexs;					// Буфер под ниндексы
	uint32_t			m_MaxAudioIndex;				// Максимальное количество элементов в индексе

	uint32_t			m_AudioPosSamples;

	uint32_t			m_LastAudioChunk;				// Последний записанный блок


    uint64_t		    m_average_frame_time;

public:
	OVI2(int VerOVSML);
	~OVI2();

	bool		            CheckExtension(wchar_t *);

	Archiv *	            CreateConteiner();

	void 					Version(V_e_r*);

	//
	//------------------------
	int Create				(const wchar_t *FileName,FileInfo *FileInfo);

	int CreateEx			(const wchar_t *FileName,LPCWSTR Pass,FileInfo *FileInfo);

	int	Open				(const wchar_t *FileName,FileInfo *FI);

	int	OpenEx				(const wchar_t *FileName, LPCWSTR Pass,FileInfo *FI);

	int IsOpen				();
	
	int GetFileInfo			(FileInfo *FileInfo);

	int	SetFileInfo			(FileInfo *FileInfo);

	int						Close(FileInfo *FI);
	
	// Автоматически разбивает по обьему или по количеству (200 кадров в чанке)
	//
	int						WriteVideoFrame(const void *VideoFrame,uint32_t SizeFrame,int KeyFlag,uint64_t Time,const void *UserData,uint32_t Size);

	int						ReadVideoFrame(long IndexFrame,void *BuffFrame,uint32_t BuffSize,VideoFrameInfo *FI);

	int						SeekVideoFrameByTime(uint64_t Time,uint32_t *IndexFrame);

	long					SeekPreviosKeyVideoFrame(const long IndexFrame);

	long					SeekNextKeyVideoFrame(const long IndexFrame);
	
	int						SetExtraData(const void *ExtraData,uint32_t BuffSize);

	int						GetExtraData(unsigned char *ExtraData,uint32_t BuffSize,uint32_t *SizeExtraData);

	int						GetInfoVideoFrame(uint32_t IndexFrame,VideoFrameInfo *FI);


	// Автоматически разбивает по ...
	//
	int						WriteAudioSample(const void *Sample,uint32_t Size,uint64_t Time);
	
	int						ReadAudioSample(uint32_t IndexSample,const void *Sample,uint32_t Size,AudioSampleInfo *ASI);

	long					SeekAudioSampleByTime(uint64_t Time,uint32_t *InxedSample);

	//
	//
	int						Refresh(int Count);
	
	int						Flush();
	
	int						Recovery(LPCWSTR FileName);

	char *					Errors(int Cod);

	int						CreateBuff(unsigned char **Buff,int SizeAtom,DWORD SizeBuff,DWORD NewSize);



//  Сервисные функции

	int						CheckFile();


	void					Debug(void);

	int						GetLastError();


private: 

	int		ReadGroupVideoFrame(uint32_t VideoChunk);

	int		ReadGroupAudioSamples(uint32_t AudioChunk);
	
	int		WrireGroupVideoFrames();

	int		WrireGroupAudioSamples();

	int		WriteHeader(bool Flag);
	
	int		ReadHeader(Header_OVI *HD);

	DWORD	MyRead(unsigned char  *Buff,DWORD SizeBuff,DWORD Position);
	
	DWORD	MyWrite(const void *Buff,DWORD SizeBuff,DWORD Position);

	int		Read_Video_Frame(int Index,unsigned char *BuffFrame,DWORD BuffSize);

	int		Encrypt(unsigned char *Buff,DWORD Size);

	int		Decrypt(unsigned char *Buff,DWORD Size);

};