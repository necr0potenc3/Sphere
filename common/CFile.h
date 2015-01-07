//
// CGFile.h
// Copyright Menace Software (www.menasoft.com).

#ifndef _INC_CFILE_H
#define _INC_CFILE_H
#pragma once

#ifndef _WIN32
#define LONG int
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "CString.h"

#ifndef OF_WRITE
#define OF_READ             O_RDONLY
#define OF_WRITE            O_WRONLY
#define OF_READWRITE        O_RDWR
#define OF_SHARE_DENY_NONE	0x00
#define OF_SHARE_DENY_WRITE	0x00	// not defined in LINUX
#define OF_CREATE		O_CREAT
#endif
#define OF_NONCRIT			0x40000000	// just a test.
#define OF_TEXT				0x20000000
#define OF_BINARY			0x10000000

#ifndef HFILE_ERROR
#define HFILE_ERROR	-1
#define HFILE unsigned int
#endif	// HFILE_ERROR

class CGrayError;
#define CFileException CGrayError

// Dupe the MFC functionality we need
class CFile
{
public:
	HFILE m_hFile;	// HFILE_ERROR
protected:
	CGString m_strFileName;
public:
	// File Name and path.
	const CGString & GetFilePath() const	// for compatibility with MFC
	{
		return( m_strFileName);
	}
	virtual bool SetFilePath( LPCTSTR pszName );

	LPCTSTR GetFileTitle() const;

	// File Open/Close 
	virtual bool Open( LPCTSTR pszName = NULL, UINT uMode = OF_READ | OF_SHARE_DENY_NONE, CFileException * e = NULL )
	{
		ASSERT( m_hFile == HFILE_ERROR );
		SetFilePath( pszName );
#ifdef _WIN32
		OFSTRUCT ofs;
		m_hFile = ::OpenFile( GetFilePath(), &ofs, uMode );
#else
		m_hFile = open( GetFilePath(), uMode );
#endif
		return( m_hFile != HFILE_ERROR );
	}
	virtual void Close()
	{
		if ( m_hFile != HFILE_ERROR )
		{
#ifdef _WIN32
			_lclose( m_hFile );
#else
			close( m_hFile );
#endif
			m_hFile = HFILE_ERROR;
		}
	}

	// File Access
	void SeekToBegin()
	{
		Seek( 0, SEEK_SET );
	}
	DWORD SeekToEnd()
	{
		return( Seek( 0, SEEK_END ));
	}
	DWORD GetLength()
	{
		// Get the size of the file.
		DWORD lPos = GetPosition();	// save current pos.
		DWORD lSize = SeekToEnd();
		Seek( lPos, SEEK_SET );	// restore previous pos.
		return( lSize );
	}

#ifdef _WIN32
	virtual DWORD GetPosition() const
	{
		return( _llseek( m_hFile, 0, SEEK_CUR ));
	}
	virtual LONG Seek( LONG lOffset = 0, UINT iOrigin = SEEK_SET )
	{
		// true = success.
		return( _llseek( m_hFile, lOffset, iOrigin ));
	}
	virtual size_t Read( void * pData, size_t dwLength ) const
	{
		// RETURN: length of the read data.
		return( _hread( m_hFile, pData, (long) dwLength ));
	}
	virtual bool Write( const void * pData, size_t dwLength ) const
	{
		return( _hwrite( m_hFile, (const char *) pData, (long) dwLength ) == (long) dwLength );
	}
#else
	virtual DWORD GetPosition() const
	{
		return( lseek( m_hFile, 0, SEEK_CUR ) );
	}
	virtual LONG Seek( LONG lOffset = 0, UINT iOrigin = SEEK_SET )
	{
		// true = success.
		if ( m_hFile <= 0 )
			return( -1 );
		return( lseek( m_hFile, lOffset, iOrigin ));
	}
	virtual size_t Read( void * pData, size_t dwLength ) const
	{
		// RETURN: length of the read data.
		return( read( m_hFile, pData, (long) dwLength ));
	}
	virtual bool Write( const void * pData, size_t dwLength ) const
	{
		return( write( m_hFile, (const char *) pData, (long) dwLength ) == (long) dwLength );
	}
#endif

	CFile()
	{
		m_hFile = HFILE_ERROR;
	}
	virtual ~CFile()
	{
		Close();
	}
};

class CGFile : public CFile	// try to be compatible with MFC CFile class.
{
private:
	static int sm_iFilesOpen;
	UINT m_uMode;	// MMSYSTEM may use 32 bit flags.
private:
	virtual bool OpenBase( void * pExtra );
	virtual void CloseBase();

public:
	// File Name and Path.

	static LPCTSTR GetFilesTitle( LPCTSTR pszPath );
	static LPCTSTR GetFilesExt( LPCTSTR pszName );
	static CGString GetMergedFileName( LPCTSTR pszBase, LPCTSTR pszName );
	static void GetStrippedDirName( TCHAR * pszFilePath );

	static int GetLastError();

	LPCTSTR GetFileExt() const;

	// File Mode stuff.
	virtual bool IsBinaryMode() const { return true; }
	UINT GetMode() const
	{
		return( m_uMode & 0x0FFFFFFF );	// get rid of OF_NONCRIT type flags
	}
	UINT GetModeFull() const
	{
		// Get the hidden elements of the Mode
		return( m_uMode );
	}
	bool IsWriteMode() const
	{
		return(( m_uMode & OF_WRITE ) ? true : false );
	}

	// File Open/Close 
	virtual bool IsFileOpen() const
	{
		return( m_hFile != HFILE_ERROR );
	}
	virtual bool Open( LPCTSTR pszName = NULL, UINT uMode = OF_READ | OF_SHARE_DENY_NONE, void * pExtra = NULL );
	virtual void Close();

	// File Access
	bool CopyFileTo( LPCTSTR pszDstFileName );

	CGFile()
	{
		m_uMode = 0;
	}
	virtual ~CGFile()
	{
		Close();
	}
};

class CFileText : public CGFile	// Try to be compatible with MFC CStdioFile
{
public:
	FILE * m_pStream;		// the current open script type file.
private:
	LPCTSTR GetModeStr() const;
#ifdef _WIN32
	bool	bNoBuffer;
#endif
protected:
	virtual bool OpenBase( void * pExtra );
	virtual void CloseBase();
public:
	CFileText()
	{
		m_pStream = NULL;
		#ifdef _WIN32
			bNoBuffer = false;
		#endif
	}
	virtual ~CFileText()
	{
		Close();
	}
	virtual LONG Seek( LONG offset = 0, UINT origin = SEEK_SET )
	{
		// true = success
		if ( ! IsFileOpen())
			return( -1 );
		if ( offset < 0 )
			return( -1 );
		if ( fseek( m_pStream, offset, origin ) != 0 )
			return( -1 );
		return ftell(m_pStream);
	}
	void Flush() const
	{
		if ( ! IsFileOpen())
			return;
		ASSERT(m_pStream);
		fflush(m_pStream);
	}
	DWORD GetPosition() const
	{
		// RETURN: -1 = error.
		if ( ! IsFileOpen())
			return (DWORD) -1;
		return( ftell(m_pStream));
	}
	size_t Read( void * pBuffer, size_t sizemax ) const
	{
		// This can return: EOF(-1) constant.
		// returns the number of full items actually read
		ASSERT(pBuffer);
		if ( IsEOF())
			return( 0 );	// LINUX will ASSERT if we read past end.
		return( fread( pBuffer, 1, sizemax, m_pStream ));
	}
	TCHAR * ReadString( TCHAR * pBuffer, size_t sizemax ) const
	{
		// Read a line of text. NULL = EOF
		ASSERT(pBuffer);
		if ( IsEOF())
			return( NULL );	// LINUX will ASSERT if we read past end.
		return( fgets( pBuffer, sizemax, m_pStream ));
	}
	bool Write( const void * pData, size_t iLen )
#ifndef _WIN32
		const
#endif
	{
		// RETURN: 1 = success else fail.
		ASSERT(pData);
		if ( ! IsFileOpen())
			return( false );
#ifdef _WIN32 //	Windows flushing, the only safe mode to cancel it ;)
		if ( !bNoBuffer )
		{
			setvbuf(m_pStream, NULL, _IONBF, 0);
			bNoBuffer = true;
		}
#endif
		int iStatus = fwrite( pData, iLen, 1, m_pStream );
#ifndef _WIN32	// However, in unix, it works
		fflush( m_pStream );
#endif
		return ( iStatus == 1 );
	}
	bool WriteString( LPCTSTR pStr )
	{
		// RETURN: < 0 = failed.
		ASSERT(pStr);
		return( Write( pStr, strlen( pStr )));
	}

	bool IsFileOpen() const
	{
		return( m_pStream != NULL );
	}
	bool IsBinaryMode() const { return false; }
	bool IsEOF() const
	{
		if ( ! IsFileOpen())
			return( true );
		return(( feof( m_pStream )) ? true : false );
	}
	size_t VPrintf( LPCTSTR pFormat, va_list args )
	{
		ASSERT(pFormat);
		if ( ! IsFileOpen())
			return( (size_t) -1 );
		TCHAR *pszTemp = Str_GetTemp();
		size_t len = vsprintf( pszTemp, pFormat, args );
		size_t lenret = fwrite( pszTemp, 1, len, m_pStream );
		if ( lenret != len )
			return( (size_t) -1 );
		return( lenret );
	}
	size_t _cdecl Printf( LPCTSTR pFormat, ... )
	{
		ASSERT(pFormat);
		va_list vargs;
		va_start( vargs, pFormat );
		int iRet = VPrintf( pFormat, vargs );
		va_end( vargs );
		return( iRet );
	}
};

#define CFileBin CGFile

#endif // _INC_CFILE_H
