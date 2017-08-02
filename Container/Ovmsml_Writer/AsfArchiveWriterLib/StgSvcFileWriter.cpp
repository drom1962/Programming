#include "StgSvcFileWriter.h"
#include <boost/bind.hpp>
#include <functional>
#include <numeric>
#include <atlcomcli.h>
#include <atlsafe.h>

const int MAX_SPLIT_MJPEG_FRAMES_COUNT = 25;
const int MAX_SPLIT_AUDIO_SEQ_SIZE = 16384;
const int MAX_SPLIT_IDR_CODECS_FRAMES_COUNT = 100;
const bool groupFrames = true;

using namespace Writer;

//========================================
// Recording via STG service
//========================================

MediaContainerServiceWriter::MediaContainerServiceWriter()
{
	videoGroupLimit_ = 0;
	audioGroupMaxBytes_ = 0;
}


HRESULT MediaContainerServiceWriter::InitFile(const std::wstring& fileName, const VideoParameters& video, const AudioParameters& audio)
{
	//OutputDebugStringA("INIT_FILE!!!!!");	
	fileName_ = fileName;

	CComPtr< IStgServiceObject > stgServ;
	HRESULT hr = stgServ.CoCreateInstance(CLSID_StgServiceObject);
	if (FAILED(hr)) {
		return hr;
	}
	hr = stgServ->CreateContainer(CComBSTR(fileName.c_str()), &containerID_);
	if (FAILED(hr))  {
		return hr;
	}
	else if (!containerID_) {
		return E_NOINTERFACE;
	}
	if(ex_data.data != NULL) {
		SetExtraData((const char *)ex_data.data, ex_data.size);
	}
	BOOL isSet = FALSE;
	std::wstring CodecType = L"";
	codec_ = video.codecType;
	videoGroupLimit_ = 0;   // for IDR-codecs (MPEG4, H.264)
	audioGroupMaxBytes_ = (audio.codecType != MUTE) ? MAX_SPLIT_AUDIO_SEQ_SIZE : 0;

	switch(video.codecType)
	{
	case H264:
		CodecType = L"H.264";
		break;
	case MJPEG:
		CodecType = L"MJPG";
		videoGroupLimit_ = (video.Fps > 0) ? video.Fps : MAX_SPLIT_MJPEG_FRAMES_COUNT;
		break;
	case MPEG4:
		CodecType = L"MPEG4";
		break;
	default:
		return E_FAIL;
	}
	isSet |= SUCCEEDED( containerID_->SetVideoFormat(CComBSTR(CodecType.c_str())) );
	isSet |= SUCCEEDED( containerID_->SetVideoParameters(video.Fps, video.BitsPerPixel, video.BitRate, 0) );
	isSet |= SUCCEEDED( containerID_->SetVideoResolution(video.Witdh, video.Height) );
	return isSet ? S_OK : E_FAIL;
}

HRESULT MediaContainerServiceWriter::SetExtraData(const char *buffer, unsigned int size)
{
	if (!containerID_) {
		return E_NOINTERFACE;
	}
	if (!size) {
		return S_FALSE;
	}
	CComSafeArray<BYTE> extra_data;
	extra_data.Add(size, reinterpret_cast<byte *>(const_cast<char*>(buffer)), false);
	return containerID_->SetVideoExtraData(extra_data);
}

template<bool video, class Cond>
inline HRESULT SerializeGroupIf(CFrameDataList & group, IMediaStorage *container, Cond cond)
{
	if (cond()) {
		CComPtr<IStream> seq;
		group.Serialize(&seq);
		if (seq) {
			return video ?  container->WriteVideoSerialData(seq) :
				container->WriteAudioSerialData(seq);
		} else
			return E_POINTER;
	}
	return S_FALSE;
}

template<bool video>
inline HRESULT SerializeGroup(CFrameDataList & group, IMediaStorage *container) {
	using namespace boost;
	return SerializeGroupIf< video >(group, container, bind(&CFrameDataList::Count, ref(group)) != 0);
}

HRESULT MediaContainerServiceWriter::CloseFile()
{
	if (!containerID_) {
		return E_NOINTERFACE;
	}
	HRESULT isEnded;
	isEnded = SerializeGroup<true >(videoGroup_, containerID_);
	isEnded = SerializeGroup<false>(audioGroup_, containerID_);
	isEnded = containerID_->EndRecording();
	containerID_.Release();
	firstFrame_ = true;
	isBegan_ = false;
	videoGroup_.Clear();
	audioGroup_.Clear();
	return isEnded;
}

HRESULT MediaContainerServiceWriter::WriteVideoFrame( const char *buffer, unsigned int size, uint64_t time, bool keyframe )
{
	if (!containerID_) {
		return E_NOINTERFACE;
	}
	if (firstFrame_)
	{
		HRESULT hr = containerID_->BeginRecording();
		if (FAILED(hr)) {
			return hr;
		}
		firstFrame_ = false;
		isBegan_ = true;
	}
	const bool & keyFlag(keyframe);
	const unsigned char* data = reinterpret_cast<const unsigned char*>(buffer);
	if (!size) {
		return E_INVALIDARG;
	}
	HRESULT hr(S_OK);

	if (groupFrames)
	{
		hr = SerializeGroupIf<true>(videoGroup_, containerID_,
			[this, keyFlag] () -> bool
		{
			const UINT count = videoGroup_.Count();
			return (keyFlag || count >= MAX_SPLIT_IDR_CODECS_FRAMES_COUNT) && (count > videoGroupLimit_);
		});
		videoGroup_.AddFrameData(keyFlag, time, size, reinterpret_cast<byte *>(const_cast<char*>(buffer)));
	} else {
		CComSafeArray<BYTE> frame;
		frame.Add(size, reinterpret_cast<byte *>(const_cast<char*>(buffer)), false);
		hr = containerID_->WriteVideoFrame(time, frame, keyFlag);
	}
	return hr;
}

HRESULT MediaContainerServiceWriter::WriteAudioFrame( const char *buffer, unsigned int size, uint64_t time )
{
	if (!containerID_) {
		return E_NOINTERFACE;
	}
	if (!size) {
		return E_INVALIDARG;
	}
	HRESULT hr(S_OK);

	if (groupFrames && audioGroupMaxBytes_ > 0)
	{
		auto count = audioGroup_.AddFrameData(false, time, size, reinterpret_cast<byte *>(const_cast<char*>(buffer)));
		size_t groupSize = std::accumulate(audioGroup_.Begin(), audioGroup_.End(), 0,
			boost::bind(std::plus< size_t >(), _1,
				boost::bind(&CFrameData::GetFrameSize, _2)));
		hr = SerializeGroupIf<false>(audioGroup_, containerID_,
			[this, groupSize]
		{
			return groupSize >= audioGroupMaxBytes_;
		});
	} else {
		CComSafeArray<BYTE> frame;
		frame.Add(size, reinterpret_cast<byte *>(const_cast<char*>(buffer)), false);
		hr = containerID_->WriteAudioFragment(time, frame, false);
	}
	return hr;
}