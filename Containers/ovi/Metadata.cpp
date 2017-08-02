#include "..\ovi\metadata.h"


#include "zlib.h"

#include <iostream>

using namespace Meta_Data;

//===================================================================================================================================================
V_e_r MTDver = { 1,0,0,0 };

void MTD::Version(V_e_r *V)
{
	if (V != nullptr)
		memcpy(V, &MTDver, sizeof(V_e_r));
}


MTD::MTD(int Ver_OVMSML)
	{
	// Заполним переменные обьекты
	m_hFile					= nullptr;

	m_MetaDataBuff			= nullptr;
	m_MetaDataBufferSize	= MAXSIZEBLOCK2;

	m_LocalBuff				= nullptr;
	m_SizeLocalBuff			= 0;

	m_MetaDataIndexs		= nullptr;

	m_ZipMetaDataBuffSize	= MAXSIZEBLOCK2 + 12;
	m_ZipMetaDataBuff		= nullptr;

	m_SizeMetaDataFrames	= 0;

	// Флаги
	m_Crypto = 0;
	};


MTD::~MTD()
	{
	if (m_hFile != nullptr)  CloseHandle(m_hFile);

	if (m_MetaDataBuff != nullptr)		free(m_MetaDataBuff);

	if (m_MetaDataIndexs != nullptr)	free(m_MetaDataIndexs);

	if (m_ZipMetaDataBuff != nullptr)	free(m_ZipMetaDataBuff);
	};


//
//  Создадим файл
//
int MTD::Create(LPCWSTR FileName, MetaDataFileInfo *FI)
	{
	if (m_hFile != nullptr)    return OVI_File;

	if (FI == nullptr)         return OVI_File;

	m_hFile = CreateFileW(FileName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
		nullptr);

	// Проверим результат
	if (m_hFile == INVALID_HANDLE_VALUE)
		{
		return GetLastError();
		}

	// Установи сжатие файл
	USHORT Comp = COMPRESSION_FORMAT_DEFAULT;
	DWORD res;

	DeviceIoControl(m_hFile,	// handle to file or directory
		FSCTL_SET_COMPRESSION,  // dwIoControlCode
		&Comp,					// input buffer
		sizeof(USHORT),			// size of input buffer
		NULL,                   // lpOutBuffer
		0,                      // nOutBufferSize
		&res,					// number of bytes returned
		NULL);					// OVERLAPPED structure


	// Возьмем флажки
	m_Mod = 1;							// Режим записи

	m_Flags = FI->Mod;

	m_H_MTD.Mod = m_Flags;				// Сохраним флаги 

	m_Flush = FI->Mod  & CountFlush;	// Возьмем режим работы Flush
	c_Flush = 0;

	// Почистим память под видео цепочку
	memset((void *)&m_MC, 0, sizeof(MetaChank));

	// Выделим буфера под видео
	if (m_MetaDataBuff==nullptr)
		if(CreateBuff(&m_MetaDataBuff, 1, 0, m_MetaDataBufferSize))												return OVI_NotAlloc;
	m_MetaDataIntoBuffers = m_MetaDataBuff;

	if (m_ZipMetaDataBuff==nullptr)
		if(CreateBuff(&m_ZipMetaDataBuff, 1, 0, m_ZipMetaDataBuffSize))											return OVI_NotAlloc;  // Для группового чтения

	// Выделим буфера под индексы
	if (m_MetaDataIndexs == nullptr)
		{
		m_MaxMetaDataIndex = 1024;
		if (CreateBuff((unsigned char **)&m_MetaDataIndexs, sizeof(ElementMetaIndex), 0, m_MaxMetaDataIndex))	return OVI_NotAlloc;
		}

	// Заполним заголовок нулями
	memset((void *)&m_H_MTD, 0, sizeof(Header_MTD));

	// Сохраним параметры
	m_H_MTD.Height = FI->Height;
	m_H_MTD.Width = FI->Width;
	m_H_MTD.MinObject = FI->MinObject;

	m_H_MTD.Ver = 2;

	m_H_MTD.CrcFile = 0;
	m_H_MTD.CrcHeader = 0;

	m_CountMataDataFrame = 0;

	m_CountMetaDataFrameIntoChunk = 0;
	m_CountMataDataFrame = 0;

	m_LastReadFrame = 0;

	m_PosMetaData = 0;

	GetFileTime(m_hFile, &m_H_MTD.Time, nullptr, nullptr);     // Временно

	return WriteHeader(false);
	}

//
//  Создадим файл
//
int MTD::CreateEx(LPCWSTR FileName, MetaDataFileInfo *FI,LPCWSTR Pass)
	{
	return S_OK;
	}

//
//  Откроем на чтение наш контейнер
//
int MTD::Open(LPCWSTR FileName, MetaDataFileInfo *FI)
{
	if (m_hFile != nullptr) return OVI_NotOpen;

	m_hFile = CreateFileW(FileName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY,
		nullptr);

	// Проверим результат
	if (m_hFile == INVALID_HANDLE_VALUE)
		{
		m_hFile = nullptr;
		return OVI_NotOpen;
		}

	m_Mod = 0;		// Режим чтения

	// Почистим память под видео цепочку
	memset((void *)&m_MC, 0, sizeof(MetaChank));

	//  Читаем заголовок
	int ret = ReadHeader(&m_H_MTD);

	if (ret)
		{
		CloseHandle(m_hFile);
		m_hFile = nullptr;
		return OVI_CrcHeader_FAIL;
		}

	FI->Height		= m_H_MTD.Height;
	FI->Width		= m_H_MTD.Width;
	FI->MinObject	= m_H_MTD.MinObject;

	// Создадим буфера
	if (m_MetaDataBuff==nullptr)
		if(CreateBuff(&m_MetaDataBuff, 1, 0, m_MetaDataBufferSize))		return OVI_NotAlloc;  // Для группового чтения


	if (m_ZipMetaDataBuff==nullptr)
		if(CreateBuff(&m_ZipMetaDataBuff, 1, 0, m_ZipMetaDataBuffSize))	return OVI_NotAlloc;  // Для группового чтения
	
	if (m_MetaDataIndexs==nullptr)
		if(CreateBuff((unsigned char **)&m_MetaDataIndexs, sizeof(ElementMetaIndex),0, m_H_MTD.CountMataDataFrame)) return OVI_NotAlloc;  // Для группового чтения


	//Refresh();  // Востановим цепочку

	int ff = 0;
	if (m_H_MTD.MainMetaDataIndex != 0L)
		{
		// Есть главных индекс для видео
		if (MyRead((unsigned char *)m_MetaDataIndexs, m_H_MTD.CountMataDataFrame * sizeof(ElementMetaIndex), m_H_MTD.MainMetaDataIndex)) return OVI_InvalidIndex;
		}
	else
		{
		m_CountMataDataFrame = 0;
		Refresh(0);  // Востановим цепочку
		ff++;
		}
	m_CountMetaDataFrameIntoChunk = 0;
	m_CountMataDataFrame = 0;

	m_LastReadFrame = 0;

	m_PosMetaData = 0;

	if (FI != nullptr)
		{
		FI->CountMetadataFrame = m_H_MTD.CountMataDataFrame;
		}

	// Есть индекс - почитаем
	if (ff>0) return OVI_NotClose;




	return S_OK;
	}

//
//  Откроем файл
//
int MTD::OpenEx(LPCWSTR FileName, MetaDataFileInfo *FI, LPCWSTR Pass)
	{
	return S_OK;
	}


//
// Статус файла
//
int  MTD::IsOpen()
	{
	if (m_hFile == nullptr)				return OVI_NotOpen;

	return S_OK;
	}


//
// Запишем линии и зоны
//
int	MTD::WriteLineAndZone(int CountLines, Line *, int CountZones, Zone *)
	{
	if (m_hFile == nullptr)				return OVI_NotOpen;

	return S_OK;
	}




















//
//  Закроем контейнер
//
int MTD::Close(MetaDataFileInfo *FI)
	{
	if (m_hFile == nullptr)				return OVI_NotOpen;

	if (m_Mod == 0)
		{
		// Он на чтение и его закроем
		CloseHandle(m_hFile);
		m_hFile = nullptr;

		return S_OK;
		}

	if (m_CountMataDataFrame == 0)	return OVI_Err1;

	// Сбросим, что осталось в буфере
	WrireGroupMetaData();

	// Сбросим индексы
	DWORD Pos = SetFilePointer(m_hFile, 0L, nullptr, FILE_CURRENT);

	if (MyWrite(m_MetaDataIndexs, sizeof(ElementMetaIndex)*(m_CountMataDataFrame), 0))
		{
		CloseHandle(m_hFile);
		m_hFile = nullptr;

		return OVI_NotWrite;
		}

	// Запишем хедер
	m_H_MTD.MainMetaDataIndex = Pos;
	m_H_MTD.CountMataDataFrame = m_CountMataDataFrame;

	WriteHeader(false);

	// Закроем файл
	CloseHandle(m_hFile);
	m_hFile = nullptr;

	if (FI != nullptr)
		{
		FI->CountMetadataFrame = m_CountMataDataFrame;
		}

	return S_OK;
	}



//
// Сбросим буфера на диск
//
int MTD::Flush()
{
	if (m_hFile == nullptr)	return OVI_NotOpen;

	if (!m_Mod)				return S_OK;

	m_H_MTD.CountMataDataFrame = m_CountMataDataFrame;

	c_Flush++;

	// Уровень надежности
	if (c_Flush == m_Flush)
		{
		//WriteHeader(true);

		FlushFileBuffers(m_hFile);
		c_Flush = 0;
		}

	return S_OK;
}




//
//  Запишем видео кадр в буфер + пользовательские данные
//
int MTD::WriteMetaData(unsigned char  *MetaData, uint32_t Size, uint64_t Time)
	{
	if (m_hFile == nullptr)		return OVI_NotOpen;

	if (m_Mod == 0)	            return OVI_ReadOnly;

	ElementMetaIndex *EMI;

	// Другой критерий разбиения на группы - по 1М
	if (m_CountMetaDataFrameIntoChunk == MAXFRAMESINTOCHANC2 || m_SizeMetaDataFrames>MAXSIZEBLOCK2)
		{
		int res = WrireGroupMetaData();
		if (res) return res;
		}

	if (m_CountMataDataFrame == m_MaxMetaDataIndex)
		{
		CreateBuff((unsigned char **)&m_MetaDataIndexs, sizeof(ElementMetaIndex), m_MaxMetaDataIndex, m_MaxMetaDataIndex + 1024);
		m_MaxMetaDataIndex += 1024;
		}

	// Запишем сами данные
	int c = Size / sizeof(MovingTarget);

	MovingTarget2	ToDisk[1024], *pToDisk = ToDisk;

	// Перегоним в более компактную структуру для записи на диск
	for (int i = 0; i < c; i++)
		{
		pToDisk->nBottom	= ((MovingTarget *)MetaData)->nBottom;
		pToDisk->nTop		= ((MovingTarget *)MetaData)->nTop;

		pToDisk->nLeft		= ((MovingTarget *)MetaData)->nLeft;
		pToDisk->nRight		= ((MovingTarget *)MetaData)->nRight;

		pToDisk->fMovAngle  = ((MovingTarget *)MetaData)->fMovAngle;

		if (pToDisk->fMovAngle < 0. || pToDisk->fMovAngle > 360.)
			{
			int i = 0;
			}

		pToDisk++;
		MetaData += sizeof(MovingTarget);
		}

	Size = c * sizeof(MovingTarget2);
	
	//  Заполнили локальный индекс
	EMI = &m_MC.IndexFrame[m_CountMetaDataFrameIntoChunk]; 
	EMI->Position	= m_SizeMetaDataFrames;
	EMI->Size		= Size;
	EMI->TimeFrame	= Time;

	//  Заполнили глобальный индекс
	EMI = &((ElementMetaIndex *)m_MetaDataIndexs)[m_CountMataDataFrame];
	EMI->Position	= m_SizeMetaDataFrames;
	EMI->Size		= Size;
	EMI->TimeFrame	= Time;


	// Сохраним данные в буфере
	if (m_MetaDataBufferSize < m_SizeMetaDataFrames + Size)
		{
		DWORD xx = m_SizeMetaDataFrames + Size;

		CreateBuff(&m_MetaDataBuff, 1, m_MetaDataBufferSize, xx);

		m_MetaDataBufferSize = xx;

		m_MetaDataIntoBuffers = m_MetaDataBuff + m_SizeMetaDataFrames;
		}
	
	memmove(m_MetaDataIntoBuffers, (const void *)(ToDisk), m_MC.IndexFrame[m_CountMetaDataFrameIntoChunk].Size);

	m_MetaDataIntoBuffers += Size;

	m_SizeMetaDataFrames  += Size;

	m_CountMataDataFrame++;

	m_CountMetaDataFrameIntoChunk++;

	return S_OK;
	}




//
//  Пишем группу кадров.
//
int MTD::WrireGroupMetaData()
	{
	int ret;

	if (m_hFile == nullptr) return OVI_NotOpen;

	if (m_Mod == 0)		return OVI_ReadOnly;

	if (m_CountMetaDataFrameIntoChunk == 0) return S_OK;

	// Вычислим размер заголовка перед группой
	int SizeHeaderChunc = ((char *)&m_MC.IndexFrame[m_CountMetaDataFrameIntoChunk]) - (char *)&m_MC;

	// Запишем Next + Count
	m_MC.CountFrameIntoChank = m_CountMetaDataFrameIntoChunk;

	DWORD Pos = SetFilePointer(m_hFile, 0L, nullptr, FILE_CURRENT);  // Текущая позиция

	if (m_H_MTD.FirstMetaDataFrame == 0)
		{
		m_H_MTD.FirstMetaDataFrame = sizeof(m_H_MTD) + m_H_MTD.CountZome *sizeof(Line) + m_H_MTD.CountZome*sizeof(Zone);
		WriteHeader(true);
		}

	// Сожьмем
	uLongf destLen = m_ZipMetaDataBuffSize;

	compress(m_ZipMetaDataBuff, &destLen, m_MetaDataBuff, m_SizeMetaDataFrames);

	m_MC.Zip		= destLen;
	m_MC.Origanal	= m_SizeMetaDataFrames;

	m_MC.NextChank  = Pos + SizeHeaderChunc + m_MC.Zip; //+ m_SizeMetaDataFrames;
	
	m_MetaDataLastNext = Pos;			// Если добавили другой блок, то нужно скорректировать
										
	Pos += SizeHeaderChunc;				// Добавим длину заголовка

	DWORD MetaDataChank = SetFilePointer(m_hFile, 0L, nullptr, FILE_CURRENT);  // Текущая позиция

	for (DWORD i = 0; i<m_CountMetaDataFrameIntoChunk; i++)
		{
		m_MC.IndexFrame[i].Position += Pos;
		                                       
		((ElementMetaIndex *)m_MetaDataIndexs)[m_CountMataDataFrame - m_CountMetaDataFrameIntoChunk + i].Position += Pos;

		m_MC.IndexFrame[i].MetaDataChank = MetaDataChank;
		((ElementMetaIndex *)m_MetaDataIndexs)[m_CountMataDataFrame - m_CountMetaDataFrameIntoChunk + i].MetaDataChank = MetaDataChank;
		}

	// Запишем индексы
	ret = MyWrite((void *)&m_MC, SizeHeaderChunc, 0);

	// Запишем кадры
	//ret = MyWrite(m_MetaDataBuff, m_SizeMetaDataFrames, 0);
	ret = MyWrite(m_ZipMetaDataBuff, m_MC.Zip, 0);

	// Запишем конец цепочки и перейдем на ее начало
	memset(&m_MC, 0, sizeof(MetaDataChank));
	ret = MyWrite((void *)&m_MC, 16, 0);
	Pos = SetFilePointer(m_hFile, -16L, nullptr, FILE_CURRENT);  // Переместимстились в начало записанного

	// Сбросим переменные
	m_MetaDataIntoBuffers = m_MetaDataBuff;
	m_CountMetaDataFrameIntoChunk = 0;
	m_SizeMetaDataFrames = 0;

	Flush();
	return S_OK;
}




// Возвращает прочитаную длину. Если 0 то кадр не поместился в буфер
//
int MTD::ReadMetaData(long IndexFrame, unsigned char *MetaData, uint32_t MetaDataSize, MetaDataInfo *MDI)
	{
	if (m_hFile == nullptr)			return OVI_NotOpen;

	if (IndexFrame<0)  IndexFrame = m_LastReadFrame;

	ElementMetaIndex *EVI2;

	EVI2 = &((ElementMetaIndex *)m_MetaDataIndexs)[IndexFrame];

	DWORD TotalSize = EVI2->Size;

	if (MetaData != nullptr && MetaDataSize <= TotalSize) return 555;  // кода пока нет

	if (EVI2->MetaDataChank != m_CurrentMetaDataChunk)
		{
		// Прочтем его группу
		ReadGroupMetadata(EVI2->MetaDataChank);

		// Сохраним ее позицию
		m_CurrentMetaDataChunk = EVI2->MetaDataChank;
		}

 	DWORD Pos = EVI2->Position - m_PosMetaData;

	if (MetaData == nullptr)
		{
		// Скопируем во внутренний буфер
		if (m_SizeLocalBuff < TotalSize)
			{
			// Расширим буфер НА 120 ПРОЦЕНТОВ
			DWORD xx = static_cast<DWORD>(TotalSize*1.2);

			if (CreateBuff(&m_LocalBuff, 1, m_SizeLocalBuff, xx)) return OVI_NotAlloc;  // Для чтения кадра во внутренний буфер

			m_SizeLocalBuff = xx;
			}

		MetaData = m_LocalBuff;
		}

	// Возьмем  кадр из буфера кадров
	memcpy(MetaData, m_MetaDataBuff + Pos, TotalSize);

	if (MDI != nullptr)
		{
		// Вернем информацию о фрейме
		MDI->Size		= EVI2->Size;
		MDI->Time		= EVI2->TimeFrame;// / 10;   // Непонятнлое деление
		MDI->Data		= MetaData;
		}
		
	return S_OK;
	}


//
// Прочитаем группу кадров
//
int MTD::ReadGroupMetadata(DWORD VideoChunk)
{
	MetaChank			m_MC;

	if (m_hFile == nullptr)			return OVI_NotOpen;

	if (VideoChunk == 0)				return OVI_NotOpen;

	// Возьмем заголовок чанка
	if (MyRead((unsigned char *)&m_MC, 16, VideoChunk) != S_OK) return -1;

	// Прочитаем локальные индексы
	if (MyRead((unsigned char *)&m_MC.IndexFrame, sizeof(ElementMetaIndex) * m_MC.CountFrameIntoChank, 0) != S_OK) return -2;

	if (m_MetaDataBufferSize<m_MC.Origanal)
		{
		// Увеличим буфер если надо
		CreateBuff(&m_MetaDataBuff, 1, m_MetaDataBufferSize, m_MC.Origanal);
		m_MetaDataBufferSize = m_MC.Origanal;
		}

	// Запомним начало кадров
	m_PosMetaData = SetFilePointer(m_hFile, 0L, nullptr, FILE_CURRENT);

	// Прочтем сжатые какдры разом
	if (MyRead(m_ZipMetaDataBuff, m_MC.Zip, 0) != S_OK) return -3;
	
	uLongf xx = MAXSIZEBLOCK2;
	
	int ret=uncompress(m_MetaDataBuff, &xx, m_ZipMetaDataBuff, m_MC.Zip);

	if (ret!= Z_OK || m_MC.Origanal != xx)		return -3;

	return S_OK;
}




//
//  Поищем видео фрейм по времени
//
int MTD::SeekMetaDataByTime(uint64_t Time, uint32_t *IndexFrame)
	{
	if (m_hFile == nullptr) return OVI_NotOpen;

	if (m_H_MTD.MainMetaDataIndex == 0) Refresh(5);

	DWORD First = 0, End = m_H_MTD.CountMataDataFrame;
	
	DWORD ss;
	// Ускорим поиск метод деления плополам (примерно в два раза)

	ss = End / 2;
	do
		{
		if (Time  < ((ElementMetaIndex *)m_MetaDataIndexs)[ss].TimeFrame)
			{
			// Первая половина
			End = ss;
			ss = End / 2;
			}
		else
			{
			// Втораяя половина
			First = ss;
			ss = (End - First) / 2 + First;
			}
	} while ((End - First) > 5);

	ElementMetaIndex *EMI;

	EMI=&((ElementMetaIndex *)m_MetaDataIndexs)[First];

	for (; First<End; First++,EMI++)
		{
		if (Time == EMI->TimeFrame)
			{
			// Нашли кадр с временем больше заданного
			if (IndexFrame != nullptr) *IndexFrame = First;

			return S_OK;
			}
		}

	return OVI_FrameNone;
	}




unsigned char *	MTD::GetLocalBufer()
	{
	return m_MetaDataBuff;
	}


//
//  Распарсим зоны
//
int	MTD::ParserZone(unsigned char *Zones, int Size, MovingTarget2 **MT)
	{
	DWORD CountZone = Size / sizeof(MovingTarget2);
	*MT = (MovingTarget2 *)Zones;
	return CountZone;
	}








//
// Перечитаем данные
//
int MTD::Refresh(int Count)
	{
	if (m_hFile == nullptr)	return OVI_NotOpen;

	if (m_Mod)				return S_OK;

	return S_OK;
	}






//
//   Переводим ошибку
//
char * MTD::Errors(int Cod)
	{
	return "";
	}


//
//   Востановим файл
//
int	MTD::Recovery(LPCWSTR FileName)
	{
			
	return 0;
	}








//
//  Запишем заголовок файла
//
int MTD::WriteHeader(bool Flag)
	{
	DWORD Size, Pos;

	//delete m_VideoIndexs;

	// Перейдем в начало файла
	if (Flag) Pos = SetFilePointer(m_hFile, 0L, nullptr, FILE_CURRENT);

	// Перейдем в начало файла
	Size = SetFilePointer(m_hFile, 0L, nullptr, FILE_BEGIN);
	if (Size != 0) return GetLastError();

	// Создадим контрольную сумму заголовка
	m_H_MTD.CrcHeader = Crc8((unsigned char *)&m_H_MTD, sizeof(Header_MTD) - 1, 0x25);

	// Запишем заголовок

	Header_MTD ss;
	memcpy(&ss, &m_H_MTD, sizeof(Header_MTD));

	
	if (MyWrite((void *)&ss, sizeof(Header_MTD), 0))
		{
		return GetLastError();
		}

	if (Flag) Size = SetFilePointer(m_hFile, Pos, nullptr, FILE_BEGIN);

	return S_OK;
	}


//
//  Прочитаем заголовок файла
//
int  MTD::ReadHeader(Header_MTD *HD)
	{
	DWORD Size;

	if (m_hFile == nullptr)		return OVI_NotOpen;

	if (HD == nullptr)				return OVI_InvalidParam;

	// Перейдем в начало файла
	Size = SetFilePointer(m_hFile, 0L, nullptr, FILE_BEGIN);
	if (Size != 0) return GetLastError();

	// Прочитаем Версию контейнера
	if (!ReadFile(m_hFile, &HD->Ver, sizeof(HD->Ver), &Size, nullptr))
		{
		return GetLastError();
		}

	// Прочтем заголовок
	if (!ReadFile(m_hFile, (Header_MTD *)&HD->VerOVSML, sizeof(Header_MTD) - sizeof(HD->Ver), &Size, nullptr))
		{
		return GetLastError();
		}

	if (Size != sizeof(m_H_MTD) - sizeof(HD->Ver)) return OVI_E_FAIL;

	// Проверим контрольную сумму заголовка
	unsigned char Crc = Crc8((unsigned char *)HD, sizeof(Header_MTD) - 1, 0x25);

	if (Crc != HD->CrcHeader) return OVI_CrcHeader_FAIL;

	return S_OK;
	}




//
//  Мое чтение
//
DWORD MTD::MyRead(unsigned char *Buff, DWORD SizeRead, DWORD Position)
{
	DWORD Pos;

	if (Position>0)
	{
		// Перейдем в позицию
		Pos = SetFilePointer(m_hFile, Position, nullptr, FILE_BEGIN);
		if (Pos != Position) return GetLastError();
	}

	// Прочтем
	if (!ReadFile(m_hFile, (void *)Buff, SizeRead, &Pos, nullptr))
	{
		return GetLastError();
	}

	if (Pos != SizeRead) return OVI_Err3;

	return S_OK;
}


//
//  Моя запись
//
DWORD MTD::MyWrite(void *Buff, DWORD SizeBuff, DWORD Position)
{
	DWORD Size;

	// Перейдем в начало файла
	if (Position>0)
	{
		Size = SetFilePointer(m_hFile, Position, nullptr, FILE_BEGIN);
		if (Size != 0) return GetLastError();
	}

	// Запишем блок
	if (!WriteFile(m_hFile, Buff, SizeBuff, &Size, nullptr))
	{
		return GetLastError();
	}

	if (Size != SizeBuff) return OVI_Err4;

	return S_OK;
}


//
//  Выдели память под основной индекс
//
int MTD::CreateBuff(unsigned char **Buff, int SizeAtom, DWORD SizeBuff, DWORD NewSize)
	{

	if (SizeBuff == 0)
		{
		// Просто выделим
		//*Buff= new unsigned char [NewSize*SizeAtom];
		*Buff = (unsigned char *)malloc(NewSize*SizeAtom);
		if (*Buff == nullptr) return OVI_NotAlloc;
		}
	else
		{
		// Типа расширем
		if (SizeBuff<NewSize)
			{
			// Выделим новый буфер
			//unsigned char *NewBuff= new unsigned char[NewSize*SizeAtom];
			unsigned char *NewBuff = (unsigned char *)malloc(NewSize*SizeAtom);
			if (NewBuff == nullptr) return OVI_NotAlloc;

			// Очистим
			memset((void *)NewBuff, 0, NewSize*SizeAtom);

			// Скопируем старый
			memcpy(NewBuff, *Buff, SizeBuff*SizeAtom);

			//delete [] *Buff;
			free((void *)*Buff);

			// Заменим буфер
			*Buff = NewBuff;

			}
		}
	return S_OK;
	}


int	MTD::Logs(void *Buff, DWORD Size)
	{
	if (m_hLog == nullptr)		return OVI_NotOpen;

	DWORD rSize;

	WriteFile(m_hLog, Buff, Size, &rSize, nullptr);

	WriteFile(m_hLog, "\n", 3, &rSize, nullptr);

	return S_OK;
	}



//
//  Зашифруем
//
int MTD::Encrypt(unsigned char *Buff, DWORD Size)
{
	if (Size>128) Size = 128;

	//AES_STREAM->Crypt((unsigned long *)Buff, Size);

	long *ss = (long *)(Buff + 2);
	int   xx = (Size / 4) - 2;

	for (int i = 0; i<xx; i = i + 2)
		ss[i] = ~ss[i];  // немного запутаем

	return 0;
}


//
//  Расшифруем
//
int MTD::Decrypt(unsigned char *Buff, DWORD Size)
{
	if (Size>128) Size = 128;

	long *ss = (long *)(Buff + 2);
	int   xx = (Size / 4) - 2;

	for (int i = 0; i<xx; i = i + 2)
		ss[i] = ~ss[i];  // немного распутаем

	//AES_STREAM->Decrypt((unsigned long *)Buff, Size);

	return 0;
}

