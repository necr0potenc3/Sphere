//
// CDATA.H
// Copyright Menace Software (www.menasoft.com).
//

#ifndef _INC_CDATA_H
#define _INC_CDATA_H

#include "CFile.h"
#include "CArray.h"

typedef BYTE * P_DATA;
#ifndef _WIN32
#define LONG long
#define DWORD unsigned long
#define WORD unsigned short
#define BOOL long
#define TRUE 1
#define FALSE 0
#endif

class CDataObject;

class CDataBuffer : public CGObListRec
{
	// Track a single block of data.
	// 7*4 = max size.
	// It may be mirrored in the file or not.

private:
	// 0 length is possible. some sort of house cleaning should dispose of them eventually.
	DWORD 	m_dwDataLength; 	// The length of the valid data in bytes

	// If the data has been loaded and locked into memory. here is the pointer.
	P_DATA 	m_pData;			// NULL = not in memory. Need to load it

	// < 0 If m_pData is loaded this is the LRU Mark. bit 31 is always set.
	long 	m_lUsage;			// How many current Locks to this data.

public:
	// The current location in an existing file. If there is an existing file.
	LONG 	m_lFileOffset;		// Where does this data reside on disk. -1 = its changed from file
#define CDATA_FILE_NOTSAVED	-1	// Data does not exist in file.

private:
	void Free();
	CDataObject * GetObj() const { return( (CDataObject *) GetParent()); }

protected:
	const void * GetTopPtr() const { return( this ); }

public:
	CDataBuffer( DWORD dwDataLength = 0, LONG lFileOffset = CDATA_FILE_NOTSAVED );
	~CDataBuffer()
	{
		Free();
	}

	BOOL ReSize( DWORD dwLength );	// resize the existing buffer to this.
	BOOL Write( LONG lFileOffset );	// write from memory if changed
	void Discard();	// Don't need this data in memory.

	P_DATA Lock( BOOL bFill = TRUE );	// read the data into memory if not already here.
	void Unlock();			// Don't need this data in memory.

	void Changed()       		// Indicate the object in memory has been changed.
	{
		m_lFileOffset = CDATA_FILE_NOTSAVED;	// No longer at this offset.
	}
	DWORD GetLength() const
	{
		return( m_dwDataLength );
	}
	P_DATA GetData() const
	{
		return( m_pData );
	}
	long GetUsage() const
	{
		return( m_lUsage );
	}
};

class CDataObject : public CGObList
{
	// A logical object of data in a file.
	// In which we may view, insert, delete or change.
	//
	friend CDataBuffer;

	//
	// The memory image.
	//
private:
	DWORD   	m_dwDataLength;		// The total length of the valid data in bytes in memory as changed.
	DWORD		m_dwDataLoaded;		// How much has been loaded to memory.
	DWORD 		m_dwDataAlign;		// Make sure data buffer size is aligned to this.

	// Cache marks.
	LONG		m_lCacheUsage;		// The LRU value for cache usage.
	LONG		m_lCacheMinUsage;	// Min buffers allowed to live.

	//
	// The associated file if any. CAN BE NULL !
	//
protected:
	CGFile * 	m_pFile;			// Pointer to the file we are representing.
	LONG 		m_lFileOffset;		// The starting offset of the logical object data in the file.
	DWORD   	m_dwFileLength;		// The total length of the valid data in bytes in the file.

	//
	// Keep a current access pointer for speed.
	//
private:
	CDataBuffer * 	m_pCurBuffer;	// NULL = invalid.
	DWORD 			m_dwCurOffset;	// Offset into the whole sound.

private:
	BOOL WriteEndData();

protected:
	BOOL CreateBuffer( DWORD dwTotalLength, LONG lFileOffset );
	void DeleteBuffer();
	BOOL SplitBuffer( DWORD dwNewSize );
	BOOL SplitBufferNext( DWORD dwNewSize );

	void Init( BOOL fFull = FALSE );
	void InitCurBuffer();
	BOOL AdvanceCurBuffer();
	void PreviousCurBuffer();
	DWORD FindCurBufferOffset( DWORD dwOffset );

	BOOL CreateBuffers( DWORD dwLength, LONG lFileOffset );
	BOOL DeleteBuffers( DWORD dwLength );

public:
	CDataObject( CGFile * pFile, LONG lFileOffset, DWORD dwFileLength );
	CDataObject();
	void Empty();
	~CDataObject()
	{
		Empty();
	}

	BOOL SetupFile( CGFile * pFile, LONG lFileOffset, DWORD dwFileLength );
	void SetAlign( DWORD dwDataAlign )
	{
		m_dwDataAlign = dwDataAlign;
	}

	BOOL Write();		// Commit all changes to the file.

	// get a data pointer from the object
	P_DATA Lock( DWORD dwOffset, DWORD * pdwSizeAvail );	// get a data pointer at offset.
	void Unlock( DWORD dwOffset, DWORD dwLengthChange = 0 ); // Done with this. (invalidate file?)
	void Change( DWORD dwOffset, DWORD dwLengthChange ); // invalidate file?

	// void ReplaceTake( DWORD dwOffset, DWORD dwSize, P_DATA pData );
	BOOL ReplaceCopy( DWORD dwOffset, DWORD dwSize, P_DATA pData, DWORD dwSizeData );
	BOOL Insert( DWORD dwOffset, DWORD dwSize ) // Insert Blank or don't care block.
	{
		return( ReplaceCopy( dwOffset, 0, NULL, dwSize ));
	}
	BOOL Delete( DWORD dwOffset, DWORD dwSize )
	{
		return( ReplaceCopy( dwOffset, dwSize, NULL, 0 ));
	}
	DWORD GetLength() const
	{
		return( m_dwDataLength );
	}
};

class CDataFileObject : public CGFile	// A single data object in a file.
{
public:
	CDataObject m_Data;
	DWORD m_dwFileLength;
public:
	CDataFileObject()
	{
		m_dwFileLength = 0;
	}

	DWORD GetLength() const
	{
		return m_dwFileLength;
	}

	bool Open( LPCSTR pszName = NULL )
	{
		// Bind the file and data.
		if ( ! CGFile::Open( pszName ))
			return( false );
		m_Data.SetupFile( this, 0, m_dwFileLength = SeekToEnd() );
		return( true );
	}

	void Close()
	{
		m_Data.Empty();
		CGFile::Close();
	}
};

#endif // _INC_CDATA_H
