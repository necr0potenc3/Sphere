//
// CScript.cpp
// Copyright Menace Software (www.menasoft.com).
//
// NOTE: all scripts should be encoded in UTF-8.
// So they may have full unicode chars inside.

#include "../graysvr/graysvr.h"

///////////////////////////////////////////////////////////////
// -CScriptKey

void CScriptKey::InitKey()
{
	m_pszArg = m_pszKey = NULL;
}

TCHAR * CScriptKey::GetArgStr( bool * fQuoted )	// this could be a quoted string ?
{
	ASSERT(m_pszKey);

	TCHAR * pStr = GetArgRaw();
	if ( *pStr != '"' )
		return( pStr );

	pStr++;
	//TCHAR * pEnd = strchr( pStr, '"' );
	// search for last qoute sybol starting from the end
	for (TCHAR * pEnd = pStr + strlen( pStr ) - 1; pEnd >= pStr; pEnd-- )
		if ( *pEnd == '"' )
		{
			*pEnd = '\0';
			if ( fQuoted )
				*fQuoted = true;
			break;
		}

	return( pStr );
}

DWORD CScriptKey::GetArgFlag( DWORD dwStart, DWORD dwMask )
{
	// No args = toggle the flag.
	// 1 = set the flag.
	// 0 = clear the flag.

	ASSERT(m_pszKey);
	ASSERT(m_pszArg);

	if ( ! HasArgs())
		return( dwStart ^ dwMask );

	else if ( GetArgVal())
		return( dwStart | dwMask );

	else
		return( dwStart &~ dwMask );
}

long CScriptKey::GetArgVal()
{
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return( Exp_GetVal( m_pszArg ));
}

long CScriptKey::GetArgRange()
{
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return( Exp_GetRange( m_pszArg ));
}

///////////////////////////////////////////////////////////////
// -CScriptKeyAlloc

TCHAR * CScriptKeyAlloc::GetKeyBufferRaw( int iLen )
{
	// iLen = length of the string we want to hold.

	ASSERT( iLen >= 0 );

	if ( iLen > SCRIPT_MAX_LINE_LEN )
		iLen = SCRIPT_MAX_LINE_LEN;
	iLen ++;	// add null.

	if ( m_Mem.GetDataLength() < iLen )
	{
		m_Mem.Alloc( iLen );
	}

	m_pszKey = m_pszArg = GetKeyBuffer();
	m_pszKey[0] = '\0';

	return m_pszKey;
}

bool CScriptKeyAlloc::ParseKey( LPCTSTR pszKey )
{
	// Skip leading white space 
	if ( ! pszKey )
	{
		GetKeyBufferRaw(0);
		return false;
	}

	GETNONWHITESPACE( pszKey );

	TCHAR * pBuffer = GetKeyBufferRaw( strlen( pszKey ));
	ASSERT(pBuffer);

	int iLen = m_Mem.GetDataLength()-1;
	strncpy( pBuffer, pszKey, iLen );
	pBuffer[iLen] = '\0';

	Str_Parse( pBuffer, &m_pszArg, "=, \t()[]{}" );
	return( true );
}

bool CScriptKeyAlloc::ParseKey( LPCTSTR pszKey, LPCTSTR pszVal )
{
	ASSERT(pszKey);

	int lenkey = strlen( pszKey );
	if ( ! lenkey )
	{
		return ParseKey(pszVal);
	}

	ASSERT( lenkey < SCRIPT_MAX_LINE_LEN-2 );

	int lenval = 0;
	if ( pszVal )
	{
		lenval = strlen( pszVal );
	}

	m_pszKey = GetKeyBufferRaw( lenkey + lenval + 1 );

	strcpy( m_pszKey, pszKey );
	m_pszArg = m_pszKey + lenkey;

	if ( pszVal )
	{
		m_pszArg ++;
		lenval = m_Mem.GetDataLength()-2;
		strcpylen( m_pszArg, pszVal, ( lenval - lenkey ) + 1 );	// strcpylen
	}

	return( true );
}

int CScriptKeyAlloc::ParseKeyEnd()
{
	// Now parse the line for comments and trailing whitespace junk
	// NOTE: leave leading whitespace for now.

	ASSERT(m_pszKey);

	int len = 0;
	for ( ; len<SCRIPT_MAX_LINE_LEN; len++ )
	{
		TCHAR ch = m_pszKey[len];
		if ( ch == '\0' )
			break;
		if ( ch == '/' && m_pszKey[len+1] == '/' )
		{
			// Remove comment at end of line.
			break;
		}
	}

	// Remove CR and LF from the end of the line.
	len = Str_TrimEndWhitespace( m_pszKey, len );
	if ( len <= 0 )		// fRemoveBlanks &&
		return 0;

	m_pszKey[len] = '\0';
	return( len );
}

void CScriptKeyAlloc::ParseKeyLate()
{
	ASSERT(m_pszKey);
	ParseKeyEnd();
	GETNONWHITESPACE( m_pszKey );
	Str_Parse( m_pszKey, &m_pszArg );
}

///////////////////////////////////////////////////////////////
// -CScript

CScript::CScript()
{
	InitBase();
}

CScript::CScript( LPCTSTR pszKey )
{
	InitBase();
	ParseKey(pszKey);
}

CScript::CScript( LPCTSTR pszKey, LPCTSTR pszVal )
{
	InitBase();
	ParseKey( pszKey, pszVal );
}

void CScript::InitBase()
{
	m_iLineNum		= 0;
	m_fSectionHead	= false;
	m_lSectionData	= 0;
	InitKey();
}

bool CScript::Open( LPCTSTR pszFilename, UINT wFlags )
{
	// If we are in read mode and we have no script file.
	// ARGS: wFlags = OF_READ, OF_NONCRIT etc
	// RETURN: true = success.

	InitBase();

	if ( pszFilename == NULL )
	{
		pszFilename = GetFilePath();
	}
	else
	{
		SetFilePath( pszFilename );
	}

	LPCTSTR pszTitle = GetFileTitle();
	if ( pszTitle == NULL || pszTitle[0] == '\0' )
		return( false );

	LPCTSTR pszExt = GetFilesExt( GetFilePath() ); 
	if ( pszExt == NULL )
	{
		TCHAR szTemp[ _MAX_PATH ];
		strcpy( szTemp, GetFilePath() );
		strcat( szTemp, GRAY_SCRIPT );
		SetFilePath( szTemp );
		wFlags |= OF_TEXT;
	}

	if ( ! CFileText::Open( GetFilePath(), wFlags ))
	{
		if ( ! ( wFlags & OF_NONCRIT ))
		{
			g_pLog->Event( LOGL_WARN, "'%s' not found...\n", (LPCTSTR) GetFilePath() );
		}
		return( false );
	}

	return( true );
}

bool CScript::ReadTextLine( bool fRemoveBlanks ) // Read a line from the opened script file
{
	// ARGS:
	// fRemoveBlanks = Don't report any blank lines, (just keep reading)
	//

	ASSERT( ! IsBinaryMode());

	while ( CFileText::ReadString( GetKeyBufferRaw(SCRIPT_MAX_LINE_LEN), SCRIPT_MAX_LINE_LEN ))
	{
		m_iLineNum++;
		if ( fRemoveBlanks )
		{
			if ( ParseKeyEnd() <= 0 )
				continue;
		}
		return( true );
	}

	m_pszKey[0] = '\0';
	return( false );
}

bool CScript::FindTextHeader( LPCTSTR pszName ) // Find a section in the current script
{
	// RETURN: false = EOF reached.
	ASSERT(pszName);
	ASSERT( ! IsBinaryMode());

	SeekToBegin();

	int len = strlen( pszName );
	ASSERT(len);
	do
	{
		if ( ! ReadTextLine(false))
		{
			return( false );
		}
		if ( IsKeyHead( "[EOF]", 5 ))
		{
			return( false );
		}
	}
	while ( ! IsKeyHead( pszName, len ));
	return( true );
}

LONG CScript::Seek( long offset, UINT origin )
{
	// Go to the start of a new section.
	// RETURN: the new offset in bytes from start of file.
	if ( offset == 0 && origin == SEEK_SET )
	{
		m_iLineNum = 0;	// so we don't have to override SeekToBegin
	}
	m_fSectionHead = false;		// unknown , so start at the beginning.
	m_lSectionData = offset;
	return( CFileText::Seek(offset,origin));
}

bool CScript::FindNextSection()
{
	EXC_TRY(("FindNextSection"));
	// RETURN: false = EOF.

	if ( m_fSectionHead )	// we have read a section already., (not at the start)
	{
		// Start from the previous line. It was the line that ended the last read.
		m_pszKey = GetKeyBuffer();
		ASSERT(m_pszKey);
		m_fSectionHead = false;
		if ( m_pszKey[0] == '[' )
			goto foundit;
	}

	while (true)
	{
		if ( ! ReadTextLine(true))
		{
			m_lSectionData = GetPosition();
			return( false );
		}
		if ( m_pszKey[0] == '[' )
			break;
	}

foundit:
	// Parse up the section name.
	m_pszKey++;
	int len = strlen( m_pszKey );
	for ( int i=0; i<len; i++ )
	{
		if ( m_pszKey[i] == ']' )
		{
			m_pszKey[i] = '\0';
			break;
		}
	}

	m_lSectionData = GetPosition();
	if ( IsSectionType( "EOF" ))
		return( false );

	Str_Parse( m_pszKey, &m_pszArg );
	return true;
	EXC_CATCH("CScript");
	return false;
}

bool CScript::FindSection( LPCTSTR pszName, UINT uModeFlags )
{
	// Find a section in the current script
	// RETURN: true = success

	ASSERT(pszName);
	if ( strlen( pszName ) > 32 )
	{
		DEBUG_ERR(( "Bad script section name\n" ));
		return( false );
	}

	TCHAR	*pszSec = Str_GetTemp();
	sprintf(pszSec, "[%s]", pszName);
	if ( FindTextHeader(pszSec))
	{
		// Success
		m_lSectionData = GetPosition();
		return( true );
	}

	// Failure Error display. (default)

	if ( ! ( uModeFlags & OF_NONCRIT ))
	{
		g_pLog->Event( LOGL_WARN, "Did not find '%s' section '%s'\n", (LPCTSTR) GetFileTitle(), (LPCTSTR) pszName );
	}
	return( false );
}

bool CScript::ReadKey( bool fRemoveBlanks )
{
	if ( ! ReadTextLine(fRemoveBlanks))
		return( false );
	if ( m_pszKey[0] == '[' )	// hit the end of our section.
	{
		m_fSectionHead = true;
		return( false );
	}
	return( true );
}

bool CScript::ReadKeyParse() // Read line from script
{
	static LPCTSTR const m_ExcKeys[] =
	{
		"key reading",
		"key initializing",
		"parsing",
	};

	EXC_TRY(("ReadKeyParse()"));
	EXC_SET(m_ExcKeys[0]);
	if ( ! ReadKey(true))
	{
		EXC_SET(m_ExcKeys[1]);
		InitKey();
		return( false );	// end of section.
	}

	ASSERT(m_pszKey);
	GETNONWHITESPACE( m_pszKey );
	EXC_SET(m_ExcKeys[2]);
	Str_Parse( m_pszKey, &m_pszArg );

	if ( !m_pszArg[0] || m_pszArg[1] != '=' || !strchr( ".*+-/%|&!", m_pszArg[0] ) )
		return true;

	EXC_SET(m_ExcKeys[2]);
	LPCTSTR	pszArgs	= m_pszArg;
	pszArgs+=2;
	GETNONWHITESPACE( pszArgs );
	TCHAR	*buf = Str_GetTemp();
	if (  m_pszArg[0] == '.' )
	{
		if ( *pszArgs == '"' )
		{
			EXC_SET(m_ExcKeys[2]);
			TCHAR *	pQuote	= strchr( pszArgs+1, '"' );
			if ( pQuote )
			{
				pszArgs++;
				*pQuote	= '\0';
			}
		}
		EXC_SET(m_ExcKeys[2]);
		sprintf( buf, "<%s>%s", m_pszKey, pszArgs );
	}
	else sprintf( buf, "<eval (<%s> %c (%s))>", m_pszKey, *m_pszArg, pszArgs );
	strcpy( m_pszArg, buf );

	return true;
	EXC_CATCH("Parsing Key");
	return false;
}

bool CScript::FindKey( LPCTSTR pszName ) // Find a key in the current section
{
	if ( strlen( pszName ) > SCRIPT_MAX_SECTION_LEN )
	{
		DEBUG_ERR(( "Bad script key name\n" ));
		return( false );
	}
	Seek( m_lSectionData );
	while ( ReadKeyParse())
	{
		if ( IsKey( pszName ))
		{
			m_pszArg = Str_TrimWhitespace( m_pszArg );
			return true;
		}
	}
	return( false );
}

bool CScript::WriteProfileStringDst( CScript * pDst, LPCTSTR pszSection, long lSectionOffset, LPCTSTR pszKey, LPCTSTR pszVal )
{
	// pszKey = NULL = delete the whole section.
	// pszVal = NULL = delete the key

	if ( IsBinaryMode())
	{
		DEBUG_ERR(( "WriteProfileStringDst binary mode\n" ));
		return( false );
	}

	int iLenSecName = 0;
	bool fFoundSection = false;
	bool fFoundKey = false;

	SeekToBegin();
	if ( pszSection )
	{
		iLenSecName = strlen( pszSection );
	}

	// Write the header stuff.
	// start writing til we get to the section we want.
	long lOffsetWhiteSpace = 0;
	while ( true )
	{
		if ( CFileText::ReadString( GetKeyBufferRaw(SCRIPT_MAX_LINE_LEN), SCRIPT_MAX_LINE_LEN ) == NULL )
		{
			break;
		}

		m_iLineNum++;
		if ( ! fFoundKey )	// not yet found the section.
		{
			// The section we want.

			if ( m_pszKey[0] == '[' )
			{
				if ( fFoundSection )
				{
					// We are leaving the section of interest.
					if ( ! fFoundKey && pszVal != NULL )
					{
						if ( lOffsetWhiteSpace )	// go back to first whitespace.
						{
							pDst->Seek( lOffsetWhiteSpace, SEEK_SET );
						}
						pDst->Printf( "%s=%s\n", pszKey, pszVal );
						if ( lOffsetWhiteSpace )
						{
							pDst->Printf( "\n" );
						}
					}
					fFoundKey = true;
				}
				else if ( pszSection )
				{
					fFoundSection = ! strnicmp( pszSection, &m_pszKey[1], iLenSecName );
					if ( fFoundSection && pszKey == NULL )
					{
						lOffsetWhiteSpace = 0;
						continue;	// delete whole section.
					}
				}

				lOffsetWhiteSpace = 0;
			}

			else
			{

				if ( ! fFoundSection && lSectionOffset )
				{
					fFoundSection = ( GetPosition() >= lSectionOffset );
				}

				if ( fFoundSection )
				{
					if ( pszKey == NULL )
						continue;	// delete whole section.
					if ( ! strnicmp( pszKey, m_pszKey, strlen( pszKey )))
					{
						fFoundKey = true;
						if ( pszVal == NULL )
							continue;	// just lose this key
						pDst->Printf( "%s=%s\n", pszKey, pszVal );
						continue;	// replace this key.
					}

					// Is this just white space ?
					LPCTSTR pNonWhite = m_pszKey;
					while ( ISWHITESPACE( pNonWhite[0] ))
						pNonWhite ++;
					if ( pNonWhite[0] != '\0' )
					{
						lOffsetWhiteSpace = 0;
					}
					else if ( ! lOffsetWhiteSpace )
					{
						lOffsetWhiteSpace = pDst->GetPosition();
					}
				}
			}
		}

		// Just copy the old stuff.
		if ( ! pDst->WriteString( m_pszKey ))
		{
			DEBUG_ERR(( "Profile Failed to write line\n" ));
			return false;
		}
	}

	if ( ! m_iLineNum )
	{
		// Failed to read anything !
		DEBUG_ERR(( "Profile Failed to read script\n" ));
		return false;
	}

	if ( ! fFoundSection && pszSection && pszVal && pszKey )
	{
		// Create the section ?
		if ( pDst->Printf( "[%s]\n", pszSection ) <= 0 )
			goto cantwrite;

		if ( pDst->Printf( "%s=%s\n", pszKey, pszVal ) <= 0 )
		{
cantwrite:
			DEBUG_ERR(( "Profile Failed to write new line\n" ));
			return false;
		}
	}

	return( true );
}

bool CScript::WriteProfileStringBase( LPCTSTR pszSection, long lSectionOffset, LPCTSTR pszKey, LPCTSTR pszVal )
{
	// Assume the script is open in read mode.
	// Key = NULL = delete the section.
	// Val = NULL = delete the key.
	// RETURN:
	//  true = it worked.
	//

	if ( IsBinaryMode())
	{
		DEBUG_ERR(( "Profile Can't write binary SCP\n" ));
		return( false );
	}

	bool fWasOpen = IsFileOpen();
	if ( ! fWasOpen )
	{
		// Might be creating a new file. not normal !
		if ( ! Open( NULL, OF_READ | OF_TEXT ))
			return( false );
	}
	else
	{
		ASSERT( ! IsWriteMode());
	}

	// Open the write side of the profile.
	CScript s;
	if ( ! s.Open( "tmpout.ini", OF_WRITE | OF_TEXT ))
	{
		DEBUG_ERR(( "Profile Can't open tmp.ini\n" ));
		return( false );
	}

	if ( ! WriteProfileStringDst( &s, pszSection, lSectionOffset, pszKey, pszVal ))
		return( false );

	// Close, rename and re-open the file.
	s.Close();
	Close();
	CloseForce();	// some will linger open !
	if ( remove( GetFilePath()))
	{
		DEBUG_ERR(( "Profile remove fail\n" ));
		return(false);
	}
	if ( rename( s.GetFilePath(), GetFilePath()))
	{
		DEBUG_ERR(( "Profile rename fail\n" ));
		return(false);
	}

	if ( fWasOpen )
	{
		if ( ! Open())
		{
			DEBUG_ERR(( "Profile src reopen fail\n" ));
			return(false);
		}
	}
	return( true );
}

void CScript::Close()
{
	// EndSection();
	CFileText::Close();
}

bool _cdecl CScript::WriteSection( LPCTSTR pszSection, ... )
{
	// Write out the section header.
	va_list vargs;
	va_start( vargs, pszSection );

	// EndSection();	// End any previous section.
	Printf( "\n[");
	VPrintf( pszSection, vargs );
	Printf( "]\n" );
	va_end( vargs );

	return( true );
}

bool CScript::WriteKey( LPCTSTR pszKey, LPCTSTR pszVal )
{
	if ( pszKey == NULL || pszKey[0] == '\0' )
	{
		return false;
	}

	TCHAR		ch;
	TCHAR *		pszSep;
	if ( pszVal == NULL || pszVal[0] == '\0' )
	{
		if ( !( pszSep = strchr( pszKey, '\n' )) )
			pszSep = strchr( pszKey, '\r' );	// acts like const_cast

		if ( pszSep )
		{
			g_Log.Event( LOGL_WARN|LOGM_CHEAT, "carriage return in key (book?) - truncating" );
			ch		= *pszSep;
			*pszSep	= '\0';
		}

		// Books are like this. No real keys.
		Printf( "%s\n", pszKey );

		if ( pszSep )
			*pszSep	= ch;
	}
	else
	{
		if ( !( pszSep = strchr( pszVal, '\n' )) )
			pszSep = strchr( pszVal, '\r' );	// acts like const_cast

		if ( pszSep )
		{
			g_Log.Event( LOGL_WARN|LOGM_CHEAT, "carriage return in key value - truncating" );
			ch		= *pszSep;
			*pszSep	= '\0';
		}
		Printf( "%s=%s\n", pszKey, pszVal );
		if ( pszSep )
			*pszSep	= ch;
	}

	return( true );
}

void _cdecl CScript::WriteKeyFormat( LPCTSTR pszKey, LPCTSTR pszVal, ... )
{
	TCHAR	*pszTemp = Str_GetTemp();
	va_list vargs;
	va_start( vargs, pszVal );
	vsprintf(pszTemp, pszVal, vargs);
	WriteKey(pszKey, pszTemp);
	va_end( vargs );
}


