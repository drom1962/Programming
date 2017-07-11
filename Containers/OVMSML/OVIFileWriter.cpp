#include "OVIFileWriter.h"

OviFileWriter::OviWriter()
	{
	m_Container=new Archive_space::OVI(1);
	}

OviFileWriter::~OviWriter()
	{

	}

bool OviFileWriter::CreateWriter(VideoCodecType vc)
	{
	return true;
	}

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

	std::wstring CodecType = L"";
	codec_ = video.codecType;

	VideoParameters vp;
	memcpy(&vp, &video, sizeof(vp));
	result = m_Container->SetVideoParameters(&vp);

	if (result < 0) return E_FAIL;

	AudioParameters ap;
	memcpy(&ap, &audio, sizeof(ap));
	//m_Container->SetAudioParameters(&ap);

	if (result < 0) return E_FAIL;

	return S_OK;
}

HRESULT OviFileWriter::SaveExtraData(const char *buffer, unsigned int size)
	{
	if(size>255) return E_FAIL;

	memcpy(m_ED, buffer, size);

	m_sizeED=size;
	
	return S_OK;
	}

HRESULT OviFileWriter::SetExtraData(const char *buffer, unsigned int size)
{
	return m_Container->SetExtraData((unsigned char *)buffer,size);
}

HRESULT OviFileWriter::CloseFile()
{
	return m_Container->Close(nullptr) >= 0 ? S_OK : E_FAIL;
}

HRESULT OviFileWriter::WriteVideoFrame( const char *buffer, unsigned int size, uint64_t time, bool keyFlag )
{
	if (!size) {
		return E_INVALIDARG;
	}
	int res = m_Container->WriteVideoFrame((unsigned char *)buffer,size,keyFlag,time,nullptr, 0);
	return 0==res ? S_OK : STG_E_WRITEFAULT;
}

HRESULT OviFileWriter::WriteAudioFrame( const char *buffer, unsigned int size, uint64_t time )
{
	return S_OK;
}

bool OviWriter::IsNeedExtraData() const
{
	return true;
}
