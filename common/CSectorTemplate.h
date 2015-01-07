//
// CSectorTemplate.h
//
#ifndef _INC_CSECTOR_H
#define _INC_CSECTOR_H
#pragma once

class CItemsDisconnectList : public CGObList
{
	// Use this just for validation purposes.
};

class CCharsDisconnectList : public CGObList
{
	// Use this just for validation purposes.
};

class CCharsActiveList : public CGObList
{
private:
	int	   m_iClients;			// How many clients in this sector now?
public:
	CServTime m_timeLastClient;	// age the sector based on last client here.
protected:
	void OnRemoveOb( CGObListRec* pObRec );	// Override this = called when removed from list.
public:
	int HasClients() const { return( m_iClients ); }
	void ClientAttach();
	void ClientDetach();
	void AddCharToSector( CChar * pChar );
	CCharsActiveList();
};

class CItemsList : public CGObList
{
	// Top level list of items.
public:
	static bool sm_fNotAMove;	// hack flag to prevent items from bouncing around too much.
protected:
	void OnRemoveOb( CGObListRec* pObRec );	// Override this = called when removed from list.
public:
	void AddItemToSector( CItem * pItem );
};

class CObPointSortArray : public CGObSortArray< CPointSort*, long >
{
	// Find a point fast.
	int CompareKey( long id, CPointSort* pBase, bool fNoSpaces ) const
	{
		ASSERT( pBase );
		return( id - pBase->GetPointSortIndex());
	}
};

class CSectorBase		// world sector
{
protected:
	int	m_index;		// sector index
	int m_map;			// sector map

private:
	CObPointSortArray	m_MapBlockCache;	//	CGrayMapBlock map cache. 
public:
	CObPointSortArray	m_Teleports;		//	CTeleport array
	CRegionLinks		m_RegionLinks;		//	CRegionBase(s) in this CSector
public:
	CCharsActiveList	m_Chars_Active;		// CChar(s) activet in this CSector.
	CCharsDisconnectList m_Chars_Disconnect;// Idle player characters. Dead NPC's, logout players and ridden horses.
	CItemsList m_Items_Timer;				// CItem(s) in this CSector that need timers.
	CItemsList m_Items_Inert;				// CItem(s) in this CSector. (no timer required)
public:
	CSectorBase();
	void Init(int index, int map);

	// Location map units.
	int GetIndex() const;
	int GetMap() const;
	CPointMap GetBasePoint() const;
	CPointMap GetMidPoint() const;
	CRectMap GetRect() const;
	bool IsInDungeon() const;

	void CheckMapBlockCache( int iTime );
	const CGrayMapBlock * GetMapBlock( const CPointMap & pt );

	// CRegionBase
	CRegionBase * GetRegion( const CPointBase & pt, DWORD dwType ) const;
	int GetRegions( const CPointBase & pt, DWORD dwType, CRegionLinks & rlist ) const;

	bool UnLinkRegion( CRegionBase * pRegionOld );
	bool LinkRegion( CRegionBase * pRegionNew );

	// CTeleport(s) in the region.
	CTeleport * GetTeleport( const CPointMap & pt ) const;
	CTeleport * GetTeleport2d( const CPointMap & pt ) const;
	bool AddTeleport( CTeleport * pTeleport );
};


#endif // _INC_CSECTOR_H