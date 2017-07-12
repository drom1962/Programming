#include <wchar.h>

#include <conio.h>

#include <string>

#include "..\..\Containers\ovi\ovi_container.h"

#include "..\..\Containers\ovi\metadata.h"


#include <stdio.h>

using namespace Meta_Data;

int wmain(int argc, wchar_t* argv[])
{

	int ret, ret1;

	MTD  *mtd1 = new MTD(1);
	OVI2 *ovi1 = new OVI2(1);

	const char *ListCodecs[] = { "NONE","MJPEG","MPEG4","H.264","H.265" };

	const char *Keys[] = { "   ","Key"};

	MetaDataFileInfo mFI;
	FileInfo		 oFI;

	// —формируем им€ файла
	if (argc == 1) return 0;

	wchar_t File[1024], File1[1024];

	wmemcpy(File, argv[1], 1024);

	ret = ovi1->Open(File, &oFI);
	if (ret != 0) return 1;

	printf("Video\n-----------------\n");
	printf("Count = %6d   %4d x %4d\n",oFI.CountVideoFrame,oFI.Width,oFI.Height);

	printf("\nCodec = %s  FPS=%5.2f   GOP=%3d Duration=%7.2f\n", ListCodecs[oFI.VideoCodec],oFI.FPS,oFI.GOP,oFI.Duration);
	
	File[wcslen(File) - 3] = 0;
	swprintf_s(File1, L"%smtd", File);

	ret = mtd1->Open(File1, &mFI);
	if (ret != 0) 
		{ 
		_getch();
		return 1; 
		}
	printf("\nMetaData\n-----------------\n");
	printf("Count =%6d",mFI.CountMetadataFrame);

	VideoFrameInfo VFI;
	/*ret = ovi1->ReadVideoFrame(15, nullptr, 0, &VFI);
	FILE *stream;

	_wfopen_s(&stream, L"D:\\Video\\xx.xx", L"w");
	fwrite(VFI.Data, 1, VFI.SizeFrame, stream);
	fclose(stream);*/

	printf("\n  Prinf frames - n/y   -");

	int ss=_getch();
	if (ss != 'y') return 0;

	for (DWORD i = 0; i < oFI.CountVideoFrame; i++)
		{
		ret = _kbhit();
		if (ret != 0)
			{
			ss=_getch();
			if (ss == 's') return 0;
			_getch();
			}

		ret = ovi1->ReadVideoFrame(i, nullptr, 0, &VFI);
		Sleep(40);
		if (ret != 0) return -1;
		printf("\n%6d - %3s  - %6d",i, Keys[VFI.TypeFrame], VFI.SizeFrame);
		}


}