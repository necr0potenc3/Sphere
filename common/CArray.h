//
// CArray.h
// Copyright Menace Software (www.menasoft.com).
//
// c++ Collections.

#ifndef _INC_CARRAY_H
#define _INC_CARRAY_H

class CMemDynamic
{
	// This item will always be dynamically allocated with new/delete!
	// Never stack or data seg based.

#ifdef _DEBUG

#define DECLARE_MEM_DYNAMIC virtual const void * GetTopPtr() const { return this; }	// Get the top level ptr.
#define COBJBASE_SIGNATURE	0xDEADBEEF	// used just to make sure this is valid.
private:
	DWORD m_dwSignature;

protected:
	virtual const void * GetTopPtr() const = 0;	// Get the top level class ptr.
public:
	bool IsValidDynamic() const
	{
		if ( m_dwSignature != COBJBASE_SIGNATURE )
		{
			return false;
		}
		return true;
	}
	CMemDynamic()
	{
		// NOTE: virtuals don't work in constructors or destructors !
		m_dwSignature = COBJBASE_SIGNATURE;
		// ASSERT( IsValidDynamic());
	}
	virtual ~CMemDynamic()
	{
		ASSERT( IsValidDynamic());
		m_dwSignature = 0;
	}

#else	// _DEBUG

#define DECLARE_MEM_DYNAMIC
public:
	bool IsValidDynamic() const
	{
		return true;
	}
	virtual ~CMemDynamic()	// always virtual so we can always use dynamic_cast correctly.
	{
	}

#endif	// _DEBUG
};

///////////////////////////////////////////////////////////
// CGObList

class CGObListRec : public CMemDynamic	// generic list record. 
{
	// This item belongs to JUST ONE LIST
	friend class CGObList ;
private:
	CGObList  * 	m_pParent;		// link me back to my parent object.
	CGObListRec * 	m_pNext;
	CGObListRec * 	m_pPrev;
public:
	CGObList  * 	GetParent() const { return m_pParent; }
	CGObListRec * 	GetNext() const { return m_pNext; }
	CGObListRec * 	GetPrev() const { return m_pPrev; }
public:
	CGObListRec()
	{
		m_pParent = NULL;	// not linked yet.
		m_pNext = NULL;
		m_pPrev = NULL;
	}
	void RemoveSelf();	// remove myself from my parent list.
	virtual ~CGObListRec()
	{
		RemoveSelf();
	}
};

class CGObList	// generic list of objects based on CGObListRec.
{
	friend class CGObListRec;
private:
	CGObListRec * m_pHead;
	CGObListRec * m_pTail;	// Do we really care about tail ? (as it applies to lists anyhow)
	int m_iCount;
private:
	void RemoveAtSpecial( CGObListRec * pObRec )
	{
		// only called by pObRec->RemoveSelf()
		OnRemoveOb( pObRec );	// call any approriate virtuals.
	}
protected:
	// Override this to get called when an item is removed from this list.
	// Never called directly. call pObRec->RemoveSelf()
	virtual void OnRemoveOb( CGObListRec* pObRec );	// Override this = called when removed from list.
public:
	CGObListRec * GetAt( int index ) const;
	// pPrev = NULL = first
	virtual void InsertAfter( CGObListRec * pNewRec, CGObListRec * pPrev = NULL );
	void InsertBefore( CGObListRec * pNewRec, CGObListRec * pNext )
	{
		// pPrev = NULL = last
		InsertAfter( pNewRec, ( pNext ) ? ( pNext->GetPrev() ) : GetTail() );
	}
	void InsertHead( CGObListRec * pNewRec )
	{
		InsertAfter( pNewRec, NULL );
	}
	void InsertTail( CGObListRec * pNewRec )
	{
		InsertAfter( pNewRec, GetTail());
	}
	void DeleteAll();
	void Empty() { DeleteAll(); }
	CGObListRec * GetHead() const { return m_pHead; }
	CGObListRec * GetTail() const { return m_pTail; }
	int GetCount() const { return m_iCount; }
	bool IsEmpty() const
	{
		return !GetCount();
	}
	CGObList()
	{
		m_pHead = NULL;
		m_pTail = NULL;
		m_iCount = 0;
	}
	virtual ~CGObList()
	{
		DeleteAll();
	}
};

inline void CGObListRec::RemoveSelf()	// remove myself from my parent list.
{
	// Remove myself from my parent list (if i have one)
	if ( GetParent() == NULL )
		return;
	m_pParent->RemoveAtSpecial( this );
	ASSERT( GetParent() == NULL );
}

///////////////////////////////////////////////////////////
// CGTypedArray<class TYPE, class ARG_TYPE>

template<class TYPE, class ARG_TYPE>
class CGTypedArray
{
	// NOTE: This will not call true constructors or destructors !
private:
	TYPE* m_pData;   // the actual array of data
	int m_nCount;     // # of elements

public:
	CGTypedArray()
	{
		m_pData = NULL;
		m_nCount = 0;
	}
	TYPE * GetBasePtr() const	// This is dangerous to use of course.
	{
		return m_pData;
	}
	int GetCount() const
	{
		return m_nCount;
	}
	int GetSize() const	// same thing just for compatibility
	{
		return m_nCount;
	}
	bool IsValidIndex( int i ) const
	{
		return ( i>=0 && i<m_nCount );
	}
	void SetCount( int nNewCount );
	void RemoveAll()
	{
		SetCount(0);
	}
	void Empty() { RemoveAll(); }
	void SetAt( int nIndex, ARG_TYPE newElement )
	{
		ASSERT(IsValidIndex(nIndex));
		DestructElements( &m_pData[nIndex], 1 );
		m_pData[nIndex] = newElement;
	}
	void SetAtGrow( int nIndex, ARG_TYPE newElement)
	{
		ASSERT(nIndex >= 0);
		if (nIndex >= m_nCount)
			SetCount( nIndex+1 );
		SetAt( nIndex, newElement );
	}
	void InsertAt( int nIndex, ARG_TYPE newElement )
	{	// Bump the existing entry here forward.
		ASSERT(nIndex >= 0);
		SetCount( (nIndex >= m_nCount) ? (nIndex+1) : (m_nCount+1) );
		memmove( &m_pData[nIndex+1], &m_pData[nIndex], sizeof(TYPE)*(m_nCount-nIndex-1));
		m_pData[nIndex] = newElement;
	}
	int Add( ARG_TYPE newElement )
	{
		// Add to the end.
		SetAtGrow( GetCount(), newElement );
		return (m_nCount-1);
	}
	void RemoveAt( int nIndex )
	{
		ASSERT(IsValidIndex(nIndex));
		DestructElements( &m_pData[nIndex], 1 );
		memmove( &m_pData[nIndex], &m_pData[nIndex+1], sizeof(TYPE)*(m_nCount-nIndex-1));
		SetCount( m_nCount-1 );
	}
	TYPE GetAt( int nIndex) const
	{
		ASSERT(IsValidIndex(nIndex));
		return m_pData[nIndex];
	}
	TYPE operator[](int nIndex) const
	{
		return GetAt(nIndex);
	}
	TYPE& ElementAt( int nIndex )
	{
		if ( !IsValidIndex(nIndex) )
		{
			ASSERT(IsValidIndex(nIndex));
		}
		return m_pData[nIndex];
	}
	TYPE& operator[](int nIndex)
	{
		return ElementAt(nIndex);
	}
	const TYPE& ElementAt( int nIndex ) const
	{
		ASSERT(IsValidIndex(nIndex));
		return m_pData[nIndex];
	}

	virtual void ConstructElements(TYPE* pElements, int nCount )
	{
		// first do bit-wise zero initialization
		memset((void*)pElements, 0, nCount * sizeof(TYPE));
	}
	virtual void DestructElements(TYPE* pElements, int nCount )
	{
		//memset((void*)pElements, 0, nCount * sizeof(*pElements));
	}

	void Copy( const CGTypedArray<TYPE, ARG_TYPE> * pArray )
	{
		if ( this != pArray )
			return;
		Empty();	// destruct any previous data.
		SetCount( pArray->GetCount());
		// call no constructors.
		memcpy( GetBasePtr(), pArray->GetBasePtr(), GetCount() * sizeof(TYPE));
	}

	const CGTypedArray<TYPE, ARG_TYPE> & operator=( const CGTypedArray<TYPE, ARG_TYPE> & array )
	{
		Copy( &array );
		return *this;
	}

	~CGTypedArray()
	{
		SetCount( 0 );
	}
};

/////////////////////////////////////////////////////////////////////////////
// CGTypedArray<TYPE, ARG_TYPE> out-of-line functions

template<class TYPE, class ARG_TYPE>
void CGTypedArray<TYPE, ARG_TYPE>::SetCount( int nNewCount )
{
	ASSERT(nNewCount >= 0);

	if (nNewCount == 0)
	{
		// shrink to nothing
		if (m_nCount)
		{
			//DestructElements( m_pData, m_nCount );
			delete [] (BYTE*) m_pData;
			m_nCount = 0;
			m_pData = NULL;
		}
		return;
	}

	if ( nNewCount > m_nCount )
	{
		TYPE * pNewData = (TYPE *) new BYTE[ nNewCount * sizeof( TYPE ) ];
		if ( m_nCount )
		{
			// copy the old stuff to the new array.
			memcpy( pNewData, m_pData, sizeof(TYPE)*m_nCount );
			delete [] (BYTE*) m_pData;	// don't call any destructors.
		}

		// Just construct or init the new stuff.
		ConstructElements( pNewData + m_nCount, nNewCount - m_nCount );
		m_pData = pNewData;
	}

	m_nCount = nNewCount;
}

/////////////////////////////////////////////////////////////////////////////
// CGPtrTypeArray

template<class TYPE>
class CGPtrTypeArray : public CGTypedArray<TYPE, TYPE>	// void*
{
protected:
	virtual void DestructElements( TYPE* pElements, int nCount )
	{
		memset( pElements, 0, nCount * sizeof(*pElements));
	}
public:
	int FindPtr( TYPE pData ) const
	{
		if ( pData == NULL )
			return( -1 );
		for ( int nIndex=0; nIndex<GetCount(); nIndex++ )
		{
			if ( GetAt(nIndex) == pData )
				return( nIndex );
		}
		return( -1 );
	}
	bool RemovePtr( TYPE pData )
	{
		int nIndex = FindPtr( pData );
		if ( nIndex < 0 )
			return( false );
		RemoveAt( nIndex );
		return( true );
	}
	bool IsValidIndex( int i ) const
	{
		if ( i < 0 || i >= GetCount())
			return( false );
		return( GetAt(i) != NULL );
	}
};

/////////////////////////////////////////////////////////////////////////////
// CGObArray

template<class TYPE>
class CGObArray : public CGPtrTypeArray<TYPE>
{
	// The point of this type is that the array now OWNS the element.
	// It will get deleted when the array is deleted.
protected:
	virtual void DestructElements( TYPE* pElements, int nCount )
	{
		// delete the objects that we own.
		for ( int i=0; i<nCount; i++ )
		{
			if ( pElements[i] != NULL )
				delete pElements[i];
		}
		CGPtrTypeArray<TYPE>::DestructElements(pElements,nCount);
	}
public:
	bool DeleteOb( TYPE pData )
	{
		return( RemovePtr( pData ));
	}
	void DeleteAt( int nIndex )
	{
		RemoveAt( nIndex );
	}
	TYPE UnLinkIndex(int index )
	{
		// Remove the object from the list so it will not get destroyed.
		TYPE data = GetAt(index);
		ElementAt(index) = NULL;
		RemoveAt(index);
		return( data );
	}
	~CGObArray()
	{
		// Make sure the virtuals get called.
		SetCount( 0 );
	}
};

////////////////////////////////////////////////////////////
// CGObSortArray = A sorted array of objects.

template<class TYPE,class KEY_TYPE>
struct CGObSortArray : public CGObArray<TYPE>
{
	int FindKeyNear( KEY_TYPE key, int & iCompareRes, bool fNoSpaces = false ) const;
	int FindKey( KEY_TYPE key ) const
	{
		// Find exact key
		int iCompareRes;
		int index = FindKeyNear( key, iCompareRes, false );
		if ( iCompareRes )
			return( -1 );
		return( index );
	}
	int AddPresorted( int index, int iCompareRes, TYPE pNew )
	{
		if ( iCompareRes > 0 )
		{
			index++;
		}
		InsertAt( index, pNew );
		return( index );
	}
	int AddSortKey( TYPE pNew, KEY_TYPE key );
	virtual int CompareKey( KEY_TYPE, TYPE, bool fNoSpaces ) const = 0;
	void DeleteKey( KEY_TYPE key )
	{
		DeleteAt( FindKey( key ));
	}
#ifdef _DEBUG
	bool TestSort() const;
#endif
};

template<class TYPE, class KEY_TYPE>
int CGObSortArray<TYPE, KEY_TYPE>::FindKeyNear( KEY_TYPE key, int & iCompareRes, bool fNoSpaces ) const
{
	// Do a binary search for the key.
	// RETURN: index
	//  iCompareRes =
	//		0 = match with index.
	//		-1 = key should be less than index.
	//		+1 = key should be greater than index
	//

	int iHigh = GetCount()-1;
	if ( iHigh < 0 )
	{
		iCompareRes = -1;
		return( 0 );
	}

	int iLow = 0;
	int i;
	while ( iLow <= iHigh )
	{
		i = (iHigh+iLow)/2;
		iCompareRes = CompareKey( key, GetAt(i), fNoSpaces );
		if ( iCompareRes == 0 )
			break;
		if ( iCompareRes > 0 )
		{
			iLow = i+1;
		}
		else
		{
			iHigh = i-1;
		}
	}
	return( i );
}

template<class TYPE, class KEY_TYPE>
int CGObSortArray<TYPE, KEY_TYPE>::AddSortKey( TYPE pNew, KEY_TYPE key )
{
	// Insertion sort.
	int iCompareRes;
	int index = FindKeyNear( key, iCompareRes );
	if ( !iCompareRes )
	{
		// duplicate should not happen ?!?
		// DestructElements is called automatically for previous.
		SetAt( index, pNew );
		return( -1 );
	}
	return AddPresorted( index, iCompareRes, pNew );
}

#endif	// _INC_CARRAY_H