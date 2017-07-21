#include <windows.h>

#include <ArchiveData.h>

#include <container.h>

using namespace Archive_space;
using namespace Meta_Data;

namespace Writer
{
class OviFileWriter
	{
private:
        Archiv			*m_Container;

		MetaData		*m_MetaData;
	
		int				m_codec;
        
		unsigned char	m_ED[1024];
		int				m_sizeED;

		std::wstring	m_NameFile;

	public:
		OviFileWriter();
		~OviFileWriter();

		bool CreateWriter       (VideoCodecType vc);

		HRESULT InitFile        (const std::wstring& fileName, const VideoParameters& video, const AudioParameters& audio);

		HRESULT WriteVideoFrame (const char *buffer, unsigned int size, uint64_t time, bool keyframe);
		HRESULT WriteAudioFrame (const char *buffer, unsigned int size, uint64_t time);

		HRESULT SaveExtraData   (const char *buffer, unsigned int size);
		HRESULT SetExtraData    (const char *buffer, unsigned int size);

		bool IsNeedExtraData();
        
		HRESULT CloseFile();

		static const wchar_t * GetFileExtension() { return L"ovi"; }

		HRESULT WriteMetaData(const char *buffer, unsigned int size, uint64_t time);
	};
}