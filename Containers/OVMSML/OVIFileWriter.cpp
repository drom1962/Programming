#include "string"

#include "OVIFileWriter.h"

#include "ovi\ovi2.h"

#include "ovi\metadata.h"

using namespace Writer;

OviFileWriter::OviFileWriter()
	{
	m_Container	=new OVI2(1);

	m_MetaData = new MTD(1);

	}


OviFileWriter::~OviFileWriter()
	{
	m_Container->Close(nullptr);

	m_MetaData->Close(nullptr);
	}

bool OviFileWriter::CreateWriter(VideoCodecType vc)
	{
	m_codec = vc;
	return true;
	}

//
//		Создадим файл
//
HRESULT OviFileWriter::InitFile(const std::wstring& fileName, const VideoParameters& video, const AudioParameters& audio)
	{
	FileInfo FI;
	memset(&FI,1,sizeof(FileInfo));

	FI.VideoCodec   =video.codecType;
	FI.VideoBitRate =video.BitRate;
	FI.Width        =video.Witdh;
	FI.Height       =video.Height;
	FI.FPS          =video.Fps;

	FI.AudioCodec   =audio.codecType;
	FI.BitsPerSample=audio.BitsPerSample;
	FI.SamplesPerSec=audio.SamplesPerSec;

	int result = m_Container->Init(fileName.c_str(), &FI);

	if (result < 0)  return STG_E_WRITEFAULT;

	if(m_sizeED > 0) m_Container->SetExtraData(m_ED, m_sizeED);
	
	m_NameFile = fileName;
	
	return S_OK;
	}

//
//		Сохраним ExtraData
//
HRESULT OviFileWriter::SaveExtraData(const char *buffer, unsigned int size)
	{
	if(size>255) return E_FAIL;

	memcpy(m_ED, buffer, size);

	m_sizeED=size;
	
	return S_OK;
	}

//
//		Рудемент
//
HRESULT OviFileWriter::SetExtraData(const char *buffer, unsigned int size)
	{
	return S_OK;
	}

//
//		Закроем
//
HRESULT OviFileWriter::CloseFile()
	{
	return m_Container->Close(nullptr) >= 0 ? S_OK : E_FAIL;
	}

//
//		Запишем кадр
//
HRESULT OviFileWriter::WriteVideoFrame( const char *buffer, unsigned int size, uint64_t time, bool keyFlag )
	{
	if (!size) 
		{
		return E_INVALIDARG;
		}

	int res = m_Container->WriteVideoFrame((unsigned char *)buffer,size,keyFlag,time,nullptr, 0);
	
	return 0==res ? S_OK : STG_E_WRITEFAULT;
	}

//
//		Запишем звук
//
HRESULT OviFileWriter::WriteAudioFrame( const char *buffer, unsigned int size, uint64_t time )
	{
	return S_OK;
	}

//
//		Нужна extradata
//
bool OviFileWriter::IsNeedExtraData()
	{
	return true;
	}


//
//		*
//
HRESULT OviFileWriter::WriteMetaData(const char *buffer, unsigned int size, uint64_t time)
	{
	if (!m_MetaData->IsOpen())
		{
		// Создадим файл

		m_MetaData->Create()
		}



	return S_OK;
	}
