#pragma once
#include "StgFileBase.h"


namespace Writer
{
	class MediaContainerWriter : public MediaContainerBase
	{
	public:
		MediaContainerWriter();
		~MediaContainerWriter();

		HRESULT InitFile(const std::wstring& fileName, const VideoParameters& video, const AudioParameters& audio);
		HRESULT WriteVideoFrame(const char *buffer, unsigned int size, uint64_t time, bool keyframe);
		HRESULT WriteAudioFrame(const char *buffer, unsigned int size, uint64_t time);
		HRESULT SetExtraData(const char *buffer, unsigned int size);
		HRESULT CloseFile();
	private:
		int containerID_;
	};

	// alias class
	class StgFileWriter : public MediaContainerWriter {};
}