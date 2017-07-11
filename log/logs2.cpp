#include <windows.h>

#include <WinIoCtl.h>

#include <time.h>

#include <string>

#include "logs2.h"

#include <stdio.h>

using namespace My_LOG;

void LOG::OpenOrCreateLog(wchar_t * Path,wchar_t *Name)
	{
	wchar_t Buff[MAX_PATH+16];

	SYSTEMTIME st;
	::GetLocalTime(&st);

	m_Day=st.wDay;

	int Flag;
	if(Path!=nullptr)
		{
		//  Попробуем создать нужную папку
		if(swprintf_s(Buff,255,L"%s%4d-%02d-%02d",Path,st.wYear,st.wMonth,m_Day)==0)
			{
			m_LOG=0;
			return;
			}

		//  Создадим папку
		if(!CreateDirectoryW(Buff,0))
			{
			if(GetLastError()!=ERROR_ALREADY_EXISTS)
				{
				m_LOG=0;
				return;
				}
			}

		// Как то его добавим к имени лог
		if(swprintf_s(Buff,255,L"%s%4d-%02d-%02d\\%s",Path,st.wYear,st.wMonth,m_Day,Name)==0)
			{
			m_LOG=0;
			return;
			}

		wmemcpy(m_Path,Path,wcslen(Path)+1);

		Flag=1;
		}
	else 
		{
		swprintf_s(Buff,255,L"%s",Name);
		m_Path[0]=0;
		Flag=2;
		}

	while(1)
		{
		m_LOG=CreateFileW(Buff,
					 GENERIC_READ | GENERIC_WRITE,
					 FILE_SHARE_READ | FILE_SHARE_WRITE,
					 nullptr,
					 OPEN_ALWAYS,
					 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
					 nullptr);


		// Проверим результат
		if(!m_LOG)
			{
			if(Flag==1)
				{
				swprintf_s(Buff,255,L"%s",Name);
				m_Path[0]=0;
				Flag=2;
				continue;
				}

			if(Flag==2)
				{
				int ret=GetLastError();
				m_LOG=0;
				//m_wLOG.clear();
				return;
				}
			}
		break;
		}
		
	// Установи сжатие файла
	USHORT Comp=COMPRESSION_FORMAT_DEFAULT;
	DWORD res;

	DeviceIoControl( m_LOG,					// handle to file or directory
                 FSCTL_SET_COMPRESSION,     // dwIoControlCode
                 &Comp,						// input buffer
                 sizeof(USHORT),			// size of input buffer
                 NULL,                      // lpOutBuffer
                 0,                         // nOutBufferSize
                 &res,						// number of bytes returned
                 NULL);						// OVERLAPPED structure

	// Сделаем разрыв
	DWORD s1s=SetFilePointer(m_LOG,0,nullptr,FILE_END);
	
	// Сделаем разрыв
	if(s1s>0)
		{
		DWORD ss;
		WriteFile(m_LOG,"\r\n",2,&ss,nullptr);
		}
	}


//
//   Откроем или созданим, когда нет его
//`
// Path = nullptr
// Path = ""
// Path = "полный путь\\"
// Nmae = nullptr  = log.log
// Nmae = имя с расширением
 LOG::LOG(wchar_t *Path,wchar_t *Name)
	{
	m_SizeLog	=1024*1024;  // 1M
	m_Level		=1;
	m_Console	=1;
	m_Rotate	=0;

	//Инициализируем критическую секцию
	InitializeCriticalSection(&LogCritical); 

	setlocale(LC_ALL,"Russian");

	if(Name!=nullptr)	swprintf_s(m_Name,64,L"%s",Name);
	else				swprintf_s(m_Name,64,L"%s",L"log.log");

	OpenOrCreateLog(Path,m_Name);
	
	};


//
//		Освободим все ...
//
LOG::~LOG()
	{
	if(m_LOG==nullptr) return;

	FlushFileBuffers(m_LOG);

	CloseHandle(m_LOG);
	
	}

//
//		Установим уровень логов
//
 int  LOG::SetParameter(int DebugLevel,int WriteToConsole)
	{

	m_Level=DebugLevel;

	m_Console=WriteToConsole;

	return 0;
	}



int LOG::Print(wchar_t *Mess)
	{
	// Типа выводить всегда
	return LOG::Print(0,Mess);
	}

//
//		Выведем сообщение 
//
int LOG::Print(int Level,wchar_t *Mess)
	{
	if( !m_LOG )	return -1;  // Не открыли

	if(Level<=m_Level)
		{
		if(Level>=0) Rotate();

		//Входим в критическую секцию
		EnterCriticalSection(&LogCritical);

		SYSTEMTIME st;
		::GetLocalTime(&st);

		// Сформируем время
		char Buff[1024];
		sprintf_s(Buff,"%4d-%02d-%02d %02d:%02d:%02d-%03d => %S\r\n",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds,Mess);

		DWORD ss;
		WriteFile(m_LOG,Buff,strlen(Buff),&ss,nullptr);

		FlushFileBuffers(m_LOG);

		if(m_Console) printf("%s",Buff);

		//Выходим из критической секции
		LeaveCriticalSection(&LogCritical);
		}

	return 0;
	}

//
//		Выведем сообщение для пользователя
//
int LOG::Print(int Level,int User,wchar_t *Mess)
	{
	if(!m_LOG)		return -1;  // Не открыли

	Rotate();

	if(Level<=m_Level)
		{
		//Входим в критическую секцию
		EnterCriticalSection(&LogCritical);

		SYSTEMTIME st;
		::GetLocalTime(&st);

		// Сформируем время
		char Buff[1024];
		sprintf_s(Buff,"%4d-%02d-%02d %02d:%02d:%02d-%03d =%d=> %S\r\n",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds,User,Mess);

		DWORD ss;
		WriteFile(m_LOG,Buff,strlen(Buff),&ss,nullptr);

		FlushFileBuffers(m_LOG);

		if(m_Console) printf("%s",Buff);

		//Выходим из критической секции
		LeaveCriticalSection(&LogCritical);
		}

	return 0;
	}


//
//		Ротацию логов
//
int LOG::Rotate()
	{
	if(!m_LOG) return 0;
	
	SYSTEMTIME st;
	::GetLocalTime(&st);

	if(m_Day!=st.wDay)
		{
		// Узнать путь к файлу
		Close();

		OpenOrCreateLog(m_Path,m_Name);

		return 0;
		}

	// Закроем



	// Переименуем




	// Откроем снова


	return 0;
	}

void LOG::Flush()
	{
	if(m_LOG==nullptr) return;

	FlushFileBuffers(m_LOG);
	}

//
//		Закроем лог
//
void LOG::Close()
	{
	if( m_LOG) 	
		{
		Print(-1,L"LOG is closed.");

		FlushFileBuffers(m_LOG);

		CloseHandle(m_LOG);
		}
	}
