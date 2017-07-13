#include <stdafx.h>

#include <windows.h>

#include <string>

#include "..\..\Containers\container.h"

using namespace Archive_space;


struct dat
	{
	wchar_t Disk;
	int		type;
	int		gop;
	int		Width;
	int		Height;
	int		Codec;
	int	    CountFrames;

	HANDLE			wr;
	DWORD			Start;
	DWORD			End;
	HANDLE			eTimer;
	};

DWORD WINAPI Writers(LPVOID lpParam);

dat			xx[1024];
HANDLE		WWW[1024];

struct Frame
	{
	int				Key;
	unsigned char	*Fr;
	DWORD			Size;
	DWORD			Time;
	};

Frame *Frames;

typedef Archive_space::Archiv *	(CALLBACK* CreateConteiners)();

long	Line = 0;

unsigned char ED[1024];
DWORD sED;

const int cc = 1;

int main()
	{
	HINSTANCE				m_Dll;

	// Считаем файл
	m_Dll = LoadLibraryA("C:\\BOLID\\ARM_ORION_PRO1_20_1\\VideoComponent\\aviConteiner.dll");
	if (m_Dll == NULL) return 1;

	CreateConteiners		m_CreateConteiner;

	m_CreateConteiner = (CreateConteiners)GetProcAddress(m_Dll, "CreateConteiner");
	if (!m_CreateConteiner)  return 2;

	Archiv *m_Con = (Archiv *)m_CreateConteiner();

	FileInfo FI;
	int ret = m_Con->Open(L"D:\\Programming\\Video\\bb.avi", &FI);

	if (ret != 0)
		return -99;
	
	m_Con->GetExtraData(ED, 1024, &sED);

	Frames = new Frame[FI.CountVideoFrame];

	VideoFrameInfo VFI;
	
	for(int i=0;i<FI.CountVideoFrame;i++)
		{
		if (m_Con->ReadVideoFrame(i, nullptr, 0, &VFI) != 0)
			{
			return -1;
			}

		Frames[i].Key	= VFI.TypeFrame;
		Frames[i].Size	= VFI.SizeFrame;
		Frames[i].Time	= VFI.TimeFrame;
		Frames[i].Fr	= (unsigned char *)malloc(VFI.SizeFrame);

		memcpy(Frames[i].Fr, VFI.Data, VFI.SizeFrame);

		}

	m_Con->Close(nullptr);

	FreeLibrary(m_Dll);


	FI.GOP = 1;

	for (int i = 1; i < FI.CountVideoFrame; i++)
		{
		if (Frames[i].Key) break;
		FI.GOP++;
		}

	for (int i=0; i < cc; i++)
		{
		if (i % 2 == 0)
			{
			xx[i].Disk = 'F';
			xx[i].type = 2;
			}
		else
			{
			xx[i].Disk = 'F';
			xx[i].type = 2;
			}

		xx[i].gop			= FI.GOP;
		xx[i].Width			= FI.Width;
		xx[i].Height		= FI.Height;
		xx[i].Codec			= FI.VideoCodec;
		xx[i].CountFrames	= FI.CountVideoFrame;

		//Sleep(22);

		// Запустим рабочий поток
		xx[i].wr=CreateThread(NULL, 1024 * 1024, &Writers, &xx[i], 0, NULL);

		if (xx[i].wr == NULL)
			{
			DWORD xx=GetLastError();
			}

		xx[i].eTimer= CreateEvent(nullptr, TRUE, FALSE, nullptr);

		WWW[i] = xx[i].eTimer;
		}

	WaitForMultipleObjects(cc, WWW, TRUE, 1000 * 1000);

	for (int i = 0; i < cc; i++)
		{
		printf("\n %d -----------%5.2f", i, (xx[i].End - xx[i].Start)/1000.);
		}


	getchar();

    return 0;
	}

DWORD WINAPI Writers(LPVOID lpParam)
	{
	HINSTANCE				m_Dll;

	wchar_t					file[1024];

	DWORD					Pid = GetCurrentThreadId();

	switch (((dat *)lpParam)->type)
		{
		// AVI
		case		1:
						m_Dll = LoadLibraryA("C:\\BOLID\\ARM_ORION_PRO1_20_1\\VideoComponent\\aviConteiner.dll");
						if (m_Dll == NULL) 
							return 1;

						swprintf_s(file, 1024, L"F:\\Test\\%d.avi", Pid);
						break;

		// OVI
		case		2:
						m_Dll = LoadLibraryA("D:\\Programming\\dll_10\\Debug\\oviConteiner.dll");
						if (m_Dll == NULL) 
							return 1;

						swprintf_s(file, 1024, L"F:\\Test\\%d.ovi", Pid);
						break;

		default:
				return 0;
		}
	InterlockedIncrement(&Line);

	printf("\n (%5d) Start",Line);

	file[0] = ((dat *)lpParam)->Disk;

	CreateConteiners		m_CreateConteiner;

	m_CreateConteiner = (CreateConteiners)GetProcAddress(m_Dll, "CreateConteiner");
	if (!m_CreateConteiner) 
		return 2;

	Archiv *m_Con = (Archiv *)m_CreateConteiner();

	FileInfo FI;

	FI.Height		= ((dat *)lpParam)->Height;
	FI.Width		= ((dat *)lpParam)->Width;
	FI.VideoCodec	= ((dat *)lpParam)->Codec;
	FI.Mod			=  WriteExtradata |  Crypto | 0;


	int ret=m_Con->Init(file, &FI);

	if (ret != 0)  return 88;

	ret=m_Con->SetExtraData(ED, sED);


	DWORD Delay = 1000 / ((dat *)lpParam)->gop;

	HANDLE	eTimer = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	unsigned char *Buff = (unsigned char *)malloc(1024 * 512);

	((dat *)lpParam)->Start=GetTickCount();


	DWORD ww;
	for (int i = 0; i < ((dat *)lpParam)->CountFrames; i++)
		{
		ret=m_Con->WriteVideoFrame(Frames[i].Fr,Frames[i].Size,Frames[i].Key,Frames[i].Time, nullptr, 0);
		if (ret != 0)
			{
			int i = 0;
			}
		if (Frames[i].Key)
			{
			InterlockedIncrement(&Line);
			printf("\n (%5d)  %d => %d ==>key (%d)",Line, Pid, ((dat *)lpParam)->type, i);
			}
		//printf("\n %d => %d ==>key (%d)", i, Frames[i].Size,Frames[i].Key);

		ww = WaitForSingleObject(eTimer, Delay);
		if (ww == WAIT_TIMEOUT)
			{
			int i = 9;
			}
		}

	ret=m_Con->Close(&FI);


	FreeLibrary(m_Dll);


	((dat *)lpParam)->End = GetTickCount();

	SetEvent(((dat *)lpParam)->eTimer);
	InterlockedIncrement(&Line);

	printf("\n (%5d) End", Line);

	}

