#include "AsfFileWriter.h"

using namespace Writer;


bool AsfFileWriter::CreateWriter(VideoCodecType vc)
{
	if (NONE == m_codec || m_codec != vc)
	{
		switch (vc)
		{
		case MJPEG:
			m_writer.reset( new CMJPEGWriter() );
			m_extraDataSupport = NULL;
			break;
		case MPEG4:
			m_writer.reset( new CMPEG4Writer() );
			m_extraDataSupport = static_cast<CDeltaFramesWriter *>( m_writer.get() );
			break;
		case H264:
			m_writer.reset( new CH264Writer() );
			m_extraDataSupport = static_cast<CDeltaFramesWriter *>( m_writer.get() );
			break;
		default:
			m_writer.reset();
			m_extraDataSupport = NULL;
			vc = NONE;
		}
		m_codec = vc;
	}
	return m_writer.get() != NULL;
}


HRESULT AsfFileWriter::InitFile( const std::wstring& fileName, const VideoParameters& video, const AudioParameters& audio )
{
	if (0 == video.Fps || !CreateWriter( video.codecType )) {
		return E_INVALIDARG;
	}
	m_fileName = fileName;
	// init video
	m_writer->SetBitsPerPixel( video.BitsPerPixel );
	m_writer->SetBitrate( video.BitRate );
	// init audio
	m_writer->SetAudioSubtype( m_writer->MakeSubtype( CWinMediaWriter::MakeFOURCc( audio.codecType ) ) );
	m_writer->SetChannels( audio.Channels );
	m_writer->SetBitsPerSample( audio.BitsPerSample );
	m_writer->SetSamplesPerSec( audio.SamplesPerSec );

	if (AAC == audio.codecType) {
		m_writer->SetCodecPrivateData(reinterpret_cast< BYTE * >("\x15\x88"), 2, CWinMediaWriter::Audio);   // set AudioSpecificConfig
	}
	// final profile
	HRESULT hRes = m_writer->CreateProfile( video.Witdh, video.Height, video.Fps );

	if SUCCEEDED(hRes) {
		hRes = OpenFile();
		// необходимо закрыть файл в случае ошибки (для WMF возникнут проблемы и запись уже не начнётся)
		if (FAILED(hRes)) {
			CloseFile();
		}
	}
	return hRes;
}


HRESULT AsfFileWriter::OpenFile()
{
	HRESULT hRes = m_writer->StartWriting(m_fileName.c_str());

	if (SUCCEEDED(hRes) && m_extraDataSupport) {
		hRes = m_extraDataSupport->WriteExtraData();
	}
	return hRes;
}


HRESULT AsfFileWriter::WriteVideoFrame( const char *buffer, unsigned int size, uint64_t time, bool keyframe )
{
	return m_writer->WriteVideoFrame( time, reinterpret_cast<LPBYTE>(const_cast<char *>(buffer)), size );
}

HRESULT AsfFileWriter::WriteAudioFrame( const char *buffer, unsigned int size, uint64_t time )
{
	return m_writer->WriteAudioFrame( time, reinterpret_cast<LPBYTE>(const_cast<char *>(buffer)), size );
}


HRESULT AsfFileWriter::CloseFile()
{
	return m_writer->EndWriting();
}


HRESULT AsfFileWriter::SetExtraData( const char *buffer, unsigned int size )
{
	if (m_extraDataSupport) {
		LPBYTE pData = reinterpret_cast<LPBYTE>(const_cast<char *>(buffer));
		m_writer->SetCodecPrivateData( pData, size, CWinMediaWriter::Video );
		return m_extraDataSupport->SetExtraData( pData, size ) ? S_OK : E_FAIL;
	}
	return S_FALSE;
}


bool AsfFileWriter::HasExtraData() const
{
	return m_extraDataSupport && m_extraDataSupport->GetExtraDataSize();
}


void AsfFileWriter::Swap(AsfFileWriter & writer)
{
	std::swap(m_writer, writer.m_writer);
	m_fileName.swap(writer.m_fileName);
	std::swap(m_codec, writer.m_codec);
	std::swap(m_extraDataSupport, writer.m_extraDataSupport);
}