//
// CItemMulti.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.

/////////////////////////////////////////////////////////////////////////////

CItemMulti::CItemMulti( ITEMID_TYPE id, CItemBase * pItemDef ) :	// CItemBaseMulti
	CItem( id, pItemDef )
{
	DEBUG_CHECK( dynamic_cast<const CItemBaseMulti*>(pItemDef));
	m_pRegion = NULL;
}

CItemMulti::~CItemMulti()
{
	MultiUnRealizeRegion();	// unrealize before removed from ground.
	DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
	// NOTE: ??? This is dangerous to iterators. The "next" item may no longer be valid !

	// Attempt to remove all the accessory junk.
	// NOTE: assume we have already been removed from Top Level

	if ( ! m_pRegion )
		return;

	CWorldSearch Area( m_pRegion->m_pt, Multi_GetMaxDist() );	// largest area.
	while (true)
	{
		CItem * pItem = Area.GetItem();
		if ( pItem == NULL )
			break;
		if ( pItem == this )	// this gets deleted seperately.
			continue;
		if ( ! Multi_IsPartOf( pItem ))
			continue;
		pItem->Delete();	// delete the key id for the door/key/sign.
	}

	delete m_pRegion;
}

int CItemMulti::Multi_GetMaxDist() const
{
	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	if ( pMultiDef == NULL )
		return( 0 );
	return( pMultiDef->GetMaxDist());
}

const CItemBaseMulti * CItemMulti::Multi_GetDef( ITEMID_TYPE id ) // static
{
	return( dynamic_cast <const CItemBaseMulti *> ( CItemBase::FindItemBase(id)));
}

bool CItemMulti::MultiRealizeRegion()
{
	// Add/move a region for the multi so we know when we are in it.
	// RETURN: ignored.

	DEBUG_CHECK( IsType(IT_MULTI) || IsType(IT_SHIP) );
	ASSERT( IsTopLevel());

	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	if ( pMultiDef == NULL )
	{
		DEBUG_ERR(( "Bad Multi type 0%x, uid=0%x\n", GetID(), (DWORD) GetUID()));
		return false;
	}

	if ( m_pRegion == NULL )
	{
		RESOURCE_ID rid;
		rid.SetPrivateUID( GetUID());
		m_pRegion = new CRegionWorld( rid );
	}

	// Get Background region.
	CPointMap pt = GetTopPoint();
	const CRegionWorld * pRegionBack = dynamic_cast <CRegionWorld*> (pt.GetRegion( REGION_TYPE_AREA ));
	ASSERT( pRegionBack );
	ASSERT( pRegionBack != m_pRegion );

	// Create the new region rectangle.
	CRectMap rect;
	reinterpret_cast<CGRect&>(rect) = pMultiDef->m_rect;
	rect.OffsetRect( pt.m_x, pt.m_y );
	m_pRegion->SetRegionRect( rect );
	m_pRegion->m_pt = pt;

	DWORD dwFlags;
	if ( IsType(IT_SHIP))
	{
		dwFlags = REGION_FLAG_SHIP;
	}
	else
	{
		// Houses get some of the attribs of the land around it.
		dwFlags = pRegionBack->GetRegionFlags();
	}
	dwFlags |= pMultiDef->m_dwRegionFlags;
	m_pRegion->SetRegionFlags( dwFlags );

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%s (%s)", (LPCTSTR) pRegionBack->GetName(), (LPCTSTR) GetName());
	m_pRegion->SetName(pszTemp);

	return m_pRegion->RealizeRegion();
}

void CItemMulti::MultiUnRealizeRegion()
{
	DEBUG_CHECK( IsType(IT_MULTI) || IsType(IT_SHIP) );

	if ( m_pRegion == NULL )
		return;

	m_pRegion->UnRealizeRegion();

	// find all creatures in the region and remove this from them.
	CWorldSearch Area( m_pRegion->m_pt, Multi_GetMaxDist() );
	while (true)
	{
		CChar * pChar = Area.GetChar();
		if ( pChar == NULL )
			break;
		if ( pChar->m_pArea != m_pRegion )
			continue;
		pChar->MoveToRegionReTest( REGION_TYPE_AREA );
	}
}

bool CItemMulti::Multi_CreateComponent( ITEMID_TYPE id, int dx, int dy, int dz, DWORD dwKeyCode )
{
	CItem * pItem = CreateTemplate( id );
	ASSERT(pItem);

	CPointMap pt = GetTopPoint();
	pt.m_x += dx;
	pt.m_y += dy;
	pt.m_z += dz;

	bool fNeedKey = false;

	switch ( pItem->GetType() )
	{
	case IT_KEY:	// it will get locked down with the house ?
	case IT_SIGN_GUMP:
	case IT_SHIP_TILLER:
		pItem->m_itKey.m_lockUID.SetPrivateUID( dwKeyCode );	// Set the key id for the key/sign.
		fNeedKey = true;
		break;
	case IT_DOOR:
		pItem->SetType(IT_DOOR_LOCKED);
fNeedKey = true;
		break;
	case IT_CONTAINER:
		pItem->SetType(IT_CONTAINER_LOCKED);
fNeedKey = true;
		break;
	case IT_SHIP_SIDE:
		pItem->SetType(IT_SHIP_SIDE_LOCKED);
		break;
	case IT_SHIP_HOLD:
		pItem->SetType(IT_SHIP_HOLD_LOCK);
		break;
	}

	pItem->SetAttr( ATTR_MOVE_NEVER | (m_Attr&(ATTR_MAGIC|ATTR_INVIS)));
	pItem->SetHue( GetHue());
	pItem->m_uidLink = GetUID();	// lock it down with the structure.

	if ( pItem->IsTypeLockable() || pItem->IsTypeLocked())
	{
		pItem->m_itContainer.m_lockUID.SetPrivateUID( dwKeyCode );	// Set the key id for the door/key/sign.
		pItem->m_itContainer.m_lock_complexity = 10000;	// never pickable.
	}

	pItem->MoveTo( pt );
	return( fNeedKey );
}

void CItemMulti::Multi_Create( CChar * pChar, DWORD dwKeyCode )
{
	// Create house or Ship extra stuff.
	// ARGS:
	//  dwKeyCode = set the key code for the doors/sides to this in case it's a drydocked ship.
	// NOTE: 
	//  This can only be done after the house is given location.

	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	// We are top level.
	if ( pMultiDef == NULL ||
		! IsTopLevel())
		return;

	if ( dwKeyCode == UID_CLEAR )
		dwKeyCode = GetUID();

	// ??? SetTimeout( GetDecayTime()); house decay ?

	bool fNeedKey = false;
	int iQty = pMultiDef->m_Components.GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		fNeedKey |= Multi_CreateComponent( (ITEMID_TYPE) pMultiDef->m_Components[i].m_id,
			pMultiDef->m_Components[i].m_dx,
			pMultiDef->m_Components[i].m_dy,
			pMultiDef->m_Components[i].m_dz,
			dwKeyCode );
	}

#if 0
	const CGrayMulti * pMultiMul = g_World.GetMultiItemDefs( GetDispID());
	if ( pMultiMul )
	{
		iQty = pMultiMul->GetItemCount();
		for ( int i=0; iQty--; i++ )
		{
			const CUOMultiItemRec * pMultiItem = pMultiMul->GetItem(i);
			ASSERT(pMultiItem);
			if ( pMultiItem->m_visible )	// client side.
				continue;
			fNeedKey |= Multi_CreateComponent( pMultiItem->GetDispID(),
				pMultiItem->m_dx,
				pMultiItem->m_dy,
				pMultiItem->m_dz,
				dwKeyCode );
		}
	}
#endif

	CItem * pKey = NULL;
	if ( fNeedKey )
	{
		// Create the key to the door.
		ITEMID_TYPE id = IsAttr(ATTR_MAGIC) ? ITEMID_KEY_MAGIC : ITEMID_KEY_COPPER ;
		pKey = CreateScript( id, pChar );
		ASSERT(pKey);
		pKey->SetType(IT_KEY);
		if ( g_Cfg.m_fAutoNewbieKeys )
			pKey->SetAttr(ATTR_NEWBIE);
		pKey->SetAttr(m_Attr&ATTR_MAGIC);
		pKey->m_itKey.m_lockUID.SetPrivateUID( dwKeyCode );
		pKey->m_uidLink = GetUID();	
	}

	Multi_GetSign();	// set the m_uidLink

	if ( pChar != NULL )
	{
		m_itShip.m_UIDCreator = pChar->GetUID();
		CItemMemory * pMemory = pChar->Memory_AddObjTypes( this, MEMORY_GUARD );

		if ( pKey )
		{
			// Put in your pack
			pChar->GetPackSafe()->ContentAdd( pKey );

			// Put dupe key in the bank.
			pKey = CreateDupeItem( pKey );
			pChar->GetBank()->ContentAdd( pKey );
			pChar->SysMessage( "The duplicate key is in your bank account" );
		}
	}
	else
	{
		// Just put the key on the front step ?
		DEBUG_CHECK( 0 );
	}
}

bool CItemMulti::Multi_IsPartOf( const CItem * pItem ) const
{
	// Assume it is in my area test already.
	// IT_MULTI
	// IT_SHIP
	ASSERT( pItem );
	if ( pItem == this )
		return( true );
	return ( pItem->m_uidLink == GetUID());
}

int CItemMulti::Multi_IsComponentOf( const CItem * pItem ) const
{
	// NOTE: Doors can actually move so this won't work for them !

	ASSERT(pItem);
	if ( ! Multi_IsPartOf( pItem ))
		return( -1 );

	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	if ( pMultiDef == NULL )
		return( -1 );

	CPointMap pt = pItem->GetTopPoint();

	int xdiff = pt.m_x - GetTopPoint().m_x ;
	int ydiff = pt.m_y - GetTopPoint().m_y;
	int iQty = pMultiDef->m_Components.GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		if ( xdiff != pMultiDef->m_Components[i].m_dx ||
			ydiff != pMultiDef->m_Components[i].m_dy )
			continue;
		return( i );
	}
	
	return( -1 );
}

CItem * CItemMulti::Multi_FindItemComponent( int iComp ) const
{
	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	if ( pMultiDef == NULL )
		return( NULL );

	return( NULL );
}

CItem * CItemMulti::Multi_FindItemType( IT_TYPE type ) const
{
	// Find a part of this multi nearby.
	if ( ! IsTopLevel())
		return( NULL );

	CWorldSearch Area( GetTopPoint(), Multi_GetMaxDist() );
	while (true)
	{
		CItem * pItem = Area.GetItem();
		if ( pItem == NULL )
			return( NULL );
		if ( ! Multi_IsPartOf( pItem ))
			continue;
		if ( pItem->IsType( type ))
			return( pItem );
	}
}

bool CItemMulti::OnTick()
{
	if ( IsType(IT_SHIP))
	{
		// Ships move on their tick.
		if ( Ship_OnMoveTick())
			return true;
	}

	return true;
}

void CItemMulti::OnMoveFrom()
{
	// Being removed from the top level.
	// Might just be moving.

	ASSERT( m_pRegion );
	m_pRegion->UnRealizeRegion();
}

bool CItemMulti::MoveTo( CPointMap pt ) // Put item on the ground here.
{
	// Move this item to it's point in the world. (ground/top level)
	if ( ! CItem::MoveTo(pt))
		return false;

	// Multis need special region info to track when u are inside them.
	// Add new region info.
	MultiRealizeRegion();
	return( true );
}

CItem * CItemMulti::Multi_GetSign()
{
	// Get my sign or tiller link.
	CItem * pTiller = m_uidLink.ItemFind();
	if ( pTiller == NULL )
	{
		pTiller = Multi_FindItemType( IsType(IT_SHIP) ? IT_SHIP_TILLER : IT_SIGN_GUMP );
		if ( pTiller == NULL )
			return( this );
		m_uidLink = pTiller->GetUID();
	}
	return( pTiller );
}

bool CItemMulti::Multi_DeedConvert( CChar * pChar )
{
	// Turn a multi back into a deed. 
	// If it has chests with stuff inside then refuse to do so ?
	// This item specifically will morph into a deed. (so keys will change!)

	CWorldSearch Area( GetTopPoint(), Multi_GetMaxDist() );
	while (true)
	{
		CItem * pItem = Area.GetItem();
		if ( pItem == NULL )
			break;
		if ( ! Multi_IsPartOf( pItem ))
			continue;
		const CItemContainer* pCont = dynamic_cast <const CItemContainer*>(pItem);
		if ( pCont == NULL )
			continue;
		if ( pCont->IsEmpty())
			continue;
		// Can't re-deed this with the container full.
		if ( pChar )
		{
			pChar->SysMessage( "Containers are not empty" );
		}
		return( false );
	}

	// Save the key code for the multi.

	return( false );
}

bool CItem::Ship_Plank( bool fOpen )
{
	// IT_SHIP_PLANK to IT_SHIP_SIDE and IT_SHIP_SIDE_LOCKED
	// This item is the ships plank.

	CItemBase * pItemDef = Item_GetDef();
	ITEMID_TYPE idState = (ITEMID_TYPE) RES_GET_INDEX( pItemDef->m_ttShipPlank.m_idState );
	if ( ! idState )
	{
		// Broken ?
		return( false );
	}
	if ( IsType(IT_SHIP_PLANK))
	{
		if ( fOpen )
			return( true );
	}
	else
	{
		DEBUG_CHECK( IsType(IT_SHIP_SIDE) || IsType(IT_SHIP_SIDE_LOCKED));
		if ( ! fOpen )
			return( true );
	}

	SetID( idState );
	Update();
	return( true );
}

void CItemMulti::Ship_Stop()
{
	// Make sure we have stopped.
	m_itShip.m_fSail = false;
	SetTimeout( -1 );
}

bool CItemMulti::Ship_SetMoveDir( DIR_TYPE dir )
{
	// Set the direction we will move next time we get a tick.
	int iSpeed = 1;
	if ( m_itShip.m_DirMove == dir && m_itShip.m_fSail )
	{
		if ( m_itShip.m_DirFace == m_itShip.m_DirMove &&
			m_itShip.m_fSail == 1 )
		{
			iSpeed = 2;
		}
		else return( false );
	}

	if ( ! IsAttr(ATTR_MAGIC ))	// make sound.
	{
		if ( ! Calc_GetRandVal(10))
		{
			Sound( Calc_GetRandVal(2)?0x12:0x13 );
		}
	}

	m_itShip.m_DirMove = dir;
	m_itShip.m_fSail = iSpeed;
	GetTopSector()->SetSectorWakeStatus();	// may get here b4 my client does.
	SetTimeout(( m_itShip.m_fSail == 1 ) ? 1*TICK_PER_SEC : (TICK_PER_SEC/5));
	return( true );
}

#define MAX_MULTI_LIST_OBJS 128

int CItemMulti::Ship_ListObjs( CObjBase ** ppObjList )
{
	// List all the objects in the structure.
	// Move the ship and everything on the deck
	// If too much stuff. then some will fall overboard. hehe.

	if ( ! IsTopLevel())
		return 0;

	int iMaxDist = Multi_GetMaxDist();

	// always list myself first. All other items must see my new region !
	int iCount = 0;
	ppObjList[iCount++] = this;

	CWorldSearch AreaChar( GetTopPoint(), iMaxDist );
	while ( iCount < MAX_MULTI_LIST_OBJS )
	{
		CChar * pChar = AreaChar.GetChar();
		if ( pChar == NULL )
			break;
		if ( pChar->IsClient())
		{
			pChar->GetClient()->addPause();	// get rid of flicker. for anyone even seeing this.
		}
		if ( ! m_pRegion->IsInside2d( pChar->GetTopPoint()))
			continue;
		int zdiff = pChar->GetTopZ() - GetTopZ();
		if ( abs( zdiff ) > 3 )
			continue;
		ppObjList[iCount++] = pChar;
	}

	CWorldSearch AreaItem( GetTopPoint(), iMaxDist );
	while ( iCount < MAX_MULTI_LIST_OBJS )
	{
		CItem * pItem = AreaItem.GetItem();
		if ( pItem == NULL )
			break;
		if ( pItem == this )	// already listed.
			continue;
		if ( ! Multi_IsPartOf( pItem ))
		{
			if ( ! m_pRegion->IsInside2d( pItem->GetTopPoint()))
				continue;
			if ( ! pItem->IsMovable())
				continue;
			int zdiff = pItem->GetTopZ() - GetTopZ();
			if ( abs( zdiff ) > 3 )
				continue;
		}
		ppObjList[iCount++] = pItem;
	}
	return( iCount );
}

bool CItemMulti::Ship_MoveDelta( CPointBase pdelta )
{
	// Move the ship one space in some direction.

	ASSERT( m_pRegion->m_iLinkedSectors );

	int znew = GetTopZ() + pdelta.m_z;
	if ( pdelta.m_z > 0 )
	{
		if ( znew >= (UO_SIZE_Z - PLAYER_HEIGHT )-1 )
			return( false );
	}
	else if ( pdelta.m_z < 0 )
	{
		if ( znew <= (UO_SIZE_MIN_Z + 3 ))
			return( false );
	}

	// Move the ship and everything on the deck
	CObjBase * ppObjs[MAX_MULTI_LIST_OBJS+1];
	int iCount = Ship_ListObjs( ppObjs );

	for ( int i=0; i <iCount; i++ )
	{
		CObjBase * pObj = ppObjs[i];
		ASSERT(pObj);
		CPointMap pt = pObj->GetTopPoint();
		pt += pdelta;
		if ( ! pt.IsValidPoint())  // boat goes out of bounds !
		{
			DEBUG_ERR(( "Ship uid=0%x out of bounds\n", (DWORD) GetUID()));
			continue;
		}
		pObj->MoveTo( pt );
		if ( pObj->IsChar())
		{
			ASSERT( m_pRegion->m_iLinkedSectors );
			pObj->RemoveFromView(); // Get rid of the blink/walk
			pObj->Update();
		}
	}

	return( true );
}

bool CItemMulti::Ship_CanMoveTo( const CPointMap & pt ) const
{
	// Can we move to the new location ? all water type ?
	if ( IsAttr(ATTR_MAGIC ))
		return( true );

	// Run into other ships ? ignore my own region.
/*
	const CRegionBase * pRegionOther = pt.GetRegion( REGION_TYPE_MULTI );
	if ( pRegionOther == m_pRegion )
		pRegionOther = NULL;
*/
	WORD wBlockFlags = CAN_I_WATER;
	signed char z = g_World.GetHeightPoint( pt, wBlockFlags, true );
	if ( ! ( wBlockFlags & CAN_I_WATER ))
	{
		return( false );
	}

	return( true );
}

static const DIR_TYPE sm_Ship_FaceDir[] =
{
	DIR_N,
	DIR_E,
	DIR_S,
	DIR_W,
};

bool CItemMulti::Ship_Face( DIR_TYPE dir )
{
	// Change the direction of the ship.

	ASSERT( IsTopLevel());
	DEBUG_CHECK( m_pRegion );
	if ( m_pRegion == NULL ) 
	{
		return false;
	}

	int i=0;
	for ( ; true; i++ )
	{
		if ( i >= COUNTOF(sm_Ship_FaceDir))
			return( false );
		if ( dir == sm_Ship_FaceDir[i] )
			break;
	}

	int iFaceOffset = Ship_GetFaceOffset();
	ITEMID_TYPE idnew = (ITEMID_TYPE) ( GetID() - iFaceOffset + i );
	const CItemBaseMulti * pMultiNew = Multi_GetDef( idnew );
	if ( pMultiNew == NULL )
	{
		return false;
	}

	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	ASSERT( pMultiDef);

	int iTurn = dir - sm_Ship_FaceDir[ iFaceOffset ];

	// ?? Are there blocking items in the way of the turn ?
	// CRectMap

	// Reorient everything on the deck
	CObjBase * ppObjs[MAX_MULTI_LIST_OBJS+1];
	int iCount = Ship_ListObjs( ppObjs );
   int i2 = 0;
	for ( i=0; i<iCount; i++ )
	{
		CObjBase * pObj = ppObjs[i];
		CPointMap pt = pObj->GetTopPoint();

		if ( pObj->IsItem())
		{
			CItem * pItem = STATIC_CAST <CItem*> (pObj);
			ASSERT( pItem );
			if ( pItem == this )
			{
				m_pRegion->UnRealizeRegion();
				pItem->SetID(idnew);
				// Adjust the region to be the new shape/area.
				MultiRealizeRegion();
				pItem->Update();
				continue;
			}

			// Is this a ship component ? transform it.
         
			//int i = Multi_IsComponentOf( pItem );
			if ( Multi_IsPartOf( pItem ) ) //i>=0 )
			{
				pItem->SetDispID( pMultiNew->m_Components[i2].m_id );
				pt = GetTopPoint();
				pt.m_x += pMultiNew->m_Components[i2].m_dx;
				pt.m_y += pMultiNew->m_Components[i2].m_dy;
				pt.m_z += pMultiNew->m_Components[i2].m_dz;
				pItem->MoveTo( pt );
            i2++;
				continue;
			}
		}

		// -2,6 = left.
		// +2,-6 = right.
		// +-4 = flip around

		int iTmp;
		int xdiff = GetTopPoint().m_x - pt.m_x;
		int ydiff = GetTopPoint().m_y - pt.m_y;
		switch ( iTurn )
		{
		case 2: // right
		case (2-DIR_QTY):
			iTmp = xdiff;
			xdiff = ydiff;
			ydiff = -iTmp;
			break;
		case -2: // left.
		case (DIR_QTY-2):
			iTmp = xdiff;
			xdiff = -ydiff;
			ydiff = iTmp;
			break;
		default: // u turn.
			xdiff = -xdiff;
			ydiff = -ydiff;
			break;
		}
		pt.m_x = GetTopPoint().m_x + xdiff;
		pt.m_y = GetTopPoint().m_y + ydiff;
		pObj->MoveTo( pt );

		if ( pObj->IsChar())
		{
			// Turn creatures as well.
			CChar * pChar = STATIC_CAST <CChar*> (pObj);
			ASSERT(pChar);
			pChar->m_dirFace = GetDirTurn( pChar->m_dirFace, iTurn );
			pChar->RemoveFromView();
			pChar->Update();
		}
	}

	m_itShip.m_DirFace = dir;
	return( true );
}

bool CItemMulti::Ship_Move( DIR_TYPE dir )
{
	if ( dir >= DIR_QTY )
		return( false );

	if ( m_pRegion == NULL )
	{
		DEBUG_ERR(( "Ship bad region\n" ));
		return false;
	}

	CPointMap ptDelta;
	ptDelta.ZeroPoint();
	ptDelta.Move( dir );

	CPointMap ptForePrv = m_pRegion->GetRegionCorner(dir);
	CPointMap ptFore = ptForePrv;
	ptFore.Move( dir );
	ptFore.m_z = GetTopZ();

	if ( ! ptFore.IsValidPoint() ||
		( ptForePrv.m_x < UO_SIZE_X_REAL && ptFore.m_x >= UO_SIZE_X_REAL && ( ptFore.m_map <= 1 )))
	{
		// Circle the globe
		// Fall off edge of world ?
		CItem * pTiller = Multi_GetSign();
		ASSERT(pTiller);
		pTiller->Speak( "Turbulent waters Cap'n", 0, TALKMODE_SAY, FONT_NORMAL );

		// Compute the new safe place to go. (where we will not overlap)
		int iMaxDist = Multi_GetMaxDist();

/*		if ( ptFore.m_x <= 0 )
		{
			
		}
		else if ( ptFore.m_x >= UO_SIZE_X_REAL )
		{
			
		}
		else if ( ptFore.m_y <= 0 )
		{
			
		}
		else if ( ptFore.m_y >= UO_SIZE_Y )
		{
			
		}

*/		return false;
	}

	// We should check all relevant corners.
	if ( ! Ship_CanMoveTo( ptFore ))
	{
cantmove:
		CItem * pTiller = Multi_GetSign();
		ASSERT(pTiller);
		pTiller->Speak( "We've stopped Cap'n", 0, TALKMODE_SAY, FONT_NORMAL );
		return false;
	}

	// left side
	CPointMap ptTmp = m_pRegion->GetRegionCorner(GetDirTurn(dir,-1));
	ptTmp.Move( dir );
	ptTmp.m_z = GetTopZ();
	if ( ! Ship_CanMoveTo( ptTmp ))
		goto cantmove;

	// right side.
	ptTmp = m_pRegion->GetRegionCorner(GetDirTurn(dir,+1));
	ptTmp.Move( dir );
	ptTmp.m_z = GetTopZ();
	if ( ! Ship_CanMoveTo( ptTmp ))
		goto cantmove;

	Ship_MoveDelta( ptDelta );

	// Move again
	GetTopSector()->SetSectorWakeStatus();	// may get here b4 my client does.
	return true;
}

bool CItemMulti::Ship_OnMoveTick()
{
	// We just got a move tick.
	// RETURN: false = delete the boat.

	if ( ! m_itShip.m_fSail )	// decay the ship instead ???
		return( true );

	// Calculate the leading point.
	DIR_TYPE dir = (DIR_TYPE) m_itShip.m_DirMove;
	if ( ! Ship_Move( dir ))
	{
		Ship_Stop();
		return( true );
	}

	SetTimeout(( m_itShip.m_fSail == 1 ) ? 1*TICK_PER_SEC : (TICK_PER_SEC/2));
	return( true );
}

void CItemMulti::OnHearRegion( LPCTSTR pszCmd, CChar * pSrc )
{
	// IT_SHIP or IT_MULTI

	const CItemBaseMulti * pMultiDef = Multi_GetDef();
	if ( pMultiDef == NULL )
		return;
	TALKMODE_TYPE		mode	= TALKMODE_SAY;

	for ( int i=0; i<pMultiDef->m_Speech.GetCount(); i++ )
	{
		CResourceLink * pLink = pMultiDef->m_Speech[i];
		ASSERT(pLink);
		CResourceLock s;
		if ( ! pLink->ResourceLock( s ))
			continue;
		DEBUG_CHECK( pLink->HasTrigger(XTRIG_UNKNOWN));
		TRIGRET_TYPE iRet = OnHearTrigger( s, pszCmd, pSrc, mode );
		if ( iRet == TRIGRET_ENDIF || iRet == TRIGRET_RET_FALSE )
			continue;
		break;
	}
}

enum
{
	SHV_MULTICREATE,
	SHV_SHIPANCHORDROP,
	SHV_SHIPANCHORRAISE,
	SHV_SHIPBACK,
	SHV_SHIPBACKLEFT,
	SHV_SHIPBACKRIGHT,
	SHV_SHIPDOWN,
	SHV_SHIPDRIFTLEFT,
	SHV_SHIPDRIFTRIGHT,
	SHV_SHIPFACE,
	SHV_SHIPFORE,
	SHV_SHIPFORELEFT,
	SHV_SHIPFORERIGHT,
	SHV_SHIPGATE,
	SHV_SHIPLAND,
	SHV_SHIPMOVE,
	SHV_SHIPSTOP,
	SHV_SHIPTURN,
	SHV_SHIPTURNLEFT,
	SHV_SHIPTURNRIGHT,
	SHV_SHIPUP,
	SHV_QTY,
};

LPCTSTR const CItemMulti::sm_szVerbKeys[SHV_QTY+1] =
{
	"MULTICREATE",
	"SHIPANCHORDROP",
	"SHIPANCHORRAISE",
	"SHIPBACK",
	"SHIPBACKLEFT",
	"SHIPBACKRIGHT",
	"SHIPDOWN",		// down one space.
	"SHIPDRIFTLEFT",
	"SHIPDRIFTRIGHT",
	"SHIPFACE",		// set the ships facing direction.
	"SHIPFORE",
	"SHIPFORELEFT",
	"SHIPFORERIGHT",
	"SHIPGATE",		// Moves the whole ship to some new point location.
	"SHIPLAND",
	"SHIPMOVE",		// move in a specified direction.
	"SHIPSTOP",
	"SHIPTURN",
	"SHIPTURNLEFT",
	"SHIPTURNRIGHT",
	"SHIPUP",		// up one space.
	NULL,
};

bool CItemMulti::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	// COMP(x).

	if ( ! strnicmp( pszKey, "COMP(", 4 ))
	{
		pszKey += 5;
		int i = Exp_GetVal(pszKey);
		SKIP_SEPERATORS(pszKey);
		pRef = Multi_FindItemComponent(i);
		return( true );
	}
	if ( ! strnicmp( pszKey, "REGION", 6 ))
	{
		pszKey += 6;
		SKIP_SEPERATORS(pszKey);
		pRef = m_pRegion;
		return( true );
	}

	return( CItem::r_GetRef( pszKey, pRef ));
}

void CItemMulti::r_DumpVerbKeys( CTextConsole * pSrc )
{
	r_DumpKeys(pSrc,sm_szVerbKeys);
	CItem::r_DumpVerbKeys(pSrc);
}

bool CItemMulti::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	LOCKDATA;
	EXC_TRY(("r_Verb('%s %s',%x)", s.GetKey(), s.GetArgStr(), pSrc));
	// Speaking in this ships region.
	// return: true = command for the ship.

	//"One (direction*)", " (Direction*), one" Moves ship one tile in desired direction and stops.
	//"Slow (direction*)" Moves ship slowly in desired direction (see below for possible directions).

	int iCmd = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF( sm_szVerbKeys )-1 );
	if ( iCmd < 0 )
	{
		return( CItem::r_Verb( s, pSrc ));
	}

	if ( ! pSrc )
		return( false );

	switch ( iCmd )
	{
	case SHV_MULTICREATE:
		{
			CGrayUID	uid( s.GetArgVal() );
			CChar *	pSrc	= uid.CharFind();
			Multi_Create( pSrc, 0 );
		}
		return true;
	}

	if ( IsAttr(ATTR_MOVE_NEVER) || ! IsTopLevel() )
		return false;

	CChar * pChar = pSrc->GetChar();

	// Only key holders can command the ship ???
	// if ( pChar && pChar->ContentFindKeyFor( pItem ))

	// Find the tiller man object.
	CItem * pTiller = Multi_GetSign();
	ASSERT( pTiller );

	// Get current facing dir.
	DIR_TYPE DirFace = sm_Ship_FaceDir[ Ship_GetFaceOffset() ];
	int DirMoveChange;
	LPCTSTR pszSpeak = NULL;

	switch ( iCmd )
	{
	case SHV_SHIPSTOP:
		// "Furl sail"
		// "Stop" Stops current ship movement.
		if ( ! m_itShip.m_fSail )
			return( false );
		Ship_Stop();
		break;
	case SHV_SHIPFACE:
		// Face this direction. do not change the direction of movement.
		if ( ! s.HasArgs())
			return( false );
		return Ship_Face( GetDirStr( s.GetArgStr()));

	case SHV_SHIPMOVE:
		// Move one space in this direction.
		// Does NOT protect against exploits !
		if ( ! s.HasArgs())
			return( false );
		m_itShip.m_DirMove = GetDirStr( s.GetArgStr());
		return Ship_Move( (DIR_TYPE) m_itShip.m_DirMove );

	case SHV_SHIPGATE:
		// Move the whole ship and contents to another place.
		if ( ! s.HasArgs())
			return( false );
		{
			CPointMap ptdelta = g_Cfg.GetRegionPoint( s.GetArgStr());
			if ( ! ptdelta.IsValidPoint())
				return( false );
			ptdelta -= GetTopPoint();
			return Ship_MoveDelta( ptdelta );
		}
		break;
	case SHV_SHIPTURNLEFT:
		// "Port",
		// "Turn Left",
		DirMoveChange = -2;
doturn:
		if ( m_itShip.m_fAnchored )
		{
anchored:
			pszSpeak = "The anchor is down <SEX Sir/Mam>!";
			break;
		}
		m_itShip.m_DirMove = GetDirTurn( DirFace, DirMoveChange );
		Ship_Face( (DIR_TYPE) m_itShip.m_DirMove );
		break;
	case SHV_SHIPTURNRIGHT:
		// "Turn Right",
		// "Starboard",	// Turn Right
		DirMoveChange = 2;
		goto doturn;
	case SHV_SHIPDRIFTLEFT: 
		// "Left",
		// "Drift Left",
		DirMoveChange = -2;
dodirmovechange:
		if ( m_itShip.m_fAnchored )
			goto anchored;
		if ( ! Ship_SetMoveDir( GetDirTurn( DirFace, DirMoveChange )))
			return( false );
		break;
	case SHV_SHIPDRIFTRIGHT: 
		// "Right",
		// "Drift Right",
		DirMoveChange = 2;
		goto dodirmovechange;
	case SHV_SHIPBACK: 
		// "Back",			// Move ship backwards
		// "Backward",		// Move ship backwards
		// "Backwards",	// Move ship backwards
		DirMoveChange = 4;
		goto dodirmovechange;
	case SHV_SHIPFORE:
		// "Forward"
		// "Foreward",		// Moves ship forward.
		// "Unfurl sail",	// Moves ship forward.
		DirMoveChange = 0;
		goto dodirmovechange;
	case SHV_SHIPFORELEFT: // "Forward left",
		DirMoveChange = -1;
		goto dodirmovechange;
	case SHV_SHIPFORERIGHT: // "forward right",
		DirMoveChange = 1;
		goto dodirmovechange;
	case SHV_SHIPBACKLEFT:
		// "backward left",
		// "back left",
		DirMoveChange = -3;
		goto dodirmovechange;
	case SHV_SHIPBACKRIGHT:
		// "backward right",
		// "back right",
		DirMoveChange = 3;
		goto dodirmovechange;
	case SHV_SHIPANCHORRAISE: // "Raise Anchor",
		if ( ! m_itShip.m_fAnchored )
		{
			pszSpeak = "The anchor is already up <SEX Sir/Mam>";
			break;
		}
		m_itShip.m_fAnchored = false;
		break;
	case SHV_SHIPANCHORDROP: // "Drop Anchor",
		if ( m_itShip.m_fAnchored )
		{
			pszSpeak = "The anchor is already down <SEX Sir/Mam>";
			break;
		}
		m_itShip.m_fAnchored = true;
		Ship_Stop();
		break;
	case SHV_SHIPTURN:
		//	"Turn around",	// Turns ship around and proceeds.
		// "Come about",	// Turns ship around and proceeds.
		DirMoveChange = 4;
		goto doturn;
	case SHV_SHIPUP: // "Up"
		{
			if ( ! IsAttr(ATTR_MAGIC ))
				return( false );

			CPointMap pt;
			pt.m_z = PLAYER_HEIGHT;
			if ( Ship_MoveDelta( pt ))
			{
				pszSpeak = "As you command <SEX Sir/Mam>";
			}
			else
			{
				pszSpeak = "Can't do that <SEX Sir/Mam>";
			}
		}
		break;
	case SHV_SHIPDOWN: // "Down"
		{
			if ( ! IsAttr(ATTR_MAGIC ))
				return( false );
			CPointMap pt;
			pt.m_z = -PLAYER_HEIGHT;
			if ( Ship_MoveDelta( pt ))
			{
				pszSpeak = "As you command <SEX Sir/Mam>";
			}
			else
			{
				pszSpeak = "Can't do that <SEX Sir/Mam>";
			}
		}
		break;
	case SHV_SHIPLAND: // "Land"
		{
			if ( ! IsAttr(ATTR_MAGIC ))
				return( false );
			signed char zold = GetTopZ();
			CPointMap pt = GetTopPoint();
			pt.m_z = zold;
			SetTopZ( -UO_SIZE_Z );	// bottom of the world where i won't get in the way.
			WORD wBlockFlags = CAN_I_WATER;
			signed char z = g_World.GetHeightPoint( pt, wBlockFlags );
			SetTopZ( zold );	// restore z for now.
			pt.InitPoint();
			pt.m_z = z - zold;
			if ( pt.m_z )
			{
				Ship_MoveDelta( pt );
				pszSpeak = "As you command <SEX Sir/Mam>";
			}
			else
			{
				pszSpeak = "We have landed <SEX Sir/Mam>";
			}
		}
		break;
	default:
		return( false );
	}

	if ( pChar )
	{
		if ( pszSpeak == NULL )
		{
			static LPCTSTR const sm_pszAye[] =
			{
				"Aye",
				"Aye Cap'n",
				"Aye <SEX Sir/Mam>",
			};
			pszSpeak = sm_pszAye[ Calc_GetRandVal( COUNTOF( sm_pszAye )) ];
		}

		TCHAR szText[ MAX_TALK_BUFFER ];
		strcpy( szText, pszSpeak );
		pChar->ParseText( szText, &g_Serv );
		pTiller->Speak( szText, 0, TALKMODE_SAY, FONT_NORMAL );
	}
	return false;
	EXC_CATCH("CItemMulti");
	return true;
}

void CItemMulti::r_DumpLoadKeys( CTextConsole * pSrc )
{
	CItem::r_DumpLoadKeys(pSrc);
}

void CItemMulti::r_Write( CScript & s )
{
	CItem::r_Write(s);
	if ( m_pRegion )
	{
		m_pRegion->r_WriteBody( s, "REGION." );
	}
}
bool CItemMulti::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	if ( !strnicmp(pszKey, "COMP", 4) )
	{
		const CItemBaseMulti *pMultiDef = Multi_GetDef();
		pszKey += 4;

		// no component uid
		if ( !*pszKey )	sVal.FormatVal(pMultiDef->m_Components.GetCount());
		else if ( *pszKey == '.' )
		{
			CMultiComponentItem	item;

			SKIP_SEPERATORS(pszKey);
			int iQty = Exp_GetVal(pszKey);

			if (( iQty < 0 ) || ( iQty >= pMultiDef->m_Components.GetCount())) return false;
			SKIP_SEPERATORS(pszKey);
			item = pMultiDef->m_Components.GetAt(iQty);

			if ( !strnicmp(pszKey, "ID", 2) ) sVal.FormatVal(item.m_id);
			else if ( !strnicmp(pszKey, "DX", 2) ) sVal.FormatVal(item.m_dx);
			else if ( !strnicmp(pszKey, "DY", 2) ) sVal.FormatVal(item.m_dy);
			else if ( !strnicmp(pszKey, "DZ", 2) ) sVal.FormatVal(item.m_dz);
			else if ( !strnicmp(pszKey, "D", 1) ) sVal.Format("%i,%i,%i", item.m_dx, item.m_dy, item.m_dz);
			else sVal.Format("%i,%i,%i,%i", item.m_id, item.m_dx, item.m_dy, item.m_dz);
		}
		else return false;
		return true;
	}
	return( CItem::r_WriteVal(pszKey, sVal, pSrc));
}

bool CItemMulti::r_LoadVal( CScript & s  )
{
	LOCKDATA;
	EXC_TRY(("r_LoadVal('%s %s')", s.GetKey(), s.GetArgStr()));
	if ( s.IsKeyHead( "REGION.", 7 ))
	{
		if ( ! IsTopLevel())
		{
			MoveTo( GetTopPoint()); // Put item on the ground here.
		}
		ASSERT( m_pRegion );
		CScript script( s.GetKey()+7, s.GetArgStr());
		return( m_pRegion->r_LoadVal( script ) );
	}
	return CItem::r_LoadVal(s);
	EXC_CATCH("CItemMulti");
	return false;
}
void CItemMulti::DupeCopy( const CItem * pItem )
{
	CItem::DupeCopy(pItem);
}

