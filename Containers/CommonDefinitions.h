#ifndef _OVMSML_COMMON_DEFINITIONS_H_
#define _OVMSML_COMMON_DEFINITIONS_H_

#include <stdexcept>   // runtime_error, exception
#include <windows.h>
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Event Service Params Flags
#define DETECT_EVENT_ID											0x00000001
#define DETECT_EVENT_NAME										0x00000002
#define DETECT_EVENT_MIN_TRIGGER_INTERVAL						0x00000004
#define DETECT_EVENT_MIN_TRIGGER_TIME							0x00000008
#define DETECT_EVENT_MAX_DURATION								0x00000010
#define DETECT_EVENT_MODE										0x00000020
#define DETECT_EVENT_ENABLED									0x00000040
#define DETECT_EVENT_WINDOW_ID									0x00000080
#define DETECT_EVENT_WINDOW_GROUP_ID							0x00000100

#define DETECT_WINDOW_NAME										0x00000001
#define DETECT_WINDOW_PRESET									0x00000002
#define DETECT_WINDOW_HISTORY									0x00000004
#define DETECT_WINDOW_SENSITIVITY								0x00000008
#define DETECT_WINDOW_MIN_OBJECT_SIZE							0x00000010
#define DETECT_WINDOW_THRESHOLD									0x00000020
#define DETECT_WINDOW_ENABLED									0x00000040
#define DETECT_WINDOW_TYPE										0x00000080
#define DETECT_WINDOW_MIN_OBJECT_WIDTH							0x00000100
#define DETECT_WINDOW_MAX_OBJECT_WIDTH							0x00000200
#define DETECT_WINDOW_MIN_OBJECT_HEIGHT							0x00000400
#define DETECT_WINDOW_MAX_OBJECT_HEIGHT							0x00000800
#define DETECT_WINDOW_MASK_TYPE									0x00001000
#define DETECT_WINDOW_SAMPLING_INTERVAL							0x00002000
#define DETECT_WINDOW_DETECTION_LEVEL							0x00004000

#define EVENT_MESSAGE_DATA_EVENT_NAME							0x0001
#define EVENT_MESSAGE_DATA_EVENT_ID								0x0002
#define EVENT_MESSAGE_DATA_DETECT_WINDOW_NAME					0x0004
#define EVENT_MESSAGE_DATA_DETECT_WINDOW_ID						0x0008
#define EVENT_MESSAGE_DATA_DETECT_WINDOWS_GROUP_ID				0x0010
#define EVENT_MESSAGE_DATA_START_OR_STOP						0x0020
#define EVENT_MESSAGE_DATA_CAMERA_BRAND							0x0040
#define EVENT_MESSAGE_DATA_CAMERA_IP_ADDRESS					0x0080
#define EVENT_MESSAGE_DATA_CAMERA_PORT							0x0100
#define EVENT_MESSAGE_DATA_CHANNEL								0x0200


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum MediaType : char
{
	mtNoFrame = 0x00,
	mtExtraData = 0x01,
	mtKeyFrame = 0x02,
	mtVideoFrame = 0x04,
	mtAudioFrame = 0x08,
	mtSetOleDate = 0x10,
	mtUncompressedForWriter = 0x20,
	mtMetaData = 0x30
};

// Frame Metadata writed in stream
#pragma pack(push, 1)

struct FrameInfoHeader
{
	MediaType mType;             // атрибуты кадра
	union {
		FILETIME fTimeStamp;     // время получения кадра
		DATE fOleTimeStamp;      // ... в формате OLE, если стоит флаг mtSetOleDate
	};
	unsigned int cbSize;         // размер буфера, записанного в поток
	unsigned int dwCrc;          // контрольная сумма метаданных
};

#pragma pack(pop)


#ifdef _DEBUG
struct FrameInfoHeaderEx : FrameInfoHeader
{
	void ASSUME_CALC_CRC() { dwCrc = CALC_FRAME_CRC(); }
	bool CHECK_FRAME()     { return CALC_FRAME_CRC() == dwCrc; }
private:
	unsigned int CALC_FRAME_CRC() {
		auto ptr = reinterpret_cast<const unsigned char *>(this);
		unsigned int crc(0), bits(0);
		for (int i = 0; i < sizeof(FrameInfoHeader) - sizeof(unsigned int); i++) {
			crc ^= (ptr[i] << bits);
			if (8*sizeof(crc) == ++bits)
				bits = 0;
		}
		return crc;
	}
};
#else
struct FrameInfoHeaderEx : FrameInfoHeader {
	void ASSUME_CALC_CRC() {}
	bool CHECK_FRAME()     { return true; }
};
#endif //_DEBUG
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum OVSpecialIdent
{
	OVMSML_BASE = 0xc8,
	OVMSML_CONTROLLING_WND_WINPROC_THUNK,
	OVMSML_OV_SYS_EXCEPTION_THUNK
};

//
//Special messages ID's
const int MSG_OVMSML_BASE = WM_USER + 10;

enum OVEvents
{
	MSG_OVMSML_NEW_VIDEO_FRAME = MSG_OVMSML_BASE,
	MSG_OVMSML_NEW_AUDIO_FRAME,
	//MSG_OVMSML_DECODE_EXTRADATA,
	MSG_OVMSML_DETECTOR_NOTIFCATION,
	MSG_OVMSML_PLAY_SOUND_NOTIFICATION,

	MSG_WRITER_START_RECORDING,
	MSG_WRITER_STOP_RECORDING,
	MSG_WRITER_RECORDING_ERROR,
	MSG_WRITER_FRAGMENT_RECORDING_START,
	MSG_WRITER_FRAGMENT_RECORDING_END,
	MSG_WRITER_SEQUENCE_PROCESSED,
	MSG_WRITER_BUFFER_IS_EMPTY,

	MSG_OVMSML_CONNECTION_LOST,
	MSG_OVMSML_CONNECT_CANCELED,
	MSG_OVMSML_MUTE_AUDIO,
	MSG_OVMSML_CONNECT_TO_CAMERA,
	MSG_DECODER_GET_AUDIO_OUT_SAMPLE,
	MSG_DECODER_REINIT,
	//add new events here
	//...
};
//
const unsigned long long OV_100NANOSECOND =				10000000ULL;
const unsigned long long OV_100NANOSECONDS_IN_MSEC =	OV_100NANOSECOND / 1000;
const unsigned long long OV_100NANOSECONDS_IN_MINUTE =	60 * OV_100NANOSECOND;
const unsigned long long OV_100NANOSECONDS_IN_HOUR =	60 * OV_100NANOSECONDS_IN_MINUTE;
const unsigned long long OV_100NANOSECONDS_IN_DAY =		24 * OV_100NANOSECONDS_IN_HOUR;
//
const int OV_BITS_PER_PIXEL =				24;
const int OV_SND_IN_BITS_PER_SAMPLE =		8;
const int OV_SND_OUT_BITS_PER_SAMPLE =		16;
const int OV_SND_COUNT_BUFFERS =			16;
const int OV_ALARM_FRAME_WIDTH =			3;

#define OV_NOTIFICATIONS_PIPE_NAME L"\\\\.\\pipe\\OV_NOTIFICATIONS_PIPE_NAME"

#include "common\error_util.h"

// deprecated definitions!
#define BREAK_ON_FAIL BREAK_IF_FAILED
#define API_BREAK_IF_FALSE(x, str) WINAPI_BREAK_IF_FALSE(x, str)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(_DEBUG) && defined(OVMSML_LOG)
inline void PRINT_TIMESTAMP(const char *s, const FILETIME & ft)
{
	SYSTEMTIME st;
	FILETIME newFt;
	FileTimeToLocalFileTime(&ft, &newFt);
	FileTimeToSystemTime(&newFt, &st);
	ATLTRACE(
		"%s в %hu.%02hu.%hu %02hu:%02hu:%02hu\n", s,
		st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond
	);
}
#else
#define PRINT_TIMESTAMP(x, y)
#endif //_DEBUG
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef const BITMAPFILEHEADER *PCBitmapFileHeader;

struct OVExceptionInformation
{
	DWORD				processId;
	DWORD				exceptionThreadId;
	PEXCEPTION_POINTERS	exceptionPointers;
	WCHAR				dumpName[MAX_PATH];
	WCHAR				logName[MAX_PATH];
};

const wchar_t * const ovdumpEvent = L"ovdump_StartEvent";
const wchar_t * const ovdumpMemory = L"ovdump_ParamsMemory";


#endif //_OVMSML_COMMON_DEFINITIONS_H_