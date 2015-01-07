//
// Ctime.cpp
//
// Replace the MFC CTime function. Must be usable with file system.
//

#include "graycom.h"
#include "CTime.h"

#ifndef _WIN32
	#include <sys/time.h>

	LONGLONG GetTickCount()
	{
		struct timeval tv;
		gettimeofday( &tv, NULL );
		return (LONGLONG) (((LONGLONG) tv.tv_sec * 1000) + ((LONGLONG) tv.tv_usec/1000));
	}
#endif


//**************************************************************
// -CGTime - absolute time

CGTime::CGTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec,
	int nDST)
{
	struct tm atm;
	atm.tm_sec = nSec;
	atm.tm_min = nMin;
	atm.tm_hour = nHour;
	//ASSERT(nDay >= 1 && nDay <= 31);
	atm.tm_mday = nDay;
	//ASSERT(nMonth >= 1 && nMonth <= 12);
	atm.tm_mon = nMonth - 1;        // tm_mon is 0 based
	//ASSERT(nYear >= 1900);
	atm.tm_year = nYear - 1900;     // tm_year is 1900 based
	atm.tm_isdst = nDST;
	m_time = mktime(&atm);
	// ASSERT(m_time != -1);       // indicates an illegal input time
}

CGTime::CGTime( struct tm atm )
{
	m_time = mktime(&atm);
	// ASSERT(m_time != -1);       // indicates an illegal input time
}

CGTime CGTime::GetCurrentTime()	// static
{
	// return the current system time
	return CGTime(::time(NULL));
}

struct tm* CGTime::GetGmtTm(struct tm* ptm) const
{
	if (ptm != NULL)
	{
		*ptm = *gmtime(&m_time);
		return ptm;
	}
	else
		return gmtime(&m_time);
}

struct tm* CGTime::GetLocalTm(struct tm* ptm) const
{
	if (ptm != NULL)
	{
		struct tm* ptmTemp = localtime(&m_time);
		if (ptmTemp == NULL)
			return NULL;    // indicates the m_time was not initialized!

		*ptm = *ptmTemp;
		return ptm;
	}
	else
		return localtime(&m_time);
}

////////////////////////////////////////////////////////////////////////////
// String formatting

#define maxTimeBufferSize       128 	// Verifies will fail if the needed buffer size is too large

LPCTSTR CGTime::Format(LPCTSTR pszFormat) const
{
	TCHAR * pszTemp = Str_GetTemp();

	if ( pszFormat == NULL )
	{
		pszFormat = "%Y/%m/%d %H:%M:%S";
	}

	struct tm* ptmTemp = localtime(&m_time);
	if (ptmTemp == NULL )
	{
		pszTemp[0] = '\0';
		return( pszTemp );
	}

	if (!strftime( pszTemp, maxTimeBufferSize, pszFormat, ptmTemp))
	{
		pszTemp[0] = '\0';
	}

	return( pszTemp );
}

LPCTSTR CGTime::FormatGmt(LPCTSTR pszFormat) const
{
	TCHAR * pszTemp = Str_GetTemp();
	if ( pszFormat == NULL )
	{
		pszFormat = "%a, %d %b %Y %H:%M:%S GMT";
	}

	struct tm* ptmTemp = gmtime(&m_time);
	if (ptmTemp == NULL )
	{
		pszTemp[0] = '\0';
		return( pszTemp );
	}

	if (!strftime( pszTemp, maxTimeBufferSize, pszFormat, ptmTemp))
	{
		pszTemp[0] = '\0';
	}
	return pszTemp;
}

//**************************************************************

static int ReadMonth( LPCTSTR pszVal )
{
	switch ( toupper( pszVal[0] ))
	{
	case 'J':
		if ( toupper( pszVal[1] ) == 'A' )
			return 0;  // january.
		else if ( toupper( pszVal[2] ) == 'N' )
			return 5; // june
		else
			return 6; // july
		break;
	case 'F': return 1; break; // february
	case 'M':
		if ( toupper( pszVal[2] ) == 'R' )
			return 2; // march
		else
			return 4; // may
		break;
	case 'A':
		if ( toupper( pszVal[1] ) == 'P' )
			return 3; // april
		else
			return 7; // august
		break;
	case 'S': return 8; // september
	case 'O': return 9; // october
	case 'N': return 10; // november
	case 'D': return 11; // december
	}
	return 0;
}

bool CGTime::Read( TCHAR * pszVal )
{
	// Read the full date format.

	TCHAR * ppCmds[10];
	int iQty = Str_ParseCmds( pszVal, ppCmds, COUNTOF(ppCmds), "/,: \t" );
	if ( ! iQty )
		return( false );

	struct tm atm;

    atm.tm_wday = 0;    /* days since Sunday - [0,6] */
    atm.tm_yday = 0;    /* days since January 1 - [0,365] */
    atm.tm_isdst = 0;   /* daylight savings time flag */

	if ( isdigit( ppCmds[0][0] ))
	{
		// new format is "1999/8/1 14:30:18"
		if ( iQty < 6 )
		{
			return( false );
		}
		atm.tm_year = atoi( ppCmds[0] ) - 1900;
		atm.tm_mon = atoi( ppCmds[1] ) - 1;
		atm.tm_mday = atoi( ppCmds[2] );
		atm.tm_hour = atoi( ppCmds[3] );
		atm.tm_min = atoi( ppCmds[4] );
		atm.tm_sec = atoi( ppCmds[5] );
	}
	else
	{
		if ( iQty < 7 )
		{
			return( false );
		}

		TCHAR ch = ppCmds[1][0];
		if ( isdigit(ch))
		{
			// or http format is : "Tue, 03 Oct 2000 22:44:56 GMT"
			atm.tm_mday = atoi( ppCmds[1] );
			atm.tm_mon = ReadMonth( ppCmds[2] );
			atm.tm_year = atoi( ppCmds[3] ) - 1900;
			atm.tm_hour = atoi( ppCmds[4] );
			atm.tm_min = atoi( ppCmds[5] );
			atm.tm_sec = atoi( ppCmds[6] );
		}
		else
		{
			// old format is "Tue Mar 30 14:30:18 1999"
			atm.tm_mon = ReadMonth( ppCmds[1] );
			atm.tm_mday = atoi( ppCmds[2] );
			atm.tm_hour = atoi( ppCmds[3] );
			atm.tm_min = atoi( ppCmds[4] );
			atm.tm_sec = atoi( ppCmds[5] );
			atm.tm_year = atoi( ppCmds[6] ) - 1900;
		}
	}

	m_time = mktime(&atm);

	if ( toupper( ppCmds[iQty-1][0] ) == 'G' )
	{
		// convert to GMT
		m_time -= (int)_timezone;
	}
	return( true );
}
