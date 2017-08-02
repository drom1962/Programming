#include "StdAfx.h"
#include "..\..\BottomLess_Buffer.h"
#include <boost\bind.hpp>
#include <algorithm>
#include <atltime.h>

using namespace bottomLessBuffer;

// ��� ������ ������������������
template<
	typename Iter,
	template<typename> class LRel,
	template<typename> class RRel
>
inline
Iter FindSequence(Iter first, Iter last, const FILETIME & time )
{
	return std::adjacent_find(
			first,
			last,
			boost::bind(
				LRel<CFileTime>(),
				boost::bind( &BottomLessBuffer::PairTimeSeq::first, _1 ),
				time) &&
			boost::bind(
				RRel<CFileTime>(),
				boost::bind( &BottomLessBuffer::PairTimeSeq::first, _2 ),
				time)
			);
}

template<typename Iter>
inline
Iter FindSequence(Iter first, Iter last, const FILETIME & time )
{
	using namespace std;
	return FindSequence<Iter, less_equal, greater>(first, last, time);
}

template<typename Iter>
inline
std::reverse_iterator< Iter > FindSequence(std::reverse_iterator< Iter > first,
										   std::reverse_iterator< Iter > last,
										   const FILETIME & time)
{
	using namespace std;
	// ��������� ����������� �������� ����� �� [a, b) -> b, �� ����������
	return next(FindSequence< reverse_iterator< Iter >, greater, less_equal >(first, last, time));
}

template<typename Iter>
Iter internalFindHelper(Iter itBegin, Iter itEnd, const FILETIME& time)
{
	if (itBegin == itEnd) {
		return itEnd;       // ����� ����
	}
	if (CFileTime(time) < itBegin->first) {
		return itBegin;     // ������� ����� ������������ ��������
	}
	if (CFileTime(time) >= std::prev(itEnd)->first) {
		return (--itEnd);   // ������� ����� ����� ��������
	}
	return FindSequence( itBegin, itEnd, time );
}

template<typename Iter>
std::reverse_iterator< Iter > internalFindHelper(std::reverse_iterator< Iter > itBegin,
												 std::reverse_iterator< Iter > itEnd,
												 const FILETIME& time)
{
	if (itBegin == itEnd) {
		return itEnd;       // ����� ����
	}
	if (CFileTime(time) >= itBegin->first) {
		return itBegin;     // ������� ����� ����� ��������
	}
	if (CFileTime(time) < prev(itEnd)->first) {
		return (--itEnd);   // ������� ����� ������������ ��������
	}
	return FindSequence( itBegin, itEnd, time );
}


bool BottomLessBuffer::setPreRecordAreaSize( int size, PreRecordType type )
{
	if( size < 0 ) {
		return false;
	}
	sh_Lock::AutoExclusiveLocker lock( locker );
	preRecAreaSize = size;
	preRecType = type;

	return true;
}


void BottomLessBuffer::setPointerOnPreRecording()
{
	sh_Lock::AutoExclusiveLocker lock( locker );
	internalSetPreRecord(curSeq);
}


void BottomLessBuffer::setPointerOnPreRecording(int size)
{
	sh_Lock::AutoExclusiveLocker lock( locker );
	internalSetPreRecord(curSeq, size);
}


bool BottomLessBuffer::setPointerToPostion(const FILETIME & key)
{
	sh_Lock::AutoExclusiveLocker lock( locker );
	curSeq = internalFind(key);
	//
	if (curSeq != buffer.end())
	{
		//internalSetPreRecord(curSeq, preRecSize);
		std::for_each(buffer.begin(), curSeq, [](ListSequences::value_type & vt) {
			vt.second.usedByWriter = true;
		});
		curSeq->second.usedByWriter = false;   // ����� �������� �� ������� ������������������ ����� �������
	}
	return curSeq != buffer.end();
}


int BottomLessBuffer::getPreRecordAreaSize(PreRecordType *type) const
{
	sh_Lock::AutoSharedLocker lock( locker );
	if (type)
		*type = preRecType;
	//
	return preRecAreaSize;
}


void BottomLessBuffer::internalSetPreRecord(iterator & curSeq, int size)
{
	// ����� ������� ��� ��� �� �������...
	// "������������" ������ ���������� �� ������������ �������� ����������...
	// �� ���� ������� ����������� � getPreRecAreaSize. �� �� ����� �� �����
	int oldPreRecSize = preRecAreaSize;
	// ���� ����-�� ���������� ������� ������, ��� ����������������, �� �� ������ �����������, ��� ������ �� ��������...
	preRecAreaSize = (std::min)(preRecAreaSize, size);
	internalSetPreRecord(curSeq);
	preRecAreaSize = oldPreRecSize;    // ������� �� �� �����
}


void BottomLessBuffer::internalSetPreRecord(iterator & curSeq)
{
	// ��������������� �� �������� ���������� ������� ��������
	if (!preRecAreaSize || buffer.empty()) {
		return;
	}
	if (prtDurationSeconds == preRecType)
	{
		if (buffer.end() == curSeq) {
			curSeq = std::prev(buffer.end());   // ���� ������� �� �������� ������ - ����� ��������� �������
		}
		CFileTime fFirstPreRecSequence( curSeq->first );
		fFirstPreRecSequence -= CFileTime::Second * preRecAreaSize;
		ListSequences::reverse_iterator revCurSeq(++curSeq);

		revCurSeq = internalFindHelper(revCurSeq, buffer.rend(), fFirstPreRecSequence);
		curSeq = revCurSeq.base();

		if (revCurSeq != buffer.rend()) {
			--curSeq;
			ATLASSERT(fFirstPreRecSequence >= curSeq->first);
			ATLASSERT(fFirstPreRecSequence >= revCurSeq->first);
			ATLASSERT(CFileTime(curSeq->first) == CFileTime(revCurSeq->first));
			ATLASSERT(curSeq->second.stream == revCurSeq->second.stream);
		}
	}
	else {
		for (int i = preRecAreaSize; curSeq != buffer.begin() && i > 0; --curSeq, --i);
	}
}


bool BottomLessBuffer::hasPreRecord() const
{
	bool bResult = false;

	if (prtDurationSeconds == preRecType)
	{
		if (preRecAreaSize) {
			unsigned long long nTicks = preRecAreaSize * CFileTime::Second;
			if (buffer.size() > 2)
				bResult = ( CFileTime( buffer.back().first ) - std::next(buffer.begin())->first) >= nTicks;
		}
		else
			bResult = !buffer.empty();
	}
	else {
		bResult = buffer.size() > static_cast< unsigned >( preRecAreaSize );
	}
	return bResult;
}



bool BottomLessBuffer::pop_front( bool removedRegardlessFlag )
{
	sh_Lock::AutoExclusiveLocker lock( locker );

	if (buffer.empty() || !hasPreRecord()) {
		return false;
	}
	iterator beg = buffer.begin(); 
	if( beg->second.usedByWriter || removedRegardlessFlag ) {
		if( beg == curSeq ) {
			++curSeq;
		}
		IStream *pIStm = beg->second.stream;
		if (pIStm)
			pIStm->Release();

		buffer.pop_front();
		ATLTRACE("BottomLessBuffer: Delete seq: %d\n", buffer.size());
		return true;
	}
	return false;
}


void BottomLessBuffer::push_back( const FILETIME & curTime, IStream *val )
{
	if( val ) {
		sh_Lock::AutoExclusiveLocker lock( locker );
		//
		val->AddRef();
		ATLASSERT(buffer.empty() || CFileTime(curTime) >= buffer.back().first);
		buffer.push_back( PairTimeSeq( curTime, Sequence(val) ) );
		ATLTRACE("BottomLessBuffer: Add seq: %d\n", buffer.size());
		if (curSeq == buffer.end())
			curSeq = std::prev(buffer.end());
	}
}


BottomLessBuffer::iterator BottomLessBuffer::internalFind(const FILETIME& time)
{
	return internalFindHelper<iterator>(buffer.begin(), buffer.end(), time);
}


BottomLessBuffer::PairTimeSeq
	BottomLessBuffer::find( const FILETIME& key, bool preRec ) const
{
	sh_Lock::AutoSharedLocker lock( locker );
	BottomLessBuffer * const mutable_this = const_cast<BottomLessBuffer *>(this);
	iterator elem = mutable_this->internalFind( key );

	if( buffer.end() == elem ) {
		return PairTimeSeq();
	}
	if (preRec) {
		mutable_this->internalSetPreRecord(elem);
	}
	elem->second.stream->AddRef();
	return *elem;
}


BottomLessBuffer::PairTimeSeq
	BottomLessBuffer::getCurrentSequence() const
{
	sh_Lock::AutoSharedLocker lock( locker );
	return (curSeq != buffer.end()) ? *curSeq : PairTimeSeq(CFileTime(), Sequence(NULL));
}


bool BottomLessBuffer::nextSequence( PairTimeSeq & seq )
{
	sh_Lock::AutoExclusiveLocker lock( locker );
	//
	if( curSeq != buffer.end() )
	{
		seq = *curSeq;
		seq.second.stream->AddRef();
		curSeq->second.usedByWriter = true;
		++curSeq;
		return true;
	}
	FILETIME f = {0};
	seq = PairTimeSeq(f, Sequence(NULL));

	return false; 
}


bool BottomLessBuffer::nextSequence() const
{
	sh_Lock::AutoSharedLocker lock( locker );
	return curSeq != buffer.end();
}


void BottomLessBuffer::clear()
{
	sh_Lock::AutoExclusiveLocker lock( locker );

	std::for_each(buffer.begin(), buffer.end(), FSequenceDeleter());
	buffer.clear();
	curSeq = buffer.end();
}