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

	MTD  *mtd1 = new MTD(1);
	OVI2 *ovi1 = new OVI2(1);

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

	printf("Frames = %d    Meta Frames = %d \n", oFI.CountVideoFrame, mFI.CountMetadataFrame);


	MetaDataInfo MDI;

	DWORD Idx;

	VideoFrameInfo VFI;


	for (int i = 0; i<mFI.CountMetadataFrame; i++)
     	{
		if (i == 5)
		{
			int i = 0;
		}
		ret = mtd1->ReadMetaData(i, nullptr, 0, &MDI);
		if (ret != 0) break;

		ret=ovi1->SeekVideoFrameByTime(MDI.Time, &Idx);
		
		if (ret != 0)
			{
			ovi1->Debug();
			printf("Not found frame = %I64d\n",MDI.Time);
			}

		}
	printf("\nPress any key\n");
	getchar();

	return 0;
}