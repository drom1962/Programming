#pragma once
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <windows.h>


namespace ext
{

class Thread : boost::noncopyable
{
	enum ThreadState
	{
		tsStopped = 0,
		tsPreparing,
		tsWorking,
		tsInterrupted
		//tsFinished
	};

public:
	typedef boost::function<void(Thread *)> APCFunction;

private:
	struct APCHeader
	{
		Thread *instance;
		APCFunction callback;
	};

	static const DWORD startThreadTimeoutMsec = 5000;

public:
	Thread();
	virtual ~Thread();

	virtual bool Start();
	virtual void Join();
	virtual void Terminate();

	bool IsWorking() const;

protected:
	virtual void Execute() = 0;
	virtual void ExecuteAPC(APCFunction f);
	bool PushCommand(APCFunction f);

	template<class Fn>
	bool PushFastCommand(Fn f);

	HANDLE GetHandle() const;

private:
	static unsigned int CALLBACK ThreadCallback( void *who );

	template<class Fn>
	static void CALLBACK APCCommand(ULONG_PTR param);

	template<>
	static void CALLBACK APCCommand< APCHeader >(ULONG_PTR param);

	void ResetState();
	bool QueueAPC(PAPCFUNC cb, ULONG_PTR data) const;
	template<typename T> bool QueueAPC(PAPCFUNC cb, T *data) const { return QueueAPC(cb, reinterpret_cast<ULONG_PTR>(data)); }

private:
	HANDLE						m_handle;
	//HANDLE  m_threadStarted;
	ThreadState					m_state;
	DWORD						m_startTicks;
	unsigned					m_threadId;
};

template<class Fn>
void CALLBACK Thread::APCCommand(ULONG_PTR param)
{
	assert(param != 0);
	std::unique_ptr< Fn > cmd(reinterpret_cast<Fn *>(param));
	(*cmd)();
}

template<class Fn>
bool Thread::PushFastCommand(Fn f)
{
	using namespace std;
	auto cmd = make_unique< Fn >(move(f));
	if (!QueueAPC(APCCommand<Fn>, cmd.get()))
	{
		return false;
	}
	cmd.release();
	return true;
}

}
