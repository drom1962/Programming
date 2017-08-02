#pragma once
#include "ArchiveData.h"
#include "ProfileManager.h"
#include "../../../../common/kind_of_video_frames.h"
#include "ArchiveData.h"
#include "CodecConstants.h"
#include <MMSystem.h>
#include <string>

#ifndef WAVE_FORMAT_RAW_AAC1
#define WAVE_FORMAT_RAW_AAC1 0xFF
#endif //WAVE_FORMAT_RAW_AAC1

namespace Writer
{

enum CodecTypes{ CMJPEG, CMPEG4, CH264 };

class CWinMediaWriter : public ATL::CComPtr<IWMWriter>
{
public:
	static const int VIDEO_STREAM_NUM = 1;
	static const int AUDIO_STREAM_NUM = 2;
	static const int VIDEO_STREAM_INDEX = VIDEO_STREAM_NUM - 1;
	static const int AUDIO_STREAM_INDEX = AUDIO_STREAM_NUM - 1;

	enum CodecType { Video, Audio };

	CWinMediaWriter(void);
	virtual ~CWinMediaWriter(void);
	
	HRESULT CreateDefaultProfile();
	HRESULT CreateCustomProfile();

	HRESULT WriteVideoFrame(UINT nFrame, LPBYTE lpFrameData, DWORD dwDataSize, DWORD dwFlags = 0);
	HRESULT WriteAudioData(/*UINT nFrame, DWORD dwDuration,*/ LPBYTE lpFrameData, DWORD dwDataSize);
	HRESULT WriteCompressedMedia(WORD wStreamNum, QWORD qwTime, LPBYTE lpFrameData, DWORD dwDataSize, DWORD dwFlags = 0);
	HRESULT WriteRawAudio(QWORD qwTime, LPBYTE lpFrameData, DWORD dwDataSize);
	HRESULT ResetInputProps();
	
	DWORD GetFPS() const { return m_dwFPS; }
	void SetFPS(DWORD dwFPS) { m_dwFPS = dwFPS;	}
	
	SIZE GetFrameSize() const { return m_FrameSize; }
	void SetFrameSize(LONG cx, LONG cy) { m_FrameSize.cx = cx, m_FrameSize.cy = cy; }
	
	WORD GetChannels() const { return m_nChannels; }
	void SetChannels(WORD nChannels) { m_nChannels = nChannels; }

	DWORD GetSamplesPerSec() const { return m_nSamplesPerSec; }
	void SetSamplesPerSec(DWORD nSamplesPerSec) { m_nSamplesPerSec = nSamplesPerSec; }

	WORD GetBitsPerSample() const { return m_wBitsPerSample; }
	void SetBitsPerSample(WORD wBitsPerSample) { m_wBitsPerSample = wBitsPerSample; }

	WORD GetBitsPerPixel() const { return m_wBitsPerPixel; }
	void SetBitsPerPixel(WORD wBitsPerPixel) { m_wBitsPerPixel = wBitsPerPixel; }

	void SetBitrate( DWORD wBitRate ){ m_dwBitRate = wBitRate; }
	DWORD GetBitRate() const { return m_dwBitRate; }

	GUID GetVideoSubtype() const { return m_VideoSubtype; }
	void SetVideoSubtype(const GUID& idVideoSubtype) { m_VideoSubtype = idVideoSubtype; }

	GUID GetAudioSubtype() const { return m_AudioSubtype; }
	void SetAudioSubtype(const GUID& idAudioSubtype) { m_AudioSubtype = idAudioSubtype; }

	void SetCodecPrivateData(const BYTE *pData, WORD wSize, CodecType c) {
		m_strExtraData[ Video == c ? VIDEO_STREAM_INDEX : AUDIO_STREAM_INDEX ].assign(
			reinterpret_cast<const char *>(pData), pData ? wSize : 0);
	}
	std::string GetCodecPrivateData(CodecType c) const {
		return m_strExtraData[ Video == c ? VIDEO_STREAM_INDEX : AUDIO_STREAM_INDEX ];
	}

	static DWORD MakeFOURCc( const AudioCodecType codecType );

	static GUID MakeSubtype(DWORD dwFourCC, const GUID& guidBase = WMMEDIASUBTYPE_Base) {
		GUID newGuid = guidBase;
		newGuid.Data1 = dwFourCC;
		return newGuid;
	}
	HRESULT EndWriting() {
		return p->EndWriting();
	}
	HRESULT StartWriting(const wchar_t *pwszFileName);

protected:
	HRESULT ConfigureVideoStream(IWMStreamConfig* pVideoConfig);
	HRESULT ConfigureAudioStream(IWMStreamConfig* pAudioConfig);

	void OnChangeVideoFormat(WMVIDEOINFOHEADER* pVideoInfo, ULONG cbFormat);
	HRESULT OnChangeAudioFormat(LPWAVEFORMATEX lpWaveForm, ULONG cbFormat);

	HRESULT FindInputFormat(DWORD dwInput, GUID guidSubType, IWMInputMediaProps** ppProps);

private:
	CProfileManager m_ProfileManager;
	SIZE m_FrameSize;
	DWORD m_dwFPS;				/* Frames per second for Video */
	WORD  m_wBitsPerPixel;		/* 24 for RGB24 (default), 32 for RGB32 */
    WORD  m_nChannels;          /* (1 = mono, 2 = stereo) */
    DWORD m_nSamplesPerSec;     /* sample rate for Audio */
	WORD  m_wBitsPerSample;     /* number of bits per sample of mono data */
	GUID  m_VideoSubtype;
	GUID  m_AudioSubtype;
	DWORD m_dwBitRate;
	std::string m_strExtraData[2];
	QWORD m_qwAudioWrittenBytes;    // Audio bytes recorded (only in AAC mode)
};


/* Base writer... */
class CBaseWriter : public CWinMediaWriter
{
public:
	CBaseWriter(LPCWSTR wszFileName = NULL)
	{
		ATLASSERT(p != NULL);
		if (wszFileName != NULL) {
			p->SetOutputFilename(wszFileName);
		}
	}
	~CBaseWriter()
	{
	}

	HRESULT CreateProfile(LONG cx, LONG cy, DWORD dwFPS)
	{
		SetFPS(dwFPS);
		SetFrameSize(cx, cy);
		return CWinMediaWriter::CreateCustomProfile();
	}

	HRESULT CreateProfile(LONG cx, LONG cy, DWORD dwFPS, WORD nChannels, DWORD nSamplesPerSec, WORD wBitsPerSample = 16)
	{
		SetChannels(nChannels);
		SetSamplesPerSec(nSamplesPerSec);
		SetBitsPerSample(wBitsPerSample);
		return CreateProfile(cx, cy, dwFPS);
	}

	virtual HRESULT WriteVideoFrame(QWORD qwTime, LPBYTE lpFrameData, DWORD dwDataSize) = 0;

	virtual HRESULT WriteAudioFrame(QWORD qwTime, LPBYTE lpFrameData, DWORD dwDataSize)
	{
		return WriteRawAudio(qwTime, lpFrameData, dwDataSize);
	}
	virtual HRESULT WriteExtraData(LPBYTE lpFrameData, DWORD dwDataSize) { return S_FALSE; }

	HRESULT WriteVideoFrame(LPBYTE lpFrameData, DWORD dwDataSize, DWORD nSample)
	{
		DWORD dwFPS = GetFPS();
		if (0 == dwFPS) {
			return E_INVALIDARG;
		}
		QWORD qwTime = 10000000ULL * nSample / dwFPS;
		return WriteVideoFrame(qwTime, lpFrameData, dwDataSize);
	}

protected:
	virtual bool IsKeyFrame(LPBYTE lpFrameData, DWORD dwDataSize) const  { return true; }

private:
	HRESULT CreateDefaultProfile() { return E_FAIL; }
	HRESULT CreateCustomProfile()  { return E_FAIL; }
};


/* For Motion JPEG video writing... */
class CMJPEGWriter : public CBaseWriter
{
public:
	CMJPEGWriter(LPCWSTR wszFileName = NULL): CBaseWriter(wszFileName)
	{
		SetVideoSubtype(MakeSubtype(MJPG_DIB));
	}
	~CMJPEGWriter()
	{
	}

	virtual HRESULT WriteVideoFrame(QWORD qwTime, LPBYTE lpFrameData, DWORD dwDataSize)
	{
		return WriteCompressedMedia(VIDEO_STREAM_NUM, qwTime, lpFrameData, dwDataSize, WM_SF_CLEANPOINT);
	}
};

/* For codecs with extradata... */
class CDeltaFramesWriter : public CBaseWriter
{
public:
	CDeltaFramesWriter(LPCWSTR wszFileName = NULL): CBaseWriter(wszFileName), m_dwExtraData(0) {}

	bool SetExtraData(LPBYTE lpFrameData, DWORD dwDataSize);

	BYTE const * GetExtraData() const { return m_pExtraData;  }

	DWORD GetExtraDataSize() const    { return m_dwExtraData; }

	virtual HRESULT WriteExtraData(LPBYTE lpFrameData, DWORD dwDataSize)
	{
		return WriteCompressedMedia(VIDEO_STREAM_NUM, 0ULL, lpFrameData, dwDataSize, WM_SF_CLEANPOINT);
	}

	HRESULT WriteExtraData()
	{
		return WriteExtraData(m_pExtraData, m_dwExtraData);
	}

//protected:
//	virtual bool IsKeyFrame(LPBYTE lpFrameData, DWORD dwDataSize) const  { return true; }

private:
	ATL::CAutoVectorPtr<BYTE> m_pExtraData;
	DWORD m_dwExtraData;
};

/* For MPEG4 video writing... */
class CMPEG4Writer : public CDeltaFramesWriter
{
public:
	CMPEG4Writer(LPCWSTR wszFileName = NULL): CDeltaFramesWriter(wszFileName)
	{
		SetVideoSubtype( MakeSubtype(fourccMPEG4) );
	}
	~CMPEG4Writer()
	{
	}
	virtual HRESULT WriteVideoFrame(QWORD qwTime, LPBYTE lpFrameData, DWORD dwDataSize);

	// Extra data for MPEG4 may be ignored
	virtual HRESULT WriteExtraData(LPBYTE lpFrameData, DWORD dwDataSize)
	{
		return /*dwDataSize ? CDeltaFramesWriter::WriteExtraData(lpFrameData, dwDataSize) :*/ S_OK;
	}

protected:
	virtual bool IsKeyFrame(LPBYTE lpFrameData, DWORD dwDataSize) const
	{
		return video_frame_detection::is_mpeg4_idr_frame(lpFrameData, dwDataSize);
	}
private:
	int hasVOSHeader(LPBYTE lpFrameData, DWORD dwDataSize) const;
};

/* For H.264 video writing... */
class CH264Writer : public CDeltaFramesWriter
{
public:
	CH264Writer(LPCWSTR wszFileName = NULL): CDeltaFramesWriter(wszFileName)
	{
		SetVideoSubtype( MakeSubtype(fourccH264) );
	}
	~CH264Writer()
	{
	}

	HRESULT WriteVideoFrame(QWORD qwTime, LPBYTE lpFrameData, DWORD dwDataSize)
	{
		// Определение ключевого кадра...
		DWORD dwFlags = CH264Writer::IsKeyFrame(lpFrameData, dwDataSize) ? WM_SF_CLEANPOINT : 0;
		return WriteCompressedMedia(VIDEO_STREAM_NUM, qwTime, lpFrameData, dwDataSize, dwFlags);
	}

protected:
	virtual bool IsKeyFrame(LPBYTE lpFrameData, DWORD dwDataSize) const
	{
		return video_frame_detection::is_h264_idr_frame(lpFrameData, dwDataSize);
	}
};

} //namespace Writer