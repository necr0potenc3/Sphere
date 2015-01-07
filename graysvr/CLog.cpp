//
// CLog.cpp
// Copyright Menace Software (www.menasoft.com).
//
#include "graysvr.h"	// predef header.

///////////////////////////////////////////////////////////////
// -CLog

bool CLog::OpenLog( LPCTSTR pszBaseDirName )	// name set previously.
{
	if ( m_fLockOpen )	// the log is already locked open
		return( false );

	if ( m_sBaseDir == NULL )
		return false;

	if ( pszBaseDirName != NULL )
	{
		if ( pszBaseDirName[0] && pszBaseDirName[1] == '\0' )
		{
			if ( *pszBaseDirName == '0' )
			{
				Close();
				return false;
			}
		}
		else
		{
			m_sBaseDir = pszBaseDirName;
		}
	}

	// Get the new name based on date.
	m_dateStamp = CGTime::GetCurrentTime();
	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, GRAY_FILE "%d-%02d-%02d.log",
		m_dateStamp.GetYear(), m_dateStamp.GetMonth(), m_dateStamp.GetDay());
	CGString sFileName = GetMergedFileName(m_sBaseDir, pszTemp);

	// Use the OF_READWRITE to append to an existing file.
	if ( CFileText::Open( sFileName, OF_SHARE_DENY_NONE|OF_READWRITE|OF_TEXT ) )
	{
		setvbuf(m_pStream, NULL, _IONBF, 0);
		return true;
	}
	return false;
}

int CLog::EventStr( DWORD wMask, LPCTSTR pszMsg )
{
	// NOTE: This could be called in odd interrupt context so don't use dynamic stuff
	int iRet = 0;
	try
	{
		if ( ! IsLogged( wMask ))	// I don't care about these.
			return( 0 );

		CThreadLockRef lock(this);

		ASSERT( pszMsg && * pszMsg );

		// Put up the date/time.
		CGTime datetime = CGTime::GetCurrentTime();	// last real time stamp.

		if ( datetime.GetDaysTotal() != m_dateStamp.GetDaysTotal())
		{
			// it's a new day, open with new day name.
			Close();	// LINUX should alrady be closed.
			
			OpenLog( NULL );
			Printf( datetime.Format(NULL));
		}
		else
		{
#ifndef _WIN32
			UINT	mode = OF_READWRITE|OF_TEXT;
			if ( !IsSetOF(OF_Multithreaded) ) mode |= OF_SHARE_DENY_WRITE;
			Open(NULL, mode);	// LINUX needs to close and re-open for each log line !
#endif
		}

		TCHAR szTime[ 32 ];
		sprintf( szTime, "%02d:%02d:", datetime.GetHour(), datetime.GetMinute() );
		m_dateStamp = datetime;

		LPCTSTR pszLabel = NULL;

		switch (wMask & 0x07)
		{
		case LOGL_FATAL:	// fatal error !
			pszLabel = "FATAL:";
			break;
		case LOGL_CRIT:	// critical.
			pszLabel = "CRITICAL:";
			break;
		case LOGL_ERROR:	// non-fatal errors.
			pszLabel = "ERROR:";
			break;
		case LOGL_WARN:
			pszLabel = "WARNING:";
			break;
		}

		// Get the script context. (if there is one)
		TCHAR szScriptContext[ _MAX_PATH + 16 ];
		if ( m_pScriptContext )
		{
			CScriptLineContext LineContext = m_pScriptContext->GetContext();
			sprintf( szScriptContext, "(%s,%d)", m_pScriptContext->GetFileTitle(), LineContext.m_iLineNum );
		}
		else
		{
			szScriptContext[0] = '\0';
		}

		// Print to screen.

#ifdef _WIN32
#ifdef _CONSOLE
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
#endif

		if ( ! ( wMask & LOGM_INIT ) && ! g_Serv.IsLoading())
		{
#ifdef _WIN32
#ifdef _CONSOLE
			SetConsoleTextAttribute( hOut, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY );
#else
			NTWindow_PostMsgColor( RGB( 127,127,0 ));
#endif
#endif

#if !defined( _WIN32 )
			g_Serv.PrintStr( "\e[0;33m" );
#endif
			g_Serv.PrintStr( szTime );

#if !defined( _WIN32 )
			g_Serv.PrintStr( "\e[0m" );
#endif

#ifdef _WIN32
#ifdef _CONSOLE
			SetConsoleTextAttribute( hOut, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY );
#else
			NTWindow_PostMsgColor( 0 );
#endif
#endif
		}

		if ( pszLabel )	// some sort of error 
		{
#ifdef _WIN32
#ifdef _CONSOLE
			SetConsoleTextAttribute( hOut, FOREGROUND_RED|BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE|FOREGROUND_INTENSITY );
#else
			NTWindow_PostMsgColor( RGB( 255,0,0 ));
#endif
#endif

#if !defined( _WIN32 )
			g_Serv.PrintStr( "\e[0;31m" );
#endif

			g_Serv.PrintStr( pszLabel );

#if !defined( _WIN32 )
			g_Serv.PrintStr( "\e[0m" );
#endif

#ifdef _WIN32
#ifdef _CONSOLE
			SetConsoleTextAttribute( hOut, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY );
#else
			NTWindow_PostMsgColor( RGB( 255,255,255 ));
#endif
#endif
		}

		if ( szScriptContext[0] )
		{
#ifdef _WIN32
#ifdef _CONSOLE
			SetConsoleTextAttribute( hOut, FOREGROUND_RED|BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE|FOREGROUND_INTENSITY );
#else
			NTWindow_PostMsgColor( RGB( 0,127,255 ));
#endif
#endif

#if !defined( _WIN32 )
			g_Serv.PrintStr( "\e[1;36m" );
#endif

			g_Serv.PrintStr( szScriptContext );

#if !defined( _WIN32 )
			g_Serv.PrintStr( "\e[0m" );
#endif

#ifdef _WIN32
#ifdef _CONSOLE
			SetConsoleTextAttribute( hOut, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE );
#else
			NTWindow_PostMsgColor( 0 );
#endif
#endif
		}
		g_Serv.PrintStr( pszMsg );

		// Back to normal color.
#ifdef _WIN32
#ifdef _CONSOLE
		SetConsoleTextAttribute( hOut, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE );
#else
		NTWindow_PostMsgColor( 0 );
#endif
#endif

		// Print to log file.
		WriteString( szTime );
		if ( pszLabel )	WriteString( pszLabel );
		if ( szScriptContext[0] )
		{
			WriteString( szScriptContext );
		}
		WriteString( pszMsg );

		iRet = 1;

#ifndef _WIN32
		Close();
#endif
	}
	catch (...)
	{
		// Not much we can do about this
		iRet = 0;
	}

	return( iRet );
}

CGTime CLog::sm_prevCatchTick;

void _cdecl CLog::CatchEvent( CGrayError * pErr, LPCTSTR pszCatchContext, ... )
{
	CGTime timeCurrent = CGTime::GetCurrentTime();
	if ( sm_prevCatchTick.GetTime() == timeCurrent.GetTime() )	// prevent message floods.
		return;
	// Keep a record of what we catch.
	try
	{
		TCHAR szMsg[512];
		LOGL_TYPE eSeverity;
		int iLen = 0;
		if ( pErr )
		{
			eSeverity = pErr->GetSeverity();
			pErr->GetErrorMessage( szMsg, sizeof(szMsg));
			iLen = strlen(szMsg);
		}
		else
		{
			eSeverity = LOGL_CRIT;
			iLen = sprintf( szMsg, "Unknown Exception", CServTime::GetCurrentTime());
		}

		iLen += sprintf( szMsg+iLen, ", in " );

		va_list vargs;
		va_start(vargs, pszCatchContext);

		iLen += vsprintf( szMsg+iLen, pszCatchContext, vargs );
		iLen += sprintf( szMsg+iLen, DEBUG_CR );

		Event( eSeverity, szMsg );
		va_end(vargs);
	}
	catch(...)
	{
		// Not much we can do about this.
		pErr = NULL;
	}
	sm_prevCatchTick = timeCurrent;
}

void _cdecl CLog::FireEvent( LOGEVENT_TYPE type, ... )
{
}

void CLog::Dump( const BYTE * pData, int len )
{
	// Just dump a bunch of bytes. 16 bytes per line.
	while ( len )
	{
		TCHAR szTmp[16*3+10];
		int j=0;
		for ( ; j < 16; j++ )
		{
			if ( ! len ) break;
			sprintf( szTmp+(j*3), "%02x ", * pData );
			len --;
			pData ++;
		}
		strcpy( szTmp+(j*3), DEBUG_CR );
		g_Serv.PrintStr( szTmp );
		WriteString( szTmp );	// Print to log file.
	}
}

