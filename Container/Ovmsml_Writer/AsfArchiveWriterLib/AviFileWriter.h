#pragma once
#include "ArchiveFileWriter.h"

// forward
class IAviFile;


namespace Writer
{
	class AviFileWriter
	{
	public:
		AviFileWriter();
		~AviFileWriter();

		bool CreateWriter(VideoCodecType vc);

		HRESULT InitFile(const std::wstring& fileName, const VideoParameters& video, const AudioParameters& audio);

		HRESULT WriteVideoFrame(const char *buffer, unsigned int size, uint64_t time, bool keyframe);
		HRESULT WriteAudioFrame(const char *buffer, unsigned int size, uint64_t time);

		HRESULT SaveExtraData(const char *buffer, unsigned int size);
		HRESULT SetExtraData(const char *buffer, unsigned int size);

		bool IsNeedExtraData() const {
			return (m_codec == H264) || (m_codec == H265);
		}
		HRESULT CloseFile();

		static const wchar_t * GetFileExtension() { return L"avi"; }

	private:
		static unsigned long GetAudioCodec(AudioCodecType codec);

	private:
		IAviFile* m_writer;
		EXTRA_DATA ex_data;
		VideoCodecType m_codec;
		unsigned long m_codecType;
	};
}