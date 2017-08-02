#include "AviFileWriter.h"
#include "..\..\..\..\MediaStorage\AviFile\AviFile\AviFile.h"
#include <atlcomcli.h>
#include <MMReg.h>
#include <MMSystem.h>

#pragma comment(lib, "AviFile")

using namespace Writer;

//avifile.dll
AviFileWriter::AviFileWriter()
{
	m_codec = NONE;
	ex_data.data = nullptr;
	ex_data.size = 0;
	m_writer = nullptr;
}


AviFileWriter::~AviFileWriter()
{
	if (ex_data.data != nullptr)
	{
		delete[] ex_data.data;
		ex_data.data = nullptr;;
	}

	if (m_writer)
	{
		m_writer->Release();
		m_writer = nullptr;
	}
}


bool AviFileWriter::CreateWriter(VideoCodecType vc)
{
	if (NONE == m_codec || m_codec != vc)
	{
		switch (vc)
		{
		case MJPEG:
			m_codecType = MAKEFOURCC('M', 'J', 'P', 'G');
			break;
		case MPEG4:
			m_codecType = MAKEFOURCC('M', 'P', '4', 'V');
			break;
		case H264:
			m_codecType = MAKEFOURCC('H', '2', '6', '4');
			break;
		case H265:
			m_codecType = MAKEFOURCC('H', 'E', 'V', 'C');
			break;
		default:
			m_codecType = 0;
			vc = NONE;
		}
		m_codec = vc;
		if (!m_writer) {
			m_writer = GetAvi();
			if (!m_writer)
				return false;
		}
	}
	return (m_codecType != 0);
}


unsigned long AviFileWriter::GetAudioCodec(AudioCodecType codec)
{
	unsigned long result = 0;
	switch (codec)
	{
	case PCM:
		result = WAVE_FORMAT_PCM;
		break;
	case ALAW:
		result = WAVE_FORMAT_ALAW;
		break;
	case MULAW:
		result = WAVE_FORMAT_MULAW;
		break;
	case G726:
		result = WAVE_FORMAT_G726ADPCM;
		break;
	case AAC:
		result = 0x00ff;
		break;
	default:
		result = 0;
	}
	return result;
}


HRESULT AviFileWriter::InitFile( const std::wstring& fileName, const VideoParameters& video, const AudioParameters& audio )
{
	if (0 == video.Fps || !CreateWriter( video.codecType )) {
		return E_INVALIDARG;
	}
	// init video & audio
	_video_params videoParams;
	videoParams.Codec = m_codecType;
	videoParams.FPS = video.Fps;
	videoParams.Width = video.Witdh;
	videoParams.Height = video.Height;

	_audio_params audioParams;
	audioParams.Codec = GetAudioCodec(audio.codecType);
	audioParams.SamplesPerSec = audio.SamplesPerSec;
	audioParams.BitsPerSample = audio.BitsPerSample;

	bool result = m_writer->CreateVideoFile(fileName.c_str(), videoParams, audioParams);
	if ((ex_data.data != nullptr) && (result))
	{
		result = m_writer->WriteExtraData(ex_data.data, ex_data.size);
	}
	HRESULT hRes = (result) ? S_OK : E_FAIL;
	return hRes;
}


HRESULT AviFileWriter::WriteVideoFrame( const char *buffer, unsigned int size, uint64_t time, bool keyframe )
{
	const unsigned char* data = reinterpret_cast<const unsigned char*>(buffer);
	unsigned long frameType = keyframe ? KeyFrameTypes::keyFrame : KeyFrameTypes::pFrame;
	bool result = m_writer->WriteVideoFrame(reinterpret_cast<LPBYTE>(const_cast<char *>(buffer)), size, time, frameType);
	HRESULT hRes = S_OK;
	if (!result) {
		DWORD error = ::GetLastError();
		hRes = (error != 0) ? HRESULT_FROM_WIN32(error) : E_FAIL;
	}
	return hRes; 
}


HRESULT AviFileWriter::WriteAudioFrame( const char *buffer, unsigned int size, uint64_t time )
{
	bool result = m_writer->WriteAudioFrame(reinterpret_cast<LPBYTE>(const_cast<char *>(buffer)), size, time);
	HRESULT hRes = S_OK;
	if (!result) {
		DWORD error = ::GetLastError();
		hRes = (error != 0) ? HRESULT_FROM_WIN32(error) : E_FAIL;
	}
	return hRes; 
}


HRESULT AviFileWriter::CloseFile()
{
	bool result = m_writer->CloseFile();
	HRESULT hRes = (result) ? S_OK : E_FAIL;
	return hRes; 
}


HRESULT AviFileWriter::SaveExtraData( const char *buffer, unsigned int size )
{
	if(ex_data.data != nullptr)
	{
		delete[] ex_data.data;
	}
	ex_data.data = new unsigned char[size];
	ex_data.size = size;
	memcpy(ex_data.data, buffer, size);
	return S_OK;
}

HRESULT AviFileWriter::SetExtraData( const char *buffer, unsigned int size )
{
	if ((m_codec == H264) || (m_codec == H265))
	{
		LPBYTE pData = reinterpret_cast<LPBYTE>(const_cast<char *>(buffer));
		bool res = m_writer->WriteExtraData(pData, size);
		return res ? S_OK : E_FAIL;
	}
	return S_FALSE;
}
