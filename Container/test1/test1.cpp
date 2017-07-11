#include "..\..\Containers\ovi\ovi_container.h"

#include <stdio.h>

typedef Archive_space::Archiv *	(CALLBACK* CreateConteiner_avi)();

int main()
	{
    int ret;

    FileInfo FI1,FI2;

	wchar_t file[1024];

	
	HINSTANCE				m_Dll1, m_Dll2;

	CreateConteiner_avi		m_CreateConteiner;


	char Fn[128];
	printf("File =");
	gets_s(Fn);

	// OVI
	m_Dll1 = LoadLibraryA("C:\\BOLID\\ARM_ORION_PRO1_20_1\\VideoComponent\\oviConteiner.dll");
	if (m_Dll1 == NULL) return 1;

	m_CreateConteiner = (CreateConteiner_avi)GetProcAddress(m_Dll1, "CreateConteiner");
	if (!m_CreateConteiner)  return 2;

	Archiv *m_Con1 = (Archiv *)m_CreateConteiner();

	swprintf_s(file, 1024, L"F:\\15_138\\2017_04_26\\%S.ovi", Fn);
    ret = m_Con1->Open(file, &FI1);
	if (ret != 0)
		{
		return 0;
		}

	wprintf(L"%s\n", file);

	printf("Codec = %d   Width = %d  Height = %d Frames = %d \n", FI1.VideoCodec, FI1.Width, FI1.Height, FI1.CountVideoFrame);

	printf("GOP = %d   FPS = %3.0f Bitrate = %d\n", FI1.GOP, FI1.FPS, FI1.VideoBitRate);


	// AVI
	m_Dll2 = LoadLibraryA("C:\\BOLID\\ARM_ORION_PRO1_20_1\\VideoComponent\\aviConteiner.dll");
	if (m_Dll2 == NULL) return 1;

	m_CreateConteiner = (CreateConteiner_avi)GetProcAddress(m_Dll2, "CreateConteiner");
	if (!m_CreateConteiner)  return 2;

	Archiv *m_Con2 = (Archiv *)m_CreateConteiner();

	swprintf_s(file, 1024, L"F:\\15_138\\2017_04_26\\%S.avi", Fn);
	ret = m_Con2->Open(file, &FI2);
	if (ret != 0)
	{
		return 0;
	}

	
	wprintf(L"%s\n", file);

	printf("Codec = %d   Width = %d  Height = %d Frames = %d \n", FI2.VideoCodec,FI2.Width, FI2.Height,FI2.CountVideoFrame);

	printf("GOP = %d   FPS = %3.0f Bitrate = %d\n", FI2.GOP,FI2.FPS, FI2.VideoBitRate);
    
	if (ret != 0)
        {
        printf("File not founded");
        getchar();
        return 0;
        }

	int xx = min(FI1.CountVideoFrame, FI2.CountVideoFrame);
	printf("\n %d \n",xx);
	printf("\n  OVI   ----------------\n");

    VideoFrameInfo VFI;

	DWORD t1, t2, ts;
	t1=GetTickCount();

	int gop = -1;
	int ss=0;
	

    for (int i = 0;i < xx; i++)
        {
        ret = m_Con1->ReadVideoFrame(i, nullptr, 0, &VFI);

		if (VFI.TypeFrame) gop++;

		if (i>0 && VFI.TypeFrame==1)
			{
		//	printf("                                   All size GOP = %d\n", ss);
			ss = VFI.SizeFrame;
			}
		else 
			ss += VFI.SizeFrame;

		
        //printf("%6i - %7i  -   %i\n", i,VFI.SizeFrame, VFI.TypeFrame);
		
        }
	t2 = GetTickCount();

	printf("Time = %d", t2 - t1);

	printf("\n  AVI   ----------------\n");

	t1 = GetTickCount();

	gop = -1;
	ss = 0;

	for (int i = 0; i < xx; i++)
	{
		ret = m_Con2->ReadVideoFrame(i, nullptr, 0, &VFI);

		if (VFI.TypeFrame) gop++;

		if (i>0 && VFI.TypeFrame == 1)
		{
			//	printf("                                   All size GOP = %d\n", ss);
			ss = VFI.SizeFrame;
		}
		else
			ss += VFI.SizeFrame;


		//printf("%6i - %7i  -   %i\n", i,VFI.SizeFrame, VFI.TypeFrame);

	}
	t2 = GetTickCount();

	printf("Time = %d", t2 - t1);


    getchar();

    return 0;
	}