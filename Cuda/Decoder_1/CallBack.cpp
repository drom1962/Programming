#include <memory.h>
#include <stdio.h>

#include "nvcuvid.h"

#include "MyCuda.h"

// Called before decoding frames and/or whenever there is a format change
//
int CUDAAPI SequenceCallback(void *UserData, CUVIDEOFORMAT *VIDEOFORMAT)
    {
	// Проверка разрешения и кодека - Зачем непонятно
	memcpy(&((cuDecoderInfo *)UserData)->VIDEO_FORMAT,VIDEOFORMAT,sizeof(CUVIDEOFORMAT));
	//printf("1");
    return 1;
    }


// Called when a picture is ready to be decoded (decode order)
//
int CUDAAPI DecodePicture(void *UserData, CUVIDPICPARAMS *PICPARAMS)
    {
	//CUresult ret=cuvidDecodePicture(((ParserFrame *)UserData)->Decoder, PICPARAMS);
	memcpy(&((cuDecoderInfo *)UserData)->PIC_PARAMS,PICPARAMS,sizeof(CUVIDPICPARAMS));
	//printf("2");
    return 1;
    }


// Called whenever a picture is ready to be displayed (display order)
//
int CUDAAPI DisplayPicture(void *UserData, CUVIDPARSERDISPINFO *PARSERDISPINFO)
    {
	memcpy(&((cuDecoderInfo *)UserData)->PARSER_DISP_INFO,PARSERDISPINFO,sizeof(CUVIDPARSERDISPINFO));
	return 1;
    }
