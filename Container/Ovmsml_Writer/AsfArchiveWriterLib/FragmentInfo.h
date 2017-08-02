#pragma once
#include <memory>
#include <atltime.h>
#include "SetupFileWriterImpl.h"
#include "New_AVG_CONTROL\bottomLessBufferLib\BottomLess_Buffer.h"


namespace Writer
{
	struct RecordParams
	{
		RecordParams(): fragmentDuration(0), preRecord(0), recordingFPS(0) {
			memset(&audioParams, 0, sizeof(audioParams));
			memset(&videoParams, 0, sizeof(videoParams));
		}
		AudioParameters		audioParams;
		VideoParameters		videoParams;
		std::wstring		baseName;
		ATL::CFileTime		startTime;
		std::string			extraData;
		int					fragmentDuration;
		int					preRecord;
		unsigned			recordingFPS;
	};

	typedef std::shared_ptr<Writer::WriterImpl> WriterPtr;

	struct FragmentInfo
	{
		WriterPtr writer;
		std::wstring fileName;
		int recordNum;
		int fragmentNum;
		int videoFrames;
		int audioFrames;
		int dropFrames;
		bool paused;

		int fragmentDuration;
		int seqNum;
		uint64_t fileSize;
		uint64_t duration;
		ATL::CFileTime startRecTime;
		ATL::CFileTime endRecTime;
		ATL::CFileTime firstFrameTime;
		ATL::CFileTime lastFrameTime;
		ATL::CFileTime stopFrameTime;

		FragmentInfo() { init(0, RecordParams()); }

		FragmentInfo( int recordNum, const RecordParams & params ) {
			init( recordNum, params );
		}
		template<class Fn3>
		void initFragment(Fn3 nameCreator) {
			initFragment();
			fileName = nameCreator(params.baseName, recordNum, fragmentNum);
		}
		void setPreRecord(const bottomLessBuffer::BottomLessBuffer & buffer);
		void setStopTime(const FILETIME & time);
		//
		int getPreRecord() const {
			return params.preRecord;
		}
		unsigned getRecordingFPS() const {
			return params.recordingFPS;
		}
		bool checkForceRecordingFrame() {
			return (!params.recordingFPS) || ((videoFrames + dropFrames++) % (params.recordingFPS + 1) == 0);
		}
		bool isPreRecordInSeconds(const FILETIME & seqTime) const {
			return startRecTime > seqTime;
		}
		bool isFirstFragment() const { return fragmentNum < 2; }
		bool isFullFragment(const FILETIME & seqTime) const;
		bool isOpen() const {
			return (startRecTime != 0) && (startRecTime == endRecTime);
		}
		bool isPaused() const { return paused; }
		void setPause(bool enabled) { paused = enabled; }
		//
		void clearStat(const ATL::CFileTime & startTime);
		void clearStat() { clearStat(0); }
		const AudioParameters & getAudioParams() const { return params.audioParams; }
		const VideoParameters & getVideoParams() const { return params.videoParams; }
	private:
		void init(int recordNum, const RecordParams & params);
		void initFragment();
	private:
		RecordParams params;
		bottomLessBuffer::
			BottomLessBuffer::
				PreRecordType preRecordType;
	};
}