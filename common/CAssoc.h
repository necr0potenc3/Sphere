//
// CAssoc.H
// Simple shared usefull base classes.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _INC_CASSOC_H
#define _INC_CASSOC_H
#pragma once

#include "CFile.h"
#include "CArray.h"

////////////////////////////////////////////////////////////////////////

#ifndef PT_REG_STRMAX
#define PT_REG_STRMAX		128
#endif
#ifndef PT_REG_ROOTKEY
#define PT_REG_ROOTKEY		HKEY_LOCAL_MACHINE
#endif

////////////////////////////////////////////////////////////////////////

// #include <shellapi.h>

enum ELEM_TYPE	// define types of structure/record elements.
{
	ELEM_VOID = 0,	// unknown what this might be. (or just 'other') (must be handled manually)
	ELEM_CSTRING,	// Size prefix.
	ELEM_STRING,	// Assume max size of REG_SIZE. NULL TERM string.
	ELEM_BOOL,		// bool = just 1 byte i guess.
	ELEM_BYTE,		// 1 byte.
	ELEM_MASK_BYTE,	// bits in a BYTE
	ELEM_WORD,		// 2 bytes
	ELEM_MASK_WORD,	// bits in a WORD
	ELEM_INT,		// Whatever the int size is. 4 i assume
	ELEM_MASK_INT,
	ELEM_DWORD,		// 4 bytes.
	ELEM_MASK_DWORD,	// bits in a DWORD

	ELEM_QTY,
};

#ifndef offsetof			// stddef.h ?
#define offsetof(s,m)   	(int)( (BYTE *)&(((s *)0)->m) - (BYTE *)0 )
#endif

struct CElementDef
{
	static const int sm_Lengths[ELEM_QTY];
	ELEM_TYPE m_type;
	UINT	m_offset;	// The offset into the class instance for this item.
	// ELEM_STRING = max size.
	// ELEM_MASK_WORD etc. = Extra masking info if needed. 
	DWORD   m_extra;

public:
	bool CanCompareMem() const
	{
		return( m_type >= ELEM_BOOL );
	}
	int CompareMem( const void * pBase1, const void * pBase2 ) const
	{
		int iLength = GetValLength();
		ASSERT( iLength > 0 );
		return( memcmp( GetValPtr(pBase1), GetValPtr(pBase2), iLength ));
	}
public:
	// get structure value.
	void * GetValPtr( const void * pBaseInst ) const
	{
		return( ((BYTE *)pBaseInst) + m_offset );
	}
	int GetValLength() const
	{
		ASSERT(m_type<ELEM_QTY);
		if ( m_type == ELEM_STRING )
		{
			return(m_extra);
		}
		return( sm_Lengths[m_type] );
	}
	int CompareVal( const void * pBase1, const void * pBase2 ) const;

	bool GetValStr( const void * pBase, CGString & sVal ) const;
	bool SetValStr( void * pBase, LPCTSTR pszVal ) const;

	// Move the element. Read or Write.
	void Serialize( CGFile & file, void * pBaseInst );
};

class CAssocReg	// associate members of some class/structure with entries in the registry.
{
	// LAST = { NULL, 0, ELEM_VOID }
public:
	LPCTSTR m_pszKey;	// A single key identifier to be cat to a base key. NULL=last
	CElementDef m_elem;
public:
	operator LPCTSTR() const
	{
		return( m_pszKey );
	}
	// get structure value.
	void * GetValPtr( const void * pBaseInst ) const
	{
		return( m_elem.GetValPtr( pBaseInst ));
	}

	void Serialize( CGFile & file, void * pBaseInst )
	{
		m_elem.Serialize( file, pBaseInst );
	}

#ifdef HKEY_CLASSES_ROOT	// _WIN32

public:
	long RegQueryPtr( LPCTSTR pszBaseKey, LPSTR pszValue ) const
	{
		// get value from the registry.
		TCHAR szKey[ PT_REG_STRMAX ];
		LONG lSize = sizeof(szKey);
		pszValue[0] = '\0';
		return( RegQueryValue( PT_REG_ROOTKEY,
			lstrcat( lstrcpy( szKey, pszBaseKey ), m_pszKey ),
			pszValue, &lSize ));
	}
	long RegSetPtr( LPCTSTR pszBaseKey, LPCTSTR pszValue ) const
	{
		// set value to the registry.
		TCHAR szKey[ PT_REG_STRMAX ];
		return( RegSetValue( PT_REG_ROOTKEY,
			lstrcat( lstrcpy( szKey, pszBaseKey ), m_pszKey ),
			REG_SZ, pszValue, lstrlen( pszValue )+1 ));
	}

public:
	long RegQuery( LPCTSTR pszBaseKey, void * pBaseInst ) const;
	long RegSet( LPCTSTR pszBaseKey, const void * pBaseInst ) const;

	long RegQueryAll( LPCTSTR pszBaseKey, void * pBaseInst ) const;
	long RegSetAll( LPCTSTR pszBaseKey, const void * pBaseInst ) const
	{
		long lRet = 0;
		for ( int i=0; this[i].m_pszKey != NULL; i++ )
		{
			lRet = this[i].RegSet( pszBaseKey, pBaseInst );
		}
		return( lRet );
	}

#endif	// HKEY_CLASSES_ROOT

};

////////////////////////////////////////////////////////////////////////

class CGStringListRec : public CGObListRec, public CGString
{
	friend class CGStringList;
	DECLARE_MEM_DYNAMIC
public:
	CGStringListRec * GetNext() const
	{
		return( (CGStringListRec *) CGObListRec :: GetNext());
	}
	CGStringListRec( LPCTSTR pszVal ) : CGString( pszVal )
	{
	}
};

class CGStringList : public CGObList 	// obviously a list of strings.
{
public:
	CGStringListRec * GetHead() const
	{
		return( (CGStringListRec *) CGObList::GetHead() );
	}
	void AddHead( LPCTSTR pszVal )
	{
		InsertHead( new CGStringListRec( pszVal ));
	}
	void AddTail( LPCTSTR pszVal )
	{
		InsertTail( new CGStringListRec( pszVal ));
	}
};

////////////////////////////////////////////////////////////////////////

class CAssocStr		// Associate a string with a DWORD value
{
	// LAST = { NULL, 0 },
public:
	DWORD m_dwVal;
	LPCTSTR m_pszLabel;	// NULL = last.
public:
	DWORD Find( LPCTSTR pszLabel ) const ;
	LPCTSTR Find( DWORD dwVal ) const ;
	LPCTSTR Find( TCHAR * pszOut, DWORD dwVal, LPCTSTR pszDef = NULL ) const ;
};

////////////////////////////////////////////////////////////////////////

class CAssocVal
{
	// Associate a value with another value.
	// LAST = { 0, 0 },
public:
	DWORD m_dwValA;
	DWORD m_dwValB;
public:
	BOOL CvtAtoB( DWORD dwValue, DWORD FAR * pdwValue ) const
	{
		for ( int i=0; this[i].m_dwValA || this[i].m_dwValB; i++ )
			if ( this[i].m_dwValA == dwValue )
		{
			*pdwValue = this[i].m_dwValB;
			return( TRUE );
		}
		return( FALSE );
	}
};

#endif // _INC_CASSOC_H

