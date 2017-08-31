#include <wchar.h>

#include <conio.h>

#include <string>

#include "..\..\Containers\ovi\ovi_container.h"

#include "..\..\Containers\ovi\metadata.h"

#include "D:\GIT\MediaServer\readers\aviconteiner\avi.h"
using namespace AVI_container;

#include "kind_of_video_frames.h"
using namespace video_frame_detection;


#include <stdio.h>

using namespace Meta_Data;

typedef Archive_space::Archiv *	(CALLBACK* CreateConteiner)();

int wmain(int argc, wchar_t* argv[])
{

	int ret, ret1;

	MTD		*mtd1 = new MTD(1);
	OVI2	*ovi1 = new OVI2(1);
	
	const char *ListCodecs[] = { "NONE","MJPEG","MPEG4","H.264","H.265" };

	const char *AudioCodecs[]= {"MUTE","PCM","ALAW","MULAW","G726","AAC"};

	const char *Keys[] = { "   ","Key"};

	MetaDataFileInfo mFI;
	FileInfo		 oFI;

	// —формируем им€ файла
	if (argc == 1) return 0;

	wchar_t File[1024], File1[1024], File2[1024];

	wmemcpy(File, argv[1], 1024);

	ret = ovi1->Open(File, &oFI);
	if (ret != 0) return 1;

	File[wcslen(File) - 3] = 0;

	unsigned char	ED[256];
	uint32_t		sED;
	ovi1->GetExtraData(ED, 1024, &sED);

	FILE *stream;

	swprintf_s(File1, L"%slog", File);
	_wfopen_s(&stream, File1, L"w");

	fprintf(stream,"Video\n-----------------\n");
	printf("Video\n-----------------\n");

	fprintf(stream, "Count = %6d   %4d x %4d\n", oFI.CountVideoFrame, oFI.Width, oFI.Height);
	printf("Count = %6d   %4d x %4d\n",oFI.CountVideoFrame,oFI.Width,oFI.Height);

	fprintf(stream, "\nCodec = %s  FPS=%5.2f   GOP=%3d Count=%d Duration=%7.2f\n", ListCodecs[oFI.VideoCodec], oFI.FPS, oFI.GOP,oFI.CountVideoFrame, oFI.Duration);
	printf("\nCodec = %s  FPS=%5.2f   GOP=%3d Count = %d Duration=%7.2f\n", ListCodecs[oFI.VideoCodec],oFI.FPS,oFI.GOP,oFI.CountVideoFrame,oFI.Duration);
	
	if (oFI.AudioCodec > 0)
		{
		fprintf(stream, "\nCodec = %s  BitsPerSample= %1d  SamplesPerSec=%4d  Count = %d\n", AudioCodecs[oFI.AudioCodec], oFI.BitsPerSample, oFI.SamplesPerSec, oFI.CountAudioSample);
		printf("\nCodec = %s  BitsPerSample= %1d  SamplesPerSec=%4d  Count = %d\n", AudioCodecs[oFI.AudioCodec], oFI.BitsPerSample, oFI.SamplesPerSec, oFI.CountAudioSample);
		}


	swprintf_s(File1, L"%smtd", File);
	ret = mtd1->Open(File1, &mFI);
	if (ret == 0)
		{
		fprintf(stream, "\nMetaData\n-----------------\n");
		printf("\nMetaData\n-----------------\n");

		fprintf(stream, "Count =%6d", mFI.CountMetaDataFrame);
		printf("Count =%6d", mFI.CountMetaDataFrame);
		}

	VideoFrameInfo VFI;
	/*ret = ovi1->ReadVideoFrame(15, nullptr, 0, &VFI);
	FILE *stream;

	_wfopen_s(&stream, L"D:\\Video\\xx.xx", L"w");
	fwrite(VFI.Data, 1, VFI.SizeFrame, stream);
	fclose(stream);*/
	fprintf(stream, "\n  Prinf frames - n/y   -");
	printf("\n  Prinf frames - n/y   -");

	int ss=_getch();
	if (ss != 'y') return 0;

	ov_decoder::video_codec codec= ov_decoder::vcodec_h264;

	DWORD t0, t00, tss = 0;
	DWORD	VideoAll = 0.0;
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
		//ret = ovi1->GetInfoVideoFrame(i, &VFI);

		/*
		t0 = GetTickCount();

		for(int i=0;i<50;i++)
		is_key_frame(codec, VFI.Data, VFI.SizeFrame);
		t00 = GetTickCount();
		tss += (t00 - t0);
		*/
		if (ret != 0) return -1;
		fprintf(stream, "\n%6d - %3s  - %6d", i, Keys[VFI.Type], VFI.Size);
		printf("\n%6d - %3s  - %6d",i, Keys[VFI.Type], VFI.Size);

		VideoAll += VFI.Size;
		}



	fprintf(stream, "\nAll = %d ", VideoAll);
	printf("\nAll = %d ", VideoAll);


	AudioSampleInfo ASI;
	DWORD AudioAll = 0.0;
	for (DWORD i = 0; i < oFI.CountAudioSample; i++)
		{
		ret = _kbhit();
		if (ret != 0)
			{
			ss = _getch();
			if (ss == 's') return 0;
			_getch();
			}

		ret = ovi1->ReadAudioSample(i, nullptr, 0, &ASI);

		if (ret != 0) return -1;
		fprintf(stream, "\n%d - %d", i,  ASI.Size);
		//printf("\n%6d - %d", i, ASI.SizeFrame);
		AudioAll += ASI.Size;
	}

	fprintf(stream,"\nAll = %d ", AudioAll);
	printf("\nAll = %d ", AudioAll);


	fprintf(stream, "\nAll = %d Video=%d   Audio=%d", oFI.SizeOviFile,VideoAll,AudioAll);
	printf("\nAll = %d Video=%d   Audio=%d", oFI.SizeOviFile, VideoAll, AudioAll);
	printf(" >>> sum = %d", VideoAll+ AudioAll);

	double xx = static_cast<double>((oFI.SizeOviFile) / (static_cast<double>(VideoAll) + static_cast<double>(AudioAll)));
	printf("====>>> %6.2f", xx-1.0);

	//printf("\n Time Check key frames= %5.2f\n", tss / 1000.);


	while(_kbhit()==0);
}