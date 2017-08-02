#pragma once
#include <wmsdk.h>
#include <atlcomcli.h>
#include <atlstr.h>

namespace Writer
{

class CProfileManager : public ATL::CComPtr<IWMProfileManager>
{
public:
	CProfileManager(void);
	virtual ~CProfileManager(void);
	ATL::CString GetProfileString(IWMProfile* pProfile);
	HRESULT FindVideoStreamConfig(const GUID& MediaSubtype, IWMStreamConfig **ppIStreamConfig);
	HRESULT FindAudioStreamConfig(const GUID& MediaSubtype, WORD nChannels, DWORD nSamplesPerSec, WORD wBitsPerSample, IWMStreamConfig **ppIStreamConfig);
};

}