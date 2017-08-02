#include "thread.h"
#include <memory>
#include <assert.h>
#include <process.h>
#include "atomic.h"
#include "cpp11\make_unique.h"
#include "error_util.h"

using ext::Thread;
using namespace std;


inline DWORD getDeltaTicks(DWORD current, DWORD prev) {
	return (current < prev) ? (0xFFFFFFFFUL + current - prev + 1) : (current - prev);
}


Thread::Thread() : m_handle(nullptr),
				   m_state( tsStopped ),
				   m_startTicks(0),
				   m_threadId(0)
{
	//m_threadStarted = ::CreateEventW( 0, true, false, 0 );
}


Thread::~Thread()
{
	if( tsWorking == m_state ) {
		Join();
	}
	//::CloseHandle( m_threadStarted );
}


unsigned int CALLBACK Thread::ThreadCallback( void *who )
{
	using namespace ov_util;
	Thread *thread = reinterpret_cast< Thread* >( who );
	ThreadState prevState = atomic_exchange32(&thread->m_state, tsWorking);

	try {
		//::SetEvent(m_threadStarted);
		thread->Execute();
		prevState = tsStopped;
	} catch (...) {
		prevState = tsInterrupted;
	}
	prevState = atomic_exchange32(&thread->m_state, prevState);
	//
	BOOST_ASSERT_MSG(tsWorking == prevState, "Invalid working state for thread");
	DWORD dw = ::WaitForSingleObjectEx(::GetCurrentThread(), 0, true);
	BOOST_ASSERT_MSG(WAIT_TIMEOUT == dw || WAIT_IO_COMPLETION == dw, "Wait thread failed");
	(void)dw;
	return 0;
}


bool Thread::Start()
{
	DWORD currentTicks = 0;
	ThreadState prevState = ov_util::atomic_exchange32(&m_state, Thread::tsPreparing);

	switch (prevState)
	{
	case tsWorking:
		return true;
	case tsStopped:
	case tsInterrupted:
		// поток не запущен. сбрасываем событие
		//::ResetEvent(m_threadStarted);
		m_startTicks = GetTickCount();
		m_handle = reinterpret_cast< HANDLE >( ::_beginthreadex(0, 0, ThreadCallback, this, 0, &m_threadId) );
		break;
	case tsPreparing:
		// timeout
		if (!IsWorking()) {
			ResetState();
			return Start();   // дубль 2
		}
		break;
	}
	return m_handle != NULL;
}


void Thread::Join()
{
	// wait if that is another thread
	if (m_handle)
	{
		BOOST_ASSERT_MSG(::GetCurrentThreadId() != m_threadId, "Join was called for itself");
		BOOST_VERIFY(WAIT_OBJECT_0 == ::WaitForSingleObject( m_handle,  INFINITE ));
		ResetState();
	}
}


void Thread::Terminate()
{
	if (m_handle)
	{
		::TerminateThread( m_handle, -1 );
		ResetState();
	}
}


void Thread::ResetState()
{
	::CloseHandle( m_handle );
	m_handle = NULL;
	m_startTicks = 0;
	m_threadId = 0;
	ov_util::atomic_exchange32(&m_state, Thread::tsStopped);
}


bool Thread::IsWorking() const
{
	ThreadState currentState( m_state );
	return (tsWorking == currentState) || (tsPreparing == currentState);
}


HANDLE Thread::GetHandle() const
{
	return m_handle;
}


template<>
void CALLBACK Thread::APCCommand< Thread::APCHeader >(ULONG_PTR param)
{
	BOOST_ASSERT_MSG(param != 0, "Null APC parameter");
	//
	unique_ptr< APCHeader > cmd(reinterpret_cast<APCHeader *>(param));
	cmd->instance->ExecuteAPC(std::move(cmd->callback));
}

void Thread::ExecuteAPC(APCFunction f)
{
	if (IsWorking()) {
		f(this);   // run this command
	}
	else {
		//puts("APC call is rejected");
	}
}


bool Thread::QueueAPC(PAPCFUNC cb, ULONG_PTR data) const
{
	if (!IsWorking()) {
		//puts("APC call is ignored (thread isn't active)");
		return false;
	}
	return ::QueueUserAPC(cb, GetHandle(), data) != 0;
}


bool Thread::PushCommand(APCFunction f)
{
	auto cmd = make_unique< APCHeader >();
	cmd->instance = this;
	cmd->callback.swap(f);

	if (!QueueAPC(APCCommand< APCHeader >, cmd.get()))
	{
		//puts("Thread::PushCommand: QueueAPC failed");
		return false;
	}
	cmd.release();
	return true;
}