//
// GRAYSRV.CPP.
// Copyright Menace Software (www.menasoft.com).
//
// game/login server for uo client
// http://www.menasoft.com for more details.
// I'm liking http://www.cs.umd.edu/users/cml/cstyle/CppCodingStandard.html as a coding standard.
//
// [menace@unforgettable.com] Dennis Robinson, http://www.menasoft.com
// [nixon@mediaone.net] Nix, Scripts
// [petewest@mindspring.com] Westy, /INFO, Maps
// [johnr@cyberstreet.com] John Davey (Garion)(The Answer Guy) docs.web pages.
// [bytor@mindspring.com] Philip Esterle (PAE)(Altiara Dragon) tailoring,weather,moons,LINUX
//
// UNKNOWN STATUS:
// [will@w_tigre.cuug.ab.ca] Will Merkins  (Smeg) LINUX
// [allmight@rospiggen.com] Mans Sjoberg (Allmight)
// [gander@jester.vte.com] Gerald D. Anderson (ROK) mailing list
// [kylej@blueworld.com] Kyle Jessup (KRJ) Line of sight.
// [udc@home.com] ZARN Brad Patten,
// [kmayers@gci.net] Keif Mayers (Avernus)
// [damiant@seanet.com] Damian Tedrow , http://www.ayekan.com/awesomedev
// [alluvian@onramp.net] Alluvian, Charles Manick Livermore
//
#include "graysvr.h"	// predef header.
#include "../common/grayver.h"	// sphere version

#if !defined(pid_t)
#define pid_t int
#endif

#ifdef _WIN32
#include "ntservice.h"	// g_Service
#include <process.h>	// getpid()
#else
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

int	ATOI( const char * str )
{
	int	res;
	sscanf( str, "%d", &res );
	return res;
}

char * ITOA(int value, char *string, int radix)
{
	sprintf(string, (radix == 16) ? "%x" : "%d", value);
	return string;
}

char * LTOA(long value, char *string, int radix)
{
	sprintf(string, (radix == 16) ? "%lx" : "%ld", value);
	return string;
}

void STRREV( char* string )
{
     char *pEnd = string;
     char temp;     
     while (*pEnd) pEnd++;  
     pEnd--;                         
     while (string < pEnd) 
     {
          temp = *pEnd;            
          *pEnd-- = *string;
          *string++ = temp;       
     }
}

#endif

CMapList::CMapList()
{
	memset(m_mapsinitalized, false, sizeof(m_mapsinitalized));
	memset(m_sizex, 0, sizeof(m_sizex));
	memset(m_sizey, 0, sizeof(m_sizey));
	memset(m_maps, true, sizeof(m_maps));
	memset(m_mapnum, -1, sizeof(m_mapnum));
	memset(m_sectorsize, 0, sizeof(m_sectorsize));

	Load(0, 0x1800, 0x1000, 64, 0);	// #0 map0.mul
	Load(1, 0x1800, 0x1000, 64, 0);	// #1 map0.mul
	Load(2, 0x900, 0x640, 64, 2);	// #2 map2.mul
	Load(3, 0xa00, 0x800, 64, 3);	// #3 map3.mul
	Load(4, 0x5a8, 0x5a8, 8, 4);	// #4 map4.mul
}

bool CMapList::Load(int map, int maxx, int maxy, int sectorsize, int realmapnum)
{
	m_sizex[map] = maxx;
	m_sizey[map] = maxy;
	m_sectorsize[map] = sectorsize;
	m_mapnum[map] = realmapnum;
	return true;
}

bool CMapList::Load(int map, char *args)
{
	if (( map < 0 ) || ( map > 255 ))
	{
		g_Log.EventError("Invalid map #%d could be initialized." DEBUG_CR, map);
		return false;
	}
	else if ( !m_mapsinitalized[map] )	// disable double intialization
	{
		TCHAR	*ppCmd[4];	// maxx,maxy,sectorsize,mapnum[like 0 for map0/statics0/staidx0]
		int		iCount = Str_ParseCmds(args, ppCmd, COUNTOF(ppCmd), ",");

		if ( !iCount )	// simple MAPX= same as disabling the map
		{
			m_maps[map] = false;
		}
		else
		{
			int	maxx = 0, maxy = 0, sectorsize = 0, realmapnum = 0;
			if ( ppCmd[0] ) maxx = atoi(ppCmd[0]);
			if ( ppCmd[1] ) maxy = atoi(ppCmd[1]);
			if ( ppCmd[2] ) sectorsize = atoi(ppCmd[2]);
			if ( ppCmd[3] ) realmapnum = atoi(ppCmd[3]);

										// zero settings of anything except the real map num means
			if ( maxx )					// skipping the argument
			{
				if (( maxx < 8 ) || ( maxx % 8 ))
				{
					g_Log.EventError("MAP%d: X coord must be multiple of 8 (%d is invalid, %d is still effective)." DEBUG_CR, map, maxx, m_sizex[map]);
				}
				else m_sizex[map] = maxx;
			}
			if ( maxy )
			{
				if (( maxy < 8 ) || ( maxy % 8 ))
				{
					g_Log.EventError("MAP%d: Y coord must be multiple of 8 (%d is invalid, %d is still effective)." DEBUG_CR, map, maxy, m_sizey[map]);
				}
				else m_sizey[map] = maxy;
			}
			if ( sectorsize )
			{
				if (( sectorsize < 8 ) || ( sectorsize % 8 ))
				{
					g_Log.EventError("MAP%d: Sector size must be multiple of 8 (%d is invalid, %d is still effective)." DEBUG_CR, map, sectorsize, m_sectorsize[map]);
				}
				else if (( m_sizex[map]%sectorsize ) || ( m_sizex[map]%sectorsize ))
				{
					g_Log.EventError("MAP%d: Map dimensions [%d,%d] must be multiple of sector size (%d is invalid, %d is still effective)." DEBUG_CR, map, m_sizex[map], m_sizey[map], sectorsize, m_sectorsize[map]);
				}
				else m_sectorsize[map] = sectorsize;
			}
			if ( realmapnum >= 0 )
				m_mapnum[map] = realmapnum;
		}
		m_mapsinitalized[map] = true;
	}
	return true;
}

void CMapList::Init()
{
	for ( int i = 0; i < 256; i++ )
	{
		if ( m_maps[i] )	// map marked as available. check whatever it's possible
		{
			//	check coordinates first
			if ( !m_sizex[i] || !m_sizey[i] || !m_sectorsize[i] || ( m_mapnum[i] == -1 ) )
				m_maps[i] = false;
		}
	}
}

bool WritePidFile(int iMode = 0)
{
	LPCTSTR	file = GRAY_FILE ".pid";
	FILE	*pidFile;

	if ( iMode == 1 )		// delete
	{
		return ( unlink(file) == 0 );
	}
	else if ( iMode == 2 )	// check for .pid file
	{
		pidFile = fopen(file, "r");
		if ( pidFile )
		{
			g_Log.Event(LOGM_INIT, GRAY_FILE ".pid already exists. Secondary launch or unclean shutdown?" DEBUG_CR);
			fclose(pidFile);
		}
		return true;
	}
	else
	{
		pidFile = fopen(file, "w");
		if ( pidFile )
		{
			pid_t spherepid = getpid();
			fprintf(pidFile,"%d\n", spherepid);
			fclose(pidFile);
			return true;
		}
		g_Log.Event(LOGM_INIT, "Cannot create pid file!" DEBUG_CR);
		return false;
	}
}

int CEventLog::VEvent( DWORD wMask, LPCTSTR pszFormat, va_list args )
{
	// This is currently not overriden but just in case we might want to.
	TCHAR	*pszTemp = Str_GetTemp();
	ASSERT( pszFormat && *pszFormat );
	//size_t len = vsprintf( szTemp, pszFormat, args );	// this cause a buffer overrun
	size_t len = _vsnprintf(pszTemp, (SCRIPT_MAX_LINE_LEN - 1), pszFormat, args);
	if ( ! len ) strncpy(pszTemp, pszFormat, (SCRIPT_MAX_LINE_LEN - 1));
	return (EventStr(wMask,pszTemp));
}

LPCTSTR const g_Stat_Name[STAT_QTY] =	// not sorted obviously.
{
	"STR",
	"INT",
	"DEX",
	"FOOD",
	"KARMA",
	"FAME",
};

const CPointMap g_pntLBThrone(1323,1624,0); // This is origin for degree, sextant minute coordinates

LPCTSTR g_szServerDescription =
	"%s Version %s "
#ifdef _WIN32
	"[WIN32]"
#else
#ifdef _BSD
	"[FreeBSD]"
#else
	"[Linux]"
#endif
#endif
#ifdef _DEBUG
	"[DEBUG]"
#endif
	" by " GRAY_URL;

int CObjBase::sm_iCount = 0;	// UID table.
LONGLONG llTimeProfileFrequency = 1000;	// time profiler

// game servers stuff.
CWorld		g_World;	// the world. (we save this stuff)
CServer		g_Serv;	// current state stuff not saved.
CResource	g_Cfg;
CMainTask	g_MainTask;
CClientTask	g_ClientTask;
CBackTask	g_BackTask;
CGrayInstall g_Install;
CVerDataMul	g_VerData;
CExpression g_Exp;	// Global script variables.
CLog		g_Log;
CEventLog * g_pLog = &g_Log;
CAccounts	g_Accounts;	// All the player accounts. name sorted CAccount
CGStringList	g_AutoComplete;	// auto-complete list
TScriptProfiler g_profiler;		// script profiler
CThreadLockBase	g_lock;			// thread lock base
CMapList	g_MapList;			// global maps information

//	Tasks
CAITask		g_threadAI;		// NPC AI thread

//////////////////////////////////////////////////////////////////
// util type stuff.

//
// Provide these functions globally.
//

class CGrayAssert : public CGrayError
{
protected:
	LPCTSTR const m_pExp;
	LPCTSTR const m_pFile;
	const unsigned m_uLine;
public:
	CGrayAssert( LOGL_TYPE eSeverity, LPCTSTR pExp, 
		LPCTSTR pFile, unsigned uLine ) :
		CGrayError( eSeverity, 0, "Assert"),
		m_pExp(pExp), m_pFile(pFile), m_uLine(uLine)
	{
	}
	virtual ~CGrayAssert()
	{
	}
	virtual BOOL GetErrorMessage( LPSTR lpszError, UINT nMaxError,	UINT * pnHelpContext )
	{
		sprintf( lpszError, "Assert pri=%d:'%s' file '%s', line %d", m_eSeverity, m_pExp, m_pFile, m_uLine );
		return( true );
	}
};

void Debug_CheckFail( LPCTSTR pExp, LPCTSTR pFile, unsigned uLine )
{
	g_Log.Event( LOGL_ERROR, "Check Fail:'%s' file '%s', line %d" DEBUG_CR, pExp, pFile, uLine );
}

#ifndef NOTASSERTDEBUG

void Debug_Assert_CheckFail( LPCTSTR pExp, LPCTSTR pFile, unsigned uLine )
{
	throw CGrayAssert( LOGL_CRIT, pExp, pFile, uLine );
}

void Assert_CheckFail( LPCTSTR pExp, LPCTSTR pFile, unsigned uLine )
{
	// These get left in release code .
	throw CGrayAssert( LOGL_CRIT, pExp, pFile, uLine );
}

#else

void Debug_Assert_CheckFail( LPCTSTR pExp, LPCTSTR pFile, unsigned uLine )
{
	g_Log.Event( LOGL_ERROR, "Check Fail:'%s' file '%s', line %d" DEBUG_CR, pExp, pFile, uLine );
}

void Assert_CheckFail( LPCTSTR pExp, LPCTSTR pFile, unsigned uLine )
{
	g_Log.Event( LOGL_ERROR, "Check Fail:'%s' file '%s', line %d" DEBUG_CR, pExp, pFile, uLine );
}

#endif

#if defined(_WIN32) && defined(_CONSOLE)

BOOL WINAPI ConsoleHandlerRoutine( DWORD dwCtrlType )
{
	//  control signal type. CTRL_C_EVENT
	// SetConsoleCtrlHandler
	switch ( dwCtrlType )
	{
	case CTRL_C_EVENT: // A CTRL+c signal was received, either from keyboard input or from a signal generated by the GenerateConsoleCtrlEvent function.
	case CTRL_BREAK_EVENT: //  A CTRL+BREAK signal was received, either from keyboard input or from a signal generated by GenerateConsoleCtrlEvent.
	case CTRL_CLOSE_EVENT: // A signal that the system sends to all processes attached to a console when the user closes the console (either by choosing the Close command from the console window's System menu, or by choosing the End Task command from the Task List).
		if ( g_Cfg.m_fSecure && ! g_Serv.IsLoading())	// enable the try code.
		{
			g_Serv.SetExitFlag( 3 );
			return( TRUE );
		}
		break;
	case CTRL_LOGOFF_EVENT: // A signal that the system sends to all console processes when a user is logging off. This signal does not indicate which user is logging off, so no assumptions can be made.
	case CTRL_SHUTDOWN_EVENT: // A signal that the system sends to all console processes when the system is shutting down
		break;
	}
	return FALSE;	// process normally.
}

#endif	// _CONSOLE

#ifdef _WIN32

class CGrayException : public CGrayError
{
	// Catch and get details on the system exceptions.
	// NULL pointer access etc.
public:
	const DWORD m_dwAddress;
public:
	CGrayException( unsigned int uCode, DWORD dwAddress ) :
		m_dwAddress(dwAddress),
		CGrayError(LOGL_CRIT, uCode, "Exception")
	{
	}
	virtual ~CGrayException()
	{
	}
	virtual BOOL GetErrorMessage( LPTSTR lpszError, UINT nMaxError,	UINT * pnHelpContext )
	{
		static LPCTSTR const m_messages[] =
		{
			"Segment Notification",
			"Guard Page Violation",
			"Data Type Misalignment",
			"Breakpoint",
			"Access Violation",
			"Invalid Handle",
			"No Memory",
			"Illegal Instruction",
			"Noncontinuable Exception",
			"Array Bounds Exceeded",
			"Float: Divide by Zero",
			"Float: Overflow",
			"Float: Stack Check",
			"Float: Underflow",
			"Integer: Divide by Zero",
			"Integer: Overflow",
			"Stack Overflow",
		};

		const char *zMsg = NULL;
		switch ( m_hError )
		{
		case STATUS_SEGMENT_NOTIFICATION:		zMsg = m_messages[0]; break;
		case STATUS_GUARD_PAGE_VIOLATION:		zMsg = m_messages[1]; break;
		case STATUS_DATATYPE_MISALIGNMENT:		zMsg = m_messages[2]; break;
		case STATUS_BREAKPOINT:					zMsg = m_messages[3]; break;
		case STATUS_ACCESS_VIOLATION:			zMsg = m_messages[4]; break;
		case STATUS_INVALID_HANDLE:				zMsg = m_messages[5]; break;
		case STATUS_NO_MEMORY:					zMsg = m_messages[6]; break;
		case STATUS_ILLEGAL_INSTRUCTION:		zMsg = m_messages[7]; break;
		case STATUS_NONCONTINUABLE_EXCEPTION:	zMsg = m_messages[8]; break;
		case STATUS_ARRAY_BOUNDS_EXCEEDED:		zMsg = m_messages[9]; break;
		case STATUS_FLOAT_DIVIDE_BY_ZERO:		zMsg = m_messages[10]; break;
		case STATUS_FLOAT_OVERFLOW:				zMsg = m_messages[11]; break;
		case STATUS_FLOAT_STACK_CHECK:			zMsg = m_messages[12]; break;
		case STATUS_FLOAT_UNDERFLOW:			zMsg = m_messages[13]; break;
		case STATUS_INTEGER_DIVIDE_BY_ZERO:		zMsg = m_messages[14]; break;
		case STATUS_INTEGER_OVERFLOW:			zMsg = m_messages[15]; break;
		case STATUS_STACK_OVERFLOW:				zMsg = m_messages[16]; break;
		}
		if ( zMsg ) sprintf(lpszError, "Exception \"%s\" code=0%0x, addr=0%0x", zMsg, m_hError, m_dwAddress);
		else sprintf(lpszError, "Exception code=0%0x, addr=0%0x", m_hError, m_dwAddress);
		return true;
	}
};

// Don't do this in Debug Builds
// Exceptions are caught by our IDE's
#if !defined( _DEBUG )

extern "C"
{

void _cdecl Sphere_Exception_Win32( unsigned int id, struct _EXCEPTION_POINTERS* pData )
{
	// WIN32 gets an exception.
	DWORD dwCodeStart = (DWORD)(BYTE *) &globalstartsymbol;	// sync up to my MAP file.

	DWORD dwAddr = (DWORD)(pData->ExceptionRecord->ExceptionAddress);
	dwAddr -= dwCodeStart;

	throw CGrayException(id, dwAddr);
}

int _cdecl _purecall()
{
	// catch this special type of C++ exception as well.
	Assert_CheckFail( "purecall", "unknown", 1 );
	return 0;
}
}	// extern "C"

#endif // _DEBUG

#endif	// _WIN32

DIR_TYPE GetDirStr( LPCTSTR pszDir )
{
	int iDir2, iDir = toupper( pszDir[0] );

	switch ( iDir )
	{
	case 'E': return( DIR_E );
	case 'W': return( DIR_W );
	case 'N':
		iDir2 = toupper(pszDir[1]);
		if ( iDir2 == 'E' ) return( DIR_NE );
		if ( iDir2 == 'W' ) return( DIR_NW );
		return( DIR_N );
	case 'S':
		iDir2 = toupper(pszDir[1]);
		if ( iDir2 == 'E' ) return( DIR_SE );
		if ( iDir2 == 'W' ) return( DIR_SW );
		return( DIR_S );
	default:
		if (( iDir >= '0' ) && ( iDir <= '7' ))
			return (DIR_TYPE)(iDir - '0');
	}
	return( DIR_QTY );
}

LPCTSTR GetTimeMinDesc( int minutes )
{
	if ( minutes < 0 )
	{
		TCHAR * pTime = Str_GetTemp();
		strcpy( pTime, "Hmmm...I can't tell what time it is!");
		return( pTime );
	}
	int minute = minutes % 60;
	int hour = ( minutes / 60 ) % 24;

	LPCTSTR pMinDif;
	if (minute<=14) pMinDif = "";
	else if ((minute>=15)&&(minute<=30)) 
		pMinDif = " a quarter past";
	else if ((minute>=30)&&(minute<=45)) 
		pMinDif = " half past";
	else
	{
		pMinDif = " a quarter till";
		hour = ( hour + 1 ) % 24;
	}

	static LPCTSTR const sm_ClockHour[] =
	{
		"midnight",
		"one",
		"two",
		"three",
		"four",
		"five",
		"six",
		"seven",
		"eight",
		"nine",
		"ten",
		"eleven",
		"noon",
	};

	LPCTSTR pTail;
	if ( hour == 0 || hour==12 )
		pTail = "";
	else if ( hour > 12 )
	{
		hour -= 12;
		if ((hour>=1)&&(hour<6))
			pTail = " o'clock in the afternoon";
		else if ((hour>=6)&&(hour<9))
			pTail = " o'clock in the evening.";
		else
			pTail = " o'clock at night";
	}
	else
	{
		pTail = " o'clock in the morning";
	}

	TCHAR * pTime = Str_GetTemp();
	sprintf( pTime, "%s %s%s.", pMinDif, sm_ClockHour[hour], pTail );
	return( pTime );
}

int FindStrWord( LPCTSTR pTextSearch, LPCTSTR pszKeyWord )
{
	// Find the pszKeyWord in the pTextSearch string.
	// Make sure we look for starts of words.

	int j=0;
	for ( int i=0; 1; i++ )
	{
		if ( pszKeyWord[j] == '\0' )
		{
			if ( pTextSearch[i]=='\0' || ISWHITESPACE(pTextSearch[i]))
				return( i );
			return( 0 );
		}
		if ( pTextSearch[i] == '\0' )
			return( 0 );
		if ( !j && i )
		{
			if ( isalpha( pTextSearch[i-1] ))	// not start of word ?
				continue;
		}
		if ( toupper( pTextSearch[i] ) == toupper( pszKeyWord[j] ))
			j++;
		else
			j=0;
	}
}

//*******************************************************************
// -CMainTask

THREAD_ENTRY_RET _cdecl CMainTask::EntryProc( void * lpThreadParameter ) // static
{
	// The main message loop.
	g_MainTask.OnCreate();
#if defined(_WIN32) && !defined(_DEBUG)
	_set_se_translator( Sphere_Exception_Win32 );	// must be called for each thread.
#endif
	while ( !Sphere_OnTick() )
	{
		Sleep(0);
	}
	g_MainTask.OnClose();
}

void CMainTask::CreateThread()
{
	// AttachInputThread to us if needed ?
	CThread::CreateThread( EntryProc );	
}

void CMainTask::CheckStuckThread()
{
	// Periodically called to check if the tread is stuck.

	static CGTime sm_timeRealPrev = 0;

	// Has real time changed ?
	CGTime timeRealCur = CGTime::GetCurrentTime();
	int iTimeDiff = timeRealCur.GetTime() - sm_timeRealPrev.GetTime();
	iTimeDiff = abs( iTimeDiff );
	
	if ( iTimeDiff < g_Cfg.m_iFreezeRestartTime )
		return;
	sm_timeRealPrev = timeRealCur;

	// Has server time changed ?
	CServTime timeCur = CServTime::GetCurrentTime();
	if ( timeCur != m_timePrev )	// Seems ok.
	{
		m_timePrev = timeCur;
		return;
	}

	if ( g_Serv.IsValidBusy())	// Server is just busy.
		return;

	if ( m_timeRestart == timeCur )
	{
		g_Log.Event( LOGL_FATAL, "Main loop freeze RESTART FAILED!" DEBUG_CR );
	}

	if ( m_timeWarn == timeCur )
	{
		// Kill and revive the main process
		g_Log.Event( LOGL_CRIT, "Main loop freeze RESTART!" DEBUG_CR );

#ifndef _DEBUG
		TerminateThread( 0xDEAD );

				// try to restart it.
		g_Log.Event( LOGL_EVENT, "Trying to restart the main loop thread" DEBUG_CR );
		CreateThread();
#endif
		m_timeRestart = timeCur;
	}
	else
	{
		g_Log.Event( LOGL_WARN, "Main loop frozen ?" DEBUG_CR );
		m_timeWarn = timeCur;
	}
}

//*******************************************************************
// -CClientTask

THREAD_ENTRY_RET _cdecl CClientTask::EntryProc( void * lpThreadParameter ) // static
{
	// The main clients message loop.
	g_ClientTask.OnCreate();
#if defined(_WIN32) && !defined(_DEBUG)
	_set_se_translator( Sphere_Exception_Win32 );	// must be called for each thread.
#endif
	while ( ! g_Serv.m_iExitFlag )
	{
		EXC_TRY(("EntryProc()"));

		if ( IsSetOF(OF_Multithreaded) )
		{
			g_Serv.OnTick();
		}
		Sleep(0);
		EXC_CATCH("CClientTask");
	}
	g_ClientTask.OnClose();
}

void CClientTask::CreateThread()
{
	// AttachInputThread to us if needed ?
	CThread::CreateThread( EntryProc );	
}

void CClientTask::CheckStuckThread()
{
	// Periodically called to check if the tread is stuck.

	static CGTime sm_timeRealPrev = 0;

	// Has real time changed ?
	CGTime timeRealCur = CGTime::GetCurrentTime();
	int iTimeDiff = timeRealCur.GetTime() - sm_timeRealPrev.GetTime();
	iTimeDiff = abs( iTimeDiff );
	
	if ( iTimeDiff < g_Cfg.m_iFreezeRestartTime )
		return;
	sm_timeRealPrev = timeRealCur;

	// Has server time changed ?
	CServTime timeCur = CServTime::GetCurrentTime();
	if ( timeCur != m_timePrev )	// Seems ok.
	{
		m_timePrev = timeCur;
		return;
	}

	if ( g_Serv.IsValidBusy())	// Server is just busy.
		return;

	if ( m_timeRestart == timeCur )
	{
		g_Log.Event( LOGL_FATAL, "Client loop freeze RESTART FAILED!" DEBUG_CR );
	}

	if ( m_timeWarn == timeCur )
	{
		// Kill and revive the client process
		g_Log.Event( LOGL_CRIT, "Client loop freeze RESTART!" DEBUG_CR );

#ifndef _DEBUG
		TerminateThread( 0xDEAD );

				// try to restart it.
		g_Log.Event( LOGL_EVENT, "Trying to restart the client loop thread" DEBUG_CR );
		CreateThread();
#endif
		m_timeRestart = timeCur;
	}
	else
	{
		g_Log.Event( LOGL_WARN, "Client loop frozen ?" DEBUG_CR );
		m_timeWarn = timeCur;
	}
}

//*******************************************************************
// NPC AI thread

THREAD_ENTRY_RET _cdecl CAITask::EntryProc( void * lpThreadParameter )
{
	g_threadAI.OnCreate();
#if defined(_WIN32) && !defined(_DEBUG)
	_set_se_translator(Sphere_Exception_Win32);
#endif
	while ( !g_Serv.m_iExitFlag )
	{
		bool	bSleeping;
		EXC_TRY(("EntryProc()"));

		if ( IsSetOF(OF_Multithreaded) && g_Cfg.m_iNpcAi )
		{
			CSector	*pSector;
			for ( int m = 0; m < 256; m++ )
			{
				if ( !g_MapList.m_maps[m] ) continue;

				for ( long l = 0; l < g_MapList.GetSectorQty(m); l++ )
				{
					pSector = g_World.GetSector(m, l);
					if ( !pSector ) continue;										// silently ignore the errors
					//	skip sleeping sectors
					bSleeping = ( -g_World.GetTimeDiff(pSector->GetLastClientTime()) > 10*60*TICK_PER_SEC );
					if ( bSleeping ) continue;

					CChar	*pChar = STATIC_CAST <CChar*>(pSector->m_Chars_Active.GetHead());
					CChar	*pCharNext;
					if ( !pChar ) continue;											// no active chars here

					for ( ; pChar ; pChar = pCharNext )
					{
						pCharNext = pChar->GetNext();

						if ( pChar->m_pNPC )
						{
							if ( g_Cfg.m_iNpcAi&NPC_AI_PATH )
							{
								pChar->NPC_Pathfinding();
								Sleep(0);
							}
							if ( g_Cfg.m_iNpcAi&NPC_AI_FOOD )
							{
								pChar->NPC_Food();
								Sleep(0);
							}
							if ( g_Cfg.m_iNpcAi&NPC_AI_EXTRA )
							{
								pChar->NPC_AI();
								Sleep(0);
							}
						}
						pChar = pCharNext;
						Sleep(0);
					}
					Sleep(10);
				}
			}
		}
		Sleep(100);
		EXC_CATCH("CAITask");
	}
	g_threadAI.OnClose();
}

void CAITask::CreateThread()
{
	CThread::CreateThread(EntryProc);	
}

void CAITask::CheckStuckThread()
{
	static CGTime sm_timeRealPrev = 0;

	// Has real time changed ?
	CGTime timeRealCur = CGTime::GetCurrentTime();
	int iTimeDiff = abs(timeRealCur.GetTime() - sm_timeRealPrev.GetTime());
	
	if ( iTimeDiff < g_Cfg.m_iFreezeRestartTime ) return;
	sm_timeRealPrev = timeRealCur;

	// Has server time changed ?
	CServTime timeCur = CServTime::GetCurrentTime();
	if ( timeCur != m_timePrev )	// Seems ok.
	{
		m_timePrev = timeCur;
		return;
	}
	else if ( g_Serv.IsValidBusy() ) return;

	if ( m_timeRestart == timeCur )
	{
		g_Log.Event(LOGL_FATAL, "NPC AI loop freeze RESTART FAILED!" DEBUG_CR);
	}

	if ( m_timeWarn == timeCur )
	{
		// Kill and revive the client process
		g_Log.Event(LOGL_CRIT, "NPC AI loop freeze RESTART!" DEBUG_CR);

#ifndef _DEBUG
		TerminateThread( 0xDEAD );

		// try to restart it.
		g_Log.Event(LOGL_EVENT, "Trying to restart the NPC AI loop thread" DEBUG_CR);
		CreateThread();
#endif
		m_timeRestart = timeCur;
	}
	else
	{
		g_Log.Event( LOGL_WARN, "NPC AI loop frozen ?" DEBUG_CR );
		m_timeWarn = timeCur;
	}
}

//*******************************************************************

int Sphere_InitServer( int argc, char *argv[] )
{
	static LPCTSTR const excInfo[] =
	{
		"",
		"loading server",
		"parsing command line arguments",
		"initializing sockets",
		"loading world",
		"loading auto-complete dictionary",
		"creating background task",
		"triggeting server start",
	};
	const char *zTemp = excInfo[0];
	int		i;
	EXC_TRY(("Sphere_InitServer(%d,%x)", argc, argv));
	ASSERT( MAX_BUFFER >= sizeof( CCommand ));
	ASSERT( MAX_BUFFER >= sizeof( CEvent ));
	ASSERT( sizeof( int ) == sizeof( DWORD ));	// make this assumption often.
	ASSERT( sizeof( ITEMID_TYPE ) == sizeof( DWORD ));
	ASSERT( sizeof( WORD ) == 2 );
	ASSERT( sizeof( DWORD ) == 4 );
	ASSERT( sizeof( NWORD ) == 2 );
	ASSERT( sizeof( NDWORD ) == 4 );
	ASSERT( sizeof(CUOItemTypeRec) == 37 );	// byte pack working ?

#ifdef _WIN32
	if ( !QueryPerformanceFrequency((LARGE_INTEGER *)&llTimeProfileFrequency)) llTimeProfileFrequency = 1000;

#if defined(_CONSOLE)
	SetConsoleTitle( GRAY_TITLE " V" GRAY_VERSION );
	SetConsoleCtrlHandler( ConsoleHandlerRoutine, true );
#endif

#if !defined(_DEBUG)
	_set_se_translator( Sphere_Exception_Win32 );
#endif

#endif // _WIN32

	zTemp = excInfo[1];
	if ( ! g_Serv.Load())
		return -3;

	if ( argc > 1 )
	{
		zTemp = excInfo[2];
		if ( ! g_Serv.CommandLine( argc, argv ))
		{
			return( -1 );
		}
	}

	WritePidFile(2);

	zTemp = excInfo[3];
	if ( ! g_Serv.SocketsInit() ) return (-9);
	zTemp = excInfo[4];
	if ( ! g_World.LoadAll() ) return (-8);

	//	load auto-complete dictionary
	zTemp = excInfo[5];
	{
		CFileText	dict;
		if ( dict.Open(GRAY_FILE ".dic", OF_READ|OF_TEXT) )
		{
			TCHAR	*pszTemp = Str_GetTemp();
			int		i(0);
			while ( !dict.IsEOF() )
			{
				dict.ReadString(pszTemp, SCRIPT_MAX_LINE_LEN-1);
				if ( *pszTemp )
				{
					char *c = strchr(pszTemp, '\r');
					if ( c ) *c = 0;
					c = strchr(pszTemp, '\n');
					if ( c ) *c = 0;
					if ( *pszTemp )
					{
						i++;
						g_AutoComplete.AddTail(pszTemp);
					}
				}
			}
			g_Log.Event(LOGM_INIT, "Auto-complete dictionary loaded (contains %i words)." DEBUG_CR, i);
			dict.Close();
		}
		else g_Log.Event(LOGM_INIT, "Auto-complete dictionary file '" GRAY_FILE ".dic' not found." DEBUG_CR);
	}

	g_Serv.SetServerMode( SERVMODE_Run );	// ready to go. ! IsLoading()
	
	// Display EF/OF Flags
	g_Cfg.PrintEFOFFlags();

	zTemp = excInfo[6];
	if ( g_Cfg.CanRunBackTask() ) g_BackTask.CreateThread();
	else g_Log.Event( LOGM_INIT, "Background task not required." DEBUG_CR );

	g_Log.Event( LOGM_INIT, g_Serv.GetStatusString( 0x24 ));
	g_Log.Event( LOGM_INIT, "Startup complete. items=%d, chars=%d" DEBUG_CR, g_Serv.StatGet(SERV_STAT_ITEMS), g_Serv.StatGet(SERV_STAT_CHARS));

#ifdef _WIN32
	g_Log.Event( LOGM_INIT, "Press '?' for console commands" DEBUG_CR );
#endif
	g_Log.FireEvent( LOGEVENT_Startup );
	
	// Trigger server start
	zTemp = excInfo[7];
	CScriptTriggerArgs	args;
	g_Serv.r_Call("f_onserver_start", &g_Serv, &args);
	return( 0 );

	EXC_CATCH(zTemp);
	return -10;
}

void Sphere_ExitServer()
{
	ASSERT(g_Serv.m_iExitFlag);
	g_Serv.SetServerMode( SERVMODE_Exiting );

	g_BackTask.WaitForClose(15);
	g_MainTask.WaitForClose(15);
	g_ClientTask.WaitForClose(15);
	g_threadAI.WaitForClose(15);

	g_Serv.SocketsClose();
	g_World.Close();

#if defined(_WIN32) && defined(_CONSOLE)
	SetConsoleCtrlHandler( ConsoleHandlerRoutine, false );
#endif

	if ( g_Serv.m_iExitFlag < 0 )
	{
		g_Log.Event( LOGM_INIT|LOGL_FATAL, "Server terminated by error %d!" DEBUG_CR, g_Serv.m_iExitFlag );
#if defined(_WIN32) && defined(_CONSOLE)
		g_Serv.SysMessage( "Press any key to exit" );
		while ( _getch() == 0 ) ;
#endif
	}
	else
	{
		g_Log.Event( LOGM_INIT|LOGL_EVENT, "Server shutdown (code %d) complete!" DEBUG_CR, g_Serv.m_iExitFlag );
	}

	g_Log.FireEvent( LOGEVENT_Shutdown );
	g_Log.Close();
}

int Sphere_OnTick()
{
	// Give the world (CMainTask) a single tick. RETURN: 0 = everything is fine.
	EXC_TRY(("Sphere_OnTick()"));

#ifdef _DEBUG
	DEBUG_CHECK( g_Log.SetScriptContext(NULL) == NULL );
	DEBUG_CHECK( g_Log.SetObjectContext(NULL) == NULL );
#endif
#ifdef _WIN32
	g_Serv.OnTick_Busy();	// not used in Linux
#endif
	g_World.OnTick();
	if ( !IsSetOF(OF_Multithreaded) )
	{
		g_Serv.OnTick();
	}

	EXC_CATCH("Main Loop");
	return( g_Serv.m_iExitFlag );
}

//*****************************************************

static void Sphere_MainMonitorLoop()
{
	// We don't check for stuck threads in Debug mode,
	// whenever we reach a breakpoint, this would trigger.
	static LPCTSTR const excInfo[] =
	{
		"",
		"ntwindow ontick",
		"main loop check",
		"background loop check",
	};
	const char *zTemp = excInfo[0];

	// Just make sure the main loop is alive every so often.
	// This should be the parent thread. try to restart it if it is not.
	ASSERT( CThread::GetCurrentThreadId() == g_Serv.m_dwParentThread );
	while ( ! g_Serv.m_iExitFlag )
	{
		EXC_TRY(("Sphere_MainMonitorLoop()"));

		if ( g_Cfg.m_iFreezeRestartTime <= 0 )
		{
			DEBUG_ERR(( "Freeze Restart Time cannot be cleared at run time" DEBUG_CR ));
			g_Cfg.m_iFreezeRestartTime = 10;
		}

#ifdef _WIN32
		zTemp = excInfo[1];
		NTWindow_OnTick( g_Cfg.m_iFreezeRestartTime * 1000 );
#else
		Sleep( g_Cfg.m_iFreezeRestartTime * 1000 );
#endif

		// Don't look for freezing when doing certain things.
		if ( g_Serv.IsLoading() || ! g_Cfg.m_fSecure )
			continue;

#if !defined( _DEBUG )
		zTemp = excInfo[2];
		g_MainTask.CheckStuckThread();
		if ( IsSetOF(OF_Multithreaded) )
		{
			g_ClientTask.CheckStuckThread();
			g_threadAI.CheckStuckThread();
		}
		zTemp = excInfo[3];
		g_BackTask.CheckStuckThread();
#endif
		EXC_CATCH(zTemp);
	}

}

//******************************************************

#if !defined( _WIN32 )

// This is used to restore the original flags on exit
void resetNonBlockingIo()
{
	termios term_caps;

	if ( tcgetattr(STDIN_FILENO, &term_caps) < 0 ) return;

	term_caps.c_lflag |= ICANON;

	if( tcsetattr(STDIN_FILENO, TCSANOW, &term_caps) < 0 ) return;
}

void setNonBlockingIo()
{
	termios term_caps;

	if( tcgetattr( STDIN_FILENO, &term_caps ) < 0 )
	{
		printf( "ERROR: Could not get the termcap settings for this terminal.\n" );
		return;
	}

	term_caps.c_lflag &= ~ICANON;

	if( tcsetattr( STDIN_FILENO, TCSANOW, &term_caps ) < 0 )
	{
		printf( "ERROR: Could not set the termcap settings for this terminal.\n" );
		return;
	}
	setbuf(stdin, NULL);
	atexit(resetNonBlockingIo);
}
#endif

void dword_q_sort(DWORD numbers[], DWORD left, DWORD right)
{
	DWORD	pivot, l_hold, r_hold;

	l_hold = left;
	r_hold = right;
	pivot = numbers[left];
	while (left < right)
	{
		while ((numbers[right] >= pivot) && (left < right)) right--;
		if (left != right)
		{
			numbers[left] = numbers[right];
			left++;
		}
		while ((numbers[left] <= pivot) && (left < right)) left++;
		if (left != right)
		{
			numbers[right] = numbers[left];
			right--;
		}
	}
	numbers[left] = pivot;
	pivot = left;
	left = l_hold;
	right = r_hold;
	if (left < pivot) dword_q_sort(numbers, left, pivot-1);
	if (right > pivot) dword_q_sort(numbers, pivot+1, right);
}

void defragSphere(char *path)
{
	CFileText	inf;
	CGFile		ouf;
	char	z[256], z1[256], buf[1024];
	int		i;
	DWORD	uid(0);
	char	*p, *p1;
	DWORD	dBytesRead;
	DWORD	dTotalMb;
	DWORD	mb10(10*1024*1024);
	DWORD	mb5(5*1024*1024);
	bool	bSpecial;
	DWORD	dTotalUIDs;

	char	c,c1,c2;
	DWORD	d;

	//	NOTE: Sure I could use CVarDefArray, but it is extremely slow with memory allocation, takes hours
	//		to read and save the data. Moreover, it takes less memory in this case and does less convertations.
#define	MAX_UID	5000000L	// limit to 5mln of objects, takes 5mln*4 = 20mb
	DWORD	*uids;

	g_Log.Event(LOGM_INIT,	"Defragmentation (UID alteration) of " GRAY_TITLE " saves." DEBUG_CR
		"Use it on your risk and if you know what you are doing since it can possibly harm your server." DEBUG_CR
		"The process can take up to several hours depending on the CPU you have." DEBUG_CR
		"After finished, you will have your '" GRAY_FILE "*.scp' files converted and saved as '" GRAY_FILE "*.scp.new'." DEBUG_CR);

	uids = (DWORD*)calloc(MAX_UID, sizeof(DWORD));
	for ( i = 0; i < 3; i++ )
	{
		strcpy(z, path);
		if ( i == 0 ) strcat(z, GRAY_FILE "statics.scp");
		else if ( i == 1 ) strcat(z, GRAY_FILE "world.scp");
		else strcat(z, GRAY_FILE "chars.scp");

		g_Log.Event(LOGM_INIT, "Reading current UIDs: %s" DEBUG_CR, z);
		if ( !inf.Open(z, OF_READ|OF_TEXT) )
		{
			g_Log.Event(LOGM_INIT, "Cannot open file for reading. Skipped!" DEBUG_CR);
			continue;
		}
		dBytesRead = dTotalMb = 0;
		while ( !feof(inf.m_pStream) )
		{
			fgets(buf, sizeof(buf), inf.m_pStream);
			dBytesRead += strlen(buf);
			if ( dBytesRead > mb10 )
			{
				dBytesRead -= mb10;
				dTotalMb += 10;
				g_Log.Event(LOGM_INIT, "Total read %u Mb" DEBUG_CR, dTotalMb);
			}
			if (( buf[0] == 'S' ) && ( strstr(buf, "SERIAL=") == buf ))
			{
				p = buf + 7;
				p1 = p;
				while ( *p1 && ( *p1 != '\r' ) && ( *p1 != '\n' )) p1++;
				*p1 = 0;

				//	prepare new uid
				*(p-1) = '0';
				*p = 'x';
				p--;
				uids[uid++] = strtoul(p, &p1, 16);
			}
		}
		inf.Close();
	}
	dTotalUIDs = uid;
	g_Log.Event(LOGM_INIT, "Totally having %u unique objects (UIDs), latest: 0%x" DEBUG_CR, uid, uids[uid-1]);

	g_Log.Event(LOGM_INIT, "Quick-Sorting the UIDs array..." DEBUG_CR);
	dword_q_sort(uids, 0, dTotalUIDs-1);

	for ( i = 0; i < 5; i++ )
	{
		strcpy(z, path);
		if ( !i ) strcat(z, GRAY_FILE "accu.scp");
		else if ( i == 1 ) strcat(z, GRAY_FILE "chars.scp");
		else if ( i == 2 ) strcat(z, GRAY_FILE "data.scp");
		else if ( i == 3 ) strcat(z, GRAY_FILE "world.scp");
		else if ( i == 4 ) strcat(z, GRAY_FILE "statics.scp");
		g_Log.Event(LOGM_INIT, "Updating UID-s in %s to %s.new" DEBUG_CR, z, z);
		if ( !inf.Open(z) )
		{
			g_Log.Event(LOGM_INIT, "Cannot open file for reading. Skipped!" DEBUG_CR);
			continue;
		}
		strcat(z, ".new");
		if ( !ouf.Open(z, OF_WRITE|OF_CREATE) )
		{
			g_Log.Event(LOGM_INIT, "Cannot open file for writing. Skipped!" DEBUG_CR);
			continue;
		}
		dBytesRead = dTotalMb = 0;
		while ( inf.ReadString(buf, sizeof(buf)) )
		{
			uid = strlen(buf);
			buf[uid] = buf[uid+1] = buf[uid+2] = 0;	// just to be sure to be in line always
							// NOTE: it is much faster than to use memcpy to clear before reading
			bSpecial = false;
			dBytesRead += uid;
			if ( dBytesRead > mb5 )
			{
				dBytesRead -= mb5;
				dTotalMb += 5;
				g_Log.Event(LOGM_INIT, "Total processed %u Mb" DEBUG_CR, dTotalMb);
			}
			p = buf;
			if ( 0 ) ;
			//	Note 28-Jun-2004
			//	mounts seems having ACTARG1 > 0x30000000. The actual UID is ACTARG1-0x30000000. The
			//	new also should be new+0x30000000. need investigation if this can help making mounts
			//	not to disappear after the defrag
			else if (( buf[0] == 'A' ) && ( strstr(buf, "ACTARG1=0") == buf ))		// ACTARG1=
				p += 8;
			else if (( buf[0] == 'C' ) && ( strstr(buf, "CONT=0") == buf ))			// CONT=
				p += 5;
			else if (( buf[0] == 'C' ) && ( strstr(buf, "CHARUID=0") == buf ))		// CHARUID=
				p += 8;
			else if (( buf[0] == 'L' ) && ( strstr(buf, "LASTCHARUID=0") == buf ))	// LASTCHARUID=
				p += 12;
			else if (( buf[0] == 'L' ) && ( strstr(buf, "LINK=0") == buf ))			// LINK=
				p += 5;
			else if (( buf[0] == 'M' ) && ( strstr(buf, "MEMBER=0") == buf ))		// MEMBER=
			{
				p += 7;
				bSpecial = true;
			}
			else if (( buf[0] == 'M' ) && ( strstr(buf, "MORE1=0") == buf ))		// MORE1=
				p += 6;
			else if (( buf[0] == 'M' ) && ( strstr(buf, "MORE2=0") == buf ))		// MORE2=
				p += 6;
			else if (( buf[0] == 'S' ) && ( strstr(buf, "SERIAL=0") == buf ))		// SERIAL=
				p += 7;
			else if ((( buf[0] == 'T' ) && ( strstr(buf, "TAG.") == buf )) ||		// TAG.=
					 (( buf[0] == 'R' ) && ( strstr(buf, "REGION.TAG") == buf )))
			{
				while ( *p && ( *p != '=' )) p++;
				p++;
			}
			else if (( i == 2 ) && strchr(buf, '='))	// spheredata.scp - plain VARs
			{
				while ( *p && ( *p != '=' )) p++;
				p++;
			}
			else p = NULL;

			//	UIDs are always hex, so prefixed by 0
			if ( p && ( *p != '0' )) p = NULL;

			//	here we got potentialy UID-contained variable
			//	check if it really is only UID-like var containing
			if ( p )
			{
				p1 = p;
				while ( *p1 &&
					((( *p1 >= '0' ) && ( *p1 <= '9' )) ||
					 (( *p1 >= 'a' ) && ( *p1 <= 'f' )))) p1++;
				if ( !bSpecial )
				{
					if ( *p1 && ( *p1 != '\r' ) && ( *p1 != '\n' )) // some more text in line
						p = NULL;
				}
			}

			//	here we definitely know that this is very uid-like
			if ( p )
			{
				c = *p1;

				*p1 = 0;
				//	here in p we have the current value of the line.
				//	check if it is a valid UID

				//	prepare converting 0.. to 0x..
				c1 = *(p-1);
				c2 = *p;
				*(p-1) = '0';
				*p = 'x';
				p--;
				uid = strtoul(p, &p1, 16);
				p++;
				*(p-1) = c1;
				*p = c2;
				//	Note 28-Jun-2004
				//	The search algourytm is very simple and fast. But maybe integrate some other, at least /2 algorythm
				//	since has amount/2 tryes at worst chance to get the item and never scans the whole array
				//	It should improve speed since defragmenting 150Mb saves takes ~2:30 on 2.0Mhz CPU
				{
					DWORD	dStep = dTotalUIDs/2;
					d = dStep;
					while ( true )
					{
						dStep /= 2;

						if ( uids[d] == uid )
						{
							uid = d | (uids[d]&0xF0000000);	// do not forget attach item and special flags like 04..
							break;
						}
						else if ( uids[d] < uid ) d += dStep;
						else d -= dStep;

						if ( dStep == 1 )
						{
							uid = 0xFFFFFFFFL;
							break; // did not find the UID
						}
					}
				}

				//	Search for this uid in the table
/*				for ( d = 0; d < dTotalUIDs; d++ )
				{
					if ( !uids[d] )	// end of array
					{
						uid = 0xFFFFFFFFL;
						break;
					}
					else if ( uids[d] == uid )
					{
						uid = d | (uids[d]&0xF0000000);	// do not forget attach item and special flags like 04..
						break;
					}
				}*/

				//	replace UID by the new one since it has been found
				*p1 = c;
				if ( uid != 0xFFFFFFFFL )
				{
					*p = 0;
					strcpy(z, p1);
					sprintf(z1, "0%x", uid);
					strcat(buf, z1);
					strcat(buf, z);
				}
			}
			//	output the resulting line
			ouf.Write(buf, strlen(buf));
		}
		inf.Close();
		ouf.Close();
	}
	free(uids);
	g_Log.Event(LOGM_INIT,	"Defragmentation complete." DEBUG_CR);
}

//	Exceptions debugging routine.
#ifdef EXCEPTIONS_DEBUG
char *g_ExcArguments;
void _cdecl exceptions_debug_init(LPCTSTR pszFormat, ...)
{
	va_list vargs;
	va_start( vargs, pszFormat );
	vsprintf(g_ExcArguments, pszFormat, vargs);
	va_end( vargs );
}
TCHAR	g_ExcStack[512*512];
int	g_ExcCurrent = 0;

TCHAR *Exc_GetStack()
{
	if ( ++g_ExcCurrent >= 64 ) g_ExcCurrent = 0;
	return (&g_ExcStack[g_ExcCurrent*512]);
}
#endif

#ifdef _WIN32
int Sphere_MainEntryPoint( int argc, char *argv[] )
#else
int _cdecl main( int argc, char * argv[] )
#endif
{
	// Initialize nonblocking IO 
	// and disable readline on linux
	#if !defined( _WIN32 )
	setNonBlockingIo();
	#endif

	g_Serv.m_iExitFlag = Sphere_InitServer( argc, argv );
	if ( ! g_Serv.m_iExitFlag )
	{
		WritePidFile();
		if (
#ifdef _WIN32
			GRAY_GetOSInfo()->dwPlatformId == VER_PLATFORM_WIN32_NT &&
#endif
			g_Cfg.m_iFreezeRestartTime )
		{
			g_MainTask.CreateThread();
			if ( IsSetOF(OF_Multithreaded) )
			{
				g_ClientTask.CreateThread();
				g_threadAI.CreateThread();
			}
			Sphere_MainMonitorLoop();
		}
		else
		{
			g_MainTask.EntryProc( 0 );
		}
	}

	Sphere_ExitServer();
	WritePidFile(true);
	return( g_Serv.m_iExitFlag );
}
