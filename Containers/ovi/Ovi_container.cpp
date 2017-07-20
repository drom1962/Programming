#include <windows.h>

#include <stdio.h>

#include <wchar.h>


#include "..\common.h"

#include "..\container.h"

#include "..\ovi\Ovi_container.h"


using namespace Archive_space;

//===================================================================================================================================================
V_e_r OviVer2={3,0,0,0};

void OVI2::Version(V_e_r *V)
	{
    if(V!=nullptr) 
	                memcpy(V,&OviVer2,sizeof(V_e_r));
    }


OVI2::OVI2(int Ver_OVMSML)
		{
		// Заполним переменные обьекты
		m_hFile=nullptr;

		m_Flush = 0;
		c_Flush = 0;

		m_VideoBufferSize		= VIDEOBUFFERSIZE;
		m_MaxVideoIndex			=30*60*60;
		m_VideoIndexs			=nullptr;
		m_VideoFramesIntoBuffers=nullptr;
		m_FrameBuff				=nullptr;
		m_FrameBuffSize			=FRAMEBUFFERSIZE;
		
		m_AudioIndexs			=nullptr;
		m_AudioFramesIntoBuffers=nullptr;
		m_AudioBufferSize		=AUDIOBUFFERSIZE;
		m_MaxAudioIndex			=30*60*60;

		m_VerOVMSML				=Ver_OVMSML;

        m_average_frame_time	=1000000;

		AES_HEADER				= new AES(256);

		char ss[] = "ab32f8c395d0b07c7b612dcb56375e0383f85b05052b894c2f083f146c46f491";
		for (int i = 0; i < 8; i++)
			{
			sscanf_s(&ss[i * 8], "%8x", &m_CifKeyForHeader[i]);
			}

		AES_HEADER->Init(m_CifKeyForHeader);
		
		AES_STREAM = new AES(256);

		// Флаги
		m_Crypto = 0;
		m_WriteMetaData			= 0;
		};


OVI2::~OVI2()
		{
		if(m_VideoIndexs!=nullptr)		
			{
			// Уделим память под нидексы
			free((void *)m_VideoBuff);
			free((void *)m_VideoIndexs);
			free(m_FrameBuff);
			}
				
		//if(m_AudioIndexs!=nullptr)
		//	{
		//	// Уделим память под нидексы
		//	free((void *)m_AudioBuff);
		//	free((void *)m_AudioIndexs);
		//	}

		if(m_hFile!=nullptr)  CloseHandle(m_hFile);
		};

//
//  Проверим расширение
//
bool OVI2::CheckExtension(wchar_t * Ext)
	{	
    if (Ext == nullptr) return false;

	if(wmemcmp(Ext,L"OVI",3)==0) return true;

	return false;
	}

//
//  Проверим расширение
//
Archiv * OVI2::CreateConteiner()
	{	
	return new OVI2(1);
	}


//
//  Создадим файл
//
int		OVI2::Init(LPCWSTR FileName,FileInfo *FI)
	{
	return Create(FileName,FI);
	};


//
//  Создадим файл с паролем
//
int OVI2::InitEx(LPCWSTR FileName, FileInfo *FI, LPCWSTR Pass)
	{
	// Обработаем пароль



	return Create(FileName, FI);
	}


//
//  Создадим файл
//
int OVI2::Create(LPCWSTR FileName,FileInfo *FI)
		{
		if(m_hFile!=nullptr)    return OVI_File;

		if(FI==nullptr)         return OVI_File;

		if(FI->VideoCodec==0)   return OVI_File;

		m_hFile=CreateFileW(FileName,
						 GENERIC_READ | GENERIC_WRITE,
						 FILE_SHARE_READ | FILE_SHARE_WRITE,
						 nullptr,
						 CREATE_ALWAYS,
						 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
						 nullptr);

		// Проверим результат
		if(m_hFile==INVALID_HANDLE_VALUE) 
			{
			return GetLastError();
			}

		// Возьмем флажки
		m_Mod=1;							// Режим записи

		m_Flags = FI->Mod;

		m_H_OVI.Mod = m_Flags;				// Сохраним флаги 

		if (m_Flags & WriteMetaData)	m_WriteMetaData = 1;   // Пишем
			
		if (m_Flags & Crypto)
			{
			AES_STREAM->Init(m_CifKeyForHeader);
			m_Crypto = 1;			//h.264 Шифруем
			}

		m_Flush = FI->Mod  & CountFlush;	// Возьмем режим работы Flush
		c_Flush = 0;
				
		GetFileTime(m_hFile,&m_H_OVI.Time,nullptr,nullptr);
		
		// Почистим память под видео цепочку
		memset((void *)&m_VC,0,sizeof(VideoChank2));
		
		// Буфер под кадры на 2М
		
		m_VideoIndexs=nullptr;
		m_VideoFramesIntoBuffers=nullptr;

		// Выделим буфера под видео
		if(CreateBuff(&m_VideoBuff,1,0,m_VideoBufferSize))							return OVI_NotAlloc;
		
		if(CreateBuff(&m_VideoIndexs,sizeof(ElementVideoIndex2),0,m_MaxVideoIndex))	return OVI_NotAlloc;
		
		// Буфер под кадры на 2М
	
		m_AudioIndexs=nullptr;
		m_AudioFramesIntoBuffers=nullptr;
		// Выделим буфера под аудио
		
		if(FI->AudioCodec>0) 
			{
			if(CreateBuff(&m_AudioBuff,1,0,m_AudioBufferSize))							return OVI_NotAlloc;

			if(CreateBuff(&m_AudioIndexs,sizeof(ElementAudioIndex),0,m_MaxAudioIndex))	return OVI_NotAlloc;
			}
		else
			{
			// нет звука
			m_H_OVI.CountVideoFrame = 0;
			}
					
		m_FreeIndex=0;								// Самый первый индех
		m_VideoFramesIntoBuffers=m_VideoBuff;

		m_CurrentVideoFrame=0;						
		m_CountVideoFrameIntoChunk=0;

		m_SizeVideoFrames=0;

		m_H_OVI.MaxSizeVideoFrame=0;

		// Заполнис заголовок нулями
		memset((void *)&m_H_OVI,0,sizeof(Header_OVI));
		
		GetFileTime(m_hFile,&m_H_OVI.Time,nullptr,nullptr);

		m_H_OVI.Ver			= 2;

		m_H_OVI.CrcFile		= 0;
		m_H_OVI.CrcHeader	= 0;

        SetFileInfo(FI);

		return WriteHeader(false);
	
		}

//
//  Откроем на чтение наш контейнер
//
int OVI2::OpenEx(LPCWSTR FileName, FileInfo *FI, LPCWSTR Pass)
	{
	// Обработаем пароль


	return Open(FileName,FI);
	}


//
//  Откроем на чтение наш контейнер
//
int OVI2::Open(LPCWSTR FileName,FileInfo *FI)
		{
		if(m_hFile!=nullptr) return OVI_NotOpen;	
		
		m_hFile=CreateFileW(FileName,
						 GENERIC_READ,
						 FILE_SHARE_READ | FILE_SHARE_WRITE,
						 nullptr,
						 OPEN_EXISTING,
						 FILE_ATTRIBUTE_READONLY,
						 nullptr);

		// Проверим результат
		if(m_hFile==INVALID_HANDLE_VALUE) 
			{
			m_hFile=nullptr;
			return OVI_NotOpen;
			}

		m_SizeOviFile=GetFileSize(m_hFile,NULL);

		m_Mod=0;		// Режим чтения

		int i = sizeof(VideoChank2);
		// Почистим память под видео цепочку
		memset((void *)&m_VC,0,sizeof(VideoChank2));

		//  Читаем заголовок
		int ret=ReadHeader(&m_H_OVI);

		if(ret)	
			{
			CloseHandle(m_hFile);
			m_hFile=nullptr;
			return OVI_CrcHeader_FAIL;
			}

		if (m_H_OVI.Mod & Crypto)
			{
			AES_STREAM->Init(m_CifKeyForHeader);
			m_Crypto = 1;			// Все шифруем
			}


		GetFileTime(m_hFile,&m_H_OVI.Time,nullptr,nullptr);     // Временно

		if(FI!=nullptr) GetFileInfo(FI);

		// Создадим буфера
		if(CreateBuff(&m_VideoBuff,1,0,m_VideoBufferSize)) return OVI_NotAlloc;  // Для группового чтения

		if(CreateBuff(&m_FrameBuff,1,0,m_FrameBuffSize)) return OVI_NotAlloc;  // Для чтения кадра во внутренний буфер

		if(m_H_OVI.CountVideoFrame>0) m_MaxVideoIndex=m_H_OVI.CountVideoFrame;
		if(CreateBuff((unsigned char **)&m_VideoIndexs,sizeof(ElementVideoIndex2),0,m_MaxVideoIndex)) return OVI_NotAlloc;

		m_LastVideoRefresh=m_H_OVI.FirstVideoFrame;
		m_LastAudioRefresh=m_H_OVI.FirstAudioFrame;

		//Refresh();  // Востановим цепочку
		int ff=0;
		if(m_H_OVI.MainVideoIndex!=0L)
			{
			// Есть главных индекс для видео
			if(MyRead((unsigned char *)m_VideoIndexs,m_H_OVI.CountVideoFrame*sizeof(ElementVideoIndex2),m_H_OVI.MainVideoIndex)) return OVI_InvalidIndex; 
			}
		else 
			{
			m_CurrentVideoFrame=0;
			Refresh(0);  // Востановим цепочку
			ff++;
			}
				
		// Вычислим максимальный размер кадра
		m_MaxVideoFrame=0;
		for(DWORD i=0;i<m_H_OVI.CountVideoFrame;i++)
			{
			if(m_MaxVideoFrame<((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeFrame) m_MaxVideoFrame=((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeFrame;
			}
		
		//if(m_H_OVI.AudioCodec>0)
		//	{
		//	// Есть аудио поток
		//	if(m_H_OVI.CountAudioFrame>0) m_MaxAudioIndex=m_H_OVI.CountAudioFrame;

		//	if(CreateBuff((unsigned char **)&m_AudioIndexs,sizeof(ElementAudioIndex),0,m_MaxAudioIndex)) return OVI_NotAlloc;

		//	if(m_H_OVI.MainAudioIndex!=0L)
		//		{
		//		// Есть главных индекс для аудио
		//		if(MyRead((unsigned char *)m_VideoIndexs,m_H_OVI.CountVideoFrame*sizeof(ElementVideoIndex2),m_H_OVI.MainVideoIndex)) return OVI_InvalidIndex; 
		//		}
		//	else 
		//		{
		//		Refresh(0);	// Востановим цепочку
		//	    //ff++;
		//		}
		//	}

		m_CurrentVideoChunk=0;
		m_LastReadFrame=0;
		m_PosFrames=0;
	
		if(m_H_OVI.MaxSizeVideoFrame==0)
			{
			// Пощитаем максимальный размер кадра

			for(DWORD i=0;i<m_H_OVI.CountVideoFrame;i++)
				{
				if(m_H_OVI.MaxSizeVideoFrame<((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeFrame)
					
					m_H_OVI.MaxSizeVideoFrame=((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeFrame;
				}
			}


		// Есть индекс - почитаем
		if(ff>0) return OVI_NotClose;
				
		return S_OK;
		}

//
// Статус файла
//
int  OVI2::IsOpen()
	{
	if(m_hFile==nullptr)				return OVI_NotOpen;	

	return S_OK;
	}

//
// Проверим контрольные суммы
//
int  OVI2::CheckFile()
	{
	if(m_hFile==nullptr)				return OVI_NotOpen;	

	// Вычислим контрольную сумму файла

	return S_OK;
	}



//
// Размер кадра. Если -1, то берем размер текущего
//
DWORD OVI2::GetMaxVideoFrame()
	{
	return m_MaxVideoFrame;
	}



//
//  Закроем контейнер
//
int OVI2::Close(FileInfo *FI)
		{
		if(m_hFile==nullptr)				return OVI_NotOpen;	

		if(m_Mod==0)		
			{
			// Он на чтение и его закроем
			CloseHandle(m_hFile);
			m_hFile=nullptr;

			return S_OK;
			}

		if(m_CurrentVideoFrame==0)	return OVI_Err1;
		
		// Сбросим, что осталось в буфере
		WrireGroupVideoFrames();

		// Сбросим индексы
		DWORD Pos=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);

		if(MyWrite(m_VideoIndexs,sizeof(ElementVideoIndex2)*(m_CurrentVideoFrame),0))
			{
			CloseHandle(m_hFile);
			m_hFile=nullptr;

			return OVI_NotWrite;
			}

		// Запишем хедер
		m_H_OVI.MainVideoIndex=Pos;
		m_H_OVI.CountVideoFrame=m_CurrentVideoFrame;
		
		m_H_OVI.Duration =static_cast<UINT>(((ElementVideoIndex2 *)m_VideoIndexs)[m_CurrentVideoFrame-1].TimeFrame/m_average_frame_time);
		m_H_OVI.mDuration=static_cast<ULONG>(((ElementVideoIndex2 *)m_VideoIndexs)[m_CurrentVideoFrame-1].TimeFrame);
		
		// Вычислим GOP
		m_H_OVI.GOP = 1;
		for (; m_H_OVI.GOP < m_H_OVI.CountVideoFrame; m_H_OVI.GOP++)
			{
			if (((ElementVideoIndex2 *)m_VideoIndexs)[m_H_OVI.GOP].TypeFrame == 1) break;
			}

		WriteHeader(false);

		if(FI!=nullptr) GetFileInfo(FI);

		// Закроем файл
		CloseHandle(m_hFile);
		m_hFile=nullptr;

		return S_OK;
		}


//
//  Запишем ExtarData
//
int OVI2::SetExtraData(unsigned char *ExtraData,DWORD BuffSize)
		{
		//return S_OK;
		if(m_hFile==nullptr)						return  OVI_NotOpen;	

		// А надо это делвать ?
		if(BuffSize<=m_H_OVI.SizeExtraData)   return 999;  // Если она вмещается в старую

		if(m_H_OVI.SizeExtraData>0)			return 888;  // Уже  ее записали
		if(m_H_OVI.FirstVideoFrame>0)			return 888;  // Уже  ее записали
		if(m_H_OVI.FirstAudioFrame>0)			return 888;  // Уже  ее записали
		
		m_H_OVI.ExtraData=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);  // Зафиксируем начало ExtraData

		if(MyWrite(ExtraData,BuffSize,0))   return 999;

		m_H_OVI.SizeExtraData=BuffSize;
				
 		WriteHeader(true);

		Flush();

		return S_OK;
		}

//
//  Прочтем ExtraData
//
int OVI2::GetExtraData(unsigned char *ExtraData,DWORD BuffSize,DWORD *SizeExtraData)
		{
		if(m_hFile==nullptr)				return OVI_NotOpen;	

		if(m_H_OVI.ExtraData==0)			return OVI_ExtraNone;

		if(BuffSize<m_H_OVI.SizeExtraData)	return 999;
						
		if(SizeExtraData!=nullptr)			*SizeExtraData=m_H_OVI.SizeExtraData;

		if(ExtraData==nullptr)				return S_OK;
		
		return MyRead(ExtraData,m_H_OVI.SizeExtraData,m_H_OVI.ExtraData);
		}


//
//  Запишем заголовок файла
//
int OVI2::WriteHeader(bool Flag)
		{
		DWORD Size,Pos;

		//delete m_VideoIndexs;

		// Перейдем в начало файла
		if(Flag) Pos=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);

		// Перейдем в начало файла
		Size=SetFilePointer(m_hFile,0L,nullptr,FILE_BEGIN);
		if(Size!=0) return GetLastError();

		// Создадим контрольную сумму заголовка
		m_H_OVI.CrcHeader=Crc8((unsigned char *)&m_H_OVI,sizeof(Header_OVI)-1,0x25);
						
		// Запишем заголовок

		Header_OVI ss;
		memcpy(&ss, &m_H_OVI, sizeof(Header_OVI));

		AES_HEADER->Crypt((unsigned long *)&ss, sizeof(m_H_OVI));

		if(MyWrite((void *)&ss,sizeof(Header_OVI),0))
			{
			return GetLastError();
			}

		if(Flag) Size=SetFilePointer(m_hFile,Pos,nullptr,FILE_BEGIN);

		return S_OK; 
		}


//
//  Прочитаем заголовок файла
//
int  OVI2::ReadHeader(Header_OVI *HD)
		{
		DWORD Size;

		if(m_hFile==nullptr)		return OVI_NotOpen;	

		if(HD==nullptr)				return OVI_InvalidParam;

		AES_HEADER->Debug();

		// Перейдем в начало файла
		Size=SetFilePointer(m_hFile,0L,nullptr,FILE_BEGIN);
		if(Size!=0) return GetLastError();

		// Прочитаем Версию контейнера
		if(!ReadFile(m_hFile,&HD->Ver,sizeof(HD->Ver),&Size,nullptr))
			{
			return GetLastError();
			}

		// Прочтем заголовок
		if(!ReadFile(m_hFile,(Header_OVI *)&HD->VerOVSML,sizeof(Header_OVI)-sizeof(HD->Ver),&Size,nullptr))
			{
			return GetLastError();
			}

		if(Size!=sizeof(m_H_OVI)-sizeof(HD->Ver)) return OVI_E_FAIL;

		AES_HEADER->Decrypt((unsigned long *)HD, sizeof(Header_OVI));
		
		// Проверим контрольную сумму заголовка
		unsigned char Crc=Crc8((unsigned char *)HD,sizeof(Header_OVI)-1,0x25);
		
		if(Crc != HD->CrcHeader) return OVI_CrcHeader_FAIL;

		return S_OK; 
		}


//
//  Запишем видео кадр в буфер + пользовательские данные
//
int OVI2::WriteVideoFrame(unsigned char  *Frame,DWORD SizeFrame,int KeyFlag,uint64_t Time,unsigned char  *UserData,DWORD SizeUserData)
		{
		if(m_hFile==nullptr)		return OVI_NotOpen;
        
        if(m_Mod==0)	            return OVI_ReadOnly;
		
		ElementVideoIndex2 *EVI2;

		//if( !(((FrameInfoHeader *)Frame)->mType & mtVideoFrame) )	return E_FAIL;  // не видео фрагмент

		// Другой критерий разбиения на группы - по 1М
		if(m_CountVideoFrameIntoChunk==MAXFRAMESINTOCHANC || m_SizeVideoFrames>MAXSIZEBLOCK)
			{
			int res=WrireGroupVideoFrames();
			if(res) return res;
			}

        if(UserData==nullptr) SizeUserData=0;

        DWORD TotalSizeFrame=SizeFrame+SizeUserData;

		//  Вычислим максимальный размер кадра
		if(m_H_OVI.MaxSizeVideoFrame<TotalSizeFrame) m_H_OVI.MaxSizeVideoFrame=TotalSizeFrame;

        if (m_CurrentVideoFrame == m_MaxVideoIndex)
            {
            CreateBuff(&m_VideoIndexs, sizeof(ElementVideoIndex2), m_MaxVideoIndex, m_MaxVideoIndex + 1024);
            m_MaxVideoIndex += 1024;
            }

		//  Заполнили локальный индекс
		EVI2 = &m_VC.IndexFrame[m_CountVideoFrameIntoChunk];
		EVI2->NumberIndex  = m_CurrentVideoFrame;
		EVI2->TypeFrame		= KeyFlag;
		EVI2->Position		= m_SizeVideoFrames;
		EVI2->SizeFrame		= SizeFrame;
		EVI2->SizeUserData	= SizeUserData;
		EVI2->TimeFrame		= Time;

		//  Заполнили глобальный индекс
		EVI2 = &((ElementVideoIndex2 *)m_VideoIndexs)[m_CurrentVideoFrame];
		EVI2->NumberIndex   = m_CurrentVideoFrame;
		EVI2->TypeFrame		= KeyFlag;
		EVI2->Position		= m_SizeVideoFrames;
		EVI2->SizeFrame		= SizeFrame;
		EVI2->SizeUserData	= SizeUserData;
		EVI2->TimeFrame		= Time;

		// Сохраним кадр в буфере
		if(m_VideoBufferSize < m_SizeVideoFrames+TotalSizeFrame)
			{
			DWORD xx=m_SizeVideoFrames+TotalSizeFrame;

			CreateBuff(&m_VideoBuff,1,m_VideoBufferSize,xx);

			m_VideoBufferSize=xx;

			m_VideoFramesIntoBuffers=m_VideoBuff+m_SizeVideoFrames;

			}

        // Запишем сам кадр
		memmove(m_VideoFramesIntoBuffers,(const void *)(Frame),m_VC.IndexFrame[m_CountVideoFrameIntoChunk].SizeFrame);
		int ret;
		if(m_Crypto && KeyFlag)
				ret=Encrypt(m_VideoFramesIntoBuffers, SizeFrame);
		
        // Запишем пользовательские данные
		if(m_WriteMetaData)
			{	
			if (UserData != nullptr)
				{
				memmove(m_VideoFramesIntoBuffers + SizeFrame, (const void *)UserData, SizeUserData);
				}
			}

		// Передвинем индекс и указатель
		Frame+=sizeof(ElementVideoIndex2);

		m_VideoFramesIntoBuffers+=TotalSizeFrame;

		m_SizeVideoFrames       +=TotalSizeFrame;

		m_CurrentVideoFrame++;

		m_CountVideoFrameIntoChunk++;

		return S_OK;
		}


//
// Прочитаем группу кадров
//
int OVI2::ReadGroupVideoFrame(DWORD VideoChunk)  
		{
        VideoChank2			m_VC;

		if(m_hFile==nullptr)			return OVI_NotOpen;

		if(VideoChunk==0)				return OVI_NotOpen;

		// Возьмем заголовок чанка
		if(MyRead((unsigned char *)&m_VC,8,VideoChunk)!=S_OK) return -1;

		// Прочитаем локальные индексы
		if(MyRead((unsigned char *)&m_VC.IndexFrame,sizeof(ElementVideoIndex2) * m_VC.CountFrameIntoChank,0)!=S_OK) return -2;

		// Пощитаем размер всех кадров
		DWORD Size=0;
        
    	for(DWORD i=0;i<m_VC.CountFrameIntoChank;i++)
			{
            Size+=(m_VC.IndexFrame[i].SizeFrame+m_VC.IndexFrame[i].SizeUserData);
			}
       
		if(m_VideoBufferSize<Size) 
			{
			// Увеличим буфер если надо
			CreateBuff(&m_VideoBuff,1,m_VideoBufferSize,Size);
			m_VideoBufferSize=Size;
			}

		// Запомним начало кадров
		m_PosFrames=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);

		// Прочтем какдры разом
		if(MyRead(m_VideoBuff,Size,0)!=S_OK) return -3;
				
		return S_OK;
		}


//
// Возвращает прочитаную длину. Если 0 то кадр не поместился в буфер
//
int OVI2::ReadVideoFrame(long IndexFrame,unsigned char *BuffFrame,DWORD BuffSize,VideoFrameInfo *VFI)  
		{
		if(m_hFile==nullptr)			return OVI_NotOpen;

		if(IndexFrame<0)  IndexFrame=m_LastReadFrame;

		ElementVideoIndex2 *EVI2;

		EVI2 = &((ElementVideoIndex2 *)m_VideoIndexs)[IndexFrame];

        DWORD TotalSizeFrame=EVI2->SizeFrame + EVI2->SizeUserData;

		if(BuffFrame!=nullptr && BuffSize<=TotalSizeFrame) return 555;  // кода пока нет

		if(EVI2->VideoChank!=m_CurrentVideoChunk)
			{
			// Прочтем его группу
			ReadGroupVideoFrame(EVI2->VideoChank);
			
			// Сохраним ее позицию
			m_CurrentVideoChunk= EVI2->VideoChank;

			}
		
		DWORD Pos= EVI2->Position-m_PosFrames;

		if(BuffFrame!=nullptr)
			{
			// Возьмем  кадр из буфера кадров
			memcpy(BuffFrame,m_VideoBuff+Pos,TotalSizeFrame);
			if (m_Crypto && EVI2->TypeFrame)
						Decrypt(BuffFrame, EVI2->SizeFrame);
			}
		else
			{
			// Скопируем во внутренний буфер
			if(m_FrameBuffSize<TotalSizeFrame) 
				{
				// Расширим буфер НА 120 ПРОЦЕНТОВ
				DWORD xx=static_cast<DWORD>(TotalSizeFrame*1.2);

				if(CreateBuff(&m_FrameBuff,1,m_FrameBuffSize,xx)) return OVI_NotAlloc;  // Для чтения кадра во внутренний буфер

				m_FrameBuffSize=xx;
				}

			memcpy(m_FrameBuff,m_VideoBuff+Pos,TotalSizeFrame); 

			if (m_Crypto && EVI2->TypeFrame)
						Decrypt(m_FrameBuff, EVI2->SizeFrame);
			}

		if(VFI!=nullptr)
			{
			// Вернем информацию о фрейме
			VFI->Codec       = m_H_OVI.VideoCodec;
			VFI->TypeFrame   = EVI2->TypeFrame;
			VFI->SizeFrame   = EVI2->SizeFrame;
            VFI->SizeUserData= EVI2->SizeUserData;
			VFI->TimeFrame = EVI2->TimeFrame; // 10;  // Делим для сервера, а зачем не помню
            VFI->Data        = m_FrameBuff;
			}

		return S_OK;
		}


//
// Прочитаем следующий ключевой кадр
//
int OVI2::ReadNextKeyVideoFrame(DWORD IndexFrame,unsigned char *BuffFrame,DWORD BuffSize,VideoFrameInfo *FI)
		{
		if(m_hFile==nullptr)		return OVI_NotOpen;

		if(BuffFrame==nullptr)		return OVI_InvalidBuffer;

		for(DWORD i=IndexFrame;i<m_H_OVI.CountVideoFrame;i++)
			{
			if(((ElementVideoIndex2 *)m_VideoIndexs)[i].TypeFrame) // Ключевой ?
				{
				// Да
				if(FI!=nullptr)
					{
					// Вернем информацию о фрейме
					FI->TypeFrame       =((ElementVideoIndex2 *)m_VideoIndexs)[i].TypeFrame;
					FI->SizeFrame       =((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeFrame;
                    FI->SizeUserData    =((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeUserData;
					FI->TimeFrame       =((ElementVideoIndex2 *)m_VideoIndexs)[i].TimeFrame/10;
					}

				return ReadVideoFrame(i,BuffFrame,BuffSize,FI);
				}
			}

		return OVI_FrameNone;
		}


//
//  Внутренняя для чтения кадра из текущей позиции
//
int OVI2::Read_Video_Frame(int Index,unsigned char *BuffFrame,DWORD BuffSize)
	{
	if(BuffFrame!=nullptr) 	
		{
		BuffSize=((ElementVideoIndex2 *)m_VideoIndexs)[Index].SizeFrame;
		BuffFrame=m_FrameBuff;

		if(m_FrameBuffSize<BuffSize) 
			{
			// Расширим буфер
			if(CreateBuff(&m_FrameBuff,1,m_FrameBuffSize,BuffSize)) return OVI_NotAlloc;  // Для чтения кадра во внутренний буфер
			}
		}
	else 
		{
		if(BuffSize<((ElementVideoIndex2 *)m_VideoIndexs)[Index].SizeFrame) return OVI_Err2;
		}
	
	return ReadVideoFrame(Index,BuffFrame,BuffSize,nullptr);

	}


//
// Размер кадра. Если -1, то берем размер текущего
//
int OVI2::SizeVideoFrame(long IndexFrame)
	{
	if(IndexFrame==-1) IndexFrame=m_LastReadFrame;
	if(IndexFrame==-2) IndexFrame=m_LastReadFrame+1;

	return ((ElementVideoIndex2 *)m_VideoIndexs)[IndexFrame].SizeFrame;  //999
	}


//
// Прочитаем следующий кадр
//
int OVI2::ReadNextVideoFrame(unsigned char *BuffFrame,DWORD BuffSize,VideoFrameInfo *FI)
		{
		if(m_hFile==nullptr)		return OVI_NotOpen;

		if((m_LastReadFrame+1)==(long)m_H_OVI.CountVideoFrame) 
			{
			if(m_H_OVI.MainVideoIndex==0) 	
				{
				//ov_util::TRACE("Refreh");
				Refresh(5);  // Файл еще пишется
				}
			if((long)m_H_OVI.CountVideoFrame==(m_LastReadFrame+1))  return OVI_InvalidIndex; 
			}

		// Перейдем на следующий
		m_LastReadFrame++;

		// Читаем кадр
		return ReadVideoFrame(m_LastReadFrame,BuffFrame,BuffSize,FI);
		}

//
// Прочитаем информацию о кадре
//
int OVI2::GetInfoVideoFrame(DWORD IndexFrame,VideoFrameInfo *VFI)
	{
	if(m_hFile==nullptr) return OVI_NotOpen;
	
	if (IndexFrame<0 || IndexFrame>m_H_OVI.CountVideoFrame)  return -1;

	if (VFI != nullptr)
		{
		// Вернем информацию о фрейме
		VFI->Codec			= m_H_OVI.VideoCodec;
		VFI->TypeFrame		= ((ElementVideoIndex2 *)m_VideoIndexs)[IndexFrame].TypeFrame;
		VFI->SizeFrame		= ((ElementVideoIndex2 *)m_VideoIndexs)[IndexFrame].SizeFrame;
		VFI->Data			= nullptr;
		VFI->SizeUserData	= ((ElementVideoIndex2 *)m_VideoIndexs)[IndexFrame].SizeUserData;
		VFI->TimeFrame		= ((ElementVideoIndex2 *)m_VideoIndexs)[IndexFrame].TimeFrame / 10;
		}

	return S_OK;
	}


//
// Прочитаем информацию о кодеке
//
int OVI2::GetVideoCodec()
	{
	if(m_hFile==nullptr) return OVI_NotOpen;

	return m_H_OVI.VideoCodec;
	}


//
// Переместим кадр
//
int OVI2::SetCurrentFrame(long Index)
	{
	if(m_hFile==nullptr)	return OVI_NotOpen;

	if(m_Mod==1)			return OVI_ReadOnly;
	// ... m_Mod=0

	if(Index<-2)			return OVI_InvalidParam;

	if(Index==-1)			m_LastReadFrame=0;

	Refresh(5);

	if(Index==-2)			m_LastReadFrame=m_CurrentVideoFrame;
	else					m_LastReadFrame=Index;

	return S_OK;
	}


//
// Прочтем номер текущего кадра
//
long OVI2::GetCurrentFrame()
	{
	if(m_hFile==nullptr)	return OVI_NotOpen;

	return m_LastReadFrame;
	}




//
// Прочитаем предыдущий ключевой кадр
//
int OVI2::ReadPreviosKeyFrame(DWORD IndexFrame,unsigned char *BuffFrame,DWORD BuffSize,VideoFrameInfo *FI)
		{
		if(m_hFile==nullptr) return OVI_NotOpen;

		if(BuffFrame==nullptr)	return OVI_InvalidBuffer;

		for(DWORD i=IndexFrame;i>0;i--)
			{
			if(((ElementVideoIndex2 *)m_VideoIndexs)[i].TypeFrame)
				{
				if(FI!=nullptr)
					{
					// Вернем информацию о фрейме
					FI->TypeFrame       =((ElementVideoIndex2 *)m_VideoIndexs)[i].TypeFrame;
					FI->SizeFrame       =((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeFrame;
                    FI->SizeUserData    =((ElementVideoIndex2 *)m_VideoIndexs)[i].SizeUserData;
					FI->TimeFrame       =((ElementVideoIndex2 *)m_VideoIndexs)[i].TimeFrame/10;
					}

				// Нашли следующий ключевой кадр
				return ReadVideoFrame(i,BuffFrame,BuffSize,FI);
				}
			}

		return OVI_FrameNone;
		}



//
//  Поищем видео фрейм по времени
//
int OVI2::SeekVideoFrameByTime(uint64_t Time,DWORD *IndexFrame)
		{
		if(m_hFile==nullptr) return OVI_NotOpen;

		if(m_H_OVI.MainVideoIndex==0) Refresh(5);


		DWORD	First=1,
				End= m_H_OVI.CountVideoFrame,
				ss;

		// Ускорим поиск метод деления плополам (примерно в два раза)
		ss = (End / 2)+1;
		do
			{
			if (Time  < ((ElementVideoIndex2 *)m_VideoIndexs)[ss].TimeFrame)
				{
				// Первая половина
				End = ss+1;
				}
			else
				{
				if (Time == ((ElementVideoIndex2 *)m_VideoIndexs)[ss].TimeFrame) 
					{
					*IndexFrame = ss;
					return S_OK;
					}
				// Втораяя половина
				First = ss+1;
				}

			ss = (End - First) / 2 + First;
			
			} while ((End - First) > 10);
		
		for(First--;First<End;First++)
			{
			if(Time <= ((ElementVideoIndex2 *)m_VideoIndexs)[First].TimeFrame)
				{
				// Нашли кадр с временем больше заданного
				if(IndexFrame!=nullptr) *IndexFrame= First;
				
				return S_OK;
				}
			}

		return OVI_FrameNone;
		}

//
//  Поищем предыдущий ключевой
//
long OVI2::SeekPreviosKeyVideoFrame(long IndexFrame)
	{
	if(m_hFile==nullptr)	return OVI_NotOpen;

	if(m_Mod==1)			return OVI_NotOpen;  // Работает только для открытых файлов    
	// ... m_Mod=0

	if(IndexFrame<0)		IndexFrame=m_LastReadFrame;

	for(;IndexFrame>=0;IndexFrame--)
		{
		if(((ElementVideoIndex2 *)m_VideoIndexs)[IndexFrame].TypeFrame==1) // Ключевой ?
			{
			return IndexFrame;
			}
		}

	return IndexFrame;
	}


//
//  Поищем следующий ключевой
//
long OVI2::SeekNextKeyVideoFrame(long IndexFrame)
	{
	DWORD Index;

	if(m_hFile==nullptr)	return OVI_NotOpen;

	if(m_Mod==1)			return OVI_NotOpen;  // Работает только для открытых файлов    
	// ... m_Mod=0

	if(IndexFrame<0)		Index=m_LastReadFrame+1;
	else					Index=IndexFrame+1;

    for(;Index<m_H_OVI.CountVideoFrame;Index++)
		{
		if(((ElementVideoIndex2 *)m_VideoIndexs)[Index].TypeFrame==1) // Ключевой ?
			{
			if(IndexFrame<0) m_LastReadFrame=Index-1;
			return Index;
			}
		}

	return Index;
	}


//
//  Запишем аудио фрейм в буфер
//
int OVI2::PutAudioFrameIntoBuff(unsigned char  *Frame)
		{
		if(m_hFile==nullptr) return OVI_NotOpen;

		return OVI_S_OK;
		}


//
//  Прочтем аудио фрейм 
//
int OVI2::ReadAudioFrame(DWORD IndexFrame,unsigned char *BuffFrame,DWORD BuffSize)
		{
		if(m_hFile==nullptr) return OVI_NotOpen;

		return OVI_S_OK;
		}


//
//  Поищем аудио фрейм по времени
//
int OVI2::SeekAudioFrameByTime(uint64_t Time,DWORD *InxedFrame)
		{
		if(m_hFile==nullptr) return OVI_NotOpen;

		return OVI_S_OK;
		}



//
//  Запишем группу аудио фреймов на диск
//
int OVI2::WrireGroupAudioFrames()
		{
		if(m_hFile==nullptr) return OVI_NotOpen;

		return OVI_S_OK;
		}

//
//  Запишем основнай данные о файле из заголовка
//
int OVI2::SetFileInfo(FileInfo *FileInfo)
		{
		if(m_hFile==nullptr) return OVI_NotOpen;

		if(FileInfo!=nullptr)
			{
			m_H_OVI.Mod				=FileInfo->Mod;

			// Передадим важные данные о файле
			m_H_OVI.VerOVSML		=FileInfo->VerOVSML;
            m_H_OVI.VideoCodec		=FileInfo->VideoCodec;
			m_H_OVI.Width			=FileInfo->Width;
			m_H_OVI.Height			=FileInfo->Height;
			m_H_OVI.VideoBitRate	=FileInfo->VideoBitRate;
			m_H_OVI.FPS				=FileInfo->FPS;

			m_H_OVI.Time			=FileInfo->Time;

			m_H_OVI.GOP				=FileInfo->GOP;
			m_H_OVI.Duration		=FileInfo->Duration;
			m_H_OVI.CountVideoFrame	=FileInfo->CountVideoFrame;

			m_H_OVI.AudioCodec		=FileInfo->AudioCodec;
			m_H_OVI.BitsPerSample	=FileInfo->AudioCodec;
			m_H_OVI.SamplesPerSec	=FileInfo->SamplesPerSec;
			m_H_OVI.CountAudioFrame	=FileInfo->CountAudioFrame;

			return WriteHeader(true);
			}

		return S_OK;
		}

//
//  Прочитаем основнай данные о файле из заголовка по имени файла
//
int OVI2::GetFileInfo2(LPCWSTR FileName,FileInfo *FileInfo)
	{
	OVI2 *Ov= new OVI2(1);

	int ret=Ov->Open(FileName,FileInfo);
	if(ret!=S_OK) return ret;

	Ov->Close(nullptr);

	return S_OK;
	}


//
//  Прочитаем основнай данные о файле из заголовка
//
int OVI2::GetFileInfo(FileInfo *FileInfo)
		{
		if(m_hFile==nullptr) return OVI_NotOpen;

		if(FileInfo!=nullptr)
			{
			FileInfo->Mod				=m_Mod;

			FileInfo->Time				=m_H_OVI.Time;

			// Передадим важные данные о файле
			FileInfo->Ver				=m_H_OVI.Ver;
			FileInfo->VerOVSML			=m_H_OVI.VerOVSML;

			FileInfo->VideoCodec		=m_H_OVI.VideoCodec;
			FileInfo->Width				=m_H_OVI.Width;
			FileInfo->Height			=m_H_OVI.Height;
			FileInfo->VideoBitRate		=m_H_OVI.VideoBitRate;
			FileInfo->FPS				=m_H_OVI.FPS;

			FileInfo->GOP				=m_H_OVI.GOP;
			FileInfo->Duration			=m_H_OVI.Duration;
			FileInfo->CountVideoFrame	=m_H_OVI.CountVideoFrame;

			FileInfo->AudioCodec		=m_H_OVI.AudioCodec;
			FileInfo->BitsPerSample		=m_H_OVI.BitsPerSample;
			FileInfo->SamplesPerSec		=m_H_OVI.SamplesPerSec;
			FileInfo->CountAudioFrame	=m_H_OVI.CountAudioFrame;

			FileInfo->SizeOviFile		=m_SizeOviFile;
			}

		return S_OK;
		}

//
//  Пишем группу кадров.
//
int OVI2::WrireGroupVideoFrames()
		{
		int ret;

		if(m_hFile==nullptr) return OVI_NotOpen;

		if(m_Mod==0)		return OVI_ReadOnly;

		if(m_CountVideoFrameIntoChunk==0) return S_OK;

		// Вычислим размер заголовка перед группой
		int SizeHeaderChunc=((char *)&m_VC.IndexFrame[m_CountVideoFrameIntoChunk])-(char *)&m_VC;

		// Запишем Next + Count
		m_VC.CountFrameIntoChank=m_CountVideoFrameIntoChunk;

		DWORD Pos=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);  // Текущая позиция

		if(m_H_OVI.FirstVideoFrame==0) 
			{
			m_H_OVI.FirstVideoFrame=sizeof(m_H_OVI)+m_H_OVI.SizeExtraData;
			WriteHeader(true);
			}

		m_VC.NextChank=Pos+SizeHeaderChunc+m_SizeVideoFrames;
		m_VideoLastNext=Pos;			// Если добавили другой блок, то нужно скорректировать

		// Добавим длину заголовка
		Pos+=SizeHeaderChunc;
		
		DWORD VideoChank=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);  // Текущая позиция

		for(DWORD i=0;i<m_CountVideoFrameIntoChunk;i++)
			{
			m_VC.IndexFrame[i].Position+=Pos;

			((ElementVideoIndex2 *)m_VideoIndexs)[m_CurrentVideoFrame-m_CountVideoFrameIntoChunk+i].Position+=Pos;

			m_VC.IndexFrame[i].VideoChank=VideoChank;
			((ElementVideoIndex2 *)m_VideoIndexs)[m_CurrentVideoFrame-m_CountVideoFrameIntoChunk+i].VideoChank=VideoChank;
			}

		// Запишем индексы
		ret=MyWrite((void *)&m_VC,SizeHeaderChunc,0);
		
		// Запишем кадры
		ret=MyWrite(m_VideoBuff,m_SizeVideoFrames,0);

		// Запишем конец цепочки и перейдем на ее начало
		memset(&m_VC,0,sizeof(VideoChank));
		ret=MyWrite((void *)&m_VC,8,0);
		Pos=SetFilePointer(m_hFile,-8L,nullptr,FILE_CURRENT);  // Переместимстились в начало записанного

		m_H_OVI.Duration =static_cast<DWORD>(m_VC.IndexFrame[m_CountVideoFrameIntoChunk-1].TimeFrame/m_average_frame_time);
		m_H_OVI.mDuration=static_cast<ULONG>(m_VC.IndexFrame[m_CountVideoFrameIntoChunk-1].TimeFrame);

		// Сбросим переменные
		m_VideoFramesIntoBuffers=m_VideoBuff;
		m_CountVideoFrameIntoChunk=0;
		m_SizeVideoFrames=0;
		
		Flush();
		return S_OK;
		}




//
// Сбросим буфера на диск
//
int OVI2::Flush()  
		{
		if(m_hFile==nullptr)	return OVI_NotOpen;

		if(!m_Mod)				return S_OK;

		m_H_OVI.CountVideoFrame=m_CurrentVideoFrame;

		c_Flush++;

		// Уровень надежности
		if (c_Flush==m_Flush) 
			{
			//WriteHeader(true);

			FlushFileBuffers(m_hFile); 
			c_Flush = 0;
			}

		return S_OK;
		}


//
// Перечитаем данные
//
int OVI2::Refresh(int Count)  
		{
		if(m_hFile==nullptr)	return OVI_NotOpen;

		if(m_Mod)				return S_OK;

		VideoChank2 *p;
		
		p=&m_VC;

		//m_H_OVI.MainVideoIndex=0L;
		//m_CurrentVideoFrame=0;
		
		// Загрузим цепочки видео
		if(m_H_OVI.MainVideoIndex==0)
			{
			ReadHeader(&m_H_OVI);
			if(Count==0) Count=9999;

            //  Раскрутим цепочку
			while(Count>0)
				{
				Count--;

				int ret=MyRead((unsigned char  *)&m_VC,8,m_LastVideoRefresh);  
				if(ret!=0)
					{
					int i=0;
					}
				
				if(m_VC.CountFrameIntoChank>100)
					{
					int i=0;
					}

				if(m_VC.NextChank==0)
					{
					int i=0;
					// Зафиксируем последнюю проверку цепочки
					//m_H_OVI.FirstVideoFrame=Pos;
					break;
					}

				MyRead((unsigned char *)m_VC.IndexFrame,sizeof(ElementVideoIndex2)*m_VC.CountFrameIntoChank,0);
			
				// Проверим на максимальный индекс
				if(m_MaxVideoIndex<m_CurrentVideoFrame+m_VC.CountFrameIntoChank)
					{
					// Нужно увеличить буфере
					int i=0;
					if(CreateBuff((unsigned char **)&m_VideoIndexs,sizeof(ElementVideoIndex2),m_MaxVideoIndex,m_MaxVideoIndex+1024)) return OVI_NotAlloc;
					m_MaxVideoIndex+=1024;
					}


				// Запишем в глобальный индекс
				ElementVideoIndex2 *EVI2;

				EVI2 = ((ElementVideoIndex2 *)m_VideoIndexs);

				for(DWORD i=0;i<m_VC.CountFrameIntoChank;i++) 
					{
					EVI2->NumberIndex	=m_CurrentVideoFrame;
					EVI2->TypeFrame		=m_VC.IndexFrame[i].TypeFrame;
					EVI2->TimeFrame		=m_VC.IndexFrame[i].TimeFrame;
					EVI2->Position		=m_VC.IndexFrame[i].Position;
					EVI2->SizeFrame		=m_VC.IndexFrame[i].SizeFrame;
					EVI2->VideoChank	=m_VC.IndexFrame[i].VideoChank;

					m_CurrentVideoFrame++;
					
					if(m_CurrentVideoFrame==m_MaxVideoIndex)
						{
						// Увеличим память под индексы
						CreateBuff(&m_VideoIndexs,sizeof(ElementVideoIndex2),m_MaxVideoIndex,m_MaxVideoIndex+1024);
						m_MaxVideoIndex+=1024;
						}

					EVI2++;
					}

				m_LastVideoRefresh=m_VC.NextChank;
				memset(&m_VC,0,sizeof(VideoChank2));  // Лишнее
				}
			m_H_OVI.CountVideoFrame=m_CurrentVideoFrame;

			return OVI_NextRefresh;
			}


		// Загрузим цепочки аудио
		if(m_H_OVI.MainAudioIndex==0)
			{
			// Пока нечего грузить

			}

		return OVI_S_OK;
			
		}
//
//  Мое чтение
//
DWORD OVI2::MyRead(unsigned char *Buff,DWORD SizeRead,DWORD Position)
		{
		DWORD Pos;
		
		if(Position>0)
			{
			// Перейдем в позицию
			Pos=SetFilePointer(m_hFile,Position,nullptr,FILE_BEGIN);
			if(Pos!=Position) return GetLastError();
			}

		// Прочтем
		if(!ReadFile(m_hFile,(void *)Buff,SizeRead,&Pos,nullptr))
			{
			return GetLastError();
			}

		if(Pos!=SizeRead) return OVI_Err3;

		return S_OK;
		}


//
//  Моя запись
//
DWORD OVI2::MyWrite(void *Buff,DWORD SizeBuff,DWORD Position)
		{
		DWORD Size;

		// Перейдем в начало файла
		if(Position>0)
			{
			Size=SetFilePointer(m_hFile,Position,nullptr,FILE_BEGIN);
			if(Size!=0) return GetLastError();
			}

		// Запишем блок
		if(!WriteFile(m_hFile,Buff,SizeBuff,&Size,nullptr))
			{
			return GetLastError();
			}

		if(Size!=SizeBuff) return OVI_Err4;

		return S_OK;
		}


//
//  Выдели память под основной индекс
//
int OVI2::CreateBuff(unsigned char **Buff,int SizeAtom,DWORD SizeBuff,DWORD NewSize)
		{

		if(SizeBuff==0)
			{
			// Просто выделим
			//*Buff= new unsigned char [NewSize*SizeAtom];
			*Buff=(	unsigned char *)malloc(NewSize*SizeAtom);
			if(*Buff==nullptr) return OVI_NotAlloc;
			}
		else
			{
			// Типа расширем
			if(SizeBuff<NewSize)
				{
				// Выделим новый буфер
				//unsigned char *NewBuff= new unsigned char[NewSize*SizeAtom];
				unsigned char *NewBuff= (unsigned char *)malloc(NewSize*SizeAtom);
				if(NewBuff==nullptr) return OVI_NotAlloc;

				// Очистим
				memset((void *)NewBuff,0,NewSize*SizeAtom);

				// Скопируем старый
				memcpy(NewBuff,*Buff,SizeBuff*SizeAtom);

				//delete [] *Buff;
				free((void *)*Buff);
				
				// Заменим буфер
				*Buff=NewBuff;
				
				}
			}
		return S_OK;
		}


int		OVI2::Logs(void *Buff,DWORD Size)
		{	
		if(m_hLog==nullptr)		return OVI_NotOpen;

		DWORD rSize;

		WriteFile(m_hLog,Buff,Size,&rSize,nullptr);
		
		WriteFile(m_hLog,"\n",3,&rSize,nullptr);

		return S_OK;
		}	




//
//  Вернем локальный буфер под кадры
//
unsigned char *	OVI2::GetLocalBufer()
	{
	return m_FrameBuff;
	}


//
//   Переводим ошибку
//
char * OVI2::Errors(int Cod)
		{
		if(Cod==OVI_NotOpen)		return "Файл не открыт";
				
		if(Cod==OVI_NotReadHeder)	return "Не смогли прочитать заголовок";
				
		if(Cod==OVI_NotClose)		return "Не смогли закрыть файл";

		return "None";
		}



//
//  Востановим файл. (не тестировал)
//
int OVI2::Recovery(LPCWSTR FileName)
	{
	if(m_hFile!=nullptr)		return OVI_NotOpen;

	// Откроем на запись
	m_hFile=CreateFileW(FileName,
						 GENERIC_READ | GENERIC_WRITE,
						 FILE_SHARE_READ | FILE_SHARE_WRITE,
						 nullptr,
						 CREATE_ALWAYS,
						 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
						 nullptr);

	// Проверим результат
	if(m_hFile==INVALID_HANDLE_VALUE) 
		{
		return OVI_Err4;
		}

	//  Читаем заголовок
	if(ReadHeader(&m_H_OVI)) return OVI_NotReadHeder;

	if(m_H_OVI.MainVideoIndex>0) return S_OK;

	// Почистим память под видео цепочку
	memset((void *)&m_VC,0,sizeof(VideoChank2));

	// Выделим буфер под индексы
	if(CreateBuff(&m_VideoIndexs,sizeof(ElementVideoIndex2),0,m_MaxVideoIndex))	return OVI_NotAlloc;

	//if(CreateBuff(&m_AudioIndexs,sizeof(ElementAudioIndex),0,m_MaxAudioIndex))	return OVI_NotAlloc;

	// Будем востанавливать главный индекс
	Refresh(0);

	// Сбросим индексы
	m_H_OVI.MainVideoIndex=SetFilePointer(m_hFile,0L,nullptr,FILE_CURRENT);

	if(MyWrite(m_VideoIndexs,sizeof(ElementVideoIndex2)*(m_CurrentVideoFrame),0))
		{
		return OVI_NotWrite;
		}

	// Запишем хедер
	m_H_OVI.CountVideoFrame=m_CurrentVideoFrame;
		
	m_H_OVI.Duration =static_cast<UINT>(((ElementVideoIndex2 *)m_VideoIndexs)[m_CurrentVideoFrame-1].TimeFrame/m_average_frame_time);
	m_H_OVI.mDuration=static_cast<ULONG>(((ElementVideoIndex2 *)m_VideoIndexs)[m_CurrentVideoFrame-1].TimeFrame);
		
	WriteHeader(false);

	return S_OK;
	}


//
//  Зашифруем
//
int OVI2::Encrypt(unsigned char *Buff,DWORD Size)
	{
	if(Size>128) Size = 128;

	AES_STREAM->Crypt((unsigned long *)Buff, Size);

	long *ss = (long *)(Buff + 2);
	int   xx = (Size / 4) - 2;

	for (int i = 0; i<xx; i=i + 2)
		ss[i] = ~ ss[i];  // немного запутаем
	
	return 0;
	}


//
//  Расшифруем
//
int OVI2::Decrypt(unsigned char *Buff,DWORD Size)
	{
	if (Size>128) Size = 128;

	long *ss = (long *)(Buff + 2);
	int   xx = (Size / 4) - 2;

	for(int i=0;i<xx;i=i+2)
	      ss[i] = ~ ss[i];  // немного распутаем
	
	AES_STREAM->Decrypt((unsigned long *)Buff, Size);

	return 0;
	}

//
//  Расшифруем
//
void OVI2::Debug(void)
{
}
