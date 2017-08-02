#include <boost\bind.hpp>
#include <numeric>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <array>

#include "..\ArchiveWriter.h"

#include "CommonDefinitions.h"
#include "common\misc_funcs.h"
#include "common\cpp11\make_unique.h"

#include <atltrace.h>

#include "ovmsml\ovifilewriter.h"
using namespace Writer;

#include "..\bottomLessBufferLib\BottomLess_Buffer.h"
using bottomLessBuffer::BottomLessBuffer;
using misc_functions_space::GetDiffFileTime;

const MediaType videoKeyFrame = static_cast<MediaType>(mtVideoFrame | mtKeyFrame);


// TODO: убрать в общий код (common_tools)
std::wstring GetErrorMessage(DWORD dwError, HMODULE hModule = nullptr)
{
	std::array<wchar_t, 4096> buffer;
	DWORD size = 
		::FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM |
			(hModule ? FORMAT_MESSAGE_FROM_HMODULE : 0),         // Its a system error
			hModule,                                             // No string to be formatted needed
			dwError,                                             // Hey Windows: Please explain this error!
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),           // Do it in the standard language
			buffer.data(),                                       // Put the message here
			buffer.size(),                                       // Number of bytes to store the message
			nullptr);
	std::wstring result;
	if (size > 0) {
		wchar_t *pwszEndLine = ::wcsstr(buffer.data(), L"\r\n");
		if (pwszEndLine || (pwszEndLine = ::wcschr(buffer.data(), L'\n')) != nullptr) {
			*pwszEndLine = L'\0';
		}
		result.assign(buffer.data());
	}
	return result;
}

int64_t GetFileSize(const std::wstring &wstrFileName)
{
	std::ifstream in(wstrFileName, std::ifstream::ate | std::ifstream::binary);
	API_BREAK_IF_FALSE(in.is_open(),
					   (std::string("File open error: ") + CW2A(GetErrorMessage(BREAK_IF_FALSE_GET_LASTERROR()).c_str()).m_psz).c_str());
	return in.tellg();
}


ArchiveWriter::ArchiveWriter( BottomLessBuffer& buffer ) : m_buffer(buffer), m_commandsExec(0)
{
	m_threadStopped = false;
	m_threadStarted = ::CreateEventW( 0, true, false, 0 );
	m_recordingProgress = ::CreateEventW( 0, true, false, 0 );
	m_synchronize = false;
	m_hInstErrorLib = ::LoadLibraryW(L"wmerror.dll");
}

ArchiveWriter::~ArchiveWriter()
{
	StopWriter();
	Join();
	::CloseHandle( m_threadStarted );
	::CloseHandle( m_recordingProgress );

	if (m_hInstErrorLib)
		::FreeLibrary(m_hInstErrorLib);
}


bool ArchiveWriter::Start()
{
	ov_util::atomic_exchange(&m_threadStopped, false);
	ov_util::atomic_exchange(&m_synchronize, false);
	//
	if (Thread::Start()) {
		ProgressRecording();
		return true;
	}
	else {
		return false;
	}
}


bool ArchiveWriter::StartRecording( int recordNum, const RecordParams & params )
{
	bool bResult = true;

	if (!IsWorking())
	{
		::ResetEvent(m_threadStarted);
		bResult = Start();
		if (bResult)
			bResult = (WAIT_OBJECT_0 == ::WaitForSingleObject(m_threadStarted, 5000));   // ждать, пока поток начнет выполн€тьс€
	}
	if (bResult)
	{
		bResult = PushCommand(recordNum, params, &ArchiveWriter::Command_StartRecording);
	}
	return bResult;
}


void ArchiveWriter::ProgressRecording()
{
	::SetEvent( m_recordingProgress );
}


bool ArchiveWriter::StartRecording(
	int recordNum,
	const std::wstring & name,
	const FILETIME& startTime,
	const AudioParameters& audioParams,
	const VideoParameters& videoParams,
	int preRecord )
{
	RecordParams params;
	params.baseName = name;
	params.startTime = startTime;
	params.videoParams = videoParams;
	params.audioParams = audioParams;
	params.preRecord = preRecord;
	//
	return StartRecording(recordNum, params);
}


void ArchiveWriter::Command_StartRecording(int recordNum, RecordParams data)
{
	auto it = m_files.find(recordNum);
	if (it != m_files.end()) {
		ErrorCallback(erInitError, L"File already exist", S_FALSE, it->second);
		return;   // already exist
	}
	// запись с другой скоростью помен€ет FPS
	if (data.recordingFPS > 0) {
		data.videoParams.Fps = data.recordingFPS;
	}
	FragmentInfo fragInfo(recordNum, data);
	auto & writer = fragInfo.writer;
	//
	try {
		// проинциализировать Writer
		// TODO: подумать о создание писалки как синглтона
		BREAK_ON_FAIL(fragInfo.writer->CreateWriter(data.videoParams.codecType), E_POINTER, "Writer object isn't created");
		//fragInfo.init(recordNum, data);

		fragInfo.setPreRecord(m_buffer);
		int preRec = fragInfo.getPreRecord();

		if (data.startTime != 0) {
			PRINT_TIMESTAMP("Start record time", data.startTime);
			data.startTime -= CFileTime::Second * preRec;
			m_buffer.setPointerToPostion(data.startTime);
			PRINT_TIMESTAMP("Set pointer to postion in buffer", data.startTime);
		}
		else {
			m_buffer.setPointerOnPreRecording(preRec);   // позиционирование от текущей позиции в буфере

			if (m_buffer.nextSequence())
				PRINT_TIMESTAMP("After set prerecording time =", m_buffer.getCurrentSequence().first);
		}
		if (preRec > 0) {
			ATLTRACE("Set prerecording: %d sec\n", preRec);
		}
		// set extradata from user
		if (fragInfo.writer->IsNeedExtraData()) {
			// ѕроинициализировать extradata
			HRESULT hRes = fragInfo.writer->SaveExtraData(data.extraData.c_str(), data.extraData.size());
			if (H264 == data.videoParams.codecType)
				BREAK_ON_FAIL(hRes, "Required extra data");
		}
		// create new file
		//HRESULT hr = OpenFile(fragInfo);
		//if ( SUCCEEDED(hr) || E_PENDING == hr )
			m_files.insert( MapFragmentInfo::value_type(fragInfo.recordNum, std::move(fragInfo)) );
	}
	catch (const com_exception &e) {
		ErrorCallback(erInitError, CA2W(e.what()), e.getHResult(), fragInfo);
	}
}


bool ArchiveWriter::StopRecording(int recordNum, const FILETIME & stopTime)
{
	if (CFileTime(stopTime) != 0)
		return PushCommand(recordNum, stopTime, &ArchiveWriter::Command_SetFragmentStopTime);
	else
		return StopRecording(recordNum);   // остановка сразу
}


bool ArchiveWriter::StopRecording(int recordNum)
{
	return PushCommand(recordNum, &ArchiveWriter::Command_StopRecording);
}


bool ArchiveWriter::CancelStopRecording(int recordNum)
{
	FILETIME stopTime = {0};
	return PushCommand(recordNum, stopTime, &ArchiveWriter::Command_SetFragmentStopTime);
}


void ArchiveWriter::Command_SetFragmentStopTime(int recordNum, FILETIME stopTime)
{
	auto fragment = CheckFragmentExist(recordNum);

	if (fragment)
	{
		const bool alreadyStop = (fragment->stopFrameTime != 0);
		fragment->setStopTime(stopTime);

		if (alreadyStop && CFileTime(stopTime) == 0)   // отмена остановки записи
		{
			EventCallback(evCancelStop);
		}
		// ћы каким то бабаем умудрились записать уже все кадры к моменту останова. ќстанавливаем запись тут
		else if (fragment->stopFrameTime != 0 &&
				 fragment->stopFrameTime <= fragment->lastFrameTime)
		{
			Command_StopRecording(recordNum);
		}
	}
}


void ArchiveWriter::Command_StopRecording(int recordNum)
{
	auto fragment = CheckFragmentExist(recordNum);
	if (fragment) {
		CloseFile(*fragment, true);
		m_files.erase(recordNum);
	}
}


void ArchiveWriter::PauseRecording()
{
	::ResetEvent(m_recordingProgress);
}


bool ArchiveWriter::PauseRecording( int recordNum, bool flag )
{
	return PushCommand(recordNum, flag, &ArchiveWriter::Command_PauseRecording);
}


void ArchiveWriter::Command_PauseRecording(int recordNum, bool enabled)
{
	auto fragment = CheckFragmentExist(recordNum);
	if (fragment) {
		fragment->setPause(enabled);
	}
}


bool ArchiveWriter::ProcessFrames(int recordNum, boost::optional<PairTimeSeq> seq)
{
	if (seq)
	{
		seq->second.stream->AddRef();
	}
	bool result = PushCommand(recordNum,
							  seq ? seq.get() : PairTimeSeq(),
							  &ArchiveWriter::Command_ProcessFrames);
	if (!result && seq)
	{
		seq->second.stream->Release();
	}
	return result;
}


void ArchiveWriter::Command_ProcessFrames(int recordNum, PairTimeSeq seq)
{
	CComPtr<IStream> sequence;
	sequence.Attach(seq.second.stream);
	auto fragment = CheckFragmentExist(recordNum);

	if (fragment)
	{
		// записать последовательность напр€мую, если она есть
		if (sequence)
			WriteSequenceAndCheckDuration(seq, *fragment);
		else
			ProcessFragment(*fragment);
	}
}


bool ArchiveWriter::DivideOnFragments(int recordNum)
{
	return PushCommand(recordNum, &ArchiveWriter::Command_DivideOnFragments);
}


void ArchiveWriter::Command_DivideOnFragments(int recordNum)
{
	auto fragment = CheckFragmentExist(recordNum);

	if (fragment && DivideOnFragments(*fragment))
		EventCallback(evManualFragmentSplit);
}


bool ArchiveWriter::DivideOnFragments(FragmentInfo & fragInfo)
{
	HRESULT hr;
	// разделить на фрагменты
	ATLTRACE("Divide on fragments (%d)\n", fragInfo.recordNum);
	hr = CloseFile(fragInfo, false);
	if (SUCCEEDED(hr)) {
		hr = OpenFile(fragInfo);
	}
	return SUCCEEDED(hr);
}


FragmentInfo * ArchiveWriter::CheckFragmentExist(int recordNum)
{
	if (recordNum <= 0)
	{
		ErrorCallback(erGetFragment, L"Invalid record number", E_INVALIDARG, FragmentInfo(recordNum, RecordParams())); 
		return nullptr;
	}
	auto elem = m_files.find(recordNum);
	if (elem == m_files.end())
	{
		ErrorCallback(erGetFragment, L"Fragment not found", HRESULT_FROM_WIN32( ERROR_NOT_FOUND ), FragmentInfo(recordNum, RecordParams())); 
		return nullptr;   // already exist
	}
	return &elem->second;
}


void ArchiveWriter::StopWriter()
{
	PushFastCommand([this] {
		ov_util::atomic_exchange(&m_threadStopped, true);
	});
	ProgressRecording();
}


std::wstring ArchiveWriter::CreateFileName( const std::wstring & wstrCameraName, int recordNum, int fragmentNum )
{
	// Generate: [CameraName]_[RecNumber]_[FragmentNumber].[Extension]
	const wchar_t *ext = WriterPtr::element_type::GetFileExtension();
	wchar_t bufName[MAX_PATH];
	swprintf_s(bufName, L"%s_%08d_%05d", wstrCameraName.c_str(), recordNum, fragmentNum);
	if (ext) {
		wcscat_s(bufName, L".");
		wcscat_s(bufName, ext);
	}
	return std::wstring(bufName);
}


unsigned ArchiveWriter::ProcessFragment(FragmentInfo & fragInfo)
{
	int frames = 0;
	//
	if (!fragInfo.isPaused())
	{
		PairTimeSeq seq;
		CComPtr<IStream> spSequence;

		// получить последовательность из буфера
		if (!m_buffer.nextSequence( seq )) {
			// буфер пуст
			ATLTRACE("Empty writer buffer: %d\n", fragInfo.recordNum);
			EventCallback(evEmptyBuffer);
		}
		else {
			spSequence.Attach(seq.second.stream);
			// записать последовательность
			frames = WriteSequenceAndCheckDuration(seq, fragInfo);
		}
	}
	return frames;
}


int ArchiveWriter::WriteSequenceAndCheckDuration(const PairTimeSeq & seq, FragmentInfo & fragInfo)
{
	int frames = 0;

	// открыть файл, если он ещЄ не открыт
	if (fragInfo.isOpen() || SUCCEEDED(OpenFile(fragInfo)))
	{
		frames = WriteSequence( seq, fragInfo );
		// поделить на фрагменты, если достигнут лимит времени
		if ( fragInfo.isFullFragment(seq.first) &&
			((fragInfo.stopFrameTime == 0) ||												// дробить на фрагменты только, если следующий фрагмент будет больше секунды
			 (fragInfo.stopFrameTime - fragInfo.lastFrameTime) >= CFileTime::Second) &&		// (в режиме отложенной остановки)
			 DivideOnFragments(fragInfo) )
		{
			EventCallback(evAutoFragmentSplit);
		}
	}
	return frames;
}


void ArchiveWriter::Execute()
{
	::CoInitialize(nullptr);
	::SetEvent(m_threadStarted);
	EventCallback(evThreadStart);
	//
	while( !m_threadStopped )
	{
		auto recordedFrames = std::accumulate(
								m_files.begin(),
								m_files.end(),
								0U,
								[this] (unsigned val, MapFragmentInfo::value_type & vt) -> unsigned
		{
			val += ProcessFragment(vt.second);
			return val;
		});
		if (m_commandsExec > 0) {
			m_commandsExec = 0;
		}
		else if (!recordedFrames && !m_threadStopped) {
			PauseRecording();
		}
		// executes APC commands from thread queue if it has
		if (WAIT_IO_COMPLETION == ::WaitForSingleObjectEx( m_recordingProgress, INFINITE, true )) {
			ProgressRecording();
			//::Sleep(50);
		}
	}
	::ResetEvent( m_recordingProgress );
	EventCallback( evThreadExit );
	::CoUninitialize();
}


void ArchiveWriter::ExecuteAPC(APCFunction f)
{
	Thread::ExecuteAPC(f);
	++m_commandsExec;
}


int ArchiveWriter::WriteSequence( const PairTimeSeq & seq, FragmentInfo & fragInfo )
{
	if( !seq.second.stream ) {
		ErrorCallback(erNullPointer, L"Stream pointer error", E_POINTER, fragInfo);
		return -1;
	}
	SequenceInfo seqInfo = { seq.first, 0, 0, 0, fragInfo.isPreRecordInSeconds(seq.first), false };
	LARGE_INTEGER pos = {0};
	ULARGE_INTEGER streamSize = {0};
	CComPtr<IStream> stream;
	seq.second.stream->Clone(&stream);
	stream->Seek( pos, STREAM_SEEK_END, &streamSize );

	// empty sequence...
	if (!streamSize.QuadPart) {
		return 0;
	}
	PRINT_TIMESTAMP("Write sequence in file", seqInfo.fTime);
	HRESULT hRes = stream->Seek( pos, STREAM_SEEK_SET, NULL );

	FrameInfoHeader fih;
	FrameInfoHeader const * const pFIH = &fih;
	const wchar_t *pwszErrMsg = NULL;
	int *pFramesCount = NULL;
	unsigned long offset = 0, readBytes;
	int frames = 0;
	const bool containsPFrame = (fragInfo.getVideoParams().codecType != MJPEG);
	bool keyframe = false;

	// write sequence
	while( offset < streamSize.LowPart )
	{
		BREAK_ON_FAIL(stream->Read(&fih, sizeof(FrameInfoHeader), &readBytes), "Read frame header");
		offset += readBytes;
		// ¬озможность оборвать запись последовательности в любой момент (кроме предзаписи)
		if (fragInfo.stopFrameTime != 0 && !seqInfo.preRecord)
		{
			// задано относительное врем€. ѕреобразовать врем€ в абсолютное
			if (0 == fragInfo.stopFrameTime.dwHighDateTime) {
				fragInfo.stopFrameTime = CFileTime(pFIH->fTimeStamp) + fragInfo.stopFrameTime.dwLowDateTime;
			}
			if (fragInfo.stopFrameTime <= pFIH->fTimeStamp)
			{
				if (fragInfo.lastFrameTime == 0) {
					fragInfo.lastFrameTime = fragInfo.stopFrameTime;
				}
				fragInfo.stopFrameTime = 0;
				StopRecording(fragInfo.recordNum);   // пользуемс€ тем, что это APC. “ем самым вызов делаем отложенным
				seqInfo.truncated = true;
				break;
			}
		}
		if( fragInfo.firstFrameTime == 0 ) {
			fragInfo.firstFrameTime = pFIH->fTimeStamp;
			ov_util::atomic_exchange(&m_synchronize, false);
		}
		// synchronize skip frames
		DoSynchronizeTimestamps( pFIH->fTimeStamp, fragInfo );
		LONGLONG timeDiff = GetDiffFileTime( pFIH->fTimeStamp, fragInfo.firstFrameTime );
		BOOST_ASSERT(timeDiff >= 0);

		std::unique_ptr<char []> streamPtr(new char [pFIH->cbSize]);
		BREAK_ON_FAIL(stream->Read(streamPtr.get(), pFIH->cbSize, &readBytes),"Read frame");
		offset += readBytes;

		switch (pFIH->mType)
		{
		case videoKeyFrame:
			keyframe = true;
		case mtVideoFrame:
			// активириван режим записи "с другой скоростью". ƒл€ кодеков,
			// в которых есть дифференциальные кадры, нужно сохранить только ключевые кадры
			if (fragInfo.getRecordingFPS() > 0) {
				if ( containsPFrame && (seqInfo.nVideoFrames > 0) ||
					!containsPFrame && !fragInfo.checkForceRecordingFrame() )
				{
					continue;
				}
			}
			hRes = fragInfo.writer->WriteVideoFrame(streamPtr.get(), pFIH->cbSize, timeDiff, keyframe);
			pFramesCount = &seqInfo.nVideoFrames;
			keyframe = false;
			break;
		case mtAudioFrame:
			hRes = fragInfo.writer->WriteAudioFrame(streamPtr.get(), pFIH->cbSize, timeDiff);
			pFramesCount = &seqInfo.nAudioFrames;
			break;
		case mtExtraData:
			hRes = fragInfo.writer->SetExtraData( streamPtr.get(), pFIH->cbSize );
			pFramesCount = &seqInfo.nExtraFrames;
			break;
		default:
			hRes = E_INVALIDARG;
			pwszErrMsg = L"Unknown frame type";
		}
		if (SUCCEEDED(hRes))
		{
			ATLTRACE("Write frame (%s): %d msec\n",
				((mtAudioFrame == pFIH->mType) ? "a" : (mtVideoFrame == pFIH->mType) ? "v" :
				 (videoKeyFrame == pFIH->mType) ? "v: KF" : "unknown"),
				static_cast<int>(timeDiff / CFileTime::Millisecond));
			++frames;
			++*pFramesCount;
			// запомнить врем€ последнего удачно записанного кадра
			//if (mtVideoFrame == pFIH->mType)
			fragInfo.lastFrameTime = pFIH->fTimeStamp;
		}
		else {
			std::wstring wstrError;
			if (pwszErrMsg) {
				wstrError = pwszErrMsg;
				pwszErrMsg = NULL;
			} else {
				wstrError = GetErrorMessage(hRes);
			}
			ErrorCallback(erRecordFrame, wstrError.empty() ? L"Frame recording error" : wstrError.c_str(), hRes, fragInfo);
			break;   // stop recording of current sequence
		}
	} // while
	// call event if we have frames
	if (frames > 0)
	{
		fragInfo.videoFrames += seqInfo.nVideoFrames;
		fragInfo.audioFrames += seqInfo.nAudioFrames;
		++fragInfo.seqNum;
		EventSequenceDone(seqInfo, fragInfo);
	}
	return frames;
}


// synchronize skip frames
bool ArchiveWriter::DoSynchronizeTimestamps(const FILETIME & frameTime, FragmentInfo & fragInfo)
{
	bool bResult = false;

	if (fragInfo.lastFrameTime != 0 && fragInfo.videoFrames >= 2 &&
		ov_util::atomic_compare_exchange(&m_synchronize, false, true))
	{
		// estimate average video frame duration
		int64_t nDuration = (fragInfo.lastFrameTime - fragInfo.firstFrameTime).GetTimeSpan();
		int64_t nAveFrameTime = nDuration / (fragInfo.videoFrames - 1);
		int64_t delay = ( CFileTime( frameTime ) - fragInfo.lastFrameTime ).GetTimeSpan() - nAveFrameTime;

		if (delay > 0)
		{
			ATLTRACE("Synchronize timestamp\n");
			fragInfo.firstFrameTime += delay;
			BOOST_ASSERT(fragInfo.lastFrameTime + delay < frameTime);
			bResult = true;
		}
	}
	return bResult;
}


std::wstring ArchiveWriter::GetErrorMessage(DWORD dwError)
{
	return ::GetErrorMessage(dwError, m_hInstErrorLib);
}


HRESULT ArchiveWriter::ErrorCallbackException(Errors cat, const std::exception & e, HRESULT code, FragmentInfo const & fragInfo)
{
	ErrorCallback(cat, CA2W(e.what()), code, fragInfo);
	return code;
}


HRESULT ArchiveWriter::OpenFile(FragmentInfo & fragInfo)
{
	BOOST_ASSERT(fragInfo.recordNum != 0);
	//
	HRESULT hRes = S_OK;
	fragInfo.initFragment( boost::bind(&ArchiveWriter::CreateFileName, this, _1, _2, _3) );   // ++fragmentNum

	if (fragInfo.fileName.empty())   // проверить пустое им€ файла
	{
		hRes = E_INVALIDARG;
		ErrorCallback(erOpenFile, L"Empty filename received", hRes, fragInfo);
		return hRes;
	}
	hRes = fragInfo.writer->InitFile(
							fragInfo.fileName,
							fragInfo.getVideoParams(),
							fragInfo.getAudioParams());
	if (SUCCEEDED(hRes))
	{
		fragInfo.startRecTime = CFileTime::GetTickCount();
		fragInfo.endRecTime = fragInfo.startRecTime;
	}
	else {
		std::wstring wstrError( GetErrorMessage(hRes) );
		ErrorCallback(erOpenFile, wstrError.empty() ? L"Unknown file open error" : wstrError.c_str(), hRes, fragInfo);
		fragInfo = FragmentInfo();
	}
	return hRes;
}


HRESULT ArchiveWriter::CloseFile(FragmentInfo & fragInfo, bool bFinalFragment)
{
	HRESULT hRes = S_FALSE;

	if ( fragInfo.isOpen() )
	{
		const Errors category = erCloseFile;
		//
		PRINT_TIMESTAMP("End record timestamp", fragInfo.lastFrameTime);
		fragInfo.duration = (fragInfo.lastFrameTime - fragInfo.firstFrameTime).GetTimeSpan();
		fragInfo.endRecTime = CFileTime::GetTickCount();
		// «акрыть файл
		try {
			BREAK_ON_FAIL(fragInfo.writer->CloseFile(), "File close error");
			if (::wcscmp(fragInfo.writer->GetFileExtension(), DummyWriter::dummyExtension) != 0) {   // дл€ реализации-заглушки файлы не создаютс€
				fragInfo.fileSize = GetFileSize(fragInfo.fileName);
			}
		}
		catch (const winapi_error_exception & e) {
			hRes = ErrorCallbackException(category, e, HRESULT_FROM_WIN32(e.getError()), fragInfo);
		}
		catch (const com_exception & e) {
			hRes = ErrorCallbackException(category, e, e.getHResult(), fragInfo);
		}
	}
	return hRes;
}