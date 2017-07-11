#include <wchar.h>

#include <string>

#include "..\..\Containers\ovi\ovi_container.h"

#include "..\..\Containers\ovi\metadata.h"

#include "..\..\Containers\metadata\mymath.h"

#include <stdio.h>


using namespace Meta_Data;

int wmain(int argc, wchar_t* argv[])
	{

	int ret;

	MTD  *mtd1 =new MTD(1);
	OVI2 *ovi1 =new OVI2(1);

	MetaDataFileInfo FI;

	FileInfo		 FI1;

	MovingTarget2	*MT=new MovingTarget2[1024];

	// —формируем им€ файла
	if (argc == 1) return 0;

	wchar_t File[1024], File1[1024];
	
	wmemcpy(File, argv[1], 1024);

	ret = ovi1->Open(File, &FI1);
	if (ret != 0) return 1;

	File[wcslen(File) - 3] = 0;
	swprintf_s(File1, L"%smtd", File);

	ret=mtd1->Open(File1, &FI);
	if (ret != 0) return 2;
	
	DWORD	FullSize = FI.Height*FI.Width;

	printf("Frames = %d    Meta Frames = %d \n",FI1.CountVideoFrame,FI.CountMetadataFrame);


	MetaDataInfo MDI;

	DWORD Idx;

	VideoFrameInfo VFI;


	DWORD t1=GetTickCount();

	Point p1, p2;

	p1.x = 1;
	p1.y = 1;

	p2.x = 900;
	p2.y = 900;


	MyMath *my = new MyMath(p1, p2);
	my->Box(p1, p2);

	FILE *stream;

	swprintf_s(File1, L"%slog", File);
	_wfopen_s(&stream,File1, L"w");

	int vss=0,vss1=0;
	int ret1=0;

	for(int i=0;i<FI1.CountVideoFrame;i++)
		{
		ret = ovi1->ReadVideoFrame(i, nullptr, 0, &VFI);
		if (ret != 0) break;

		vss1 += VFI.SizeFrame;

		ret=mtd1->SeekMetaDataByTime(VFI.TimeFrame, &Idx);
		if (ret!=0) 
			{
			if (ret1)
				{
				fprintf(stream, "%1d => %5d   %6d", VFI.TypeFrame, i, VFI.SizeFrame);
				printf("%1d => %5d   %6d", VFI.TypeFrame, i, VFI.SizeFrame);

				fprintf(stream, " \n");
				printf(" \n");
				}
			continue;
			}

		memset(MT, 0, sizeof(MovingTarget2) * 1000);
		ret=mtd1->ReadMetaData(Idx, (unsigned char *)MT, 1000*16, &MDI);
		vss += MDI.Size+sizeof(ElementMetaIndex)*2;
		
		ret=mtd1->ParserZone(MDI.Data, MDI.Size, &MT);
		if (ret1 == 0 && ret > 0) 		
			ret1 = 1;

		if (ret1)
			{
			fprintf(stream, "%1d => %5d   %6d", VFI.TypeFrame, i, VFI.SizeFrame);
			printf("%1d => %5d   %6d", VFI.TypeFrame, i, VFI.SizeFrame);
			}

		if (ret1)
			{
			fprintf(stream, " MD = %6d  Zon = %3d Angle =", MDI.Size, ret);
			printf(" MD = %6d  Zon = %3d Angle =", MDI.Size, ret);
			}
		
		double ff,ss,ss1,ss2,ss3;

		Point pp1,pp2;

		for (int i = 0; i < ret; i++)
			{
			double ss, ss1, ss2, ss3;
			ss=my->Association(p1, p2, &MT[i]);
			ss1 = my->Association(&MT[i]);

			pp1.x = MT[i].nTop;
			pp1.y = MT[i].nLeft;

			pp2.x = MT[i].nBottom;
			pp2.y = MT[i].nRight;

			ss2 = my->Association(p1, p2, pp1, pp2);

			ss3 = my->Association(pp1, pp2);

			if(ss>0)
			ff = ss / (FullSize/100);


			if (ss != ss1 && ss2!=ss3)
				{
				if (ret1)
					{	
					fprintf(stream, "Error calculate sum");
					printf("Error calculate sum");
					}
				break;
				}
			if (ret1)
				{
				fprintf(stream, " %3.0f", MT[i].fMovAngle);
				printf(" %3.0f", MT[i].fMovAngle);
				}
			}
		if (ret1)
			{
			fprintf(stream, " \n");
			printf(" \n");
			}
		}

	fclose(stream);
	
	DWORD t2 = GetTickCount();

	printf("Time = %5.2f\n",(t2-t1)/1000.);


	printf("AllMD = %d  %d",vss,vss1);
	getchar();

	return 0;
}