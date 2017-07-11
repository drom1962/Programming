#include <windows.h>

#include <string>

#include "..\..\Containers\container.h"

#include "..\..\Containers\common.h"

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

dat			xx;

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
	/*
	DWORD m_CifKeyForHeader[32];

	AES *zz = new AES(128);

	char ss[] = "ab32f8c395d0b07c7b612dcb56375e0383f85b05052b894c2f083f146c46f491";
	for (int i = 0; i < 8; i++)
		{
		sscanf_s(&ss[i * 8], "%8x", &m_CifKeyForHeader[i]);
		}


	zz->Init(m_CifKeyForHeader);

	char Buff[] = "Привет от AES111111111111";

	long *vr = (long *)(Buff + 2);

	for (int i = 0; i < 10000; i++)
		{
		zz->Crypt((WORD *)Buff, 18);

		vr[0] = ~vr[0];


		zz->Decrypt((WORD *)Buff, 18);
		}

*/



	HINSTANCE				m_Dll,m_Dll1;

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

	for (int i = 0; i<FI.CountVideoFrame; i++)
		{
		if (m_Con->ReadVideoFrame(i, nullptr, 0, &VFI) != 0)
			{
			return -1;
			}

		Frames[i].Key = VFI.TypeFrame;
		Frames[i].Size = VFI.SizeFrame;
		Frames[i].Time = VFI.TimeFrame;
		Frames[i].Fr = (unsigned char *)malloc(VFI.SizeFrame);

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

	xx.type = 2;
	xx.gop = FI.GOP;
	xx.Width = FI.Width;
	xx.Height = FI.Height;
	xx.Codec = FI.VideoCodec;
	xx.CountFrames = FI.CountVideoFrame;


	// Запустим рабочий поток
	xx.wr = CreateThread(NULL, 1024 * 1024, &Writers, &xx, 0, NULL);




	//m_Dll = LoadLibraryA("C:\\BOLID\\ARM_ORION_PRO1_20_1\\VideoComponent\\oviConteiner.dll");
	m_Dll = LoadLibraryA("D:\\Programming\\dll_10\\Debug\\oviConteiner.dll");
	if (m_Dll == NULL) return 1;

	m_CreateConteiner = (CreateConteiners)GetProcAddress(m_Dll, "CreateConteiner");
	if (!m_CreateConteiner)  return 2;

	Archiv *m_Con1 = (Archiv *)m_CreateConteiner();

	FileInfo FI1;
	
	Sleep(6000);

	ret = m_Con1->Open(L"F:\\Test\\refresh.ovi", &FI1);
	
	while (1)
		{
		Sleep(2000);
		m_Con1->ReadVideoFrame(0, nullptr, 0, &VFI);
		if (m_Con1->Refresh(1) == 0) break;


		m_Con1->GetFileInfo(&FI1);
		printf("\n %d", FI1.CountVideoFrame);
		}

	printf("Press any key\n");
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
		break;

		// OVI
	case		2:
		m_Dll = LoadLibraryA("D:\\Programming\\dll_10\\Debug\\oviConteiner.dll");
		if (m_Dll == NULL)
			return 1;
		break;

	default:
		return 0;
	}
	InterlockedIncrement(&Line);

	printf("\n (%5d) Start", Line);

	file[0] = ((dat *)lpParam)->Disk;

	CreateConteiners		m_CreateConteiner;

	m_CreateConteiner = (CreateConteiners)GetProcAddress(m_Dll, "CreateConteiner");
	if (!m_CreateConteiner)
		return 2;

	Archiv *m_Con = (Archiv *)m_CreateConteiner();

	FileInfo FI;

	FI.Height = ((dat *)lpParam)->Height;
	FI.Width = ((dat *)lpParam)->Width;
	FI.VideoCodec = ((dat *)lpParam)->Codec;
	FI.Mod = Crypto | 1;


	int ret = m_Con->Init(L"F:\\Test\\refresh.ovi", &FI);

	if (ret != 0)  return 88;

	ret = m_Con->SetExtraData(ED, sED);


	DWORD Delay = 1000 / ((dat *)lpParam)->gop;

	HANDLE	eTimer = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	unsigned char *Buff = (unsigned char *)malloc(1024 * 512);

	((dat *)lpParam)->Start = GetTickCount();


	DWORD ww;
	for (int i = 0; i < ((dat *)lpParam)->CountFrames; i++)
	{
		ret = m_Con->WriteVideoFrame(Frames[i].Fr, Frames[i].Size, Frames[i].Key, Frames[i].Time, nullptr, 0);
		if (ret != 0)
		{
			int i = 0;
		}
		if (Frames[i].Key)
		{
			InterlockedIncrement(&Line);
			printf("\n (%5d)  %d => %d ==>key (%d)", Line, Pid, ((dat *)lpParam)->type, i);
		}
		//printf("\n %d => %d ==>key (%d)", i, Frames[i].Size,Frames[i].Key);

		ww = WaitForSingleObject(eTimer, Delay);
		if (ww == WAIT_TIMEOUT)
		{
			int i = 9;
		}
	}

	ret = m_Con->Close(&FI);


	FreeLibrary(m_Dll);


	((dat *)lpParam)->End = GetTickCount();

	SetEvent(((dat *)lpParam)->eTimer);
	InterlockedIncrement(&Line);

	printf("\n (%5d) End", Line);

}

