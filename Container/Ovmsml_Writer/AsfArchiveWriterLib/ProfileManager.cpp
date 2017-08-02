#include "ProfileManager.h"
#include <atlstr.h>

using namespace Writer;


CProfileManager::CProfileManager(void)
{
	WMCreateProfileManager(&p);
	ATLASSERT(p != NULL);
}


CProfileManager::~CProfileManager(void)
{
}

CString CProfileManager::GetProfileString(IWMProfile* pProfile)
{
	CString strData;
	if (p != NULL && pProfile != NULL)
	{
		DWORD cbLength = 0;
		p->SaveProfile(pProfile, NULL, &cbLength);
		CTempBuffer<WCHAR> data(cbLength);
		p->SaveProfile(pProfile, data, &cbLength);
		strData = CString(data, cbLength);
	}
	// 
	return strData;
}

HRESULT CProfileManager::FindVideoStreamConfig(const GUID& MediaSubtype, IWMStreamConfig **ppIStreamConfig)
{
	HRESULT hr = E_FAIL;
	if (ppIStreamConfig == NULL)
	{
		return E_INVALIDARG;
	}
	// Codec information...
	CComQIPtr<IWMCodecInfo> pCodecInfo(p);
	if (pCodecInfo != NULL)
	{
		DWORD dwCodecs = 0;
		hr = pCodecInfo->GetCodecInfoCount(WMMEDIATYPE_Video, &dwCodecs);
		for (DWORD i = 0; i < dwCodecs; i++)
		{
			// Codec formats...
			DWORD dwFormats = 0;
			hr = pCodecInfo->GetCodecFormatCount(WMMEDIATYPE_Video, i, &dwFormats);
			if (dwFormats > 0)
			{
				CComPtr<IWMStreamConfig> pStreamConfig;
				hr = pCodecInfo->GetCodecFormat(WMMEDIATYPE_Video, i, 0, &pStreamConfig);
				if (pStreamConfig != NULL)
				{
					// Media properties...
					CComQIPtr<IWMMediaProps> pMediaProps = pStreamConfig;
					if (pMediaProps != NULL)
					{
						DWORD dwTypeSize = 0;
						hr = pMediaProps->GetMediaType(NULL, &dwTypeSize);
						CTempBuffer<BYTE> TypeBuff(dwTypeSize);
						WM_MEDIA_TYPE* pMediaType = reinterpret_cast<WM_MEDIA_TYPE*>((LPBYTE)TypeBuff);
						hr = pMediaProps->GetMediaType(pMediaType, &dwTypeSize);
						if (pMediaType->subtype == MediaSubtype)
						{
							hr = pCodecInfo->GetCodecFormat(WMMEDIATYPE_Video, i, 0, ppIStreamConfig);
							break;
						}
					}
				}
			}
		}
	}
	//
	return hr;
}


HRESULT CProfileManager::FindAudioStreamConfig(const GUID& MediaSubtype, WORD nChannels, DWORD nSamplesPerSec, WORD wBitsPerSample, IWMStreamConfig **ppIStreamConfig)
{
	HRESULT hr = E_FAIL;
	if (ppIStreamConfig == NULL)
	{
		return E_INVALIDARG;
	}
	// Перебор всех кодеков...
	CComQIPtr<IWMCodecInfo> pCodecInfo(p);
	if (pCodecInfo != NULL)
	{
		BOOL found = FALSE;
		DWORD dwCodecs = 0;
		hr = pCodecInfo->GetCodecInfoCount(WMMEDIATYPE_Audio, &dwCodecs);
		while (dwCodecs > 0 && !found)
		{
			dwCodecs--;
			// Форматы кодеков...
			DWORD dwFormats = 0;
			hr = pCodecInfo->GetCodecFormatCount(WMMEDIATYPE_Audio, dwCodecs, &dwFormats);
			for (DWORD n = 0; n < dwFormats; n++)
			{
				CComPtr<IWMStreamConfig> pConfig;
				hr = pCodecInfo->GetCodecFormat(WMMEDIATYPE_Audio, dwCodecs, n, &pConfig);
				if (pConfig != NULL)
				{
					// Свойства формата аудио-енкодера...
					CComQIPtr<IWMMediaProps> pMediaProps = pConfig;
					if (pMediaProps != NULL)
					{
						DWORD dwTypeSize = 0;
						hr = pMediaProps->GetMediaType(NULL, &dwTypeSize);
						CTempBuffer<BYTE> TypeBuff(dwTypeSize);
						WM_MEDIA_TYPE* pMediaType = reinterpret_cast<WM_MEDIA_TYPE*>((LPBYTE)TypeBuff);
						hr = pMediaProps->GetMediaType(pMediaType, &dwTypeSize);
						WAVEFORMATEX* pWaveEx = (WAVEFORMATEX*)pMediaType->pbFormat;

						if (pWaveEx != NULL && (MediaSubtype == GUID_NULL || MediaSubtype == pMediaType->subtype) &&
							pWaveEx->nChannels == nChannels &&
							pWaveEx->nSamplesPerSec == nSamplesPerSec &&
							pWaveEx->wBitsPerSample == wBitsPerSample)
						{
							hr = pCodecInfo->GetCodecFormat(WMMEDIATYPE_Audio, dwCodecs, n, ppIStreamConfig);
							found = TRUE;
							break;
						}
					}
				}
			}
		}
		return found ? S_OK : NS_E_INVALID_INPUT_FORMAT;
	}
	// 
	return hr;
}

