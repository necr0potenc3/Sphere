//
// CSectorTemplate.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "../graysvr/graysvr.h"

////////////////////////////////////////////////////////////////////////
// -CCharsActiveList

CCharsActiveList::CCharsActiveList()
{
	m_timeLastClient.Init();
	m_iClients = 0;
}

void CCharsActiveList::OnRemoveOb( CGObListRec * pObRec )
{
	// Override this = called when removed from group.
	CChar * pChar = STATIC_CAST <CChar*>(pObRec);
	ASSERT( pChar );
	DEBUG_CHECK( pChar->IsTopLevel());
	if ( pChar->IsClient())
	{
		ClientDetach();
		m_timeLastClient = CServTime::GetCurrentTime();	// mark time in case it's the last client
	}
	CGObList::OnRemoveOb(pObRec);
	DEBUG_CHECK( pChar->GetParent() == NULL );
	pChar->SetContainerFlags(UID_O_DISCONNECT);
}

void CCharsActiveList::AddCharToSector( CChar * pChar )
{
	ASSERT( pChar );
	// ASSERT( pChar->m_pt.IsValid());
	if ( pChar->IsClient())
	{
		ClientAttach();
	}
	CGObList::InsertHead(pChar);
}

void CCharsActiveList::ClientAttach()
{
	m_iClients++;
}

void CCharsActiveList::ClientDetach()
{
	DEBUG_CHECK(m_iClients>0);
	m_iClients--;
}

//////////////////////////////////////////////////////////////
// -CItemList

bool CItemsList::sm_fNotAMove = false;

void CItemsList::OnRemoveOb( CGObListRec * pObRec )
{
	// Item is picked up off the ground. (may be put right back down though)
	CItem * pItem = STATIC_CAST <CItem*>(pObRec);
	ASSERT( pItem );
	DEBUG_CHECK( pItem->IsTopLevel());
	DEBUG_CHECK( pItem->GetTopPoint().IsValidPoint());

	if ( ! sm_fNotAMove )
	{
		pItem->OnMoveFrom();	// IT_MULTI, IT_SHIP and IT_COMM_CRYSTAL
	}

	CGObList::OnRemoveOb(pObRec);
	DEBUG_CHECK( pItem->GetParent() == NULL );
	pItem->SetContainerFlags(UID_O_DISCONNECT);	// It is no place for the moment.
}

void CItemsList::AddItemToSector( CItem * pItem )
{
	// Add to top level.
	// Either MoveTo() or SetTimeout is being called.
	ASSERT( pItem );
	CGObList::InsertHead( pItem );
}

//////////////////////////////////////////////////////////////////
// -CSectorBase

CSectorBase::CSectorBase()
{
	m_map = 0;
	m_index = 0;
}

void CSectorBase::Init(int index, int map)
{
	if (( map < 0 ) || ( map >= 256 ) || !g_MapList.m_maps[map] )
	{
		g_Log.EventError("Trying to initalize a sector %d in unsupported map #%d. Defaulting to 0,0." DEBUG_CR, index, map);
	}
	else if (( index < 0 ) || ( index >= g_MapList.GetSectorQty(map) ))
	{
		m_map = map;
		g_Log.EventError("Trying to initalize a sector by sector number %d out-of-range for map #%d. Defaulting to 0,%d." DEBUG_CR, index, map, map);
	}
	else
	{
		m_index = index;
		m_map = map;
	}
}

int CSectorBase::GetIndex() const
{
	return m_index;
}

int CSectorBase::GetMap() const
{
	return m_map;
}

void CSectorBase::CheckMapBlockCache( int iTime )
{
	// Clean out the sectors map cache if it has not been used recently.
	// iTime == 0 = delete all.

	int iQty = m_MapBlockCache.GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		CGrayMapBlock * pMapBlock = STATIC_CAST <CGrayMapBlock *>(m_MapBlockCache[i]);
		ASSERT(pMapBlock);
		if ( iTime <= 0 || pMapBlock->m_CacheTime.GetCacheAge() >= iTime )
		{
			m_MapBlockCache.DeleteAt(i);
			i--;
			iQty--;
		}
	}
}


const CGrayMapBlock * CSectorBase::GetMapBlock( const CPointMap & pt )
{
	// Get a map block from the cache. load it if not.
	ASSERT( pt.IsValidXY());
	CPointMap pntBlock( UO_BLOCK_ALIGN(pt.m_x), UO_BLOCK_ALIGN(pt.m_y));
	ASSERT( m_MapBlockCache.GetCount() <= (UO_BLOCK_SIZE * UO_BLOCK_SIZE));

	CGrayMapBlock * pMapBlock;

	// Find it in cache.
	int i = m_MapBlockCache.FindKey( pntBlock.GetPointSortIndex());
	if ( i >= 0 )
	{
		pMapBlock = STATIC_CAST <CGrayMapBlock *>(m_MapBlockCache[i]);
		ASSERT(pMapBlock);
		pMapBlock->m_CacheTime.HitCacheTime();
		return( pMapBlock );
	}
	// else load it.
	try
	{
		pMapBlock = new CGrayMapBlock(pntBlock);
		ASSERT(pMapBlock);
	}
	catch (...)
	{
		return NULL;
	}

	// Add it to the cache.
	m_MapBlockCache.AddSortKey( pMapBlock, pntBlock.GetPointSortIndex() );
	return( pMapBlock );
}

bool CSectorBase::IsInDungeon() const
{
	// ??? in the future make this part of the region info !
	// What part of the maps are filled with dungeons.
	// Used for light / weather calcs.
	CPointMap pt = GetBasePoint();

	if ( pt.m_map > 1 ) return false;

	int x1=(pt.m_x-UO_SIZE_X_REAL);
	if ( x1 < 0 )
		return( false );

	x1 /= 256;
	switch ( pt.m_y / 256 )
	{
	case 0:
	case 5:
		return( true );
	case 1:
		if (x1!=0)
			return( true );
		break;
	case 2:
	case 3:
		if (x1<3)
			return( true );
		break;
	case 4:
	case 6:
		if (x1<1)
			return( true );
		break;
	case 7:
		if (x1<2)
			return( true );
		break;
	}
	return( false );
}

CRegionBase * CSectorBase::GetRegion( const CPointBase & pt, DWORD dwType ) const
{
	// Does it match the mask of types we care about ?
	// Assume sorted so that the smallest are first.
	//
	// REGION_TYPE_AREA => RES_AREA = World region area only = CRegionWorld
	// REGION_TYPE_ROOM => RES_ROOM = NPC House areas only = CRegionBase.
	// REGION_TYPE_MULTI => RES_WORLDITEM = UID linked types in general = CRegionWorld

	int iQty = m_RegionLinks.GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		CRegionBase * pRegion = m_RegionLinks[i];
		ASSERT(pRegion);

		ASSERT( pRegion->GetResourceID().IsValidUID());
		if ( pRegion->GetResourceID().IsItem())
		{
			if ( ! ( dwType & REGION_TYPE_MULTI ))
				continue;
		}
		else if ( pRegion->GetResourceID().GetResType() == RES_AREA )
		{
			if ( ! ( dwType & REGION_TYPE_AREA ))
				continue;
		}
		else
		{
			if ( ! ( dwType & REGION_TYPE_ROOM ))
				continue;
		}

		if ( ! pRegion->m_pt.IsSameMap(pt.m_map))
			continue;
		if ( ! pRegion->IsInside2d( pt ))
			continue;
		return( pRegion );
	}
	return( NULL );
}

// Balkon: get regions list (to cicle through intercepted house regions)
int CSectorBase::GetRegions( const CPointBase & pt, DWORD dwType, CRegionLinks & rlist ) const
{
	int iQty = m_RegionLinks.GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		CRegionBase * pRegion = m_RegionLinks[i];
		ASSERT(pRegion);

		ASSERT( pRegion->GetResourceID().IsValidUID());
		if ( pRegion->GetResourceID().IsItem())
		{
			if ( ! ( dwType & REGION_TYPE_MULTI ))
				continue;
		}
		else if ( pRegion->GetResourceID().GetResType() == RES_AREA )
		{
			if ( ! ( dwType & REGION_TYPE_AREA ))
				continue;
		}
		else
		{
			if ( ! ( dwType & REGION_TYPE_ROOM ))
				continue;
		}

		if ( ! pRegion->m_pt.IsSameMap(pt.m_map))
			continue;
		if ( ! pRegion->IsInside2d( pt ))
			continue;
		rlist.Add( pRegion );
	}
	return( rlist.GetCount() );
}

bool CSectorBase::UnLinkRegion( CRegionBase * pRegionOld )
{
	// NOTE: What about unlinking it from all the CChar(s) here ? m_pArea
	ASSERT(pRegionOld);
	return m_RegionLinks.RemovePtr(pRegionOld);
}

bool CSectorBase::LinkRegion( CRegionBase * pRegionNew )
{
	// link in a region. may have just moved !
	// Make sure the smaller regions are first in the array !
	// Later added regions from the MAP file should be the smaller ones, 
	//  according to the old rules.
	ASSERT(pRegionNew);
	ASSERT( pRegionNew->IsOverlapped( GetRect()));
	int iQty = m_RegionLinks.GetCount();

	for ( int i=0; i<iQty; i++ )
	{
		CRegionBase * pRegion = m_RegionLinks[i];
		ASSERT(pRegion);
		if ( pRegionNew == pRegion )
		{
			DEBUG_ERR(( "region already linked!\n" ));
			return false;
		}

		if ( pRegion->IsOverlapped(pRegionNew))
		{
			// NOTE : We should use IsInside() but my version isn't completely accurate for it's FALSE return
			if ( pRegion->IsEqualRegion( pRegionNew ))
			{
				DEBUG_ERR(( "Conflicting region!\n" ));
				return( false );
			}
			if ( pRegionNew->IsInside(pRegion))	// it is accurate in the TRUE case.
				continue;

			// must insert before this.
			m_RegionLinks.InsertAt( i, pRegionNew );
			return( true );
		}

		DEBUG_CHECK( iQty >= 1 );
	}

	m_RegionLinks.Add( pRegionNew );
	return( true );
}

CTeleport * CSectorBase::GetTeleport2d( const CPointMap & pt ) const
{
	// Any teleports here at this point ?

	int i = m_Teleports.FindKey( pt.GetPointSortIndex());
	if ( i < 0 )
		return( NULL );
	return STATIC_CAST <CTeleport *>( m_Teleports[i]);
}

CTeleport * CSectorBase::GetTeleport( const CPointMap & pt ) const
{
	// Any teleports here at this point ?

	CTeleport * pTeleport = GetTeleport2d( pt );
	if ( pTeleport == NULL )
		return( NULL );

	int zdiff = pt.m_z - pTeleport->m_z;	
	if ( abs(zdiff) > 5 )
		return( NULL );

	// Check m_map ?
	if ( ! pTeleport->IsSameMap( pt.m_map ))
		return( NULL );

	return( pTeleport );
}

bool CSectorBase::AddTeleport( CTeleport * pTeleport )
{
	// NOTE: can't be 2 teleports from the same place !
	// ASSERT( Teleport is actually in this sector !

	int i = m_Teleports.FindKey( pTeleport->GetPointSortIndex());
	if ( i >= 0 )
	{
		DEBUG_ERR(( "Conflicting teleport %s!\n", pTeleport->WriteUsed() ));
		return( false );
	}
	m_Teleports.AddSortKey( pTeleport, pTeleport->GetPointSortIndex());
	return( true );
}

CPointMap CSectorBase::GetBasePoint() const
{
	// What is the coord base of this sector. upper left point.
	ASSERT( m_index >= 0 && m_index < g_MapList.GetSectorQty(m_map) );
	CPointMap pt(( m_index % g_MapList.GetSectorCols(m_map)) * g_MapList.GetSectorSize(m_map),
		( m_index / g_MapList.GetSectorCols(m_map) ) * g_MapList.GetSectorSize(m_map),
		0,
		m_map);
	return( pt );
}

CPointMap CSectorBase::GetMidPoint() const
{
	CPointMap pt = GetBasePoint();
	pt.m_x += g_MapList.GetSectorSize(pt.m_map)/2;	// East
	pt.m_y += g_MapList.GetSectorSize(pt.m_map)/2;	// South
	return( pt );
}

CRectMap CSectorBase::GetRect() const
{
	// Get a rectangle for the sector.
	CPointMap pt = GetBasePoint();
	CRectMap rect;
	rect.m_left = pt.m_x;
	rect.m_top = pt.m_y;
	rect.m_right = pt.m_x + g_MapList.GetSectorSize(pt.m_map);	// East
	rect.m_bottom = pt.m_y + g_MapList.GetSectorSize(pt.m_map);	// South
	rect.m_map = pt.m_map;
	return( rect );
}
