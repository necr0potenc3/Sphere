//
// cthread.cpp
//

#include "graycom.h"
#include "../graysvr/graysvr.h"

#ifdef _WIN32
#include <process.h>    // _beginthread, _endthread
#else	// LINUX
#include <pthread.h>	// pthread_create
#endif

/////////////
// -CThread

void CThread::CreateThread( PTHREAD_ENTRY_PROC pEntryProc, void * pArgs )
{
	if ( IsActive())
		return;	// Already active.

	fprintf( stderr, "Creating thread." DEBUG_CR );

#ifdef _WIN32
	m_hThread = _beginthread( pEntryProc, 0, pArgs );
	if ( m_hThread == -1 )
	{
		m_hThread = NULL;
	}
#else
	pthread_attr_init(&m_hThread_attr);
	int status = pthread_create( &m_hThread, &m_hThread_attr, pEntryProc, pArgs );
	if ( status != 0 )
	{
		DEBUG_ERR(("Unable to create new thread\n"));
		m_hThread = NULL;
	}
#endif
}

void CThread::CreateThread( PTHREAD_ENTRY_PROC pEntryProc )
{
	CreateThread( pEntryProc, this );
}

DWORD CThread::GetCurrentThreadId() // static
{
#ifdef _WIN32
	return( ::GetCurrentThreadId());
#else
	return((DWORD) pthread_self());
#endif
}

bool CThread::TerminateThread( DWORD dwExitCode )
{
	if ( ! IsActive())
		return true;
#ifdef _WIN32
	if ( m_dwThreadID == GetCurrentThreadId())
	{
		_endthread();	// , dwExitCode
	}
	else
	{
		// Terminate a thread that is not us.
		BOOL fRet = ::TerminateThread( (HANDLE)m_hThread, dwExitCode );
		if ( fRet )
		{
			CloseHandle( (HANDLE) m_hThread );
		}
		else
		{
			DEBUG_CHECK( fRet );
		}
	}
#else
	pthread_attr_destroy(&m_hThread_attr);
	// LINUX code to force a thread to exit !
	// But it may not be THIS thread !
	if ( m_dwThreadID == (DWORD) pthread_self())
	{
		pthread_exit((void *) &dwExitCode);
	}
	else
	{
		// Terminate a thread that is not us.
		pthread_kill( m_hThread, 1 );	// int sig
	}
#endif
	OnClose();
	return( true );
}

void CThread::WaitForClose( int iSec )
{
	// wait for this thread to close.
	int icount = iSec*10;
	while ( IsActive() && icount -- )
	{
		Sleep(100);	// milliseconds
	}

	// Try to terminate it hard.
	TerminateThread( -1 );
}

void CThread::OnClose()
{
	m_hThread = NULL;
}

//*************************************************8
// -CThreadLockableObj

CThreadLockableObj::CThreadLockableObj()
{
#ifdef _WIN32
	InitializeCriticalSection(&m_LockSection);
#else
	pthread_mutex_init(&m_LockSection,0);
#endif
}
CThreadLockableObj::~CThreadLockableObj()
{
	// DEBUG_CHECK( ! IsLocked() );
#ifdef _WIN32
	DeleteCriticalSection(&m_LockSection);
#else
	pthread_mutex_destroy(&m_LockSection);
#endif
}

bool CThreadLockableObj::IsLocked()
{
	// NOTE: In Win98 the m_LockSection is not correctly defined !
#ifdef _WIN32
 // This is not accurate in NT vs win98
	return( m_LockSection.OwningThread != 0 );
#else
	// There isn't a way to simply check if a mutex is locked.
	if ( pthread_mutex_trylock(&m_LockSection) )
	{
		// It wasn't locked....but now it is...
		pthread_mutex_unlock(&m_LockSection);
		return false;
	}
	else
		return true;
#endif
}

void CThreadLockableObj::UnThreadLock()
{
	return;	// Vjaka: temporaly test-disabled
	// DEBUG_CHECK( IsLocked());	// may have several locks on the same thread.
#ifdef _WIN32
	LeaveCriticalSection(&m_LockSection);
#else
	pthread_mutex_unlock(&m_LockSection);
#endif
}

void CThreadLockableObj::DoThreadLock()
{
	return;	// Vjaka: temporaly test-disabled
	// code to lock a thread.
	// This will wait forever for the resource to be free !
#ifdef _WIN32
	EnterCriticalSection(&m_LockSection);
#else
	// ??? No idea if this will handle multi access by the same thread correctly !
	// it probably is ok, but if not, then we must check if this is already locked by our same thread and skip it !
	// Nope...it doesn't
	if ( !IsLocked() )
	pthread_mutex_lock(&m_LockSection);
#endif
	// DEBUG_CHECK( IsLocked());	// may have several locks on the same thread.
}

//*************************************************8
// -CThreadLockRef

void CThreadLockRef::ReleaseRef()
{
	if ( m_pLink != NULL )
	{
		m_pLink->UnThreadLock();
		m_pLink = NULL;
	}
}

void CThreadLockRef::SetRef( CThreadLockableObj* pLink )
{
	ReleaseRef();
	if (pLink)
	{
		pLink->DoThreadLock();
		m_pLink = pLink;
	}
}

//*************************************************8
// -CThreadLockBase

CThreadLockBase::CThreadLockBase()
{
#ifdef _WIN32
	if ( SUCCEEDED(CoInitialize(NULL)) )
	{
		CoUninitialize();
		m_bInMTA = true;
	}
	else m_bInMTA = false;
#else
	m_bInMTA = true;
#endif

	if ( m_bInMTA )
#ifdef _WIN32
		InitializeCriticalSection(&m_csSafeAccess);
#else
		pthread_mutex_init(&m_csSafeAccess, NULL);
#endif
	m_iInCrit = 0;
}

CThreadLockBase::~CThreadLockBase()
{
	if ( m_bInMTA )
	{
		if ( m_iInCrit )
		{
#ifdef _WIN32
			LeaveCriticalSection(&m_csSafeAccess);
#else
			pthread_mutex_unlock(&m_csSafeAccess);
#endif
			m_iInCrit = 0;
		}
#ifdef _WIN32
		DeleteCriticalSection(&m_csSafeAccess);
#else
		pthread_mutex_destroy(&m_csSafeAccess);
#endif
		m_bInMTA = false;
	}
}

// Note by Furio: this code doesn't prevent deadlocks! Better rewriting with TryEnter.. and _trylock ???
void CThreadLockBase::Lock()
{
	if ( m_bInMTA )
	{
//		if ( !g_Serv.IsLoading() ) DEBUG_ERR(("Entering Critical section (%i)\n", m_iInCrit));
		if ( !m_iInCrit )
		{
#ifdef _WIN32
			EnterCriticalSection(&m_csSafeAccess);
#else
			pthread_mutex_lock(&m_csSafeAccess);
#endif
			m_iInCrit++;
		}
	}
}

void CThreadLockBase::Unlock()
{
	if ( m_bInMTA )
	{
//		if ( !g_Serv.IsLoading() ) DEBUG_ERR(("Leaving Critical section (%i)\n", m_iInCrit));
		m_iInCrit--;
		if ( !m_iInCrit ) 
#ifdef _WIN32
			LeaveCriticalSection(&m_csSafeAccess);
#else
			pthread_mutex_unlock(&m_csSafeAccess);
#endif
	}
}

//*************************************************8
// -CThreadLock

CThreadLock::CThreadLock()
{
	g_lock.Lock();
}
CThreadLock::~CThreadLock()
{
	g_lock.Unlock();
}
