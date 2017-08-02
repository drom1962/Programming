#include "StgFileWriter.h"
#include "..\..\..\..\MediaStorage\MediaContainer\MediaContainer.h"

#pragma comment(lib, "MediaContainer")

using namespace Writer;

//========================================
// Recording via MediaContainer.dll (storage)
//========================================

MediaContainerWriter::MediaContainerWriter() : containerID_(0)
{
}

MediaContainerWriter::~MediaContainerWriter()
{
}

HRESULT MediaContainerWriter::InitFile(const std::wstring& fileName, const VideoParameters& video, const AudioParameters& audio)
{
	//OutputDebugStringA("INIT_FILE!!!!!");	
	fileName_ = fileName;
	containerID_ = CreateContainer(fileName.c_str());
	if (containerID_ < 0) 
	{
		return E_FAIL;
	}

	if(ex_data.data != NULL)
		SetExtraData((const char *)ex_data.data, ex_data.size);

	BOOL isSet = FALSE;
	std::wstring CodecType = L"";
	codec_ = video.codecType;
	switch(video.codecType)
	{
	case H264:
		CodecType = L"H.264";
		break;
	case MJPEG:
		CodecType = L"MJPG";
		break;
	case MPEG4:
		CodecType = L"MPEG4";
		break;
	default:
		return E_FAIL;
	}
	isSet |= SetVideoFormat(containerID_, CodecType.c_str());
	isSet |= SetVideoParameters(containerID_, video.Fps, video.BitsPerPixel, video.BitRate, 0);
	isSet |= SetVideoResolution(containerID_, video.Witdh, video.Height);

	return ((containerID_ >= 0) && (isSet)) ? S_OK : E_FAIL;
}

HRESULT MediaContainerWriter::SetExtraData(const char *buffer, unsigned int size)
{	
	void* extra_data = reinterpret_cast<void*>(const_cast<char*>(buffer));
	return SetVideoExtraData(containerID_, extra_data, size) ? S_OK : E_FAIL;
}

HRESULT MediaContainerWriter::CloseFile()
{
	BOOL isEnded = ::EndRecording(containerID_);
	BOOL isClosed = ::CloseContainer(containerID_);
	containerID_ = 0;
	firstFrame_ = true;
	isBegan_ = false;
	return (isClosed && isEnded) ? S_OK : E_FAIL;
}

HRESULT MediaContainerWriter::WriteVideoFrame( const char *buffer, unsigned int size, uint64_t time, bool keyframe )
{
	if (firstFrame_)
	{
		isBegan_ = ::BeginRecording(containerID_) != FALSE;
		if (isBegan_)
		{
			firstFrame_ = false;
		}
	}
	const unsigned char* data = reinterpret_cast<const unsigned char*>(buffer);
	void* frame = reinterpret_cast<void*>(const_cast<char*>(buffer));
	BOOL isWritten = ::WriteVideoFrame(containerID_, time, frame, size, keyframe);
	return (isWritten && isBegan_) ? S_OK : E_FAIL;
}

HRESULT MediaContainerWriter::WriteAudioFrame( const char *buffer, unsigned int size, uint64_t time )
{
	return S_OK;
}