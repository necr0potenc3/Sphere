//
// CDATA.CPP
// Menace Software	9/1/97
//
// Store an object of binary data.
// In which we may view, insert, delete or change data in memory or file
// This may be sound or other file data.
//

#ifdef _WIN32
#include <windows.h>
#endif
#include <assert.h>
#include <stdio.h>
#include "common.h"
#include "CString.h"

#ifdef _WIN32
#include <windowsx.h>
#endif
#include "CData.h"

//***************************************************************************
// CDataBuffer
//

CDataBuffer::CDataBuffer( DWORD dwDataLength, LONG lFileOffset )
{
	// constructor.
	m_lFileOffset = lFileOffset;
	m_dwDataLength = dwDataLength;
	m_pData = NULL;		// not yet allocated. (allocate on demand)
	m_lUsage = 0;		// Not used.
}

void CDataBuffer::Free()
{
	if ( m_pData != NULL )
	{
		assert( m_lUsage < 0 );
#ifdef _WIN32
		GlobalFreePtr( m_pData );
#else
		free( m_pData );
#endif
		m_pData = NULL;
		m_lUsage = 0;
		GetObj()->m_dwDataLoaded -= m_dwDataLength;
	}
	else
	{
		assert( m_lUsage == 0 );
	}
}

void CDataBuffer::Discard()
{
	// Don't need this data in memory. Discard it.
	if ( m_lUsage > 0 || m_lFileOffset == CDATA_FILE_NOTSAVED )
	{
		// DEBUG_ERR(( "Buffer:Discard usage = %d BAD\n", m_lUsage ));
		return;
	}
	Free();
}

P_DATA CDataBuffer::Lock( BOOL bFill )
{
	// PURPOSE:
	//  read the data into memory if not already here.
	// ARGS:
	//  bFill = do I care what data is here ? (I may just be about to overwrite it anyhow)
	//

	if ( this == NULL )
	{
		DEBUG_ERR(( "Buf:Lock NULL\n" ));
		return( NULL );
	}

	if ( m_pData == NULL && m_dwDataLength )
	{
		assert( m_lUsage == 0 );

#ifdef WIN32
		m_pData = (P_DATA) GlobalAllocPtr( GHND, m_dwDataLength );
#else
		m_pData = (P_DATA) malloc(m_dwDataLength);
#endif
		if ( m_pData == NULL )
		{
#ifdef WIN32
			DEBUG_ERR(( "Buffer:Lock GlobalAllocPtr( %ld ) FAIL\n", m_dwDataLength ));
#else
			DEBUG_ERR(( "Buffer:Lock malloc( %ld ) FAIL\n", m_dwDataLength ));
#endif
			return( NULL );
		}

		//
		// Now read it in if it exists as part of the file.
		//
		if ( bFill )
		{
			if ( m_lFileOffset != CDATA_FILE_NOTSAVED )
			{
				if ( GetObj()->m_pFile->Seek( m_lFileOffset, SEEK_SET ) != m_lFileOffset )
				{
					DEBUG_ERR(( "Buffer:Lock Seek( %ld ) FAIL\n", m_lFileOffset ));
					return( NULL );
				}
				if ( GetObj()->m_pFile->Read( m_pData, m_dwDataLength ) <= 0 )
				{
					DEBUG_ERR(( "Buffer:Lock Read( %ld ) FAIL\n", m_dwDataLength ));
					return( NULL );
				}
			}
			else
			{
				//
				// This is just undefined memory space ? Zero it ?
				//
			}
		}

		GetObj()->m_dwDataLoaded += m_dwDataLength;
	}
	else if ( m_lUsage < 0 )
	{
		m_lUsage = 0;
	}

	m_lUsage ++;
	return( m_pData );
}

void CDataBuffer::Unlock()
{
	//
	// Mark the data as unused.
	// Discard the data if nobody is using it ?
	//

	if ( m_lUsage <= 0 )
	{
		DEBUG_ERR(( "Buffer:Unlock count %d BAD\n", m_lUsage ));
		return;
	}

	if ( m_lUsage > 32 )
	{
		DEBUG_ERR(( "Buffer:Warning HIGH count %d\n", m_lUsage ));
	}

	if ( --m_lUsage == 0 )
	{
		if ( m_pData != NULL )
		{
			// Set the LRU Mark
			m_lUsage = GetObj()->m_lCacheUsage;
		}
	}
}

BOOL CDataBuffer::Write( LONG lFileOffset )
{
	// write from memory if changed
	if ( lFileOffset == m_lFileOffset ) return( TRUE ); 	// No need to write already here !

	//
	// Lock the data if not already loaded.
	// This could be a move from high file offset to low file offset ?
	//
	if ( ! Lock()) return( FALSE );

	if ( GetObj()->m_pFile->Seek( lFileOffset, SEEK_SET ) != lFileOffset )
	{
	bailout:
		DEBUG_ERR(( "Buffer:Write Seek( %ld ) FAIL\n", lFileOffset ));
		Unlock();	// Done with the data.
		return( FALSE );
	}

	if ( ! GetObj()->m_pFile->Write( m_pData, m_dwDataLength ))
		goto bailout;

	Unlock();						// Done with the data.
	m_lFileOffset = lFileOffset;	// commited to file.
	return( TRUE );
}

BOOL CDataBuffer::ReSize( DWORD dwLength )
{
	// PURPOSE:
	//  Resize the existing buffer to this size.
	//  Create a new buffer after this for the rest of the data.
	// NOTE:
	//  Buffers should never be made bigger only smaller !!!
	//  This does not invalidate the object.
	//  This is best if the data is swapped out !
	// RETURN:
	//  TRUE = OK.

	if ( dwLength >= m_dwDataLength )
	{
		if ( dwLength == m_dwDataLength )	// This means no change.
			return( TRUE );
		DEBUG_ERR(( "Buffer:ReSize new %ld > old %ld\n", dwLength, m_dwDataLength ));
		return( FALSE );
	}

	if ( m_lUsage > 0 )
	{
		DEBUG_ERR(( "Buffer:ReSize usage=%d\n", m_lUsage ));
	}

	if ( m_pData != NULL )
	{
		// The data is in memory. Too bad now we have to move it.

#ifdef _WIN32
		m_pData = (P_DATA) GlobalReAllocPtr( m_pData, dwLength, GHND );
#else
		m_pData = (P_DATA) realloc( m_pData, dwLength );
#endif
		if ( m_pData == NULL )
		{
#ifdef _WIN32
			DEBUG_ERR(( "Buffer:ReSize GlobalReallocPtr( %ld, %ld ) FAIL\n", dwLength, m_dwDataLength ));
#else
			DEBUG_ERR(( "Buffer:ReSize realloc( %ld, %ld ) FAIL\n", dwLength, m_dwDataLength ));
#endif
			return( FALSE );
		}
	}

	m_dwDataLength = dwLength;
	return( TRUE );
}

//***************************************************************************
// CDataObject

void CDataObject::Init( BOOL fFull )
{
	m_dwDataLength = 0;
	m_dwDataLoaded = 0;

	if ( fFull )
	{
		m_dwDataAlign = 0x4000;	// default size = 16K

		m_pFile = NULL;
		m_lFileOffset = CDATA_FILE_NOTSAVED;
		m_dwFileLength = 0;
	}

	m_lCacheUsage = -1;
	m_lCacheMinUsage = -1;

	InitCurBuffer();
}

CDataObject::CDataObject()
{
	//
	// PURPOSE:
	//  Create a new data object with no file !
	//
	Init( TRUE );
}

void CDataObject::Empty()
{
	// assume we asked the user to save any changes !
	//
	// Destroy the list of objects.
	//
	CGObList::DeleteAll();

	//
	// Unlink ourselves from the file.
	//
	Init( FALSE );	// clear values.
}

BOOL CDataObject::SetupFile( CGFile * pFile, LONG lFileOffset, DWORD dwFileLength )
{
	//
	// Associate the data object with a (open) file.
	//
	if ( m_pFile != NULL )
	{
		DeleteAll();
	}

	m_pFile = pFile;
	m_lFileOffset = lFileOffset;
	m_dwFileLength = dwFileLength;

	//
	// Init the list of buffers. Use the default chunk size.
	//
	return( CreateBuffers( dwFileLength, lFileOffset ));
}

CDataObject::CDataObject( CGFile * pFile, LONG lFileOffset, DWORD dwFileLength )
{
	//
	// PURPOSE:
	//  Create the data object.
	//
	Init( TRUE );	// clear values.
	SetupFile( pFile, lFileOffset, dwFileLength );
}

BOOL CDataObject::CreateBuffer( DWORD dwDataLength, LONG lFileOffset )
{
	// Insert data space into the object.
	// It may or may not be initialized.
	// NOTE:
	// 	Move m_pCurBuffer to pBufferNew.

	CDataBuffer * pBufferNew = new CDataBuffer( dwDataLength, lFileOffset );
	if ( pBufferNew == NULL )
	{
		DEBUG_ERR(( "CreateBuffer FAIL\n" ));
		return( FALSE );
	}

	//
	// Link pBufferNew buffer AFTER m_pCurBuffer.
	// Move m_pCurBuffer to pBufferNew.
	//
	InsertAfter( pBufferNew, m_pCurBuffer );

	//
	// Advance m_pCurBuffer to pBufferNew.
	//
	if ( m_pCurBuffer == NULL )		// Its the first.
	{
		m_dwCurOffset = 0;
	}
	else
	{
		m_dwCurOffset += m_pCurBuffer->GetLength();
	}
	m_pCurBuffer = pBufferNew;

	//
	// Track total memory object length.
	//
	m_dwDataLength += dwDataLength;
	return( TRUE );
}

void CDataObject::DeleteBuffer()
{
	//
	// Remove m_pCurBuffer from the list.
	// Move m_pCurBuffer to the next. m_dwCurOffset stays the same.
	//

	assert( m_pCurBuffer != NULL );

	CDataBuffer * pBufferNext = (CDataBuffer *) m_pCurBuffer->GetNext();
	m_pCurBuffer->RemoveSelf();

	//
	// Track memory object length.
	//
	m_dwDataLength -= m_pCurBuffer->GetLength();

	//
	// Destroy it.
	//
	delete m_pCurBuffer;

	//
	// Move m_pCurBuffer to the next buffer.
	//
	m_pCurBuffer = pBufferNext;
}

void CDataObject::InitCurBuffer()
{
	//
	// Set m_pCurBuffer to the First buffer.
	//
	m_pCurBuffer = (CDataBuffer *) GetHead();
	m_dwCurOffset = 0;
}

BOOL CDataObject::AdvanceCurBuffer()
{
	//
	// Move m_pCurBuffer to the m_pNext.
	// RETURN:
	//  TRUE = Can advance.

	assert( m_pCurBuffer != NULL );

	CDataBuffer * pNext = (CDataBuffer*) m_pCurBuffer->GetNext();
	if ( pNext == NULL ) return( FALSE );

	DWORD dwDataLength = m_pCurBuffer->GetLength();
	if ( ! dwDataLength )
	{
		// Is this neccessary ? Record may finish with unused blocks ?
		DeleteBuffer();	// house cleaning.
	}
	else
	{
		m_dwCurOffset += dwDataLength;
		m_pCurBuffer = pNext;
	}

	return( TRUE );
}

void CDataObject::PreviousCurBuffer()
{
	assert( m_pCurBuffer != NULL );

	m_pCurBuffer = (CDataBuffer*) m_pCurBuffer->GetPrev();
	if ( m_pCurBuffer == NULL ) return;	// This should not happen !?
	m_dwCurOffset -= m_pCurBuffer->GetLength();
}

BOOL CDataObject::SplitBuffer( DWORD dwNewSize )
{
	//
	// Split the m_pCurBuffer into two buffers.
	// Leave m_pCurBuffer the same.
	// This does not change the size of the object just the distribution of the buffers.
	//

	assert( m_pCurBuffer != NULL );

	if ( ! dwNewSize )
		return( TRUE );         // not a split but thats ok.
	if ( dwNewSize >= m_pCurBuffer->GetLength())	// Nothing to do.
		return( TRUE );

	//
	// Create a new buffer after this.
	//
	LONG lFileOffset = m_pCurBuffer->m_lFileOffset;
	if ( lFileOffset != CDATA_FILE_NOTSAVED )
	{
		lFileOffset += dwNewSize;
	}

	DWORD dwSizeNext = m_pCurBuffer->GetLength() - dwNewSize;
	if ( ! CreateBuffer( dwSizeNext, lFileOffset ))
		return( FALSE );
	CDataBuffer * pBuffer = m_pCurBuffer;	// The new buffer created.
	PreviousCurBuffer();	// back to the first buffer.

	//
	// Move the data if it is in memory. (read from disk or created new).
	//
	P_DATA pCurData = m_pCurBuffer->GetData();
	if ( pCurData != NULL )	// Is loaded ?
	{
#ifdef _DEBUG
		if ( dwSizeNext != pBuffer->GetLength())
		{
			DEBUG_ERR(( "SplitBuffer BAD SIZE %ld!=%ld\n", dwSizeNext, pBuffer->GetLength()));
			return( FALSE );
		}
#endif
		P_DATA pData = pBuffer->Lock( FALSE );
		if ( pData != NULL )
		{
#ifdef _WIN32
			hmemcpy( pData, pCurData+dwNewSize, dwSizeNext );
#else
			memcpy( pData, pCurData+dwNewSize, dwSizeNext );
#endif
			pBuffer->Unlock();

		}
	}

	//
	// Resize the first buffer down to its new size.
	//
	return( m_pCurBuffer->ReSize( dwNewSize ));
}

BOOL CDataObject::SplitBufferNext( DWORD dwNewSize )
{
	if ( dwNewSize )
	{
		//
		// Must Split the current buffer.
		// What we want is after it.
		//
		if ( ! SplitBuffer( dwNewSize ))
			return( FALSE );
		AdvanceCurBuffer();
	}
	return( TRUE );
}

BOOL CDataObject::CreateBuffers( DWORD dwLength, LONG lFileOffset )
{
	//
	// PURPOSE:
	//  Add buffers to increase the size of the object.
	//  Add after m_pCurBuffer.
	//  Move m_pCurBuffer to the last buffer added.
	//

	DWORD dwBufferLength = m_dwDataAlign;
	for ( ; dwLength; dwLength -= dwBufferLength )
	{
		if ( dwBufferLength > dwLength ) dwBufferLength = dwLength;
		if ( ! CreateBuffer( dwBufferLength, lFileOffset ))
			return( FALSE );
		if ( lFileOffset != CDATA_FILE_NOTSAVED )
		{
			lFileOffset += dwBufferLength;
		}
	}

	return( TRUE );
}

BOOL CDataObject::DeleteBuffers( DWORD dwLength )
{
	//
	// Delete the m_pCurBuffer and all others till dwLength is reached.
	// Move m_pCurBuffer to the next. m_dwCurOffset stays the same.
	//

	while ( dwLength )
	{
		if ( m_pCurBuffer == NULL )
		{
			DEBUG_ERR(( "Obj:DeleteBuffers beyond end of range !\n" ));
			return( FALSE );
		}

		//
		// Split the last buffer in the chain.
		//
		if ( ! SplitBuffer( dwLength ))
			return( FALSE );

		//
		// Remove the buffer and move to next.
		//
		dwLength -= m_pCurBuffer->GetLength();
		DeleteBuffer();
	}

	return( TRUE );
}

DWORD CDataObject::FindCurBufferOffset( DWORD dwOffset )
{
	//
	// PURPOSE:
	// 	Find a byte offset into the data object.
	//  Set m_pCurBuffer to the buffer.
	// RETURN:
	// 	a sub offset (size) into the m_pCurBuffer.
	// NOTE:
	//  if past the end then return the last buffer pointer, offset >= last len.
	//

	if ( m_pCurBuffer == NULL || dwOffset == 0 )
	{
		InitCurBuffer();
		//  if at the start the return first buffer, offset 0.
		if ( m_pCurBuffer == NULL )
			return( 0 );
	}

	if ( dwOffset > m_dwCurOffset )		// seek foreward
	{
		while ( dwOffset >= m_dwCurOffset + m_pCurBuffer->GetLength())
		{
			if ( ! AdvanceCurBuffer())
				break;
		}
	}
	else
	{
		while ( dwOffset < m_dwCurOffset )	// seek backwards.
		{
			PreviousCurBuffer();
		}
	}

	return( dwOffset - m_dwCurOffset );		// offset into the current buffer.
}

P_DATA CDataObject::Lock( DWORD dwOffset, DWORD * pdwLengthAvail )
{
	//
	// PURPOSE:
	//  get a data pointer at offset.
	// ARGS:
	//  pdwLengthAvail = return how much contiguous data is here
	// NOTE:
	//  This MUST be complimented by Unlock().
	//

	if ( dwOffset >= m_dwDataLength )
		return( NULL );

	dwOffset = FindCurBufferOffset( dwOffset );
	P_DATA pData = m_pCurBuffer->Lock();
	if ( pData == NULL )
		return( NULL );

	* pdwLengthAvail = m_pCurBuffer->GetLength() - dwOffset;
	return( pData + dwOffset );
}

void CDataObject::Change( DWORD dwOffset, DWORD dwLengthChange )
{
	// Mark all this as having changed.

}

void CDataObject::Unlock( DWORD dwOffset, DWORD dwLengthChange )
{
	//
	// PURPOSE:
	//	Modified the Lock() data. (invalidate file)
	// ARGS:
	//  dwOffset = Arg passed to Lock() before
	//  dwLengthChange = 0 = not changed.
	// NOTE:
	//  This must be called for a previous call to Lock().
	//  The LengthChange MUST be <= pdwLengthAvail that was returned by Lock() !!!
	//

	dwOffset = FindCurBufferOffset( dwOffset );

	LONG lFileOffset = m_pCurBuffer->m_lFileOffset;	// Save old file offset.

	if ( dwLengthChange && lFileOffset != CDATA_FILE_NOTSAVED )
	{
		//
		// Part of the changed buffer still valid for the file
		// Split out just the changed part from the buffer.
		//
#ifdef _DEBUG
		if ( dwLengthChange + dwOffset > m_pCurBuffer->GetLength())
		{
			DEBUG_ERR(( "DataObj:Unlock Changed %ld,%ld\n", dwOffset, dwLengthChange ));
		}
#endif

		m_pCurBuffer->Unlock();
		if ( ! SplitBufferNext( dwOffset ))
			return;

		m_pCurBuffer->Changed();
	}
	else
	{
		m_pCurBuffer->Unlock();
	}
}

BOOL CDataObject::ReplaceCopy( DWORD dwOffset, DWORD dwLength, P_DATA pData, DWORD dwDataLength )
{
	//
	// PURPOSE:
	// 	Replace an existing part of the object with a new piece of data
	//  This could be an insert, delete or change.
	// ARGS:
	//  dwLength = the old length to replace.
	//  pData = The new data to replace with.
	//  dwDataLength = replace with this new length.
	//

	if ( ! dwLength && ! dwDataLength ) return( TRUE );	// Nothing to do.

	//
	// Start a new buffer at this offset.
	//
	dwOffset = FindCurBufferOffset( dwOffset );
	if ( dwOffset < m_pCurBuffer->GetLength())	// Not past end.
	{
		if ( ! SplitBufferNext( dwOffset ))
			return( FALSE );

		//
		// Delete current buffers.
		//
		if ( ! DeleteBuffers( dwLength )) return( FALSE );

		if ( m_pCurBuffer == NULL )		// deleted to end !
		{
			FindCurBufferOffset( m_dwDataLength );
		}
		else
		{
			PreviousCurBuffer();	// New space is before the current.
		}
	}

	//
	// Insert new space after the current.
	//
	if ( dwDataLength )
	{
		CDataBuffer * pBuffer = m_pCurBuffer;	// add after this.
		if ( ! CreateBuffers( dwDataLength, CDATA_FILE_NOTSAVED ))
			return( FALSE );

		//
		// Copy new data into space.
		//
		if ( pData != NULL )
		{
			pBuffer = (CDataBuffer *)( ( pBuffer == NULL ) ? GetHead() :
				pBuffer->GetNext());	// First buffer we just created.

			while ( dwDataLength )
			{
				DWORD dwLengthCopy = pBuffer->GetLength();
				if ( dwLengthCopy > dwDataLength ) dwLengthCopy = dwDataLength;
#ifdef _WIN32
				hmemcpy( pBuffer->GetData(), pData, dwLengthCopy );
#else
				memcpy( pBuffer->GetData(), pData, dwLengthCopy );
#endif
				pData += dwLengthCopy;
				dwDataLength -= dwLengthCopy;
				pBuffer = (CDataBuffer *) pBuffer->GetNext();	// Next buffer we just created.
			}
		}
	}

	return( TRUE );
}

BOOL CDataObject::WriteEndData()
{
	//
	// PURPOSE:
	//  Move the end data from one spot in the file to another.
	//

	return( TRUE );
}

BOOL CDataObject::Write()
{
	//
	// PURPOSE:
	// 	Commit all changes back to the file.
	// RETURN:
	//  TRUE = we are happy.
	//

	if ( m_pFile == NULL ) return( FALSE );	// There is no file !!!

	//
	// Re-open the file in write mode ???
	//
	DWORD dwLength = m_pFile->SeekToEnd();
	DWORD dwEndSize = dwLength - ((DWORD) m_lFileOffset ) - m_dwFileLength;

	//
	// If the old file is smaller size we must move the end data first.
	//
	if ( dwEndSize && ( m_dwFileLength < m_dwDataLength ))
	{
		WriteEndData();
	}

	InitCurBuffer();	// Start at the beginning.

	//
	// Loop through the data and write it out.
	// Make sure we don't overwrite the data not yet read !
	//
	LONG lFileOffset = m_lFileOffset;	// The write offset.
	CDataBuffer * pBufferRead = ( CDataBuffer *) GetHead();

	while ( m_pCurBuffer != NULL )		// iterate the list.
	{
		LONG lFileOffsetNext = lFileOffset + m_pCurBuffer->GetLength();
		if ( lFileOffset != m_pCurBuffer->m_lFileOffset )	// buffer is ok here ?
		{
			//
			// Make sure the reads are ahead of this write
			//
			while ( 1 )
			{
				if ( pBufferRead->m_lFileOffset >= lFileOffsetNext ) break; // no need to get yet.
				pBufferRead->Lock();	// make sure it is in memory.
				pBufferRead = (CDataBuffer *) pBufferRead->GetNext();
			}

			//
			// Write it to its new offset.
			//
			if ( ! m_pCurBuffer->Write( lFileOffset ))
			{
				//
				// Kill the read aheads???
				//
				while ( m_pCurBuffer != pBufferRead )
				{
					m_pCurBuffer->Unlock();	// done with it.
					AdvanceCurBuffer();
				}
				return( FALSE );
			}
		}
		else
		{
			// if ( pBuffer )

		}

		//
		// Next.
		//
		lFileOffset = lFileOffsetNext;
		m_pCurBuffer->Unlock();	// done with it.
		AdvanceCurBuffer();
	}

	//
	// If the file is larger than the data we must move the end data down.
	//
	if ( dwEndSize && ( m_dwFileLength > m_dwDataLength ))
	{
		WriteEndData();
	}

	//
	// Changes have been committed.
	//
	if ( m_dwFileLength != m_dwDataLength )
	{
		m_dwFileLength = m_dwDataLength;
		m_pFile->SeekToEnd();	// m_dwFileSize = ((DWORD) m_lFileOffset ) + m_dwFileLength + dwEndSize;
	}

	return( TRUE );
}
