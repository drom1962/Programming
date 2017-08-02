#include "MediaWriter.h"
#include <Vfw.h>

#pragma comment (lib, "Wmvcore.lib")

#define VIDEO_STREAM_NAME		OLESTR("Video Stream")
#define AUDIO_STREAM_NAME		OLESTR("Audio Stream")
#define VIDEO_CONNECTION_NAME	OLESTR("Video")
#define AUDIO_CONNECTION_NAME	OLESTR("Audio")

#define BREAK_ON_FAIL(hr) \
	if (FAILED(hr)) { \
		break; } \

using namespace Writer;


inline DWORD GetMediaTypeSize(IWMMediaProps* pMediaProps)
{
	DWORD dwSize = 0;
	if (pMediaProps != NULL)
	{
		HRESULT hr = pMediaProps->GetMediaType(NULL, &dwSize);
		if (FAILED(hr))
		{
			return 0;
		}
	}
	// 
	return dwSize;
}

CWinMediaWriter::CWinMediaWriter(void)
{
	/* ЗНАЧЕНИЯ ПО УМОЛЧАНИЮ */
	// - Параметры видео...
	m_dwFPS = 15;
	m_wBitsPerPixel = 24;
	m_FrameSize.cx = 320;
	m_FrameSize.cy = 240;
	m_VideoSubtype = GUID_NULL;
	// - Параметры аудио...
	m_nChannels = 1;
	m_wBitsPerSample = 16;
	m_nSamplesPerSec = 22050;
	m_AudioSubtype = GUID_NULL;
	// Создание объекта...
	WMCreateWriter(NULL, &p);
	ATLASSERT(p != NULL);
	m_dwBitRate = 0;
	m_qwAudioWrittenBytes = 0;
}


CWinMediaWriter::~CWinMediaWriter(void)
{
}


HRESULT CWinMediaWriter::FindInputFormat(
							DWORD dwInput,
							GUID guidSubType,
							IWMInputMediaProps** ppProps)
{
	DWORD cFormats = 0;
	DWORD cbSize   = 0;

	WM_MEDIA_TYPE* pType = NULL;
	CComPtr<IWMInputMediaProps> pProps;

	// Set the ppProps parameter to point to NULL. This will
	//  be used to check the results of the function later.
	*ppProps = NULL;

	// Find the number of formats supported by this input.
	IWMWriter *& pWriter = p;
	HRESULT hr = pWriter->GetInputFormatCount(dwInput, &cFormats);

	do {
		BREAK_ON_FAIL(hr);
		// Loop through all of the supported formats.
		for (DWORD formatIndex = 0; formatIndex < cFormats; formatIndex++)
		{
			// Get the input media properties for the input format.
			hr = pWriter->GetInputFormat(dwInput, formatIndex, &pProps);
			BREAK_ON_FAIL(hr);
			// Get the size of the media type structure.
			hr = pProps->GetMediaType(NULL, &cbSize);
			BREAK_ON_FAIL(hr);
			// Allocate memory for the media type structure.
			pType = (WM_MEDIA_TYPE*) new (std::nothrow) BYTE[cbSize];
			if (pType == NULL)
			{
				hr = E_OUTOFMEMORY;
				break;
			}
			// Get the media type structure.
			hr = pProps->GetMediaType(pType, &cbSize);
			BREAK_ON_FAIL(hr);

			if(pType->subtype == guidSubType)
			{
				*ppProps = pProps.Detach();
				break;
			}
			// Clean up for next iteration.
			delete [] pType;
			pType = NULL;
		} // End for formatIndex.
		// If execution made it to this point, no matching format was found.
		if (!*ppProps)
			hr = NS_E_INVALID_INPUT_FORMAT;
	} while (false);

	delete [] pType;
	return hr;
}


DWORD CWinMediaWriter::MakeFOURCc( const AudioCodecType codecType )
{
	switch( codecType )
	{
		case PCM:   return WAVE_FORMAT_PCM;
		case AAC:   return WAVE_FORMAT_RAW_AAC1;
		case G726:  return WAVE_FORMAT_G726_ADPCM;
		case ALAW:  return WAVE_FORMAT_ALAW;
		case MULAW: return WAVE_FORMAT_MULAW;
		default:
			ATLTRACE("Writer: Audio FOURCC not found\n");
			return WAVE_FORMAT_UNKNOWN;
	}
}


HRESULT CWinMediaWriter::CreateCustomProfile()
{
	HRESULT hr = E_FAIL;
	CComPtr<IWMProfile> pProfile;

	hr = m_ProfileManager->CreateEmptyProfile(WMT_VER_9_0, &pProfile);

	if (pProfile != NULL)
	{
		// Добавление видео-потока...
		CComPtr<IWMStreamConfig> pVideoConfig;
		hr = pProfile->CreateNewStream(WMMEDIATYPE_Video, &pVideoConfig);
		if (pVideoConfig != NULL)
		{
			hr = ConfigureVideoStream(pVideoConfig);
			hr = pProfile->AddStream(pVideoConfig);
		}
		// Добавление аудио-потока...
		if( m_nChannels )
		{
			if ( LOWORD(GetAudioSubtype().Data1) == WAVE_FORMAT_RAW_AAC1 )
			{
				CComPtr<IWMStreamConfig> pAudioConfig;
				hr = m_ProfileManager.FindAudioStreamConfig(WMMEDIASUBTYPE_WMAudioV8, m_nChannels, m_nSamplesPerSec, m_wBitsPerSample, &pAudioConfig);
				if (SUCCEEDED(hr))
				{
					pAudioConfig->SetStreamNumber(AUDIO_STREAM_NUM);
					pAudioConfig->SetConnectionName(AUDIO_CONNECTION_NAME);
					pAudioConfig->SetStreamName(AUDIO_STREAM_NAME);
					hr = pProfile->AddStream(pAudioConfig);
				}
			}
			else {
				CComPtr<IWMStreamConfig> pAudioConfig;
				hr = pProfile->CreateNewStream(WMMEDIATYPE_Audio, &pAudioConfig);
				if (pAudioConfig != NULL)
				{
					hr = ConfigureAudioStream(pAudioConfig);
					hr = pProfile->AddStream(pAudioConfig);
				}
			}
		}
		// Запись параметров профиля...
		hr = p->SetProfile(pProfile);
	}
	// 
	return hr;
}

HRESULT CWinMediaWriter::CreateDefaultProfile()
{
	HRESULT hr = E_FAIL;
	CComPtr<IWMProfile> pProfile;

	hr = m_ProfileManager->CreateEmptyProfile(WMT_VER_9_0, &pProfile); //

	if (pProfile != NULL)
	{
		// Добавление видео-потока...
		CComPtr<IWMStreamConfig> pVideoConfig;
		hr = m_ProfileManager.FindVideoStreamConfig(WMMEDIASUBTYPE_WVC1, &pVideoConfig);

		if (pVideoConfig != NULL)
		{
			// Настройки видео...
			hr = ConfigureVideoStream(pVideoConfig);
			hr = pProfile->AddStream(pVideoConfig);
		}
		// Добавление аудио-потока...
		CComPtr<IWMStreamConfig> pAudioConfig;
		hr = m_ProfileManager.FindAudioStreamConfig(WMMEDIASUBTYPE_WMAudioV8, m_nChannels, m_nSamplesPerSec, m_wBitsPerSample, &pAudioConfig);

		if (pAudioConfig != NULL)
		{
			pAudioConfig->SetStreamNumber(AUDIO_STREAM_NUM);
			pAudioConfig->SetConnectionName(AUDIO_CONNECTION_NAME);
			pAudioConfig->SetStreamName(AUDIO_STREAM_NAME);
			hr = pProfile->AddStream(pAudioConfig);
		}
		// Запись параметров...
		hr = p->SetProfile(pProfile);
	}
	// 
	return hr;
}

HRESULT CWinMediaWriter::ConfigureAudioStream(IWMStreamConfig* pAudioConfig)
{
	HRESULT hr = E_FAIL;
	//
	if (m_AudioSubtype != GUID_NULL)
	{
		CComQIPtr<IWMMediaProps> pAudioProps = pAudioConfig;
		if (pAudioProps != NULL)
		{
			DWORD dwExtra = m_strExtraData[ AUDIO_STREAM_INDEX ].size();
			DWORD cbType = GetMediaTypeSize(pAudioProps);
			CTempBuffer<BYTE> TypeBuff( cbType + dwExtra );
			WM_MEDIA_TYPE* pMediaType = (WM_MEDIA_TYPE*)(LPBYTE)TypeBuff;
			if (pMediaType != NULL)
			{
				memset(TypeBuff, 0, cbType);
				// Измение текущих параметров аудио...
				pAudioProps->GetMediaType(pMediaType, &cbType);
				LPWAVEFORMATEX lpWave = (LPWAVEFORMATEX)pMediaType->pbFormat;
				hr = OnChangeAudioFormat(lpWave, pMediaType->cbFormat);
				//
				if (SUCCEEDED(hr)) {
					pMediaType->subtype = m_AudioSubtype;
					pMediaType->cbFormat += dwExtra;
					// Поток WMVCORE.DLL подвешивает процессор, если битрейт видео будет больше
					// битрейта аудио в несколько тысяч раз (наблюдается при 8000, начинает наблюдаться при 5000)
					//if (lpWave->nAvgBytesPerSec) {
					//	hr = pAudioConfig->SetBitrate( 8*lpWave->nAvgBytesPerSec );
					//}
					// Запись параметров...
					hr = pAudioProps->SetMediaType(pMediaType);
				}
			}
		}
		// Дополнительные параметры аудио-потока...
		hr = pAudioConfig->SetStreamNumber(AUDIO_STREAM_NUM);
		hr = pAudioConfig->SetConnectionName(AUDIO_CONNECTION_NAME);
		hr = pAudioConfig->SetStreamName(AUDIO_STREAM_NAME);
	}
	return hr;
}

HRESULT CWinMediaWriter::OnChangeAudioFormat(LPWAVEFORMATEX lpWaveForm, ULONG cbFormat)
{
	HRESULT hRes = E_INVALIDARG;

	if (lpWaveForm != NULL && cbFormat >= sizeof(WAVEFORMATEX))
	{
		if (m_AudioSubtype != GUID_NULL)
		{
			lpWaveForm->wFormatTag = LOWORD(m_AudioSubtype.Data1);
		}
		lpWaveForm->nChannels = m_nChannels;
		lpWaveForm->nSamplesPerSec = m_nSamplesPerSec;
		lpWaveForm->wBitsPerSample = m_wBitsPerSample;
		lpWaveForm->nBlockAlign = m_nChannels * m_wBitsPerSample / 8;
		lpWaveForm->nAvgBytesPerSec = lpWaveForm->nBlockAlign * m_nSamplesPerSec;

		DWORD dwExtra = 0;
		hRes = S_OK;

		if (lpWaveForm->wFormatTag != WAVE_FORMAT_PCM)
		{
			dwExtra = m_strExtraData[ AUDIO_STREAM_INDEX ].size();

			if (!dwExtra)
				hRes = S_FALSE;
			else if (cbFormat >= sizeof(WAVEFORMATEX) + dwExtra) {
				memcpy(
					reinterpret_cast<BYTE *>(lpWaveForm) + sizeof(WAVEFORMATEX),
					m_strExtraData[ AUDIO_STREAM_INDEX ].c_str(),
					dwExtra);
			} else {
				dwExtra = 0;
				hRes = E_FAIL;
			}
		}
		lpWaveForm->cbSize = static_cast<WORD>(dwExtra);
	}
	return hRes;
}

HRESULT CWinMediaWriter::ConfigureVideoStream(IWMStreamConfig* pVideoConfig)
{
	HRESULT hr = E_FAIL;
	CComQIPtr<IWMVideoMediaProps> pVideoProps = pVideoConfig;
	if (pVideoProps != NULL)
	{
		DWORD dwExtra = m_strExtraData[ VIDEO_STREAM_INDEX ].size();
		DWORD cbType = GetMediaTypeSize(pVideoProps);
		CTempBuffer<BYTE> TypeBuff(cbType + dwExtra);
		WM_MEDIA_TYPE* pMediaType = (WM_MEDIA_TYPE*)(LPBYTE)TypeBuff;
		if (pMediaType != NULL)
		{
			// Измение текущих параметров видео...
			hr = pVideoProps->GetMediaType(pMediaType, &cbType);
			WMVIDEOINFOHEADER* pVideoInfo = (WMVIDEOINFOHEADER*)pMediaType->pbFormat;
			OnChangeVideoFormat(pVideoInfo, pMediaType->cbFormat);
			//pMediaType->lSampleSize = pVideoInfo->bmiHeader.biSizeImage;
			if (m_VideoSubtype != GUID_NULL)
			{
				pMediaType->subtype = m_VideoSubtype;
				pMediaType->bTemporalCompression = true;
				pMediaType->bFixedSizeSamples = false;
				pMediaType->lSampleSize = 0;
				pMediaType->cbFormat += dwExtra;
			}
			if( !m_dwBitRate ) {
				// Set VBR flag
				CComQIPtr<IWMPropertyVault> pVideoProps(pVideoConfig);
				BYTE val = 1;
				hr = pVideoProps->SetProperty(g_wszVBREnabled, WMT_TYPE_BOOL, &val, 4);
				hr = pVideoConfig->SetBitrate( pVideoInfo->dwBitRate );
			}
			else {
				pVideoInfo->dwBitRate = m_dwBitRate * 1000;
				hr = pVideoConfig->SetBitrate( pVideoInfo->dwBitRate );
			}
			hr = pVideoProps->SetMediaType(pMediaType);
		}
		hr = pVideoConfig->SetStreamNumber(VIDEO_STREAM_NUM);
		hr = pVideoConfig->SetConnectionName(VIDEO_CONNECTION_NAME);
		hr = pVideoConfig->SetStreamName(VIDEO_STREAM_NAME);
	}
	//
	return hr;
}

void CWinMediaWriter::OnChangeVideoFormat(WMVIDEOINFOHEADER* pVideoInfo, ULONG cbFormat)
{
	if (pVideoInfo != NULL && cbFormat > 0)
	{
		RECT newSize = { 0, 0, m_FrameSize.cx, m_FrameSize.cy };
		// Инициализация структуры...
		pVideoInfo->rcSource = newSize;
		pVideoInfo->rcTarget = newSize;

		if (m_VideoSubtype != GUID_NULL) {
			pVideoInfo->bmiHeader.biCompression = m_VideoSubtype.Data1;
		}
		pVideoInfo->bmiHeader.biWidth  = m_FrameSize.cx;
		pVideoInfo->bmiHeader.biHeight = m_FrameSize.cy;
		pVideoInfo->bmiHeader.biBitCount = m_wBitsPerPixel;
		pVideoInfo->bmiHeader.biSizeImage = m_wBitsPerPixel * m_FrameSize.cx * m_FrameSize.cy;
		pVideoInfo->dwBitRate = pVideoInfo->bmiHeader.biSizeImage * m_dwFPS /* / 256*/; // Приблизительное значение
		pVideoInfo->bmiHeader.biSizeImage /= 8;
		pVideoInfo->AvgTimePerFrame = 10000000ULL / m_dwFPS;

		DWORD dwExtra = m_strExtraData[ VIDEO_STREAM_INDEX ].size();
		if (dwExtra > 0) {
			memcpy(
				reinterpret_cast<BYTE *>(pVideoInfo) + sizeof(WMVIDEOINFOHEADER),
				m_strExtraData[ VIDEO_STREAM_INDEX ].c_str(),
				dwExtra);
		}
	}
}


HRESULT CWinMediaWriter::StartWriting(const wchar_t *pwszFileName)
{
	HRESULT hr = ResetInputProps();
	if (FAILED(hr)) {
		return hr;
	}
	hr = p->SetOutputFilename(pwszFileName);
	if (FAILED(hr)) {
		return hr;
	}
	m_qwAudioWrittenBytes = 0;

	return p->BeginWriting();
}


HRESULT CWinMediaWriter::WriteRawAudio(QWORD qwTime, LPBYTE lpFrameData, DWORD dwDataSize)
{
	if (LOWORD(GetAudioSubtype().Data1) == WAVE_FORMAT_RAW_AAC1)
	{
		return WriteAudioData(lpFrameData, dwDataSize);
	}
	else
	{
		return WriteCompressedMedia(AUDIO_STREAM_NUM, qwTime, lpFrameData, dwDataSize);
	}
}


HRESULT CWinMediaWriter::WriteVideoFrame(UINT nFrame, LPBYTE lpFrameData, DWORD dwDataSize, DWORD dwFlags)
{
	if (lpFrameData == NULL)
	{
		return E_INVALIDARG;
	}
	// Создание буфера...
	HRESULT hr = E_FAIL;
	CComPtr<INSSBuffer> pSample;
	hr = p->AllocateSample(dwDataSize, &pSample);
	if (pSample != NULL)
	{
		LPBYTE lpBuffer = NULL;
		pSample->GetBuffer(&lpBuffer);
		if (lpBuffer != NULL)
		{
			memcpy(lpBuffer, lpFrameData, dwDataSize);
		}
		// Запись данных в поток...
		QWORD qwTime = 10000000ULL * nFrame / m_dwFPS;
		hr = p->WriteSample(VIDEO_STREAM_INDEX, qwTime, 0, pSample);
	}
	// 
	return hr;
}

HRESULT CWinMediaWriter::WriteAudioData(LPBYTE lpFrameData, DWORD dwDataSize)
{
	if (lpFrameData == NULL)
	{
		return E_INVALIDARG;
	}
	// Создание буфера...
	HRESULT hr = E_FAIL;
	CComPtr<INSSBuffer> pSample;
	hr = p->AllocateSample(dwDataSize, &pSample);
	if (pSample != NULL)
	{
		LPBYTE lpBuffer = NULL;
		pSample->GetBuffer(&lpBuffer);
		if (lpBuffer != NULL)
		{
			memcpy(lpBuffer, lpFrameData, dwDataSize);
		}
		// Запись в поток...
		const int nAveBytesRate = m_nChannels * m_wBitsPerSample * m_nSamplesPerSec / 8;
		QWORD qwTime = 10000000ULL * m_qwAudioWrittenBytes / nAveBytesRate;

		hr = p->WriteSample(AUDIO_STREAM_INDEX, qwTime, 0, pSample);

		if (SUCCEEDED(hr))
			m_qwAudioWrittenBytes += dwDataSize;
	}
	return hr;
}

HRESULT CWinMediaWriter::WriteCompressedMedia(WORD wStreamNum, QWORD qwTime, LPBYTE lpFrameData, DWORD dwDataSize, DWORD dwFlags)
{
	if (lpFrameData == NULL)
	{
		return E_INVALIDARG;
	}
	// Создание буфера...
	HRESULT hr = E_FAIL;
	CComPtr<INSSBuffer> pSample;
	hr = p->AllocateSample(dwDataSize, &pSample);
	if (pSample != NULL)
	{
		LPBYTE lpBuffer = NULL;
		hr = pSample->GetBuffer(&lpBuffer);
		if (lpBuffer != NULL)
		{
			memcpy(lpBuffer, lpFrameData, dwDataSize);
		}
		// Запись в поток...
		CComQIPtr<IWMWriterAdvanced> pWriterAdvanced(p);
		if (pWriterAdvanced != NULL)
		{
			hr = pWriterAdvanced->WriteStreamSample(wStreamNum, qwTime, 0, 0, dwFlags, pSample);
		}
	}
	//
	return hr;
}


HRESULT CWinMediaWriter::ResetInputProps()
{
	HRESULT hr(E_FAIL);
	DWORD dwInputs = 0;
	hr = p->GetInputCount(&dwInputs);
	for (DWORD i = 0; i < dwInputs; i++)
	{
		hr = p->SetInputProps(i, NULL);
	}
	// эти грабли нужны, чтобы для AAC звук в формате PCM (приходивший от таких же граблей в Манагере)
	// мог быть закодирован кодеком WMA (хоть какое, а решение проблемы с записью AAC в WMF )
	if ( WAVE_FORMAT_RAW_AAC1 == LOWORD(GetAudioSubtype().Data1) )
	{
		CComPtr<IWMInputMediaProps> pProps;
		CComPtr<IWMProfile> pProfile;

		hr = FindInputFormat(AUDIO_STREAM_INDEX, WMMEDIASUBTYPE_PCM, &pProps);

		if (pProps) {
			DWORD cbSize;
			hr = pProps->GetMediaType(NULL, &cbSize);

			if (SUCCEEDED(hr))
			{
				// Allocate memory for the media type structure.
				CTempBuffer<BYTE> pBuffer( cbSize );
				WM_MEDIA_TYPE* pType  = reinterpret_cast< WM_MEDIA_TYPE * >(static_cast<BYTE *>(pBuffer));
				if (pType == NULL) {
					hr = E_OUTOFMEMORY;
				}
				else {
					GUID oldSubType = GetAudioSubtype();
					// Get the media type structure.
					hr = pProps->GetMediaType(pType, &cbSize);
					WAVEFORMATEX *pWave = reinterpret_cast<WAVEFORMATEX *>(pType->pbFormat);
					SetAudioSubtype( GUID_NULL );
					hr = OnChangeAudioFormat(pWave, pType->cbFormat);
					SetAudioSubtype( oldSubType );
					// Set new media properties
					if (SUCCEEDED(hr))
						hr = p->SetInputProps(AUDIO_STREAM_INDEX, pProps);
				}
			}
		}
	}
	return hr;
}

bool CDeltaFramesWriter::SetExtraData(LPBYTE lpFrameData, DWORD dwDataSize)
{
	if (dwDataSize > 0)
	{
		if (m_dwExtraData != dwDataSize) {
			LPBYTE pBuffer = new BYTE [dwDataSize];
			if (!pBuffer) {
				return false;
			}
			m_pExtraData.Free();
			m_pExtraData.Attach(pBuffer);
			m_dwExtraData = dwDataSize;
		}
		memcpy(m_pExtraData, lpFrameData, dwDataSize);
		return true;
	}
	else if (!lpFrameData) {
		m_pExtraData.Free();
		m_dwExtraData = 0;
		return true;
	}
	return false;
}


HRESULT CMPEG4Writer::WriteVideoFrame(QWORD qwTime, LPBYTE lpFrameData, DWORD dwDataSize)
{
	DWORD dwFlags = 0, cbExtraData = 0;
	ATL::CAutoVectorPtr<BYTE> m_pExtraData;

	if (CMPEG4Writer::IsKeyFrame(lpFrameData, dwDataSize))
	{
		cbExtraData = GetExtraDataSize();
		// Add extra data in frame if it not found
		if (cbExtraData && !hasVOSHeader(lpFrameData, dwDataSize)/* && qwTime > 0*/)
		{
			m_pExtraData.Attach( new BYTE[dwDataSize + cbExtraData] );
			memcpy(m_pExtraData, GetExtraData(), cbExtraData);
			memcpy(m_pExtraData + cbExtraData, lpFrameData, dwDataSize);
			dwDataSize += cbExtraData;
			//WriteCompressedMedia(VIDEO_STREAM_NUM, qwTime, const_cast<LPBYTE>(GetExtraData()), cbExtraData, WM_SF_CLEANPOINT);
		}
		dwFlags = WM_SF_CLEANPOINT;
	}
	return WriteCompressedMedia(VIDEO_STREAM_NUM, qwTime, m_pExtraData ? m_pExtraData : lpFrameData, dwDataSize, dwFlags);
}


int CMPEG4Writer::hasVOSHeader(LPBYTE lpFrameData, DWORD dwDataSize) const
{
	if (!lpFrameData)
		return -1;
	LPBYTE const endFrame = lpFrameData + dwDataSize - 4 + 1;
	// Find VOS header
	while (lpFrameData < endFrame) {
		if (*lpFrameData == 0x00 &&
			*(lpFrameData + 1) == 0x00 &&
			*(lpFrameData + 2) == 0x01)
		{
			return 0xB0 == *(lpFrameData + 3) ? 1 : 0;
		}
		++lpFrameData;
	}
	return -1;
}