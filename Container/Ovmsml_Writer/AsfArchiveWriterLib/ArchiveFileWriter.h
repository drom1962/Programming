#pragma once
#include <string>
#include <memory>
#include <stdint.h>
#include "ArchiveData.h"
#include <Windows.h>


namespace Writer
{
	typedef struct EXTRA_DATA_
	{
		unsigned char *data;
		int size;
	}EXTRA_DATA;

#define HR_STUB_IMPL { return S_FALSE; }

	// Interface definition (static)
	class DummyWriter
	{
	public:
		static const wchar_t *dummyExtension;
	public:
		DummyWriter(){}
		HRESULT InitFile( const std::wstring& fileName, const VideoParameters& video, const AudioParameters& audio ) HR_STUB_IMPL;
		HRESULT WriteVideoFrame( const char *buffer, unsigned int size, uint64_t time, bool keyframe) HR_STUB_IMPL;
		HRESULT WriteAudioFrame( const char *buffer, unsigned int size, uint64_t time ) HR_STUB_IMPL;
		HRESULT SaveExtraData( const char *buffer, unsigned int size ) HR_STUB_IMPL;
		HRESULT SetExtraData( const char *buffer, unsigned int size ) HR_STUB_IMPL;
		HRESULT CloseFile() HR_STUB_IMPL;

		bool CreateWriter(VideoCodecType vc)		{ return true; }
		bool IsNeedExtraData() const				{ return false; }
		static const wchar_t * GetFileExtension()	{ return dummyExtension; }
	};

#undef HR_STUB_IMPL
}