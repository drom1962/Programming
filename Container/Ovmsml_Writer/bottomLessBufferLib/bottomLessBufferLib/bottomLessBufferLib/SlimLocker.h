#pragma once

#include <boost/thread/shared_mutex.hpp>

namespace sh_Lock{

	class boost_shared_locker{
		public:
			boost_shared_locker(){}
			void exclusiveLock(){
				m_mtx.lock();
			}
			bool tryExclusiveLock() {
				return m_mtx.try_lock();
			}
			void exclusiveUnlock() {
				m_mtx.unlock();
			}
			void sharedLock(){
				m_mtx.lock_shared();
			}
			bool trySharedLock(){
				return m_mtx.try_lock_shared();
			}
			void sharedUnlock(){
				m_mtx.unlock_shared();
			}

		private:
			boost::shared_mutex m_mtx;
			boost_shared_locker( const boost_shared_locker & );
			boost_shared_locker const & operator=( const boost_shared_locker & );
	};

#if WINVER >= 0x0600
	class slim_locker{
		public:
			slim_locker(){
				::InitializeSRWLock( &lock );
			}
			
			void exclusiveLock(){
				::AcquireSRWLockExclusive( &lock );
			}

			bool tryExclusiveLock(){
				return 0 != ::TryAcquireSRWLockExclusive( &lock );
			}

			void exclusiveUnlock(){
				::ReleaseSRWLockExclusive( &lock );
			}

			void sharedLock(){
				::AcquireSRWLockShared( &lock );
			}

			bool trySharedLock(){
				return 0 != ::TryAcquireSRWLockShared( &lock );
			}

			void sharedUnlock(){
				::ReleaseSRWLockShared( &lock );
			}

			::SRWLOCK* handle(){
				return &lock;
			}

		private:
			::SRWLOCK lock;
			slim_locker( const slim_locker & );
			slim_locker const & operator=( const slim_locker & );
	};
#endif

	class AutoSharedLocker{
		public:
			AutoSharedLocker( boost_shared_locker& lock ) : locker( lock ){
				locker.sharedLock();
			}
			~AutoSharedLocker(){
				locker.sharedUnlock();
			}
		private:
			boost_shared_locker& locker;
			AutoSharedLocker( const boost_shared_locker& );
			AutoSharedLocker const & operator=( const boost_shared_locker& );
	};

	class AutoExclusiveLocker{
		public:
			AutoExclusiveLocker( boost_shared_locker& lock ) : locker( lock ){
				locker.exclusiveLock();
			}
			~AutoExclusiveLocker(){
				locker.exclusiveUnlock();
			}
		private:
			boost_shared_locker& locker;
			AutoExclusiveLocker( const boost_shared_locker& );
			AutoExclusiveLocker const & operator=( const boost_shared_locker& );
	};

}