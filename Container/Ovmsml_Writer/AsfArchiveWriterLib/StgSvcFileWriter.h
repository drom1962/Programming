#pragma once
#include "StgFileBase.h"
#include "..\..\..\..\MediaStorage\StorageService\StorageService_i.h"
#include "..\..\..\..\MediaStorage\StorageService\VideoStorage\SerialDataStream.h"

namespace Writer
{
	class MediaContainerServiceWriter : public MediaContainerBase
	{
	public:
		MediaContainerServiceWriter();
		~MediaContainerServiceWriter() {}

		HRESULT InitFile(const std::wstring& fileName, const VideoParameters& video, const AudioParameters& audio);
		HRESULT WriteVideoFrame(const char *buffer, unsigned int size, uint64_t time, bool keyframe);
		HRESULT WriteAudioFrame(const char *buffer, unsigned int size, uint64_t time);
		HRESULT SetExtraData(const char *buffer, unsigned int size);
		HRESULT CloseFile();

	private:
		ATL::CComPtr< IMediaStorage > containerID_;
		CFrameDataList videoGroup_;
		CFrameDataList audioGroup_;
		size_t videoGroupLimit_;
		size_t audioGroupMaxBytes_;
	};

	class StgSvcFileWriter : public MediaContainerServiceWriter {}; 
}