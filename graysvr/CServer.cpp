//
// CServer.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.

#include "../common/grayver.h"	// sphere version
#include "../common/CAssoc.h"

#include <signal.h>

#ifdef _WIN32
#include "ntservice.h"	// g_Service
#else
#include <exception>
#include <setjmp.h>
//#include <vga.h>
#endif
////////////////////////////////////////////////////////
// -CTextConsole

CChar * CTextConsole::GetChar() const
{
	return( const_cast <CChar *>( dynamic_cast <const CChar *>( this )));
}

int CTextConsole::OnConsoleKey( CGString & sText, TCHAR nChar, bool fEcho )
{
	// eventaully we should call OnConsoleCmd
	// RETURN:
	//  0 = dump this connection.
	//  1 = keep processing.
	//  2 = process this.

	if ( sText.GetLength() >= 80 )
	{
commandtoolong:
		SysMessage( "Command too long" DEBUG_CR );
		sText.Empty();
		return( 0 );
	}

	if ( nChar == '\r' || nChar == '\n' )
	{
		if ( fEcho )
		{
			SysMessage( DEBUG_CR );
		}
		return 2;
	}
	else if ( nChar == 9 )			// TAB (auto-completion)
	{
		LPCTSTR	p;
		LPCTSTR tmp;
		int		inputLen;
		bool	matched(false);

		//	extract up to start of the word
		p = sText.GetPtr() + sText.GetLength();
		while (( p >= sText.GetPtr() ) && ( *p != '.' ) && ( *p != ' ' ) && ( *p != '/' ) && ( *p != '=' )) p--;
		p++;
		inputLen = strlen(p);

		// search in the auto-complete list for starting on P, and save coords of 1st and Last matched
		CGStringListRec	*firstmatch = NULL;
		CGStringListRec	*lastmatch = NULL;
		CGStringListRec	*curmatch = NULL;	// the one that should be set
		for ( curmatch = g_AutoComplete.GetHead(); curmatch != NULL; curmatch = curmatch->GetNext() )
		{
			if ( !strnicmp(curmatch->GetPtr(), p, inputLen) )	// matched
			{
				if ( firstmatch == NULL ) firstmatch = lastmatch = curmatch;
				else lastmatch = curmatch;
			}
			else if ( lastmatch ) break;					// if no longer matches - save time by instant quit
		}

		if (( firstmatch != NULL ) && ( firstmatch == lastmatch ))	// there IS a match and the ONLY
		{
			tmp = firstmatch->GetPtr() + inputLen;
			matched = true;
		}
		else if ( firstmatch != NULL )						// also make SE (if SERV/SERVER in dic) to become SERV
		{
			p = tmp = firstmatch->GetPtr();
			tmp += inputLen;
			inputLen = strlen(p);
			matched = true;
			for ( curmatch = firstmatch->GetNext(); curmatch != lastmatch->GetNext(); curmatch = curmatch->GetNext() )
			{
				if ( strnicmp(curmatch->GetPtr(), p, inputLen) )	// mismatched
				{
					matched = false;
					break;
				}
			}
		}

		if ( matched )
		{
			if ( fEcho ) SysMessage(tmp);
			sText += tmp;
			if ( sText.GetLength() > 80 ) goto commandtoolong;
		}
		return 1;
	}

	if ( fEcho )
	{
		// Echo
		TCHAR szTmp[2];
		szTmp[0] = nChar;
		szTmp[1] = '\0';
		SysMessage( szTmp );
	}

	if ( nChar == 8 )
	{
		if ( sText.GetLength())	// back key
		{
			sText.SetLength( sText.GetLength() - 1 );
		}
		return 1;
	}

	sText += nChar;
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// -CServer

CServer::CServer() : CServerDef( GRAY_TITLE, CSocketAddressIP( SOCKET_LOCAL_ADDRESS ))
{
	m_iExitFlag = 0;
	m_fResyncPause = false;
	m_uSizeMsgMax = 0;

	m_iAdminClients = 0;

	m_timeShutdown.Init();

	m_fConsoleTextReadyFlag = false;

	m_nGuestsCur = 0;
	m_sServVersion = GRAY_VERSION;

	// we are in start up mode. // IsLoading()
	SetServerMode( SERVMODE_Loading );

	memset(m_PacketFilter, 0, sizeof(m_PacketFilter));
}

CServer::~CServer()
{
}


#ifndef _WIN32
void _cdecl Signal_Hangup( int sig = 0 ) // If shutdown is initialized
{
	// handle freeze loop restarts!!!
	pthread_exit(0);
}

void _cdecl Signal_Terminate( int sig = 0 ) // If shutdown is initialized
{
	sigset_t set;
	
	g_Log.Event( LOGL_FATAL, "Signal_Terminate" DEBUG_CR );
	if ( sig )
	{
		signal( sig, &Signal_Terminate);

        sigemptyset(&set);
        sigaddset(&set, sig);
        sigprocmask(SIG_UNBLOCK, &set, NULL);
	}

	throw CGrayError( LOGL_FATAL, sig, "Signal_Terminate" );
}

void _cdecl Signal_Illegal_Instruction( int sig = 0 )
{
	sigset_t set;

	g_Log.Event( LOGL_FATAL, "Signal_Illegal_Instruction" DEBUG_CR );
	if ( sig )
	{
		signal( sig, &Signal_Illegal_Instruction );

        sigemptyset(&set);
        sigaddset(&set, sig);
        sigprocmask(SIG_UNBLOCK, &set, NULL);
	}

	throw CGrayError( LOGL_FATAL, sig, "Signal_Illegal_Instruction" );
}
#endif

void CServer::SetSignals( bool fMsg )
{
	// We have just started or we changed Secure mode.

#ifndef _WIN32
	// set_terminate(  &Exception_Terminate );
	// set_unexpected( &Exception_Unexpected );
	if ( g_Cfg.m_fSecure )
	{
		signal( SIGHUP,		&Signal_Hangup		);
		signal( SIGTERM,	&Signal_Terminate	);
		signal( SIGQUIT,	&Signal_Terminate	);
		signal( SIGINT,		&Signal_Terminate	);
		signal( SIGABRT,	&Signal_Terminate	);
		signal( SIGILL,		&Signal_Terminate	);
		signal( SIGSEGV,	&Signal_Illegal_Instruction	);
		signal( SIGFPE,		&Signal_Illegal_Instruction	);
		signal( SIGPIPE,	SIG_IGN			);

		g_Log.Event( (IsLoading() ? 0 : LOGL_EVENT) | LOGM_INIT,
			       	"Signal handlers installed." DEBUG_CR );
	}
	else
	{
		signal(SIGHUP,	SIG_DFL	);
		signal(SIGTERM, SIG_DFL	);
		signal(SIGQUIT, SIG_DFL	);
		signal(SIGINT,  SIG_DFL	);
		signal(SIGABRT, SIG_DFL	);
		signal(SIGILL,  SIG_DFL	);
		signal(SIGFPE,	SIG_DFL	);
		signal(SIGSEGV, SIG_DFL	);
		signal(SIGPIPE, SIG_DFL	);
		g_Log.Event( (IsLoading() ? 0 : LOGL_EVENT) | LOGM_INIT,
			       	"Signal handlers UNinstalled." DEBUG_CR );
	}
#endif

	if ( fMsg && !IsLoading() )
	{
		g_World.Broadcast( g_Cfg.m_fSecure ?
			"The world is now running in SECURE MODE" :
			"WARNING: The world is NOT running in SECURE MODE" );
	}
}

void CServer::SetServerMode( SERVMODE_TYPE mode )
{
	m_iModeCode = mode;
#ifdef _WIN32
	NTWindow_SetWindowTitle();
#endif
}

bool CServer::IsValidBusy() const
{
	// We might appear to be stopped but it's really ok ?
	// ? 
	switch ( m_iModeCode )
	{
	case SERVMODE_Saving:
		if ( g_World.IsSaving())
			return true;
		break;
	case SERVMODE_Loading:
	case SERVMODE_RestockAll:	// these may look stuck but are not.
		return( true );
	}
	return( false );
}

bool CServer::OnTick_Busy() const
{
	// We are busy doing stuff but give the pseudo background stuff a tick.
	// RETURN: true = keep going.
#ifdef _WIN32
	g_Service.OnTick();
	if ( ! g_MainTask.IsActive() )	// this is the main task.
	{
		NTWindow_OnTick(0);
	}
#endif
	return( ! m_iExitFlag );
}

void CServer::SetExitFlag( int iFlag )
{
	if ( m_iExitFlag )
		return;
	m_iExitFlag = iFlag;
}

void CServer::Shutdown( int iMinutes ) // If shutdown is initialized
{
	if ( iMinutes == 0 )
	{
		if ( ! m_timeShutdown.IsTimeValid() )
			return;
		m_timeShutdown.Init();
		g_World.Broadcast("Shutdown has been interrupted.");
		return;
	}

	SetPollTime();

	if ( iMinutes < 0 )
	{
		iMinutes = g_World.GetTimeDiff( m_timeShutdown ) / ( 60 * TICK_PER_SEC );
	}
	else
	{
		m_timeShutdown = CServTime::GetCurrentTime() + ( iMinutes * 60 * TICK_PER_SEC );
	}

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "Server going down in %i minute(s).", iMinutes);
	g_World.Broadcast(pszTemp);
}

bool CServer::TestServerIntegrity()
{
	// set a checksum for the server code and constant data.
	// if it changes then exit.
	// globalstartdata
	// globalstopdata

#ifdef _DEBUG
	static const DWORD sm_dwCheckSum = 0;
#else
	static const DWORD sm_dwCheckSum = 0;
#endif

	DWORD * pdwCodeStart = (DWORD *)(void*) globalstartsymbol;
	DWORD * pdwCodeStop = (DWORD *)(void*) globalendsymbol;
	DWORD dwCheckSum = 0;

	ASSERT( pdwCodeStart < pdwCodeStop );
	for ( ; pdwCodeStart < pdwCodeStop; pdwCodeStart++ )
	{
		dwCheckSum += *pdwCodeStart;
	}

	// SetExitFlag( -100 );

	return dwCheckSum;
}

void CServer::SysMessage( LPCTSTR pszMsg ) const
{
	// Print just to the main console.
	if ( pszMsg == NULL || ISINTRESOURCE(pszMsg))
		return;
#if defined(_CONSOLE)
	fputs( pszMsg, stdout );	// print out locally as well.
#elif _WIN32
	NTWindow_PostMsg( pszMsg );
#endif
	g_Log.FireEvent( LOGEVENT_ServerMsg, pszMsg );
}

void CServer::PrintTelnet( LPCTSTR pszMsg ) const
{
	if ( ! m_iAdminClients )
		return;

	for ( CClient * pClient = GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if (( pClient->GetConnectType() == CONNECT_TELNET ) && pClient->GetAccount() )
		{
			if ( !pClient->GetAccount()->IsPriv(PRIV_TELNET_SHORT) )	// this client accepts broadcasts
				pClient->SysMessage(pszMsg);
		}
	}
}

void CServer::PrintStr( LPCTSTR pszMsg ) const
{
	// print to all consoles.
	SysMessage( pszMsg );
	PrintTelnet( pszMsg );
}

int CServer::PrintPercent( long iCount, long iTotal )
{
	// These vals can get very large. so use MulDiv to prevent overflow. (not IMULDIV)
	DEBUG_CHECK( iCount >= 0 );
	DEBUG_CHECK( iTotal >= 0 );
	if ( iTotal <= 0 )
		return( 100 );

    int iPercent = MulDiv( iCount, 100, iTotal );
	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%d%%", iPercent);
	int len = strlen(pszTemp);

	PrintTelnet(pszTemp);
	
#ifdef _CONSOLE
	SysMessage(pszTemp);
#endif
	while ( len-- )
	{
		PrintTelnet( "\x08" );	// backspace over it.
	#ifdef _CONSOLE
 		SysMessage( "\x08" );	// backspace over it.
	#endif
	}

#ifdef _CONSOLE
#else
	#ifdef _WIN32
	NTWindow_SetWindowTitle(pszTemp);
	#endif
#endif

#ifdef _WIN32
	OnTick_Busy();
#endif
	return iPercent;
}

int CServer::GetAgeHours() const
{
	return( CServTime::GetCurrentTime().GetTimeRaw() / (60*60*TICK_PER_SEC));
}

LPCTSTR CServer::GetStatusStringRegister( bool fFirst ) const
{
	// we are registering ourselves.
	TCHAR * pTemp = Str_GetTemp();

	if ( fFirst )
	{
		TCHAR szVersion[128];
		sprintf( pTemp, "%s, Name=%s, RegPass=%s, Port=%d, Ver=%s, TZ=%d, EMail=%s, URL=%s, Lang=%s, CliVer=%s, AccApp=%d" DEBUG_CR,
			(LPCTSTR)g_Cfg.m_sVerName.GetPtr(),
			GetName(),
			(LPCTSTR) m_sRegisterPassword,
			m_ip.GetPort(),
			(LPCTSTR) g_Serv.m_sServVersion.GetPtr(),
			m_TimeZone,
			(LPCTSTR) m_sEMail,
			(LPCTSTR) m_sURL,
			(LPCTSTR) m_sLang,
			m_ClientVersion.WriteClientVer(szVersion),
			m_eAccApp
			);
	}
	else
	{
		int		iAccounts	= StatGet(SERV_STAT_ACCOUNTS);
		int		iClients	= StatGet(SERV_STAT_CLIENTS);
		int		iHours		= GetAgeHours()/24;

		sprintf( pTemp, "%s, Name=%s, RegPass=%s, Age=%i, Accounts=%d, Clients=%i, Items=%i, Chars=%i, Mem=%iK, Notes=%s" DEBUG_CR,
			(LPCTSTR)g_Cfg.m_sVerName.GetPtr(),
			GetName(),
			(LPCTSTR) m_sRegisterPassword,
			iHours,
			iAccounts,
			iClients,
			StatGet(SERV_STAT_ITEMS),
			StatGet(SERV_STAT_CHARS),
			StatGet(SERV_STAT_MEM)/1024,
			(LPCTSTR) m_sNotes
			);
	}
	return( pTemp );
}


LPCTSTR CServer::GetStatusString( BYTE iIndex ) const
{
	// NOTE: The key names should match those in CServerDef::r_LoadVal
	// A ping will return this as well.
	// 0 or 0x21 = main status.

	TCHAR * pTemp = Str_GetTemp();
	int		iClients	= StatGet(SERV_STAT_CLIENTS);
	int		iHours		= GetAgeHours()/24;

	switch ( iIndex )
	{
	case 0x21:	// '!'
		// typical (first time) poll response.
		{
			TCHAR szVersion[128];
			sprintf( pTemp, "%s, Name=%s, Port=%d, Ver=%s, TZ=%d, EMail=%s, URL=%s, Lang=%s, CliVer=%s" DEBUG_CR,
				(LPCTSTR)g_Cfg.m_sVerName.GetPtr(),
				GetName(),
				m_ip.GetPort(),
				(LPCTSTR) g_Serv.m_sServVersion.GetPtr(),
				m_TimeZone,
				(LPCTSTR) m_sEMail,
				(LPCTSTR) m_sURL,
				(LPCTSTR) m_sLang,
				m_ClientVersion.WriteClientVer(szVersion)
				);
		}
		break;
	case 0x22: // '"'
		{
		// shown in the INFO page in game.
		sprintf( pTemp, "%s, Name=%s, Age=%i, Clients=%i, Items=%i, Chars=%i, Mem=%iK" DEBUG_CR,
			(LPCTSTR)g_Cfg.m_sVerName.GetPtr(),
			GetName(),
			iHours,
			iClients,
			StatGet(SERV_STAT_ITEMS),
			StatGet(SERV_STAT_CHARS),
			StatGet(SERV_STAT_MEM)/1024
			);
		}
		break;
	case 0x23:
	default:	// default response to ping.
		
		sprintf( pTemp, "%s, Name=%s, Age=%i, Ver=%s, TZ=%d, EMail=%s, URL=%s, Clients=%i" DEBUG_CR,
			(LPCTSTR)g_Cfg.m_sVerName.GetPtr(),
			GetName(),
			iHours,
			(LPCTSTR) g_Serv.m_sServVersion.GetPtr(),
			m_TimeZone,
			(LPCTSTR) m_sEMail,
			(LPCTSTR) m_sURL,
			iClients
			);
		break;
	case 0x24: // '$'
		// show at startup.
		sprintf( pTemp, "Admin=%s, URL=%s, Lang=%s, TZ=%d" DEBUG_CR,
			(LPCTSTR) m_sEMail,
			(LPCTSTR) m_sURL,
			(LPCTSTR) m_sLang,
			m_TimeZone
			);
		break;
	}

	return( pTemp );
}

//*********************************************************

void CServer::ListServers( CTextConsole * pConsole ) const
{
	ASSERT( pConsole );

	TCHAR *pszMsg = Str_GetTemp();
	for ( int i=0; true; i++ )
	{
		CServerRef pServ = g_Cfg.Server_GetDef(i);
		if ( pServ == NULL )
			break;
		sprintf(pszMsg, "%d:NAME=%s, STATUS=%s" DEBUG_CR, i, (LPCTSTR) pServ->GetName(), (LPCTSTR) pServ->GetStatus());
		pConsole->SysMessage(pszMsg);
	}
}


void CServer::ListClients( CTextConsole * pConsole ) const
{
	// Mask which clients we want ?
	// Give a format of what info we want to SHOW ?

	ASSERT( pConsole );
	CChar * pCharCmd = pConsole->GetChar();

	TCHAR *pszMsg = Str_GetTemp();
	for ( CClient * pClient = GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		// CSocketAddress PeerName = pClient->m_Socket.GetPeerName();
		CChar * pChar = pClient->GetChar();
		if ( pChar )
		{
			if ( pCharCmd &&
				! pCharCmd->CanDisturb( pChar ))
			{
				continue;
			}

			TCHAR chRank = '=';
			if ( pClient->IsPriv(PRIV_GM) || pClient->GetPrivLevel() >= PLEVEL_Counsel )
			{
				chRank = ( pChar && pChar->IsDND()) ? '*' : '+';
			}

			sprintf(pszMsg, "%x:Acc%c'%s', (%s) Char='%s',(%s)" DEBUG_CR,
				pClient->m_Socket.GetSocket(),
				chRank,
				(LPCTSTR) pClient->GetAccount()->GetName(),
				(LPCTSTR) pClient->m_PeerName.GetAddrStr(),
				(LPCTSTR) pChar->GetName(),
				(LPCTSTR) pChar->GetTopPoint().WriteUsed());
		}
		else
		{
			if ( pConsole->GetPrivLevel() < pClient->GetPrivLevel())
			{
				continue;
			}
			LPCTSTR pszState;
			switch ( pClient->GetConnectType() )
			{
			case CONNECT_TELNET:	pszState = "TelNet"; break;
			case CONNECT_HTTP:		pszState = "Web"; break;
			default: pszState = "NOT LOGGED IN"; break;
			}

			sprintf(pszMsg, "%x:Acc='%s', (%s) %s" DEBUG_CR,
				pClient->m_Socket.GetSocket(),
				pClient->GetAccount() ? (LPCTSTR) pClient->GetAccount()->GetName() : "<NA>",
				(LPCTSTR) pClient->m_PeerName.GetAddrStr(),
				(LPCTSTR) pszState );
		}
		pConsole->SysMessage(pszMsg);
	}
}

bool CServer::OnConsoleCmd( CGString & sText, CTextConsole * pSrc )
{
	// RETURN: false = boot the client.
	int		len = sText.GetLength();
	char	low = tolower(sText[0]);

	if ( len <= 0 ) return true;
	if (( len > 2 ) || (( len == 2 ) && ( sText[1] != '#' ))) goto longcommand;

	switch ( low )
	{
	case '?':
		pSrc->SysMessagef(
			"Available Commands:" DEBUG_CR
			"# = Immediate Save world (## to save both world and statics)" DEBUG_CR
			"A = Accounts file update" DEBUG_CR
			"B message = Broadcast a message" DEBUG_CR
			"C = Clients List (%d)" DEBUG_CR
			"D = Dump data to external file" DEBUG_CR
			"E = Clear internal variables (like script profile)" DEBUG_CR
			"G = Garbage collection" DEBUG_CR
			"H = Hear all that is said (%s)" DEBUG_CR
			"I = Information" DEBUG_CR
			"L = Toggle log file (%s)" DEBUG_CR
			"P = Profile Info (%s)" DEBUG_CR
			"R = Resync Pause" DEBUG_CR
			"S = Secure mode toggle (%s)" DEBUG_CR
			"V = Verbose Mode (%s)" DEBUG_CR
			"X = immediate exit of the server (X# to save world and statics before exit)" DEBUG_CR
			,
			m_Clients.GetCount(),
			g_Log.IsLoggedMask( LOGM_PLAYER_SPEAK ) ? "ON" : "OFF",
			g_Log.IsFileOpen() ? "OPEN" : "CLOSED",
			m_Profile.IsActive() ? "ON" : "OFF",
			g_Cfg.m_fSecure ? "ON" : "OFF",
			g_Log.IsLogged( LOGL_TRACE ) ? "ON" : "OFF"
		);
		break;
	case 'h':	// Hear all said.
		{
			CScript script( "HEARALL" );
			r_Verb( script, pSrc );
			break;
		}
	case 's':
		{
			CScript script( "SECURE" );
			r_Verb( script, pSrc );
			break;
		}
	case 'l': // Turn the log file on or off.
		{
			CScript script( "LOG" );
			r_Verb( script, pSrc );
			break;
		}
	case 'v':
		{
			CScript script( "VERBOSE" );
			r_Verb( script, pSrc );
			break;
		}
	case 'i':
		{
/*
			extern int gAmount;
			extern ULONG gMemAmount;
			extern int gReallocs;
			pSrc->SysMessagef("total strings: %d, memory: %u, reallocs: %d" DEBUG_CR, gAmount, gMemAmount, gReallocs);
*/
			CScript script( "INFORMATION" );
			r_Verb( script, pSrc );
			break;
		}
	case 'x':
		if (( len > 1 ) && ( sText[1] == '#' ))	//	X# - exit with save. Such exit is not protected by secure mode
		{
			if ( g_Serv.m_fResyncPause ) goto do_resync;
			g_World.Save(true);
			g_World.SaveStatics();
			g_Log.Event( LOGL_FATAL, "Immediate Shutdown initialized!" DEBUG_CR);
			SetExitFlag(1);
		}
		else if ( g_Cfg.m_fSecure ) pSrc->SysMessage( "NOTE: Secure mode prevents keyboard exit!" DEBUG_CR );
		else
		{
			g_Log.Event( LOGL_FATAL, "Immediate Shutdown initialized!" DEBUG_CR);
			SetExitFlag( 1 );
		}
		break;
	case 'a':
		// Force periodic stuff
		g_Accounts.Account_SaveAll();
		g_Cfg.OnTick(true);
		break;
	case 'g':
		if ( g_Serv.m_fResyncPause )
		{
do_resync:
			pSrc->SysMessage( "Not allowed during resync pause. Use 'R' to restart." DEBUG_CR);
			break;
		}
		if ( g_World.IsSaving())
		{
do_saving:
			// Is this really true ???
			pSrc->SysMessage( "Not allowed during background worldsave. Use '#' to finish." DEBUG_CR);
			break;
		}
		g_World.GarbageCollection();
		break;
	case '#':	//	# - save world, ## - save both world and statics
		// Start a syncronous save or finish a background save synchronously
		if ( g_Serv.m_fResyncPause ) goto do_resync;
		g_World.Save(true);
		if (( len > 1 ) && ( sText[1] == '#' )) g_World.SaveStatics();	// ## means
		break;
	case 'c':	// List all clients on line.
		ListClients( pSrc );
		break;
	case 'r':	// resync Pause mode. Allows resync of things in files.
		if ( g_World.IsSaving())
			goto do_saving;
		SetResyncPause( !m_fResyncPause, pSrc );
		break;
	case 'p':	// Display profile information.
		// ? Peer status. Show status of other servers we list.
		{
			pSrc->SysMessagef( "Profiles %s: (%d sec total)" DEBUG_CR, m_Profile.IsActive() ? "ON" : "OFF", m_Profile.GetActiveWindow());
			for ( int i=0; i < PROFILE_QTY; i++ )
			{
				pSrc->SysMessagef( "%-10s = %s" DEBUG_CR, (LPCTSTR) m_Profile.GetName((PROFILE_TYPE) i), (LPCTSTR) m_Profile.GetDesc((PROFILE_TYPE) i ));
			}
			if ( IsSetEF(EF_Script_Profiler) )
			{
				if ( g_profiler.initstate != 0xf1 ) pSrc->SysMessage("Scripts profiler is not initialized" DEBUG_CR);
				else if ( !g_profiler.called ) pSrc->SysMessage("Script profiler is not yet informational" DEBUG_CR);
				else
				{
					LONGLONG	average = g_profiler.total / g_profiler.called;
					TScriptProfiler::TScriptProfilerFunction	*pFun;
					TScriptProfiler::TScriptProfilerTrigger		*pTrig;
					DWORD	divby(1);

					divby = llTimeProfileFrequency/1000;

					pSrc->SysMessagef("Scripts: called %d times and took %i.%04i msec (%i.%04i msec average). Reporting with highest average." DEBUG_CR,
						g_profiler.called,
						(int)(g_profiler.total/divby),
						(int)(((g_profiler.total*10000)/(divby))%10000),
						(int)(average/divby),
						(int)(((average*10000)/(divby))%10000)
					);
					for ( pFun = g_profiler.FunctionsHead; pFun != NULL; pFun = pFun->next )
					{
						if ( pFun->average > average )
						{
							pSrc->SysMessagef("FUNCTION '%s' called %d times, took %i.%04i msec average (%i.%04i min, %i.%04i max), total: %i.%04i msec" DEBUG_CR,
								pFun->name,
								pFun->called,
								(int)(pFun->average/divby),
								(int)(((pFun->average*10000)/(divby))%10000),
								(int)(pFun->min/divby),
								(int)(((pFun->min*10000)/(divby))%10000),
								(int)(pFun->max/divby),
								(int)(((pFun->max*10000)/(divby))%10000),
								(int)(pFun->total/divby),
								(int)(((pFun->total*10000)/(divby))%10000)
							);
						}
					}
					for ( pTrig = g_profiler.TriggersHead; pTrig != NULL; pTrig = pTrig->next )
					{
						if ( pTrig->average > average )
						{
							pSrc->SysMessagef("TRIGGER '%s' called %d times, took %i.%04i msec average (%i.%04i min, %i.%04i max), total: %i.%04i msec" DEBUG_CR,
								pTrig->name,
								pTrig->called,
								(int)(pTrig->average/divby),
								(int)(((pTrig->average*10000)/(divby))%10000),
								(int)(pTrig->min/divby),
								(int)(((pTrig->min*10000)/(divby))%10000),
								(int)(pTrig->max/divby),
								(int)(((pTrig->max*10000)/(divby))%10000),
								(int)(pTrig->total/divby),
								(int)(((pTrig->total*10000)/(divby))%10000)
							);
						}
					}
					pSrc->SysMessage("Report complete!" DEBUG_CR);
				}
			}
			else pSrc->SysMessage("Script profiler is turned OFF" DEBUG_CR);
		}
		break;
	case 'e':
		{
			if ( IsSetEF(EF_Script_Profiler) )
			{
				if ( g_profiler.initstate == 0xf1 )
				{
					TScriptProfiler::TScriptProfilerFunction	*pFun;
					TScriptProfiler::TScriptProfilerTrigger		*pTrig;

					for ( pFun = g_profiler.FunctionsHead; pFun != NULL; pFun = pFun->next )
					{
						pFun->average = pFun->called = pFun->max = pFun->min = pFun->total = 0;
					}
					for ( pTrig = g_profiler.TriggersHead; pTrig != NULL; pTrig = pTrig->next )
					{
						pTrig->average = pTrig->called = pTrig->max = pTrig->min = pTrig->total = 0;
					}

					g_profiler.called = g_profiler.total = 0;
					pSrc->SysMessage("Scripts profiler info cleared" DEBUG_CR);
				}
			}
			pSrc->SysMessage("Complete!" DEBUG_CR);
		}
		break;
	case 'd':
		{
			LPCTSTR		pszKey	= sText;			pszKey++;
			GETNONWHITESPACE( pszKey );
			if ( tolower(*pszKey) == 'a' )
			{
				pszKey++;	GETNONWHITESPACE( pszKey );
				if ( !g_World.DumpAreas( pSrc, pszKey ) )
					pSrc->SysMessage( "Area dump failed." DEBUG_CR );
				else
					pSrc->SysMessage( "Area dump successful." DEBUG_CR );
			}
			else
			{
				pSrc->SysMessage(	"Options:" DEBUG_CR
									"A filename\t\t"	"dump areas to file"
									DEBUG_CR );
			}
		}
		break;
#ifdef _DEBUG
	case '^':	// hang forever. (intentionally)
		while (true)
		{
		}
		break;
	case '@':	// cause null pointer error. (intentionally)
		{
			CChar * pChar	= NULL;
			pChar->SysMessage( "Test." );
		}
		break;
#endif
	default:
		goto longcommand;
	}
	goto endconsole;

longcommand:
	if (( len > 1 ) && ( sText[1] != ' ' ) || ( low == 'b' ))
	{
		LPCTSTR	pszText = sText;

		if ( g_Cfg.IsConsoleCmd(low) ) pszText++;

		CScript	script(pszText);
		if ( !g_Cfg.CanUsePrivVerb(this, pszText, pSrc) )
		{
			pSrc->SysMessagef("not privleged for command '%s'" DEBUG_CR, pszText);
		}
		else if ( !r_Verb(script, pSrc) )
		{
			pSrc->SysMessagef("unknown command '%s'" DEBUG_CR, pszText);
		}
	}
	else pSrc->SysMessagef("unknown command '%s'" DEBUG_CR, (LPCTSTR)sText);

endconsole:
	sText.Empty();
	return( true );
}

//************************************************

bool CServer::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	if ( isdigit( pszKey[0] ))
	{
		int i=1;
		while ( isdigit( pszKey[i] ))
			i++;
		if ( pszKey[i] == '.' )
		{
			int index = atoi( pszKey );	// must use this to stop at .
			pRef = g_Cfg.Server_GetDef(index);
			pszKey += i + 1;
			return( true );
		}
	}
	if ( g_Cfg.r_GetRef( pszKey, pRef ))
	{
		return( true );
	}
	if ( g_World.r_GetRef( pszKey, pRef ))
	{
		return( true );
	}
	return( CScriptObj::r_GetRef( pszKey, pRef ));
}

void CServer::r_DumpVerbKeys( CTextConsole * pSrc )
{
	CScriptObj::r_DumpKeys(pSrc,sm_szVerbKeys);
	CServerDef::r_DumpVerbKeys(pSrc);
}
void CServer::r_DumpLoadKeys( CTextConsole * pSrc )
{
	g_Cfg.r_DumpLoadKeys(pSrc);
	g_World.r_DumpLoadKeys(pSrc);
	CServerDef::r_DumpLoadKeys(pSrc);
}

bool CServer::r_LoadVal( CScript &s )
{
	if ( g_Cfg.r_LoadVal(s) )
		return true;
	if ( g_World.r_LoadVal(s) )
		return true;
	return CServerDef::r_LoadVal(s);
}

bool CServer::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	if ( !strnicmp(pszKey, "ACCOUNT.", 8) )
	{
		pszKey += 8;
		int num = Exp_GetVal(pszKey);
		SKIP_SEPERATORS(pszKey);

		if (( num >= 0 ) && ( num < g_Accounts.Account_GetCount() ) && *pszKey )
		{
			CAccountRef pAccount = g_Accounts.Account_Get(num);
			if ( pAccount )
				return pAccount->r_WriteVal(pszKey, sVal, pSrc);
		}
		return false;
	}

	// Just do stats values for now.
	if ( g_Cfg.r_WriteVal(pszKey, sVal, pSrc) )
		return true;
	if ( g_World.r_WriteVal(pszKey, sVal, pSrc) )
		return true;
	return CServerDef::r_WriteVal(pszKey, sVal, pSrc);
}

void CServer::r_Write( CScript &s )
{
	s.WriteSection( g_Cfg.GetResourceBlockName(RES_SPHERE));
	s.WriteKey( "NAME", GetName());
	CServerDef::r_WriteData( s );
	g_Cfg.r_Write(s);
}

enum SV_TYPE
{
	SV_ACCOUNT,
	SV_ALLCLIENTS,
	SV_B,
	SV_BLOCKIP,
	SV_DELETEFILE,
	SV_EXPORT,
	SV_HEARALL,
	SV_IMPORT,
	SV_INFORMATION,
	SV_LOAD,
	SV_LOG,
	SV_RESPAWN,
	SV_RESTOCK,
	SV_RESTORE,
	SV_RESYNC,
	SV_SAVE,
	SV_SAVEINI,
	SV_SAVESTATICS,
	SV_SECURE,
	SV_SERVLIST,
	SV_SHUTDOWN,
	SV_UNBLOCKIP,
	SV_VERBOSE,
	SV_WRITEFILE,
	SV_QTY,
};

LPCTSTR const CServer::sm_szVerbKeys[SV_QTY+1] =
{
	"ACCOUNT",
	"ALLCLIENTS",
	"B",
	"BLOCKIP",
	"DELETEFILE",
	"EXPORT",
	"HEARALL",
	"IMPORT",
	"INFORMATION",
	"LOAD",
	"LOG",
	"RESPAWN",
	"RESTOCK",
	"RESTORE",
	"RESYNC",
	"SAVE",
	"SAVEINI",
	"SAVESTATICS",
	"SECURE",
	"SERVLIST",
	"SHUTDOWN",
	"UNBLOCKIP",
	"VERBOSE",
	"WRITEFILE",
	NULL,
};

bool CServer::r_Verb( CScript &s, CTextConsole * pSrc )
{
	LOCKDATA;
	EXC_TRY(("r_Verb('%s %s',%x)", s.GetKey(), s.GetArgStr(), pSrc));
	ASSERT(pSrc);

	TCHAR *pszMsg = Str_GetTemp();
	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF( sm_szVerbKeys )-1 );
	switch (index)
	{
	case SV_ACCOUNT: // "ACCOUNT"
		return g_Accounts.Account_OnCmd(s.GetArgRaw(), pSrc);

	case SV_ALLCLIENTS:	// "ALLCLIENTS"
		{
			// Send a verb to all clients
			for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
			{
				if ( pClient->GetChar() == NULL )
					continue;
				CScript script( s.GetArgStr() );
				pClient->GetChar()->r_Verb( script, pSrc );
			}
		}
		break;

	case SV_B: // "B"
		g_World.Broadcast( s.GetArgStr());
		break;

	case SV_BLOCKIP:
		if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
			return( false );
		if ( g_Cfg.SetLogIPBlock( s.GetArgRaw(), true ))
		{
			pSrc->SysMessage( "IP Blocked" DEBUG_CR );
		}
		else
		{
			pSrc->SysMessage( "IP Already Blocked" DEBUG_CR );
		}
		break;
	case SV_EXPORT: // "EXPORT" name [chars] [area distance]
		if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
			return( false );
		if ( s.HasArgs())
		{
			TCHAR * Arg_ppCmd[5];
			int Arg_Qty = Str_ParseCmds( s.GetArgRaw(), Arg_ppCmd, COUNTOF( Arg_ppCmd ));
			if ( ! Arg_Qty )
			{
				break;
			}
			// IMPFLAGS_ITEMS
			if ( ! g_World.Export( Arg_ppCmd[0], pSrc->GetChar(),
				(Arg_Qty>=2)? atoi(Arg_ppCmd[1]) : IMPFLAGS_ITEMS,
				(Arg_Qty>=3)? atoi(Arg_ppCmd[2]) : SHRT_MAX ))
			{
				pSrc->SysMessage( "Export failed" DEBUG_CR );
			}
		}
		else
		{
			pSrc->SysMessage( "EXPORT name [flags] [area distance]" );
		}
		break;

	case SV_HEARALL:	// "HEARALL" = Hear all said.
		{
		g_Log.SetLogMask( s.GetArgFlag( g_Log.GetLogMask(), LOGM_PLAYER_SPEAK ));
		sprintf(pszMsg, "Hear All %s." DEBUG_CR, g_Log.IsLoggedMask(LOGM_PLAYER_SPEAK) ? "Enabled" : "Disabled" );
		}
		break;

	case SV_INFORMATION:
		pSrc->SysMessage( GetStatusString( 0x22 ));
		pSrc->SysMessage( GetStatusString( 0x24 ));
		break;

	case SV_IMPORT: // "IMPORT" name [flags] [area distance]
		if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
			return( false );
		if (s.HasArgs())
		{
			TCHAR * Arg_ppCmd[5];
			int Arg_Qty = Str_ParseCmds( s.GetArgRaw(), Arg_ppCmd, COUNTOF( Arg_ppCmd ));
			if ( ! Arg_Qty )
			{
				break;
			}
			// IMPFLAGS_ITEMS
			if ( ! g_World.Import( Arg_ppCmd[0], pSrc->GetChar(),
				(Arg_Qty>=2)?atoi(Arg_ppCmd[1]) : IMPFLAGS_BOTH,
				(Arg_Qty>=3)?atoi(Arg_ppCmd[2]) : SHRT_MAX ))
			{
				pSrc->SysMessage( "Import failed" DEBUG_CR );
			}
			// addReSync();
		}
		else
		{
			pSrc->SysMessage( "IMPORT name [flags] [area distance]" );
		}
		break;
	case SV_LOAD:
		// Load a resource file.
		if ( g_Cfg.LoadResourcesAdd( s.GetArgStr()) == NULL )
			return( false );
		return( true );

	case SV_LOG:	// "LOG" = Turn the log file on or off.
		bool	oldstate;
		bool	newstate;
		
		oldstate		= g_Log.IsFileOpen();
		if ( 0 ) ;
		else if ( !strcmpi( s.GetArgStr(), "@enable" ) )
			newstate	= true;
		else if ( !strcmpi( s.GetArgStr(), "@disable" ) )
			newstate	= false;
		else if ( !strcmpi( s.GetArgStr(), "@toggle" ) )
			newstate	= !oldstate;
		else
		{
			// log a string
			g_Log.Event( LOGM_CLIENTS_LOG| LOGL_EVENT, "%s" DEBUG_CR, s.GetArgStr() );
			break;
				
		}
		
		sprintf(pszMsg, "Log file %s%s." DEBUG_CR,
			oldstate == newstate ? "already " : "",
			newstate ? "enabled" : "disabled" );

		if ( newstate == oldstate )
			;
		else if ( newstate )
			g_Log.OpenLog();
		else
			g_Log.Close();
		break;

	case SV_RESPAWN:
		g_World.RespawnDeadNPCs();
		break;

	case SV_RESTOCK:
		// set restock time of all vendors in World.
		// set the respawn time of all spawns in World.
		g_World.Restock();
		break;

	case SV_RESTORE:	// "RESTORE" backupfile.SCP Account CharName
		if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
			return( false );
		if (s.HasArgs())
		{
			TCHAR * Arg_ppCmd[4];
			int Arg_Qty = Str_ParseCmds( s.GetArgRaw(), Arg_ppCmd, COUNTOF( Arg_ppCmd ));
			if ( ! Arg_Qty )
			{
				break;
			}
			if ( ! g_World.Import( Arg_ppCmd[0], pSrc->GetChar(),
				IMPFLAGS_BOTH|IMPFLAGS_ACCOUNT, SHRT_MAX,
				Arg_ppCmd[1], Arg_ppCmd[2] ))
			{
				pSrc->SysMessage( "Restore failed" DEBUG_CR );
			}
			else
			{
				pSrc->SysMessage( "Restore success" DEBUG_CR );
			}
		}
		break;
	case SV_RESYNC:
		{
			if ( !m_fResyncPause )
			{
				SetResyncPause(true, pSrc);
				SetResyncPause(false, pSrc);
			}
			else SetResyncPause(false, pSrc);	// this is impossible from script since they're stopped but still useful in console
			break;
		}
	case SV_SAVE: // "SAVE" x
		g_World.Save(s.GetArgVal());
		break;
	case SV_SAVESTATICS:
		g_World.SaveStatics();
		break;
	case SV_SECURE: // "SECURE"
		g_Cfg.m_fSecure = s.GetArgFlag( g_Cfg.m_fSecure, true );
		SetSignals();
		sprintf(pszMsg, "Secure mode %s." DEBUG_CR, g_Cfg.m_fSecure ? "re-enabled" : "disabled" );
		break;

	case SV_SAVEINI:
		g_Cfg.SaveIni();
		break;

	case SV_SERVLIST:	// "SERVLIST",
		ListServers( pSrc );
		break;

	case SV_SHUTDOWN: // "SHUTDOWN"
		Shutdown(( s.HasArgs()) ? s.GetArgVal() : 15 );
		break;

	case SV_UNBLOCKIP:	// "UNBLOCKIP"
		if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
			return( false );
		if ( g_Cfg.SetLogIPBlock( s.GetArgRaw(), false ))
		{
			pSrc->SysMessage( "IP UN-Blocked" DEBUG_CR );
		}
		else
		{
			pSrc->SysMessage( "IP Was NOT Blocked" DEBUG_CR );
		}
		break;

	case SV_VERBOSE: // "VERBOSE"
		g_Log.SetLogLevel(( g_Log.GetLogLevel() >= LOGL_TRACE ) ? LOGL_EVENT : LOGL_TRACE );
		sprintf(pszMsg, "Verbose display %s." DEBUG_CR, (LPCTSTR) ( g_Log.IsLogged(LOGL_TRACE) ? "Enabled" : "Disabled" ));
		break;

	case SV_WRITEFILE:
		if (!IsSetOF(OF_FileCommands))
			return(0);
		if (s.HasArgs())
		{
			TCHAR	*wfArgv[1];
			FILE	*wfFD;

			if ( Str_ParseCmds(s.GetArgRaw(), wfArgv, COUNTOF(wfArgv)) < 2 )
				break;
			strcat(wfArgv[1], "\n");

			wfFD = fopen(wfArgv[0], "a");
			if ( !wfFD ) return 0;
			fputs(wfArgv[1], wfFD);
			fclose(wfFD);
		}
		break;

	case SV_DELETEFILE:
		if (!IsSetOF(OF_FileCommands))
			return(0);
		unlink(s.GetArgRaw());
		break;

	default:
		return( CScriptObj::r_Verb( s, pSrc ));
	}

	if ( *pszMsg )
		pSrc->SysMessage(pszMsg);

	return true;
	EXC_CATCH("CServer");
	return false;
}

//*********************************************************

extern void defragSphere(char *);

bool CServer::CommandLine( int argc, TCHAR * argv[] )
{
	// Console Command line.
	// This runs after script file enum but before loading the world file.
	// RETURN:
	//  true = keep running after this.
	//

	// NT Service switches:
	// -install			to install the service
	// -remove			to remove the service
	// -debug <params>	to run as a console application for debugging

	for ( int argn=1; argn<argc; argn++ )
	{
		TCHAR * pArg = argv[argn];
		if ( ! _IS_SWITCH( pArg[0] ))
			continue;

		pArg ++;
		TCHAR ch = toupper( pArg[0] );

		switch ( ch )
		{
		case '?':
			PrintStr( GRAY_TITLE " " DEBUG_CR
				"Command Line Switches:" DEBUG_CR
				"-? This help list." DEBUG_CR
				"-D Dump global variable DEFNAMEs to defs.txt" DEBUG_CR
				"-Gpath/to/saves/ Defrags sphere saves" DEBUG_CR
				"-INSTALL as NT service" DEBUG_CR
				"-Lfilename Load this file for the world" DEBUG_CR
				"-Nstring Set the sphere name." DEBUG_CR
				"-P# Set the port number." DEBUG_CR
				"-Ofilename Output console to this file name" DEBUG_CR
				"-Q Quit when finished." DEBUG_CR
				"-1## Dump itemdata.mul with this flag mask to items.txt" DEBUG_CR
				"-2 Dump the ground tiles database to terrain.txt" DEBUG_CR
				"-3 Dump a list of all char types to npcs.txt" DEBUG_CR
				"-4 Xref itemdata.mul with the scripts. list unscripted items." DEBUG_CR
				"-5 Xref char ANIM= defs with ANIM.IDX" DEBUG_CR
				);
			return( false );
		case 'P':
			// Set the port.
			m_ip.SetPort( atoi( pArg+1 ));
			continue;
		case 'N':
			// Set the system name.
			SetName( pArg+1 );
			continue;
		case 'L':	// load a particular backup world file.
			if ( ! g_World.LoadAll( pArg+1 ))
				return( false );
			continue;
		case 'D':
			// dump all the defines to a file.
			{
				CFileText File;
				if ( ! File.Open( "defs.txt", OF_WRITE|OF_TEXT ))
					return( false );
				int i = 0;
				for ( ; i < g_Exp.m_VarDefs.GetCount(); i++ )
				{
					if ( !( i%0x1ff ))
						PrintPercent( i, g_Exp.m_VarDefs.GetCount());
					File.Printf( "%s=%s\n",
						g_Exp.m_VarDefs[i]->GetKey(), g_Exp.m_VarDefs[i]->GetValStr());
				}
			}
			continue;
		case 'G':
			//	defrag sphere saves
			defragSphere(pArg + 1);
			continue;
		case 'O':
			// put the log output here
			if ( g_Log.Open( pArg+1, OF_SHARE_DENY_WRITE|OF_READWRITE|OF_TEXT ))
			{
				g_Log.m_fLockOpen = true;
			}
			continue;

		case 'X':
		case 'Q':
			// Exit the server.
			return( false );

		case '1':
			// dump the items database. as just a text doc.
			// Argument = hex bitmask to search thru.
			{
			DWORD dwMask = 0xFFFFFFFF;	// UFLAG4_ANIM
			if ( pArg[1] == '0' )
			{
				dwMask = ahextoi( pArg+1 );
				argn++;
			}

			CFileText File;
			if ( ! File.Open( "items.txt", OF_WRITE|OF_TEXT ))
				return( false );
			int i = 0;
			for ( ; i < ITEMID_MULTI; i++ )
			{
				if ( !( i%0x1ff ))
					PrintPercent( i, ITEMID_MULTI );

				CUOItemTypeRec item;
				if ( ! CItemBase::GetItemData((ITEMID_TYPE) i, &item ))
					continue;
				if ( ! ( item.m_flags & dwMask ))
					continue;

				File.Printf( "%04x: %08x,W%02x,L%02x,?%08x,A%08x,?%04x,H%02x,'%s'\n",
					i,
					item.m_flags,
					item.m_weight,
					item.m_layer,
					item.m_dwUnk6,
					item.m_dwAnim,
					item.m_wUnk14,
					item.m_height,
					item.m_name );
			}
			}
			return( false );

		case '2':
			// dump the ground tiles database.
			{
			CFileText File;
			if ( ! File.Open( "terrain.txt", OF_WRITE|OF_TEXT ))
				return( false );

			for ( int i=0; i<TERRAIN_QTY; i++ )
			{
				CGrayTerrainInfo block( (TERRAIN_TYPE) i);

				if ( ! block.m_flags &&
					! block.m_index &&	// just counts up.  0 = unused.
					! block.m_name[0] )
					continue;

				File.Printf( "%04x: %08x,%04x,'%s'\n",
					i,
					block.m_flags,
					block.m_index,	// just counts up.  0 = unused.
					block.m_name );
			}
			}
			continue;

		case '3':
			// dump a list of npc's and chars from all RES_CHARDEF
			{
			CFileText File;
			if ( ! File.Open( "npcs.txt", OF_WRITE|OF_TEXT ))
				return( false );

			for ( int i=0; i<COUNTOF(g_Cfg.m_ResHash.m_Array); i++ )
			for ( int j=0; j<g_Cfg.m_ResHash.m_Array[i].GetCount(); j++ )
			{
				CResourceDef* pResDef = g_Cfg.m_ResHash.m_Array[i][j];
				ASSERT(pResDef);
				if ( pResDef->GetResType() != RES_CHARDEF )
					continue;
				CCharBase * pCharDef = CCharBase::FindCharBase((CREID_TYPE) pResDef->GetResourceID().GetResIndex() );
				if ( pCharDef == NULL )
					continue;
				File.Printf( "[%04x] '%s'\n", i, pCharDef->GetTypeName());
			}
			}
			continue;

		case '4':
			// Xref the RES_ITEMDEF blocks with the items database to make sure we have defined all.
			g_Cfg.ResourceTestItemMuls();
			continue;

		case '5':
			// xref the ANIM= lines in RES_CHARDEF with ANIM.IDX
			g_Cfg.ResourceTestCharAnims();
			return( false );

		default:
			g_Log.Event( LOGM_INIT|LOGL_CRIT, "Don't recognize command line data '%s'" DEBUG_CR, (LPCTSTR)( argv[argn] ));
			break;
		}
	}

	return( true );
}

void CServer::SetResyncPause( bool fPause, CTextConsole * pSrc )
{
	ASSERT(pSrc);
	if ( fPause )
	{
		m_fResyncPause = true;
		pSrc->SysMessage( "Server is PAUSED for Resync." DEBUG_CR );
		g_World.Broadcast( g_Cfg.GetDefaultMsg( DEFMSG_SERVER_RESYNC_START ) );

		g_Cfg.Unload(true);
		SetServerMode( SERVMODE_ResyncPause );
	}
	else
	{
		pSrc->SysMessage( "Resync Restart" DEBUG_CR );

		// Set all the SCP files to re-index.
		// Relock the SCP files.
		SetServerMode( SERVMODE_ResyncLoad );	// IsLoading()
		if ( ! g_Cfg.Load(true))
		{
			pSrc->SysMessage( "Resync FAILED!" DEBUG_CR );
			g_World.Broadcast( g_Cfg.GetDefaultMsg( DEFMSG_SERVER_RESYNC_FAILED ) );
		}
		else
		{
			pSrc->SysMessage( "Resync Complete!" DEBUG_CR );
			g_World.Broadcast( g_Cfg.GetDefaultMsg( DEFMSG_SERVER_RESYNC_SUCCESS ) );
		}
		m_fResyncPause = false;
		SetServerMode( SERVMODE_Run );	// ready to go. ! IsLoading()
	}
}

//*********************************************************

CClient * CServer::SocketsReceive( CGSocket & socket, bool fGod ) // Check for messages from the clients
{
	CSocketAddress client_addr;
	SOCKET hSocketClient = socket.Accept( client_addr );
	if ( hSocketClient < 0 || hSocketClient == INVALID_SOCKET )	// LINUX case is signed ?
	{
		// NOTE: Client_addr might be invalid.
		g_Log.Event( LOGL_FATAL|LOGM_CLIENTS_LOG, "Failed at client connection to '%s'(?)" DEBUG_CR, (LPCTSTR) client_addr.GetAddrStr());
		return NULL;
	}

	CLogIP * pLogIP = g_Cfg.FindLogIP( client_addr, true );
	if ( pLogIP == NULL || pLogIP->CheckPingBlock( true ))
	{
		// kill it by allowing it to go out of scope.
		CGSocket sockjunk( hSocketClient );
		return NULL;
	}

	// too many connecting on this IP
	if ( ( g_Cfg.m_iConnectingMaxIP > 0
		&& pLogIP->m_iConnecting > g_Cfg.m_iConnectingMaxIP )
	||   (  g_Cfg.m_iClientsMaxIP > 0 
		&& pLogIP->m_iConnected > g_Cfg.m_iClientsMaxIP ) )
	{
		// kill
		CGSocket sockjunk( hSocketClient );
		return NULL;
	}


	return( new CClient( hSocketClient ));
}

void CServer::SocketsReceive() // Check for messages from the clients
{
	//	Do not accept packets while loading
	if ( m_iModeCode > SERVMODE_ResyncPause ) return;

	// What sockets do I want to look at ?
	fd_set readfds;
	FD_ZERO(&readfds);
	SOCKET hSocket;
	int nfds;

	if ( g_Cfg.m_iUseGodPort )
	{
		hSocket = m_SocketGod.GetSocket();
		FD_SET( hSocket, &readfds);
		nfds = hSocket;
	}

	hSocket = m_SocketMain.GetSocket();
	FD_SET( hSocket, &readfds);
	if ( hSocket > nfds )
		nfds = hSocket;

	int	connecting	= 0;
	
	CClient * pClientNext;
	CClient * pClient = GetClientHead();
	for ( ; pClient!=NULL; pClient = pClientNext )
	{
		pClientNext = pClient->GetNext();
		if ( !pClient->m_Socket.IsOpen() || pClient->m_fClosed )
		{
			delete pClient;
			continue;
		}

		if ( pClient->IsConnecting() )
		{
			connecting++;
			if ( connecting > g_Cfg.m_iConnectingMax )
			{
				delete pClient;
				continue;
			}
		}

		hSocket = pClient->m_Socket.GetSocket();
		FD_SET(hSocket,&readfds);
		if ( hSocket > nfds )
			nfds = hSocket;
	}

	if ( connecting > g_Cfg.m_iConnectingMax )
	{
		 g_Log.Event( LOGL_WARN|LOGM_CHEAT,
		"%d clients in connect mode (max %d), closing %d" DEBUG_CR,
		connecting,  g_Cfg.m_iConnectingMax,
		connecting - g_Cfg.m_iConnectingMax );
	}
	// we task sleep in here. NOTE: this is where we give time back to the OS.

	m_Profile.Start( PROFILE_IDLE );

	timeval Timeout;	// time to wait for data.
	Timeout.tv_sec=0;
	Timeout.tv_usec=100;	// micro seconds = 1/1000000
	int ret = select( nfds+1, &readfds, NULL, NULL, &Timeout );
	if ( ret <= 0 )
	{
		m_Profile.Start( PROFILE_OVERHEAD );
		return;
	}

	m_Profile.Start( PROFILE_NETWORK_RX );

	// Any events from clients ?
	for ( pClient = GetClientHead(); pClient!=NULL; pClient = pClientNext )
	{
		pClientNext = pClient->GetNext();
		if ( !pClient->m_Socket.IsOpen() || pClient->m_fClosed )
		{
			delete pClient;
			continue;
		}

		if ( FD_ISSET( pClient->m_Socket.GetSocket(), &readfds ))
		{
			pClient->m_timeLastEvent = CServTime::GetCurrentTime();	// We should always get pinged every couple minutes or so
			if ( !pClient->xRecvData() )
			{
				delete pClient;
				continue;
			}
			if ( pClient->m_fClosed )		// can happen due to data received
			{
				delete pClient;
				continue;
			}
		}
		else
		{
			// NOTE: Not all CClient are game clients.

			int iLastEventDiff = -g_World.GetTimeDiff( pClient->m_timeLastEvent );

			if ( g_Cfg.m_iDeadSocketTime &&
				iLastEventDiff > g_Cfg.m_iDeadSocketTime &&
				pClient->GetConnectType() != CONNECT_TELNET )
			{
				// We have not talked in several minutes.
				DEBUG_ERR(( "%x:Dead Socket Timeout" DEBUG_CR, pClient->m_Socket.GetSocket()));
				delete pClient;
				continue;
			}
			if ( pClient->IsConnectTypePacket())
			{
				if ( iLastEventDiff > 1*60*TICK_PER_SEC &&
					-g_World.GetTimeDiff( pClient->m_timeLastSend ) > 1*60*TICK_PER_SEC )
				{
					// Send a periodic ping to the client. If no other activity !
					pClient->addPing(0);
				}
			}
		}

		// On a timer allow the client to walk.
		// catch up with real time !
		pClient->addWalkCode( EXTDATA_WalkCode_Add, 1 );
	}

	if ( g_Cfg.m_iUseGodPort )
	{
		// Any new connections ? what if there are several ?
		if ( FD_ISSET( m_SocketGod.GetSocket(), &readfds))
		{
			SocketsReceive( m_SocketGod, true );
		}
	}

	if ( FD_ISSET( m_SocketMain.GetSocket(), &readfds))
	{
		SocketsReceive( m_SocketMain, false );
	}

	m_Profile.Start( PROFILE_OVERHEAD );
}

void CServer::SocketsFlush() // Sends ALL buffered data
{
	for ( CClient * pClient = GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		pClient->xFlush();
		pClient->addPause( false );	// always turn off pause here if it is on.
		pClient->xFlush();
	}
}

void CServer::OnTick()
{
	TIME_PROFILE_INIT;
	if ( IsSetSpecific )
		TIME_PROFILE_START;
#if defined(_CONSOLE)
#ifdef _WIN32
	while ( _kbhit())
	{
		int iRet = OnConsoleKey( m_sConsoleText, _getch(), true );
#else
        // Do a select operation on the stdin handle and check
        // if there is any input waiting.
        fd_set consoleFds;
        FD_ZERO( &consoleFds );
        FD_SET( STDIN_FILENO, &consoleFds );

        timeval tvTimeout;
        tvTimeout.tv_sec = 0;
        tvTimeout.tv_usec = 1;

        if( select( 1, &consoleFds, 0, 0, &tvTimeout ) )
	{
		int c = fgetc( stdin );
		int iRet = OnConsoleKey( m_sConsoleText, c, false );
#endif
		if ( iRet == 2 )
		{
			m_fConsoleTextReadyFlag = true;
		}
	}
#endif

	if ( m_fConsoleTextReadyFlag )
	{
		CGString sText = m_sConsoleText;	// make a copy.
		m_sConsoleText.Empty();	// done using this.
		m_fConsoleTextReadyFlag = false; // rady to use again
		OnConsoleCmd( sText, this );
	}

	SetValidTime();	// we are a valid game server.

	// Check clients for incoming packets.
	// Do this on a timer so clients with faster connections can't overwealm the system.
	SocketsReceive();

	if ( ! IsLoading())
	{
		m_Profile.Start( PROFILE_CLIENTS );

		for ( CClient * pClient = GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
		{
			if ( ! pClient->xHasData()) continue;

#if !defined( _DEBUG ) && !defined( NO_INTERNAL_EXCEPTIONS )
			try
			{
#endif
				pClient->xProcessMsg( pClient->xDispatchMsg() );
#if !defined( _DEBUG ) && !defined( NO_INTERNAL_EXCEPTIONS )
			}
			catch ( CGrayError &e )	// catch all
			{
				g_Log.CatchEvent( &e, "Server OnTick" );
				// clear this clients messages. so it won't do the same bad thing next time.
				pClient->xProcessMsg( NULL );
			}
			catch (...)	// catch all
			{
				g_Log.CatchEvent( NULL, "Server OnTick" );
				// clear this clients messages. so it won't do the same bad thing next time.
				pClient->xProcessMsg( NULL );
			}
#endif
		}
	}

	m_Profile.Start( PROFILE_NETWORK_TX );
	SocketsFlush();
	m_Profile.Start( PROFILE_OVERHEAD );

	if ( m_timeShutdown.IsTimeValid() )
	{
		if ( g_World.GetTimeDiff(m_timeShutdown) <= 0 )
		{
			SetExitFlag( 2 );
		}
		else if ( GetTimeSinceLastPoll() >= ( 60 * TICK_PER_SEC ))
		{
			Shutdown(-1);
		}
	}

	g_Cfg.OnTick(false);
	if ( IsSetSpecific )
	{
		TIME_PROFILE_END;
		int	hi = TIME_PROFILE_GET_HI;
		if ( hi > 5 )
		{
			DEBUG_ERR(("CServer::OnTick() [socket operations] took %d.%d to run\n", hi, TIME_PROFILE_GET_LO));
		}
	}
}

bool CServer::SocketsInit( CGSocket & socket, bool fGod ) 
{
	// Initialize socket
	if ( ! socket.Create())
	{
		g_Log.Event( LOGL_FATAL|LOGM_INIT, "Unable to create socket!" DEBUG_CR);
		return( false );
	}

	// getsockopt retrieve the value of socket option SO_MAX_MSG_SIZE.
	// If the data is too long to pass atomically through the underlying protocol,
	// the error WSAEMSGSIZE is returned, and no data is transmitted.

	m_uSizeMsgMax = 0;
	int iSize = sizeof(m_uSizeMsgMax);
	int iRet = socket.GetSockOpt( SO_SNDBUF, &m_uSizeMsgMax, &iSize );	// SO_MAX_MSG_SIZE SO_MAX_MSG_SIZE

	linger lval;
	lval.l_onoff = 0;
	lval.l_linger = 10;

	iRet = socket.SetSockOpt( SO_LINGER, (const char*) &lval, sizeof(lval));
	if ( iRet )
	{
		DEBUG_ERR(( "setsockopt linger FAIL?" DEBUG_CR ));
	}

#ifdef _WIN32
	// blocking io.
	DWORD lVal = 1;	// 0 =  block
	iRet = socket.IOCtlSocket( FIONBIO, &lVal );
	DEBUG_CHECK( iRet==0 );
#endif
	//BOOL fon=1;
	//iRet = socket.SetSockOpt( SO_REUSEADDR, &fon, sizeof(fon));
	//DEBUG_CHECK( iRet==0 );

	// Bind to just one specific port if they say so.
	CSocketAddress SockAddr = m_ip;
	if ( fGod )
	{
		WORD	port = g_Cfg.m_iUseGodPort;
		if ( port <= 1 )
			port = m_ip.GetPort()+1000;
		SockAddr.SetPort(port);
	}
	iRet = socket.Bind(SockAddr);
	if (iRet<0)
	{
		// Probably already a server running.
		g_Log.Event( LOGL_FATAL|LOGM_INIT, "Unable to bind listen socket %s port %d - Error code: %i" DEBUG_CR,
			(LPCTSTR) SockAddr.GetAddrStr(), SockAddr.GetPort(), iRet );
		return( false );
	}

	socket.Listen();
	return( true );
}

bool CServer::SocketsInit() // Initialize sockets
{
	if ( g_Cfg.m_iUseGodPort )
	{
		if ( ! SocketsInit( m_SocketGod, true ))
			return( false );
	}
	if ( ! SocketsInit( m_SocketMain, false ))
		return( false );

	// What are we listing our port as to the world.
	// Tell the admin what we know.

	TCHAR szName[ _MAX_PATH ];
	struct hostent * pHost = NULL;
	int iRet = gethostname( szName, sizeof( szName ));
	if ( iRet )
	{
		strcpy( szName, m_ip.GetAddrStr());
	}
	else
	{
		pHost = gethostbyname( szName );
		if ( pHost != NULL &&
			pHost->h_addr != NULL &&
			pHost->h_name &&
			pHost->h_name[0] )
		{
			strcpy( szName, pHost->h_name );
		}
	}

	g_Log.Event( LOGM_INIT, "Server started on '%s' port %d." DEBUG_CR, szName, m_ip.GetPort());

	if ( ! iRet )
	{
		if ( pHost == NULL || pHost->h_addr == NULL )	// can't resolve the address.
		{
			g_Log.Event( LOGL_CRIT|LOGM_INIT, "gethostbyname does not resolve the address." DEBUG_CR );
		}
		else
		{
			for ( int j=0; pHost->h_aliases[j]; j++ )
			{
				g_Log.Event( LOGM_INIT, "Alias '%s'." DEBUG_CR, (LPCTSTR) pHost->h_aliases[j] );
			}
			// h_addrtype == 2
			// h_length = 4
			for ( int i=0; pHost->h_addr_list[i] != NULL; i++ )
			{
				CSocketAddressIP ip;
				ip.SetAddrIP( *((DWORD*)( pHost->h_addr_list[i] ))); // 0.1.2.3
				if ( ! m_ip.IsLocalAddr() && ! m_ip.IsSameIP( ip ))
				{
					continue;
				}
				g_Log.Event( LOGM_INIT, "Monitoring IP '%s'." DEBUG_CR, (LPCTSTR) ip.GetAddrStr());
			}
		}
	}
	return( true );
}

void CServer::SocketsClose()
{
	m_SocketMain.Close();
	if ( g_Cfg.m_iUseGodPort )
		m_SocketGod.Close();
	m_Clients.DeleteAll();
}

bool CServer::Load()
{
	static LPCTSTR const excInfo[] =
	{
		"",
		"init winsock",
		"init rand",
		"loading ini",
		"log write",
		"setting signals",
		"loading scripts",
		"finilizing",
	};
	const char *zTemp = excInfo[0];
	EXC_TRY(("CServer::Load()"));
	DEBUG_CHECK( IsLoading());

	// Keep track of the thread that is the parent.
	m_dwParentThread = CThread::GetCurrentThreadId();


#ifdef _WIN32
	zTemp = excInfo[1];
	if ( ! m_SocketMain.IsOpen() )
	{
		WSADATA wsaData;
		int err = WSAStartup(MAKEWORD(2,2), &wsaData);
		if ( err )
		{
			if ( err == WSAVERNOTSUPPORTED )
			{
				err = WSAStartup(MAKEWORD(1,1), &wsaData);
				if ( err ) goto nowinsock;
			}
			else
			{
nowinsock:		g_Log.Event(LOGL_FATAL|LOGM_INIT, "Winsock 1.1 not found!" DEBUG_CR);
				return( false );
			}
		}
//		DEBUG_ERR(("WSAStartup: version [Max:%d,Sel:%d], status [%s %s], sockets limit [%d]" DEBUG_CR, wsaData.wHighVersion, wsaData.wVersion, wsaData.szDescription, wsaData.szSystemStatus, wsaData.iMaxSockets));
	}
#endif

	zTemp = excInfo[2];
#ifdef _DEBUG
	srand( 0 ); // regular randomizer.
#else
	srand( CWorldClock::GetSystemClock()); // Perform randomize
#endif

	zTemp = excInfo[3];
	if ( !g_Cfg.LoadIni(false) )
		return false;

	zTemp = excInfo[4];
	g_Log.WriteString( DEBUG_CR );		// blank space in log.

	g_Log.Event( LOGM_INIT, g_szServerDescription, GRAY_TITLE, GRAY_VERSION );
	g_Log.Event( LOGM_INIT, DEBUG_CR );
	g_Log.Event( LOGM_INIT, "Compiled at " __DATE__ " (" __TIME__ ")" DEBUG_CR );
	g_Log.Event( LOGM_INIT, DEBUG_CR );
	
	zTemp = excInfo[5];
	SetSignals( );

	zTemp = excInfo[6];
	if ( !g_Cfg.Load(false) )
		return( false );
	
	zTemp = excInfo[7];
	TCHAR szVersion[128];
	g_Log.Event( LOGM_INIT, "ClientVersion=%s" DEBUG_CR, (LPCTSTR) m_ClientVersion.WriteClientVer( szVersion ));
	if ( ! m_ClientVersion.IsValid())
	{
		g_Log.Event( LOGL_FATAL|LOGM_INIT, "Bad Client Version '%s'" DEBUG_CR, szVersion );
		return( false );
	}

#ifdef _WIN32
	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, GRAY_TITLE " V" GRAY_VERSION " - %s", (LPCTSTR) GetName());
	SetConsoleTitle(pszTemp);
#endif

	return true;
	EXC_CATCH(zTemp);
	return false;
}


