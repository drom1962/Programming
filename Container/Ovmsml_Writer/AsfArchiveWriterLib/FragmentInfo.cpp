#include "FragmentInfo.h"
#include "CommonDefinitions.h"
#include "New_AVG_CONTROL\bottomLessBufferLib\BottomLess_Buffer.h"

using namespace Writer;
using bottomLessBuffer::BottomLessBuffer;


void FragmentInfo::clearStat(const CFileTime & startTime)
{
	this->startRecTime = startTime;
	this->endRecTime = startTime;
	this->firstFrameTime = 0;
	this->lastFrameTime = 0;
	this->fileSize = 0;
	this->videoFrames = 0;
	this->audioFrames = 0;
	this->dropFrames = 0;
	this->duration = 0;
	this->seqNum = 0;
	fileName.clear();
}


void FragmentInfo::initFragment()
{
	++this->fragmentNum;
	clearStat();
}


void FragmentInfo::init(int recordNum, const RecordParams & params)
{
	this->recordNum = recordNum;
	this->fragmentNum = 0;
	this->fragmentDuration = params.fragmentDuration;
	this->preRecordType = BottomLessBuffer::prtDurationSeconds;
	this->params = params;
	this->paused = false;
	this->writer = std::make_shared<WriterPtr::element_type>();
	clearStat();
}


void FragmentInfo::setPreRecord(const bottomLessBuffer::BottomLessBuffer & buffer)
{
	int preRecord = buffer.getPreRecordAreaSize(&preRecordType);
	if (params.preRecord < 0) {
		params.preRecord = preRecord;
	}
}


void FragmentInfo::setStopTime(const FILETIME & time)
{
	if (CFileTime(time) == 0)
	{
		ATLTRACE("Reset stop time\n");
		stopFrameTime = 0;
	}
	else if (0 == time.dwHighDateTime)
	{
		// если времени кадра ещё нет, то считать от времени первого пришедшего кадра
		stopFrameTime = lastFrameTime + time.dwLowDateTime;
		PRINT_TIMESTAMP("Set relative stop time", stopFrameTime);
	}
	else
	{
		stopFrameTime = time;
		PRINT_TIMESTAMP("Set stop time", stopFrameTime);
	}
}


bool FragmentInfo::isFullFragment(const FILETIME & seqTime) const
{
	if (fragmentDuration > 0)
	{
		switch (preRecordType)
		{
		case BottomLessBuffer::prtSequencesCount:
			return seqNum >= fragmentDuration;
		case BottomLessBuffer::prtDurationSeconds:
			return (startRecTime != 0) && (lastFrameTime - firstFrameTime)
						>= CFileTime::Second * (isFirstFragment() ? (fragmentDuration + params.preRecord) :
																	 fragmentDuration);
		}
	}
	return false;
}