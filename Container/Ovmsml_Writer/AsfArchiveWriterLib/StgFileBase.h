#pragma once
#include "ArchiveFileWriter.h"


namespace Writer
{
	class MediaContainerBase
	{
	public:
		bool CreateWriter(VideoCodecType vc);

		HRESULT SaveExtraData(const char *buffer, unsigned int size);
		bool IsNeedExtraData() const;

		static const wchar_t * GetFileExtension() { return L"stg"; }

	protected:
		MediaContainerBase();
		~MediaContainerBase();

	protected:
		VideoCodecType codec_;
		std::wstring fileName_;
		bool firstFrame_;
		bool isBegan_;
		EXTRA_DATA ex_data;
	};
}