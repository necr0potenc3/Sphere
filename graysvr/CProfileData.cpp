//
// CProfileData.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.


//////////////////////////////////////////////////////////
// -CProfileData
void CProfileData::SetActive( int iSampleSec )
{
	m_iActiveWindowSec = iSampleSec;
	memset( m_AvgTimes, 0, sizeof( m_AvgTimes ));
	memset( m_CurTimes, 0, sizeof( m_CurTimes ));
	memset( m_PrvTimes, 0, sizeof( m_PrvTimes ));
	m_iAvgCount		= 1;

	if ( ! m_iActiveWindowSec )
		return;

#ifdef _WIN32
	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&m_CurTime) )
		m_CurTime = GetTickCount();
#else
	// ??? Find a high precision timer for LINUX.
	m_CurTime		= GetTickCount();	// our own function
#endif
	m_CurTask = PROFILE_OVERHEAD;
	m_TimeTotal = 0;
}



void CProfileData::Start( PROFILE_TYPE id )
{
	// Stop the previous task and start a new one.
	ASSERT( id < PROFILE_TIME_QTY );
	if ( ! m_iActiveWindowSec )
		return;

	// Stop prev task.
	ASSERT( m_CurTask < PROFILE_TIME_QTY );

	if ( m_TimeTotal >= llTimeProfileFrequency * m_iActiveWindowSec )
	{
		for ( int i = 0; i < PROFILE_QTY; i++ )
		{
			if ( m_iAvgCount < 4 )
				memcpy( m_AvgTimes, m_CurTimes, sizeof( m_AvgTimes ));
			else
			{
			if ( m_PrvTimes[i].m_Time > llTimeProfileFrequency )
				m_PrvTimes[i].m_Time	= llTimeProfileFrequency;
			m_AvgTimes[i].m_Time	= (((m_AvgTimes[i].m_Time * 90)
									+ (m_PrvTimes[i].m_Time*10))/100);
			m_AvgTimes[i].m_iCount	= (((m_AvgTimes[i].m_iCount * 95)
									+ (m_PrvTimes[i].m_iCount*10))/100);
			}
		}

		++m_iAvgCount;

		memcpy( m_PrvTimes, m_CurTimes, sizeof( m_PrvTimes ));
		memset( m_CurTimes, 0, sizeof( m_CurTimes ));
		m_TimeTotal = 0;

		// ??? If NT we can push these values out to the registry !
	}

	// Get the current precise time.
	LONGLONG CurTime;
#ifdef _WIN32
	if ( ! QueryPerformanceCounter((LARGE_INTEGER *) &CurTime ))
	{
		CurTime = GetTickCount();
	}
#else
	CurTime		= GetTickCount();	// our own function
#endif

	// accumulate the time for this task.
	LONGLONG Diff = ( CurTime - m_CurTime );
	m_TimeTotal += Diff;
	m_CurTimes[m_CurTask].m_Time += Diff;
	m_CurTimes[m_CurTask].m_iCount ++;

	// We are now on to the new task.
	m_CurTime = CurTime;
	m_CurTask = id;
}




PROFILE_TYPE	CProfileData::GetCurrentTask()
{
	return m_CurTask;
}


LPCTSTR CProfileData::GetName( PROFILE_TYPE id ) const
{
	ASSERT( id < PROFILE_QTY );
	static LPCTSTR const sm_pszProfileName[PROFILE_QTY] =
	{
		"IDLE",
		"OVERHEAD",		// PROFILE_OVERHEAD
		"NETWORK_RX",	// Just get client info and monitor new client requests. No processing.
		"CLIENTS",		// Client processing.
		"NETWORK_TX",
		"CHARS",
		"ITEMS",
		"NPC_AI",
		"SCRIPTS",
		"DATA_TX",
		"DATA_RX",
	};
	return( sm_pszProfileName[id] );
}

LPCTSTR CProfileData::GetDesc( PROFILE_TYPE id ) const
{
	ASSERT( id < PROFILE_QTY );
	TCHAR * pszTmp = Str_GetTemp();
	int iCount		= m_PrvTimes[id].m_iCount;


	if ( id >= PROFILE_TIME_QTY )
	{
		sprintf( pszTmp, "%i (avg: %i) bytes", (int) m_PrvTimes[id].m_Time, m_AvgTimes[id].m_Time );
	}
	else
	{
		sprintf( pszTmp, "%i.%04is   avg: %i.%04is     [samples:%5i  avg:%5i ]  runtime: %is",
			(int)( m_PrvTimes[id].m_Time / ( llTimeProfileFrequency )),
			(int)((( m_PrvTimes[id].m_Time * 10000 ) / ( llTimeProfileFrequency )) % 10000 ),
			(int) ( m_AvgTimes[id].m_Time / ( llTimeProfileFrequency )),
			(int) ((( m_AvgTimes[id].m_Time * 10000 ) / ( llTimeProfileFrequency )) % 10000 ),
			iCount,
			(int) m_AvgTimes[id].m_iCount,
			m_iAvgCount );
	}
	return( pszTmp );
}

bool CProfileData::r_Load(CScript &s)
{
	while ( s.ReadKeyParse() )
	{
		bool	fQuoted = false;
		LPCTSTR	args;
		LPCTSTR key;
		int		pf;

		args = s.GetArgStr(&fQuoted);
		if ( strlen(args) >= 32 ) goto invalidformat;
		key = s.GetKey();
		for ( pf = 0; pf < PROFILE_QTY; pf++ )
		{
			if ( !strcmp(GetName((PROFILE_TYPE)pf), key) )
			{
				char	z[32];
				char	*p;
	
				strcpy(z, args);
				if ( (p = strchr(z, '.')) == NULL ) goto invalidformat;
				*p = 0;
				p++;
				char	*p1;

				if ( (p1 = strchr(p, '.')) == NULL ) goto invalidformat;
				*p1 = 0;
				p1++;

				m_AvgTimes[pf].m_Time = ((LONGLONG)atoi(z) * llTimeProfileFrequency) + atoi(p);
				m_AvgTimes[pf].m_iCount = atoi(p1);
				break;
invalidformat:
				g_Log.Event(LOGL_ERROR|LOGM_INIT, "Invalid format '%s' for profile type '%s'" DEBUG_CR, args, key);
				return false;
			}
		}
		if ( pf == PROFILE_QTY )
		{
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Undefined profile type '%s'" DEBUG_CR, key);
			return false;
		}
	}
	return true;
}

bool CProfileData::r_Write(CScript &s)
{
	s.WriteSection("PROFILES");

	int pf;
	for ( pf = 0; pf < PROFILE_QTY; pf++ )
	{
		s.WriteKeyFormat(GetName((PROFILE_TYPE)pf), "%i.%04i.%i",
			(int) ( m_AvgTimes[pf].m_Time / (llTimeProfileFrequency)),
			(int) ((( m_AvgTimes[pf].m_Time * 10000 ) / (llTimeProfileFrequency)) % 10000 ),
			m_AvgTimes[pf].m_iCount);
	}
	return true;
}
