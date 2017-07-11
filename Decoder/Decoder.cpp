

#include <wchar.h>

#include <conio.h>

#include <stdio.h>

#include <string>

#include "..\Containers\ovi\ovi_container.h"

#include "..\Containers\ovi\metadata.h"

#include <windows.h>

#include <stdio.h>


#include "Bradski_Devis.h"

#include "decoder.h"

#include "detector02.h"

using namespace Meta_Data;

int wmain(int argc, wchar_t* argv[])
{

	int ret,ret1;

	MTD  *mtd1 = new MTD(1);
	OVI2 *ovi1 = new OVI2(1);

	Ffmpeg *Fmpg = new Ffmpeg();

	MetaDataFileInfo mFI;

	FileInfo		 oFI;

	MovingTarget2	*MT = new MovingTarget2[1024];

	// —формируем им€ файла
	if (argc == 1) return 0;

	wchar_t File[1024], File1[1024];

	wmemcpy(File, argv[1], 1024);

	ret = ovi1->Open(File, &oFI);
	if (ret != 0) return 1;

	File[wcslen(File) - 3] = 0;
	swprintf_s(File1, L"%smtd", File);

	ret = mtd1->Open(File1, &mFI);
	if (ret != 0) return 2;

	DWORD	FullSize = oFI.Height*oFI.Width;

	//printf("Frames = %d    Meta Frames = %d \n", oFI.CountVideoFrame, mFI.CountMetadataFrame);

	FILE *stream;
	char Buff[1024];

	swprintf_s(File1, L"%slog", File);
	_wfopen_s(&stream, File1, L"w");

	Bradski_Devis *Tar = new Bradski_Devis();

	Tar->SetMinObjSize(1.0);

	VideoFrameInfo VFI;

	Frame Fr;
	FrameInfo FI;

	unsigned char  ED[265];
	DWORD		  sED;

	ret=ovi1->GetExtraData(ED, 256, &sED);
	Fr.Data = ED;
	Fr.Size = sED;

	DecoderInfo DI;
	DI.Height = oFI.Height;
	DI.Width  = oFI.Width;
	DI.Codec = oFI.VideoCodec;

	ret1=Fmpg->Init(0, &DI);


	ret1=Fmpg->Decode(&Fr, &FI);
	HANDLE eTimer = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	DWORD Delay = 1000 / oFI.FPS;

	IplImage Frame1, Frame2;

	unsigned char * Maps = (unsigned char *)malloc(oFI.Height*oFI.Width * 3+ sizeof(BITMAPINFOHEADER));

	BITMAPINFOHEADER Bh;
	memset(&Bh, 0, sizeof(BITMAPINFOHEADER));
	Bh.biWidth = oFI.Width;
	Bh.biHeight = oFI.Height;
	Bh.biBitCount = 24;
	
	long long Det=CreateDetector02();

	ret=InitDetector02(Det, 0.03, 0.005, 0.05,2);


	printf("Size object = ");
	int Obj;
	scanf_s("%d", &Obj);

	SetMinObjSizeDetector02(Det, Obj);
		

	// окно дл€ отображени€ картинки
	//cvNamedWindow("original", CV_WINDOW_NORMAL);
	
	//IplImage *Frame=cvCreateImage(cvSize(oFI.Width, oFI.Height), 8, 3);
	//cvZero(Frame);
	/*
	IplImage *image1 = cvLoadImage("E:\\10.jpg", CV_LOAD_IMAGE_UNCHANGED);

	printf("[i] channels:  %d\n", image1->nChannels);
	printf("[i] pixel depth: %d bits\n", image1->depth);
	printf("[i] width:       %d pixels\n", image1->width);
	printf("[i] height:      %d pixels\n", image1->height);
	printf("[i] image size:  %d bytes\n", image1->imageSize);
	printf("[i] width step:  %d bytes\n", image1->widthStep);

	cvShowImage("original", image1);

	cvWaitKey(0);
	*/
	Delay = 1000/(oFI.FPS/oFI.GOP);
	//Delay = 1000;

	DWORD Idx;
	MetaDataInfo MDI;
	int dd=0,mm = 0;
	for (int i = 0; i < oFI.CountVideoFrame; i++)
		{
		ret=ovi1->ReadVideoFrame(i, nullptr, 0, &VFI);
		if (ret != 0)  return -1;

		Fr.Data = VFI.Data;
		Fr.Size = VFI.SizeFrame;
		Fr.Type = VFI.TypeFrame;

		ret1 = Fmpg->Decode(&Fr, &FI);
		if (ret1 != 0)  return -2;

		//memcpy(Frame->imageData, FI.Data+sizeof(BMP), FI.Size);

		// показываем картинку
		//cvShowImage("original", Frame);

		//ret=cvWaitKey(1);
		//if (ret != -1)  return 1;
		fprintf(stream, "%5d = %I64d = %1d = %6d =",i,VFI.TimeFrame,VFI.TypeFrame,VFI.SizeFrame);
		 printf        ("%5d = %I64d = %1d = %6d =",i,VFI.TimeFrame,VFI.TypeFrame,VFI.SizeFrame);

		DWORD ww = WaitForSingleObject(eTimer, Delay);

		if (VFI.TypeFrame == 1)
			{
			UpdateDetector02InverseMetaResize(Det, FI.Data, nullptr, 30, (__int64)&Bh, 1, VFI.TimeFrame);
			ret=IsMovingDetector02(Det);
			if (ret == 1)
				{
				dd++;
				fprintf(stream, "   Detect");
				printf("   Detect");
				}
			}

		ret = mtd1->SeekMetaDataByTime(VFI.TimeFrame, &Idx);
		if (ret == 0)
			 {
			mtd1->ReadMetaData(Idx, nullptr, 0, &MDI);
			 printf("  --- >MetaData - %I64d",MDI.Time);
			 mm++;
			}

		fprintf(stream, "\n");
		printf("\n");

		i=ovi1->SeekNextKeyVideoFrame(i)-1;
		
		ret = _kbhit();
		if (ret != 0)
			{
			char ss = _getch();
			switch (ss)
				{
				case 's':
					fclose(stream);
					return 0;

				default:
					_getch();
				}
			}
		}
	fprintf(stream, "Count metadata = %d\n",mm);
	printf("%d Count metadata = %d - %d\n",dd,mFI.CountMetadataFrame, mm);

	fclose(stream);

	_getch();
	return 0;
}