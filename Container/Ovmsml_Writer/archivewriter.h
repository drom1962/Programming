#ifndef _ARCHIVE_WRITER_H_
#define _ARCHIVE_WRITER_H_
#include <boost\bind.hpp>
#include <boost\optional.hpp>
#include <memory>
#include <list>
#include <atltime.h>
#include "..\..\common\Locker.h"
#include "..\..\common\atomic.h"

#include "AsfArchiveWriterLib\Thread.h"
#include "AsfArchiveWriterLib\ArchiveFileWriter.h"
#include "AsfArchiveWriterLib\FragmentInfo.h"
#include "..\bottomLessBufferLib\BottomLess_Buffer.h"



namespace Writer
{
	enum Events {
		evThreadStart,
		evThreadExit,
		evEmptyBuffer,
		evAutoFragmentSplit,
		evManualFragmentSplit,
		evCancelStop
	};

	enum Errors {
		erOther,
		erInitError,
		erOpenFile,
		erCloseFile,
		erRecordFrame,
		erGetFragment,
		erNullPointer
	};

	struct SequenceInfo {
		FILETIME fTime;
		int nVideoFrames;
		int nAudioFrames;
		int nExtraFrames;
		bool preRecord;
		bool truncated;
	};

	class ArchiveWriter : public ext::Thread
	{
		enum RecordingThreadState
		{
			rtsWaitUserRequest,
			rtsOpenFile,
			rtsRecording
		};
		typedef std::map<int, FragmentInfo> MapFragmentInfo;
		typedef FragmentInfo & FragmentInfoRef;

	protected:
		typedef locker_space::CLocker Locker;
		typedef locker_space::CInterThreadObject< RecordParams > SafeParams;
		typedef locker_space::CObjectLocker< const RecordParams > CLockParams;
		typedef locker_space::CObjectLocker< RecordParams > LockParams;

	public:
		typedef bottomLessBuffer::BottomLessBuffer::PairTimeSeq PairTimeSeq;

		ArchiveWriter( bottomLessBuffer::BottomLessBuffer& buffer );
		~ArchiveWriter();

		virtual bool Start();
		//безопасно завершает выполнение потока записи
		void StopWriter();

		// функции, отвечающие за запись
		bool ProcessFrames(int recordNum, boost::optional<PairTimeSeq> seq = boost::none);

		//создаёт поток записи (если не создан) и начинает запись последовательностей из буфера, начиная с startTime
		bool StartRecording(int recordNum, const std::wstring & name, const FILETIME& startTime, const AudioParameters& audioParams, const VideoParameters& videoParams, int preRecord = 0);
		bool StartRecording(int recordNum, const RecordParams & params);
		//приостанавливает поток записи
		bool PauseRecording(int recordNum, bool flag);
		bool StopRecording(int recordNum);
		bool StopRecording(int recordNum, const FILETIME & stopTime);
		bool CancelStopRecording(int recordNum);
		bool DivideOnFragments(int recordNum);

	protected:
		virtual void Execute();
		virtual void ExecuteAPC(APCFunction f);
		virtual void EventCallback(Events e) {}
		virtual void EventSequenceDone(const SequenceInfo & seqInfo, const FragmentInfo & fragInfo) {}
		virtual void ErrorCallback(Errors e, const wchar_t *error, HRESULT code, FragmentInfo const & fragInfo) {}
		virtual std::wstring CreateFileName( const std::wstring & wstrCameraName, int recordNum, int fragmentNum = 1 );
		virtual HRESULT OpenFile(FragmentInfo & fragInfo);
		virtual HRESULT CloseFile(FragmentInfo & fragInfo, bool bFinalFragment);
		//
		std::wstring GetErrorMessage(DWORD dwError);

		// использовать для пропуска кадров при паузе
		void SynchronizeTimestamps()
		{
			ov_util::atomic_exchange(&m_synchronize, TRUE);
		}
		void ProgressRecording();

	private:
		void InitParams( LockParams & lock, int recordNum, const std::wstring & wstrBaseName, const FILETIME& startTime, const AudioParameters& audioParams, const VideoParameters& videoParams, int preRecord );
		void PauseRecording();
		bool DoSynchronizeTimestamps(const FILETIME & frameTime, FragmentInfo & fragInfo);

		// TODO: Выпилить в FragmentInfo
		int WriteSequence( const PairTimeSeq & seq, FragmentInfo & fragInfo );
		int WriteSequenceAndCheckDuration(const PairTimeSeq & seq, FragmentInfo & fragInfo);
		unsigned ProcessFragment(FragmentInfo & fragInfo);

		template<class Fn>
		void CheckFragmentExistAndDoWorkWithFragment(int recordNum, Fn f)
		{
			auto p = CheckFragmentExist(recordNum);
			if (p) {
				f(*p);
			}
			++m_commandsExec;
		}
		template<class Fn>
		bool PushCommand(int recordNum, Fn f) {
			return Thread::PushFastCommand(boost::bind(&ArchiveWriter::CheckFragmentExistAndDoWorkWithFragment<Fn>, this, recordNum, std::move(f)));
		}
		template<class T>
		bool PushCommand(int recordNum, T value, void (ArchiveWriter::*f)(int, T) ) {
			//return PushCommand(recordNum, boost::bind(f, this, _1, std::move(value)));
			return Thread::PushCommand(boost::bind(f, this, recordNum, std::move(value)));
		}
		bool PushCommand(int recordNum, void (ArchiveWriter::*f)(int) ) {
			//return PushCommand(recordNum, boost::bind(f, this, _1));
			return Thread::PushCommand(boost::bind(f, this, recordNum));
		}

		void Command_StartRecording(int recordNum, RecordParams data);
		void Command_PauseRecording(int recordNum, bool enabled);
		void Command_ProcessFrames(int recordNum, PairTimeSeq seq);
		void Command_SetFragmentStopTime(int recordNum, FILETIME stopTime);
		void Command_StopRecording(int recordNum);
		void Command_DivideOnFragments(int recordNum);
		bool DivideOnFragments(FragmentInfo & fragInfo);

		FragmentInfo * CheckFragmentExist(int recordNum);
		HRESULT ErrorCallbackException(Errors cat, const std::exception & e, HRESULT code, FragmentInfo const & fragInfo);

	private:
		bottomLessBuffer::BottomLessBuffer & m_buffer;
		MapFragmentInfo m_files;   // only for current thread
		HANDLE m_threadStarted;
		HANDLE m_recordingProgress;
		BOOL m_threadStopped;
		BOOL m_synchronize;
		HMODULE m_hInstErrorLib;
		int m_commandsExec;
	};
}

#endif //_ARCHIVE_WRITER_H_