#include "..\..\Decoders\decoder.h"

#include "..\..\Containers\common.h"

Decoder::~Decoder() {};

//
//
//
Ffmpeg::Ffmpeg()
	{
	m_dec = nullptr;
	m_LocalBuff = nullptr;
	m_Feature = Decod;
	}

Ffmpeg::~Ffmpeg()
	{
	}


int Ffmpeg::Feature()
	{
	return m_Feature;
	}

//
//    Создание декодера
//
int	Ffmpeg::Init(int Dev,DecoderInfo *DI)
	{

	const decoder_id decoderImplID(decoder_FFMpeg);

	m_dec = create_video_decoder(decoderImplID);

	auto st = m_dec->init_instance(0);

	video_codec		videoCodec=(video_codec)DI->Codec;
	st = m_dec->set_video_decoder_params(videoCodec,
		DI->Width,
		DI->Height,
		img_format_rgb24);
	
	// Заполним битмап хедер
	FillBitMap(&m_BitMap, DI->Height, DI->Width);
	
	m_dec->set_alloc_picture_callback(&Ffmpeg::MyAlloc, this,nullptr);
	
	return st;
	}


//
//		Декодирование кадра
//
int	Ffmpeg::Decode(Frame *Frame, FrameInfo *FI)
	{
	if (m_dec == nullptr) return -1;

	output_frame	of;

	auto st = m_dec->decode_frame(Frame->Data,Frame->Size, &of);

	FI->Size = of.size;
	FI->Data = (unsigned char *)m_LocalBuff;
	
	return S_OK;
	}


int	Ffmpeg::Destroy()
	{
	if (m_dec == nullptr) return -1;

	m_dec->unload();

	return S_OK;
	}


void * Ffmpeg::MyAlloc(size_t cb, void *oldptr, void *arg)
	{

	char *m_LocalBuff = nullptr;

	if (static_cast<Ffmpeg *>(arg)->m_LocalBuff == NULL)
		{
		cb += sizeof(BMP);

		m_LocalBuff = (char *)malloc(cb);

		if (m_LocalBuff == nullptr)  return nullptr;

		memcpy(m_LocalBuff,&static_cast<Ffmpeg *>(arg)->m_BitMap, sizeof(BMP));

		static_cast<Ffmpeg *>(arg)->m_LocalBuff = m_LocalBuff;

		m_LocalBuff += sizeof(BMP);
		
		}
		
	return m_LocalBuff;
	}