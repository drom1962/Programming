#ifndef _METADATA_
#define _METADATA_

#include <windows.h>
#include <stdint.h>


#include "..\container.h"

#include "..\common.h"

#pragma pack(push,1)

struct Header_MTD								    // Структура заголовка файла
	{
    unsigned char		Ver;						// Версия контейнера
    unsigned char		VerOVSML;					// Версия писалки

	unsigned int		Mod;

	FILETIME			Time;

	// Параметры потока
	int					Height;						// Ширина
	int					Width;						// Высота
	int					MinObject;					// Минимальный размер обьекта

	// Описание линий пересечения
	int					CountLines;					// Количество линий
	int					PointLines;					// Начало в файле

	// Описание зон детекции
	int					CountZome;					// Количество зон
	int					PointZone;					// Начало зон в файле

	int					FirstMetaDataFrame;			// Начало мета данных

	// Индексы
	int					PointMetaData;				// Начало цепочек метаданных
	
	int					CountMataDataFrame;
	
	int					MainMetaDataIndex;			// Главный индекс
                                                    
    DWORD				CrcFile;					// Контрольная сумма файла
    unsigned char		CrcHeader;					// Контрольная сумма заголовка
	};


struct ElementMetaIndex
	{
	uint64_t			TimeFrame;					// Временная метка

    DWORD				Position;					//  
    uint16_t			Size;						// Размер 

    DWORD				MetaDataChank;
	};
	

struct MetaChank
	{
    DWORD					NextChank;				            // Следующий блок в файле
    DWORD					CountFrameIntoChank;	            // Количество фреймов в блоке
	DWORD					Origanal;
	DWORD					Zip;
    ElementMetaIndex		IndexFrame[200];					// Данные кадра для индекса 
                                                                // В конце индексов сами кадры
	};
#pragma pack(pop)


struct Line
	{
	int		x;
	int		y;
	int		angle;
	int		direction;
	};

struct Zone
	{
	int		Status;
	Point	xy1;
	Point	xy2;
	};

	
enum
	{
	MAXFRAMESINTOCHANC2	= 200,
	MAXSIZEBLOCK2		= 1024 * 32
	};

struct MovingTarget  // От Миши
	{
	DWORD		dwZoneID;
	DWORD		dwObjID;
	LONG		nLeft;		// y1
	LONG		nTop;		// x1
	LONG		nRight;		// y2
	LONG		nBottom;	// x2
	double		fMovAngle;
	};



struct MovingTarget2  // На диск
	{
	uint16_t	nTop;		// x1
	uint16_t	nLeft;		// y1
	
	uint16_t	nRight;		// y2
	uint16_t	nBottom;	// x2
	
	float		fMovAngle;
	};



using namespace Meta_Data;

class   MTD : public MetaData
{
private: 
	int					m_VerOVMSML;
	HANDLE		 		m_hLog;						// 

	HANDLE				m_hFile;

	unsigned int		m_SizeLocalBuff;
	unsigned char		*m_LocalBuff;
	
	Header_MTD			m_H_MTD;

	int					m_Crypto;

	int					m_Flags;
	int					m_Flush,c_Flush;
	int					m_Mod;

	MetaChank			m_MC;
	int					m_SizeMetaDataFrames;

	int					m_CountMataDataFrame;		// Всего кадров с метаданными

	unsigned char		*m_MetaDataBuff;			// Групповой буфер
	int					m_MetaDataBufferSize;		// Его размер
	                    
	unsigned char		*m_ZipMetaDataBuff;			// Запакованный буфер
	int					m_ZipMetaDataBuffSize;		// Его длина

	unsigned char		*m_MetaDataIntoBuffers;		// Текущая позиция в буфере

	int					m_CurrentSizeMetaData;
	int					m_MetaDataLastNext;

	int					m_PosMetaData;

	
	int					m_CountMetaDataFrameIntoChunk;
	int					m_CurrentMetaDataChunk;
	int					m_LastReadFrame;

	ElementMetaIndex	*m_MetaDataIndexs;
	int					m_MaxMetaDataIndex;

	
public:
	MTD(int VerOVSML);
	~MTD();

	void 				Version(V_e_r*);

	int					Create(LPCWSTR FileName, MetaDataFileInfo *FileInfo);

	int					CreateEx(LPCWSTR FileName, MetaDataFileInfo *FI, LPCWSTR Pass);

	int					Open(LPCWSTR FileName, MetaDataFileInfo *FI);

	int					OpenEx(LPCWSTR FileName, MetaDataFileInfo *FI, LPCWSTR Pass);

	int					IsOpen();

	int					WriteLineAndZone(int CountLines, Line *,int CountZones, Zone *);

	// Автоматически разбивает по обьему или по количеству (200 кадров в чанке)
	//
	int					WriteMetaData(unsigned char  *MetaData, DWORD Size, uint64_t Time);

	int					ReadMetaData(long IndexFrame,unsigned char *BuffFrame,DWORD BuffSize, MetaDataInfo *MDI);

	int					SeekMetaDataByTime(uint64_t Time,DWORD *IndexFrame);

	int					Refresh(int Count);
	
	int					Flush();
	
	int					Close(MetaDataFileInfo *FI);

	char *				Errors(int Cod);

	int					Recovery(LPCWSTR FileName);

	unsigned char *		GetLocalBufer();


	int					ParserZone(unsigned char *Zones, int Size, MovingTarget2 **);


private: 
	int		WrireGroupMetaData();

	int		ReadGroupMetadata(DWORD MetaDataChunk);
	
	int		CreateBuff(unsigned char **Buff, int SizeAtom, DWORD SizeBuff, DWORD NewSize);

	int		WriteHeader(bool Flag);
	
	int		ReadHeader(Header_MTD *HD);

	DWORD	MyRead(unsigned char  *Buff,DWORD SizeBuff,DWORD Position);
	
	DWORD	MyWrite(void *Buff,DWORD SizeBuff,DWORD Position);

	int		Logs(void *Buff,DWORD Size);

	int		Encrypt(unsigned char *Buff,DWORD Size);

	int		Decrypt(unsigned char *Buff,DWORD Size);
};

#endif