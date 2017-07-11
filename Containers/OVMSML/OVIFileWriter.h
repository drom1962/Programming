#include <windows.h>

#include <ArchiveData.h>

#include <container.h>

using namespace Archiv_space;

namespace Writer
{
class OviFileWriter
	{
private:
        Archiv m_Container;

		int		m_codec;
        
	public:
		OviFileWriter();
		~OviFileWriter();

		bool CreateWriter       (VideoCodecType vc);

		HRESULT InitFile        (const std::wstring& fileName, const VideoParameters& video, const AudioParameters& audio);

		HRESULT WriteVideoFrame (const char *buffer, unsigned int size, uint64_t time, bool keyframe);
		HRESULT WriteAudioFrame (const char *buffer, unsigned int size, uint64_t time);

		HRESULT SaveExtraData   (const char *buffer, unsigned int size);
		HRESULT SetExtraData    (const char *buffer, unsigned int size);

		bool IsNeedExtraData() const {
			return m_codec == H264;
		}
        
		HRESULT CloseFile();

		static const wchar_t * GetFileExtension() { return L"ovi"; }
	};
}