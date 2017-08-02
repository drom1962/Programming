#pragma once
#include "ArchiveFileWriter.h"
#include "MediaWriter.h"


namespace Writer
{
	class AsfFileWriter
	{
	public:
		AsfFileWriter() : m_codec(NONE), m_extraDataSupport(NULL) {}

		~AsfFileWriter() {}

		bool CreateWriter(VideoCodecType vc);

		HRESULT InitFile(const std::wstring& fileName, const VideoParameters& video, const AudioParameters& audio);

		HRESULT WriteVideoFrame(const char *buffer, unsigned int size, uint64_t time, bool keyframe);
		HRESULT WriteAudioFrame(const char *buffer, unsigned int size, uint64_t time);

		HRESULT SaveExtraData(const char *buffer, unsigned int size) {
			return SetExtraData(buffer, size);
		}
		HRESULT SetExtraData(const char *buffer, unsigned int size);

		bool IsNeedExtraData() const {
			return m_extraDataSupport != NULL;
		}
		HRESULT CloseFile();

		static const wchar_t * GetFileExtension() { return L"asf"; }

	private:
		HRESULT OpenFile();
		bool HasExtraData() const;
		void Swap(AsfFileWriter & writer);

	private:
		std::auto_ptr< CBaseWriter > m_writer;
		std::wstring m_fileName;
		VideoCodecType m_codec;
		CDeltaFramesWriter *m_extraDataSupport;
	};
}