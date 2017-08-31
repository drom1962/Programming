#include <wchar.h>

#include <string>

#include "..\..\Containers\ovi\ovi_container.h"

#include "..\..\Containers\ovi\metadata.h"

#include "..\..\Containers\metadata\mymath.h"

#include <stdio.h>


using namespace Meta_Data;

int wmain(int argc, wchar_t* argv[])
	{
	OVI2	*ff;

	ff = new OVI2(1);	

	FileInfo oFI;

	VideoFrameInfo	VFI;

	ret=ff->Open(L"D:\\Video\\a1.ovi",&oFI);
	uint32_t Ind;
	int i = oFI.CountVideoFrame-7;
	for(;i<oFI.CountVideoFrame;i++)
		{
		ret=ff->GetInfoVideoFrame(i, &VFI);

		ret=ff->SeekVideoFrameByTime(VFI.Time+111111111111, &Ind);


		printf("%7d - %7d",i,Ind);

		if (i != (Ind))
			{
			printf("---\n");
			getchar();
			int i = 0;
			}
		printf("\n");






		}

	getchar();
	return 0;
}