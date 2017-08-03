#include "string"

#include "OVIFileWriter.h"

#include "ovi\ovi_container.h"

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

	FI.Mod= Crypto_AES && 5;  // Режим работы контейнера

	int result = m_Container->Create(fileName.c_str(), &FI);

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
	m_Container->Close(nullptr);

	m_MetaData->Close(nullptr);

	return S_OK;
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

	int res = m_Container	->WriteVideoFrame((unsigned char *)buffer,size,keyFlag,time,nullptr, 0);
	
	return 0==res ? S_OK : STG_E_WRITEFAULT;
	}

//
//		Запишем звук
//
HRESULT OviFileWriter::WriteAudioFrame( const char *buffer, unsigned int size, uint64_t time )
	{
	wchar_t Buff[128];
	swprintf_s(Buff,L"\nSize = %d\n",size);
	
	OutputDebugString(Buff);


	int ret=m_Container->WriteAudioSample((unsigned char *)buffer,size,time);


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
//		Запись данных
//
HRESULT OviFileWriter::WriteMetaData(const char *buffer, unsigned int size, uint64_t time)
	{
	if (!m_MetaData->IsOpen())
		{
		// Создадим файл
		MetaDataFileInfo MDFI;
		memset(&MDFI,0, sizeof(MetaDataFileInfo));

		m_NameFile.replace(m_NameFile.find(L".ovi"), 4, L".mtd");

		m_MetaData->Create(m_NameFile.c_str(),&MDFI);
		}

	// Запишем
	m_MetaData->WriteMetaData((unsigned char *)buffer,size,time);

	return S_OK;
	}
