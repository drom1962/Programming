#pragma once

#include <list>
#include <Ole2.h>
#include <WinBase.h>
#include "..\..\common\Locker.h"
#include "bottomLessBufferLib\bottomLessBufferLib\SlimLocker.h"



namespace bottomLessBuffer
{
	struct Sequence
	{
		Sequence( IStream* stream ) : stream( stream ), usedByWriter( false ){};
		Sequence() : stream( 0 ), usedByWriter( false ){};
 		IStream* stream;
		bool usedByWriter;
	};

	class BottomLessBuffer
	{
	public:
		typedef std::pair< FILETIME, Sequence > PairTimeSeq;

		enum PreRecordType
		{
			prtSequencesCount,
			prtDurationSeconds
		};
	private:
		typedef std::list< PairTimeSeq > ListSequences;
		typedef ListSequences::iterator iterator;
		typedef ListSequences::const_iterator const_iterator;

		struct FSequenceDeleter {
			void operator ()(PairTimeSeq & elem) const {
				if (elem.second.stream) {
					elem.second.stream->Release();
					elem.second.stream = NULL;
				}
			}
		};

	public:
		BottomLessBuffer( int preRecSize ):
			preRecAreaSize( preRecSize ),
			preRecType(prtSequencesCount)
		{
			curSeq = buffer.end();
		}
		BottomLessBuffer() :
			preRecAreaSize(0),
			preRecType(prtSequencesCount)
		{
			curSeq = buffer.end();
		}
		~BottomLessBuffer() {
			clear();
		}

		bool pop_front( bool removedRegardlessFlag = false );

		//вставляет элемент val c ключом равным текущему времени
		void push_back( const FILETIME & curTime, IStream *val );
		//
		void push_back( const PairTimeSeq & seq ) {
			push_back(seq.first, seq.second.stream);
		}
		//возвращает итератор на первый элемент с ключом = key
		//или в случае если такого элемента не существует возвращает
		//первый элемент с ключом меньшим key
		//с флагом preRec возвращает итератор на элемент
		//предшествующий найденному на i позиций ( 0 <= i <= preRecAreaSize )
		PairTimeSeq find( const FILETIME& key, bool preRec = false ) const;
		//
		PairTimeSeq getCurrentSequence() const;

		bool nextSequence( PairTimeSeq & seq );
		bool nextSequence() const;

		bool setPointerToPostion(const FILETIME & key);
		bool setPreRecordAreaSize( int size, PreRecordType type );
		void setPointerOnPreRecording(int size);
		void setPointerOnPreRecording();

		size_t count() const { 
			sh_Lock::AutoSharedLocker lock( locker );
			return buffer.size(); 
		}
		int getPreRecordAreaSize(PreRecordType *type = NULL) const;
		void clear();

	private:
		iterator internalFind( const FILETIME& key );
		void internalSetPreRecord(iterator & curSeq, int size);
		void internalSetPreRecord(iterator & curSeq);
		bool hasPreRecord() const;

	private:
		iterator curSeq;
		mutable sh_Lock::boost_shared_locker locker;
		PreRecordType preRecType;
		int preRecAreaSize;
		ListSequences buffer;
	};

}