//
// CFile.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graycom.h"

#ifndef _WIN32
#include <errno.h>	// errno
extern int errno;
#endif

int CGFile::sm_iFilesOpen = 0;	// debug statistical info

bool CFile::SetFilePath( LPCTSTR pszName )
{
	if ( pszName == NULL )
		return false;
	if ( ! m_strFileName.CompareNoCase( pszName ))
		return( true );
	bool fIsOpen = ( m_hFile != HFILE_ERROR );
	if ( fIsOpen )
	{
		Close();
	}
	m_strFileName = pszName;
	if ( fIsOpen )
	{
		return( Open( NULL, OF_READ|OF_BINARY )); // GetMode()	// open it back up. (in same mode as before)
	}
	return( true );
}

LPCTSTR CFile::GetFileTitle() const
{
	return( CGFile::GetFilesTitle( GetFilePath()));
}

//***************************************************************************
// -CGFile

int CGFile::GetLastError()	// static
{
#ifdef _WIN32
	return ::GetLastError();
#else
	return errno;
#endif
}

void CGFile::GetStrippedDirName( TCHAR * pszFilePath ) // static
{
	// Remove the file name from this and just leave the path.
	// leave the trailing /
	int len = strlen( pszFilePath );
	while ( len > 0 )
	{
		if ( pszFilePath[len] == ':' )
			break;
		if ( pszFilePath[len] == '\\' ||
			pszFilePath[len] == '/' ) // Might be LINUX
		{
			break;
		}
		pszFilePath[len] = '\0';
		len --;
	}
}

CGString CGFile::GetMergedFileName( LPCTSTR pszBase, LPCTSTR pszName ) // static
{
	// Merge path and file name.

	TCHAR szFilePath[ _MAX_PATH ];
	if ( pszBase && pszBase[0] )
	{
		strcpy( szFilePath, pszBase );
		int len = strlen( szFilePath );
		if ( len && szFilePath[ len-1 ] != '\\' &&
			szFilePath[ len-1 ] != '/' )	// Might be LINUX
		{
			strcat( szFilePath, "\\" );
		}
	}
	else
	{
		szFilePath[0] = '\0';
	}
	if ( pszName )
	{
		strcat( szFilePath, pszName );
	}
	return( (CGString) szFilePath );
}

LPCTSTR CGFile::GetFilesTitle( LPCTSTR pszPath )	// static
{
	// Just use COMMDLG.H GetFileTitleA(LPCSTR, LPSTR, WORD) instead ?
	// strrchr
	int len = strlen(pszPath);
	while ( len>0 )
	{
		len--;
		if ( pszPath[len] == '\\' || pszPath[len] == '/' )
		{
			len++;
			break;
		}
	}
	return( pszPath + len );
}

LPCTSTR CGFile::GetFilesExt( LPCTSTR pszName )	// static
{
	// get the EXTension including the .
	int lenall = strlen( pszName );
	int len = lenall;
	while ( len>0 )
	{
		len--;
		if ( pszName[len] == '\\' || pszName[len] == '/' )
			break;
		if ( pszName[len] == '.' )
		{
			return( pszName + len );
		}
	}
	return( NULL );	// has no ext.
}

LPCTSTR CGFile::GetFileExt() const
{
	// get the EXTension including the .
	return( GetFilesExt( GetFilePath()));
}

bool CGFile::OpenBase( void * pExtra )
{
	UNREFERENCED_PARAMETER(pExtra);
#ifdef _WIN32
	OFSTRUCT ofs;
	m_hFile = ::OpenFile( GetFilePath(), &ofs, GetMode());
#else
	m_hFile = open( GetFilePath(), GetMode());
#endif
	return( IsFileOpen());
}

void CGFile::CloseBase()
{
	CFile::Close();
}

bool CGFile::Open( LPCTSTR pszFilename, UINT uModeFlags, void FAR * pExtra )
{
	// RETURN: true = success.
	// OF_BINARY | OF_WRITE
	if ( pszFilename == NULL )
	{
		if ( IsFileOpen())
			return( true );
	}
	else
	{
		Close();	// Make sure it's closed first.
	}

	if ( pszFilename == NULL )
		pszFilename = GetFilePath();
	else
		m_strFileName = pszFilename;

	if ( m_strFileName.IsEmpty())
		return( false );

	m_uMode = uModeFlags;
	if ( ! OpenBase( pExtra ))
		return( false );

	sm_iFilesOpen++;
	ASSERT(sm_iFilesOpen>=0);
	return( true );
}

void CGFile::Close()
{
	if ( ! IsFileOpen())
		return;

	sm_iFilesOpen--;
	ASSERT(sm_iFilesOpen>=0);

	CloseBase();
	m_hFile = HFILE_ERROR;
}

bool CGFile::CopyFileTo( LPCTSTR pszDstFileName )
{
	if ( ! IsFileOpen())
	{
		if ( ! Open())
			return( false );
	}
	else
	{
		SeekToBegin();
	}

	CGFile sDst;
	if ( ! sDst.Open( pszDstFileName, OF_WRITE|OF_CREATE|OF_BINARY ))
	{
		return( false );
	}

	BYTE * pData = new BYTE [ 32 * 1024 ];	// temporary buffer.
	ASSERT(pData);
	bool fSuccess = true;

	while ( true )
	{
		size_t iSize = Read( pData, 32 * 1024 );
		if ( iSize < 0 )
		{
			fSuccess = false;
			break;
		}
		if ( iSize == 0 )
			break;
		sDst.Write( pData, iSize );
		if ( iSize < 32 * 1024 )
			break;
	}

	delete [] pData;
	return( fSuccess );
}

//***************************************************************************
// -CFileText

LPCTSTR CFileText::GetModeStr() const
{
	// end of line translation is crap. ftell and fseek don't work correctly when you use it.
	// fopen() args
	if ( IsBinaryMode())
		return ( IsWriteMode()) ? "wb" : "rb";
	if ( GetMode() & OF_READWRITE )
		return "a+b";
	if ( GetMode() & OF_CREATE )
		return "w";
	if ( IsWriteMode() )
		return "w";
	else
		return "rb";	// don't parse out the \n\r
}

void CFileText::CloseBase()
{
	if ( IsWriteMode())
	{
		fflush(m_pStream);
	}
	bool fSuccess = ( fclose( m_pStream ) == 0 );
	DEBUG_CHECK( fSuccess );
	m_pStream = NULL;
}

bool CFileText::OpenBase( void FAR * pszExtra )
{
	// Open a file.
	m_pStream = fopen( GetFilePath(), GetModeStr());
	if ( m_pStream == NULL )
	{
		return( false );
	}
	// Get the low level handle for it.
	m_hFile = (HFILE) fileno(m_pStream);
	return( true );
}

//
// CGString file support
//

int CGString::ReadZ( CFile * pFile, int iLenMax )
{
	//@------------------------------------------------------------------------
	// PURPOSE:
	//  Read in a new string from an open MMSYSTEM file.
	// ARGS:
	//  hmmio = the open file.
	//  iLen = The length of the string to read. NOT THE NULL !
	// RETURN:
	//  <= 0 = error or no valid string.
	//  length of the string.
	//@------------------------------------------------------------------------

	if ( ! SetLength( iLenMax ))
		return( -1 );

	if ( pFile->Read( m_pchData, iLenMax ) != (DWORD) iLenMax )
		return( -1 );

	//
	// Make sure it is null terminated.
	//
	m_pchData[ iLenMax ] = '\0';
	return( iLenMax );
}

bool CGString::WriteZ( CFile * pFile )
{
	//@------------------------------------------------------------------------
	// PURPOSE:
	//  Write a string AND NULL out to the file.
	// ARGS:
	//  hmmio = the open file.
	// NOTE:
	//  Standard RIFF strings are NULL terminated !
	// RETURN:
	//  string length. -1 = error.
	//@------------------------------------------------------------------------

	if ( ! pFile->Write( m_pchData, GetLength()+1 ))
		return( false );
	return( true );
}

int CGString::ReadFrom( CFile * pFile, int iLenMax )
{
	//@------------------------------------------------------------------------
	// PURPOSE:
	//  Read in a new string from an open MMSYSTEM file.
	// ARGS:
	//  hmmio = the open file.
	//  iLen = The length of the string to read. NOT THE NULL !
	// RETURN:
	//  <= 0 = error or no valid string.
	//  length of the string.
	//@------------------------------------------------------------------------

	WORD wLength = 0;
	if ( pFile->Read( &wLength, sizeof(WORD) ) != sizeof(WORD) )
		return( -1 );

	TCHAR * pData = GetBuffer(wLength+1);
	ASSERT(pData);

	if ( pFile->Read( pData, wLength ) != wLength )
		return( -1 );

	//
	// Make sure it is null terminated.
	//
	pData[ wLength ] = '\0';
	ReleaseBuffer();
	return( wLength );
}

bool CGString::WriteTo( CFile * pFile ) const
{
	//@------------------------------------------------------------------------
	// PURPOSE:
	//  Write a string AND NULL out to the file.
	// ARGS:
	//  hmmio = the open file.
	// NOTE:
	//  This is NOT NULL term. tho Standard RIFF strings are!
	// RETURN:
	//  string length. -1 = error.
	//@------------------------------------------------------------------------

	WORD wLength = GetLength();
	pFile->Write( &wLength, 2 );
	pFile->Write( m_pchData, wLength );
	return( true );
}


