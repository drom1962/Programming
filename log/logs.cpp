
#include "stdafx.h"

#include <windows.h>

#include <time.h>

#include <string>

#include "logs.h"

#include <stdio.h>

using namespace My_LOG;

void LOG::OpenOrCreateLog(wchar_t * Path)
	{
	wchar_t Buff[MAX_PATH+16];

	SYSTEMTIME st;
	::GetLocalTime(&st);

	int Flag;
	if(Path!=nullptr)
		{
		//  ��������� ������� ������ �����
		swprintf_s(Buff,255,L"%s\\%4d-%02d-%02d",Path,st.wYear,st.wMonth,st.wDay);

		CreateDirectory(Buff,0);

		// ��� �� ��� ������� � ����� ���
		swprintf_s(Buff,255,L"%s\\%4d-%02d-%02d\\MediaServer.log",Path,st.wYear,st.wMonth,st.wDay);

		Flag=1;
		}
	else 
		{
		swprintf_s(Buff,255,L"MediaServer.log");
		m_Path[0]=0;
		Flag=2;
		}
	
	m_Day=st.wDay;

	while(1)
		{
		m_LOG=_wfsopen(Buff, L"a", _SH_DENYWR);

		// �������� ���������
		if(!m_LOG)
			{
			if(Flag==1)
				{
				swprintf_s(Buff,255,L"MediaServer.log");
				m_Path[0]=0;
				Flag=2;
				continue;
				}

			if(Flag==2)
				{
				int ret=GetLastError();
				//m_wLOG.clear();
				return;
				}
			}
		break;
		}
		
	// ������� ������
	fseek(m_LOG,0,SEEK_END);
	fpos_t ss;
	fgetpos(m_LOG,&ss);
	if(ss>0)
		{
		fputs("\r\n",m_LOG);
		fflush(m_LOG);
		}
	}


//
//   ������� ��� ��������, ����� ��� ���  (���� ���� ����������)
//`
 __declspec(noinline) LOG::LOG(wchar_t *Path)
	{
	m_SizeLog	=1024*1024;  // 1M
	m_Level		=1;
	m_Console	=1;
	m_Rotate	=0;

	//�������������� ����������� ������
	InitializeCriticalSection(&LogCritical); 

	setlocale(LC_ALL,"Russian");

	OpenOrCreateLog(Path);
	
	};


//
//		��������� ��� ...
//
LOG::~LOG()
	{
	if( m_LOG) 	fclose(m_LOG);
	}

//
//		��������� ������� �����
//
 __declspec(noinline) int  LOG::SetParameter(int DebugLevel,int WriteToConsole)
	{

	m_Level=DebugLevel;

	m_Console=WriteToConsole;

	return 0;
	}



int LOG::Print(wchar_t *Mess)
	{
	// ���� �������� ������
	return LOG::Print(0,Mess);
	}

//
//		������� ��������� 
//
int LOG::Print(int Level,wchar_t *Mess)
	{
	if( !m_LOG )	return -1;  // �� �������

	if(Level<=m_Level)
		{
		Rotate();

		//������ � ����������� ������
		EnterCriticalSection(&LogCritical);

		SYSTEMTIME st;
		::GetLocalTime(&st);

		// ���������� �����
		char Buff[1024];
		sprintf_s(Buff,"%4d-%02d-%02d %02d:%02d:%02d-%03d => %S\r\n",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds,Mess);

		if(fputs(Buff,m_LOG)<0) return -1;

		fflush(m_LOG);

		if(m_Console) printf("%s",Buff);

		//������� �� ����������� ������
		LeaveCriticalSection(&LogCritical);
		}

	return 0;
	}

//
//		������� ��������� ��� ������������
//
int LOG::Print(int Level,int User,wchar_t *Mess)
	{
	if(!m_LOG)		return -1;  // �� �������

	Rotate();

	if(Level<=m_Level)
		{
		//������ � ����������� ������
		EnterCriticalSection(&LogCritical);

		SYSTEMTIME st;
		::GetLocalTime(&st);

		// ���������� �����
		char Buff[1024];
		sprintf_s(Buff,"%4d-%02d-%02d %02d:%02d:%02d-%03d =%d=> %S\r\n",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds,User,Mess);

		if(fputs(Buff,m_LOG)<0) return -1;
		
		Flush();

		if(m_Console) printf("%s",Buff);

		//������� �� ����������� ������
		LeaveCriticalSection(&LogCritical);
		}

	return 0;
	}


//
//		������� �����
//
int LOG::Rotate()
	{
	if(!m_LOG) return 0;
	
	SYSTEMTIME st;
	::GetLocalTime(&st);

	if(m_Day!=st.wDay)
		{
		// ������ ���� � �����
		fclose(m_LOG);

		OpenOrCreateLog(m_Path);

		return 0;
		}

	// �������



	// �����������




	// ������� �����


	return 0;
	}

void LOG::Flush()
	{
	if(!m_LOG)	fflush(m_LOG);

	}

//
//		������� ���
//
void LOG::Close()
	{
	if( !m_LOG) 	fclose(m_LOG);
	}
