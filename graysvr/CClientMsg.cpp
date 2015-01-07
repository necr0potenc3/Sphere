//
// CClientMsg.cpp
// Copyright Menace Software (www.menasoft.com).
//
// Game server messages. (No login stuff)
//

#include "graysvr.h"	// predef header.
#include "CClient.h"

/////////////////////////////////////////////////////////////////
// -CClient stuff.

void CClient::addPause( bool fPause )
{
	// 0 = restart. 1=pause
	DEBUG_CHECK( this );
	if ( this == NULL )
		return;
	if ( m_pChar == NULL )
		return;
	if ( m_fPaused == fPause )
		return;

	if ( ! fPause )
		UpdateStats();
/*
	{
		if ( m_fUpdateStats )
		{
			// update all my own stats.
			addCharStatWindow( m_pChar->GetUID());
		}
	}
*/
	if (IsSetEF(EF_No_Pause_Packet))
		return;

	m_fPaused = fPause;
	CCommand cmd;
	cmd.Pause.m_Cmd = XCMD_Pause;
	cmd.Pause.m_Arg = fPause;
	xSendPkt( &cmd, sizeof( cmd.Pause ));
}

bool CClient::addDeleteErr( DELETE_ERR_TYPE code )
{
	// code
	if ( code == DELETE_SUCCESS )
		return true;

	DEBUG_ERR(( "%x:Bad Char Delete Attempted %d\n", m_Socket.GetSocket(), code ));

	CCommand cmd;
	cmd.DeleteBad.m_Cmd = XCMD_DeleteBad;
	cmd.DeleteBad.m_code = code;
	xSendPkt( &cmd, sizeof( cmd.DeleteBad ));
	xFlush();
	return( false );
}

void CClient::addExtData( EXTDATA_TYPE type, const CExtData * pData, int iSize)
{
	CCommand cmd;
	int iSizeTotal = iSize + sizeof(cmd.ExtData) - sizeof(cmd.ExtData.m_u);
	cmd.ExtData.m_Cmd = XCMD_ExtData;
	cmd.ExtData.m_len = iSizeTotal;
	cmd.ExtData.m_type = type;
	memcpy( &(cmd.ExtData.m_u), pData, iSize );
	xSendPkt( &cmd, iSizeTotal );
}

void CClient::addOptions()
{
	CCommand cmd;

#if 0
	// set options ? // ??? not needed ?
	for ( int i=1; i<=3; i++ )
	{
		cmd.Options.m_Cmd = XCMD_Options;
		cmd.Options.m_len = sizeof( cmd.Options );
		cmd.Options.m_index = i;
		xSendPkt( &cmd, sizeof( cmd.Options ));
	}
#endif

	//cmd.ReDrawAll.m_Cmd = XCMD_ReDrawAll;	// ???
	//xSendPkt( &cmd, sizeof( cmd.ReDrawAll ));
}

void CClient::addTime( bool bCurrent )
{
	// Send time. (real or game time ??? why ?)
	CCommand cmd;
	
	if ( bCurrent )
	{
		long lCurrentTime = (CServTime::GetCurrentTime()).GetTimeRaw();
		cmd.Time.m_Cmd = XCMD_Time;
		cmd.Time.m_hours = ( lCurrentTime / ( 60*60*TICK_PER_SEC )) % 24;
		cmd.Time.m_min   = ( lCurrentTime / ( 60*TICK_PER_SEC )) % 60;
		cmd.Time.m_sec   = ( lCurrentTime / ( TICK_PER_SEC )) % 60;
	}
	else
	{
		cmd.Time.m_Cmd = XCMD_Time;
		cmd.Time.m_hours = 0;
		cmd.Time.m_min   = 0;
		cmd.Time.m_sec   = 0;
	}
	
	xSendPkt( &cmd, sizeof( cmd.Time ));
}

void CClient::addObjectRemoveCantSee( CGrayUID uid, LPCTSTR pszName )
{
	// Seems this object got out of sync some how.
	if ( pszName == NULL ) pszName = "it";
	SysMessagef( "You can't see %s", pszName );
	addObjectRemove( uid );
}

void CClient::addObjectRemove( CGrayUID uid )
{
	// Tell the client to remove the item or char
	DEBUG_CHECK( m_pChar );
	addPause();
	CCommand cmd;
	cmd.Remove.m_Cmd = XCMD_Remove;
	cmd.Remove.m_UID = uid;
	xSendPkt( &cmd, sizeof( cmd.Remove ));
}

void CClient::addObjectRemove( const CObjBase * pObj )
{
	addObjectRemove( pObj->GetUID());
}

void CClient::addRemoveAll( bool fItems, bool fChars )
{
	addPause();
	if ( fItems )
	{
		// Remove any multi objects first ! or client will hang
		CWorldSearch AreaItems( GetChar()->GetTopPoint(), UO_MAP_VIEW_RADAR );
		AreaItems.SetAllShow( IsPriv( PRIV_ALLSHOW ));	// show logged out chars?
		while (true)
		{
			CItem * pItem = AreaItems.GetItem();
			if ( pItem == NULL )
				break;
			addObjectRemove( pItem );
		}
	}
	if ( fChars )
	{
		// Remove any multi objects first ! or client will hang
		CWorldSearch AreaChars( GetChar()->GetTopPoint(), UO_MAP_VIEW_SIZE );
		AreaChars.SetAllShow( IsPriv( PRIV_ALLSHOW ));	// show logged out chars?
		while (true)
		{
			CChar * pChar = AreaChars.GetChar();
			if ( pChar == NULL )
				break;
			addObjectRemove( pChar );
		}
	}
}

void CClient::addObjectLight( const CObjBase * pObj, LIGHT_PATTERN iLightPattern )		// Object light level.
{
	// ??? this does not seem to work !
	if ( pObj == NULL )
		return;
	CCommand cmd;
	cmd.LightPoint.m_Cmd = XCMD_LightPoint;
	cmd.LightPoint.m_UID = pObj->GetUID();
	cmd.LightPoint.m_level = iLightPattern;	// light level/pattern.
	xSendPkt( &cmd, sizeof( cmd.LightPoint ));
}

void CClient::addItem_OnGround( CItem * pItem ) // Send items (on ground)
{
	ASSERT(pItem);
	// Get base values.
	DWORD dwUID = pItem->GetUID();
	ITEMID_TYPE wID = pItem->GetDispID();
	HUE_TYPE wHue = pItem->GetHue();
	WORD wAmount = ( pItem->GetAmount() > 1 ) ? pItem->GetAmount() : 0;
	CPointMap pt = pItem->GetTopPoint();
	BYTE bFlags = 0;
	BYTE bDir = DIR_N;

	// Modify the values for the specific client/item.
	bool fHumanCorpse = ( wID == ITEMID_CORPSE && CCharBase::IsHumanID( pItem->GetCorpseType() ));

	if ( m_pChar->IsStatFlag( STATF_Hallucinating ))
	{
		// cmd.Put.m_id = pItem->GetDispID();	// ??? random displayable item ?
		wHue = Calc_GetRandVal( HUE_DYE_HIGH );	// restrict colors
	}
	else if ( wID != ITEMID_CORPSE )
	{
		if ( wHue & HUE_UNDERWEAR )	// on monster this just colors the underwaer. thats it.
			wHue = 0;
		else if (( wHue & HUE_MASK_HI ) > HUE_QTY )
			wHue &= HUE_MASK_LO | HUE_TRANSLUCENT;
		else
			wHue &= HUE_MASK_HI | HUE_TRANSLUCENT;
	}
	else
	{
		// allow HUE_UNDERWEAR colors only on corpses
		if (( wHue & HUE_MASK_HI ) > HUE_QTY )
			wHue &= HUE_MASK_LO | HUE_UNDERWEAR | HUE_TRANSLUCENT;
		else
			wHue &= HUE_MASK_HI | HUE_UNDERWEAR | HUE_TRANSLUCENT;
	}

	if ( m_pChar->CanMove( pItem, false ))	// wID != ITEMID_CORPSE &&
	{
		bFlags |= ITEMF_MOVABLE;
	}

	if ( IsPriv(PRIV_DEBUG))
	{
		// just use safe numbers
		wID = ITEMID_WorldGem;	// bigger item ???
		pt.m_z = m_pChar->GetTopZ();
		wAmount = 0;
		bFlags |= ITEMF_MOVABLE;
	}
	else
	{
		if ( ! m_pChar->CanSeeItem( pItem ))
		{
			bFlags |= ITEMF_INVIS;
		}
		if ( pItem->Item_GetDef()->Can( CAN_I_LIGHT ))	// ??? Gate or other ? IT_LIGHT_LIT ?
		{
			if ( pItem->IsTypeLit())
			{
				bDir = pItem->m_itLight.m_pattern;
			}
			else
			{
				bDir = LIGHT_LARGE;
			}
		}
		else if ( wID == ITEMID_CORPSE )	// IsType( IT_CORPSE )
		{
			// If this item has direction facing use it
			bDir = pItem->m_itCorpse.m_facing_dir;	// DIR_N
		}
	}

	// Pack values in our strange format.
	CCommand cmd;
	BYTE * pData = cmd.m_Raw + 9;

	cmd.Put.m_Cmd = XCMD_Put;

	if ( wAmount ) dwUID |= UID_F_RESOURCE;	// Enable amount feild
	cmd.Put.m_UID = dwUID;

	cmd.Put.m_id = wID;
	if ( wAmount )
	{
		PACKWORD(pData,wAmount);
		pData += 2;
	}

	if ( bDir ) pt.m_x |= 0x8000;
	PACKWORD(pData,pt.m_x);
	pData += 2;

	if ( wHue ) pt.m_y |= 0x8000;	 // Enable m_wHue and m_movable
	if ( bFlags ) pt.m_y |= 0x4000;
	PACKWORD(pData,pt.m_y);
	pData += 2;

	if ( bDir )
	{
		pData[0] = bDir;
		pData++;
	}

	pData[0] = pt.m_z;
	pData++;

	if ( wHue )
	{
		PACKWORD(pData,wHue);
		pData += 2;
	}

	if ( bFlags )	// m_flags = ITEMF_MOVABLE (020, 080)
	{
		pData[0] = bFlags;
		pData++;
	}

	int iLen = pData - (cmd.m_Raw);
	ASSERT( iLen );
	cmd.Put.m_len = iLen;

	xSendPkt( &cmd, iLen );

	if ( pItem->IsType(IT_SOUND))
	{
		addSound( (SOUND_TYPE) pItem->m_itSound.m_Sound, pItem, pItem->m_itSound.m_Repeat );
	}

	if ( ! IsPriv(PRIV_DEBUG) && fHumanCorpse )	// cloths on corpse
	{
		CItemCorpse * pCorpse = dynamic_cast <CItemCorpse*> (pItem);
		ASSERT(pCorpse);

		// send all the items on the corpse.
		addContents( pCorpse, false, true, false );
		// equip the proper items on the corpse.
		addContents( pCorpse, true, true, false );
	}

	addAOSTooltip( pItem );
}

void CClient::addItem_Equipped( const CItem * pItem )
{
	ASSERT(pItem);
	// Equip a single item on a CChar.
	CChar * pChar = dynamic_cast <CChar*> (pItem->GetParent());
	ASSERT( pChar != NULL );
	DEBUG_CHECK( pItem->GetEquipLayer() < LAYER_SPECIAL );

	if ( ! m_pChar->CanSeeItem( pItem ) && m_pChar != pChar )
		return;

	LAYER_TYPE layer = pItem->GetEquipLayer();

	CCommand cmd;
	cmd.ItemEquip.m_Cmd = XCMD_ItemEquip;
	cmd.ItemEquip.m_UID = pItem->GetUID();
	cmd.ItemEquip.m_id = ( layer == LAYER_BANKBOX ) ? ITEMID_CHEST_SILVER : pItem->GetDispID();
	cmd.ItemEquip.m_zero7 = 0;
	cmd.ItemEquip.m_layer = layer;
	cmd.ItemEquip.m_UIDchar = pChar->GetUID();

	HUE_TYPE wHue;
	GetAdjustedItemID( pChar, pItem, wHue );
	cmd.ItemEquip.m_wHue = wHue;

	xSendPkt( &cmd, sizeof( cmd.ItemEquip ));

	addAOSTooltip( pItem );
}

void CClient::addItem_InContainer( const CItem * pItem )
{
	ASSERT(pItem);
	CItemContainer * pCont = dynamic_cast <CItemContainer*> (pItem->GetParent());
	if ( pCont == NULL )
		return;

	CPointBase pt = pItem->GetContainedPoint();

	// Add a single item in a container.
	CCommand cmd;
	cmd.ContAdd.m_Cmd = XCMD_ContAdd;
	cmd.ContAdd.m_UID = pItem->GetUID();
	cmd.ContAdd.m_id = pItem->GetDispID();
	cmd.ContAdd.m_zero7 = 0;
	cmd.ContAdd.m_amount = pItem->GetAmount();
	cmd.ContAdd.m_x = pt.m_x;
	cmd.ContAdd.m_y = pt.m_y;
	cmd.ContAdd.m_UIDCont = pCont->GetUID();

	HUE_TYPE wHue;
	if ( m_pChar->IsStatFlag( STATF_Hallucinating ))
	{
		wHue = Calc_GetRandVal( HUE_DYE_HIGH );	// restrict colors
	}
	else
	{
		wHue = pItem->GetHue() & HUE_MASK_HI;
		if ( wHue > HUE_QTY )
			wHue &= HUE_MASK_LO;
	}
	cmd.ContAdd.m_wHue = wHue;

	xSendPkt( &cmd, sizeof( cmd.ContAdd ));

	addAOSTooltip( pItem );
}

void CClient::addItem( CItem * pItem )
{
	if ( pItem == NULL )
		return;
	addPause();
	if ( pItem->IsTopLevel())
	{
		addItem_OnGround( pItem );
	}
	else if ( pItem->IsItemEquipped())
	{
		addItem_Equipped( pItem );
	}
	else if ( pItem->IsItemInContainer())
	{
		addItem_InContainer( pItem );
	}
}

int CClient::addContents( const CItemContainer * pContainer, bool fCorpseEquip, bool fCorpseFilter, bool fShop, bool fHasSkill ) // Send Backpack (with items)
{
	// NOTE: We needed to send the header for this FIRST !!!
	// 1 = equip a corpse
	// 0 = contents.

	addPause();

	bool fLayer[LAYER_HORSE];
	memset( fLayer, 0, sizeof(fLayer));

	CCommand cmd;

	// send all the items in the container.
	int count = 0;
	for ( CItem* pItem=pContainer->GetContentHead(); pItem!=NULL; pItem=pItem->GetNext())
	{
		LAYER_TYPE layer;

		if ( fHasSkill )
		{
			if ( !m_pChar->SkillResourceTest( &(pItem->Item_GetDef()->m_SkillMake), (ITEMID_TYPE) 0 ) )
				continue;
		}

		if ( fCorpseFilter )	// dressing a corpse is different from opening the coffin!
		{
			layer = (LAYER_TYPE) pItem->GetContainedLayer();
			ASSERT( layer < LAYER_HORSE );
			switch ( layer )	// don't put these on a corpse.
			{
			case LAYER_NONE:
			case LAYER_PACK:	// these display strange.
			// case LAYER_LIGHT:
				continue;
			case LAYER_HIDDEN:
				DEBUG_CHECK(0);
				continue;
			}
			// Make certain that no more than one of each layer goes out to client....crashes otherwise!!
			if ( fLayer[layer] )
			{
				DEBUG_CHECK( !fLayer[layer]);
				continue;
			}
			fLayer[layer] = true;
		}
		if ( fCorpseEquip )	// list equipped items on a corpse
		{
			ASSERT( fCorpseFilter );
			cmd.CorpEquip.m_item[count].m_layer	= pItem->GetContainedLayer();
			cmd.CorpEquip.m_item[count].m_UID	= pItem->GetUID();
		}
		else	// Content items
		{
			cmd.Content.m_item[count].m_UID		= pItem->GetUID();
			cmd.Content.m_item[count].m_id		= pItem->GetDispID();
			cmd.Content.m_item[count].m_zero6	= 0;
			cmd.Content.m_item[count].m_amount	= pItem->GetAmount();
			if ( fShop )
			{
				CItemVendable * pVendItem = dynamic_cast <CItemVendable *> (pItem);
				if ( ! pVendItem )
					continue;
				if ( ! pVendItem->GetAmount())
					continue;
				if ( pVendItem->IsType(IT_GOLD))
					continue;
				if ( pVendItem->GetAmount() > g_Cfg.m_iVendorMaxSell )
				{
					cmd.Content.m_item[count].m_amount = g_Cfg.m_iVendorMaxSell;
				}
				cmd.Content.m_item[count].m_x	= count;
				cmd.Content.m_item[count].m_y	= count;
			}
			else
			{
				CPointBase pt = pItem->GetContainedPoint();
				cmd.Content.m_item[count].m_x	= pt.m_x;
				cmd.Content.m_item[count].m_y	= pt.m_y;
			}
			cmd.Content.m_item[count].m_UIDCont	= pContainer->GetUID();

			HUE_TYPE wHue;
			if ( m_pChar->IsStatFlag( STATF_Hallucinating ))
			{
				wHue = Calc_GetRandVal( HUE_DYE_HIGH );
			}
			else
			{
				wHue = pItem->GetHue() & HUE_MASK_HI;
				if ( wHue > HUE_QTY )
					wHue &= HUE_MASK_LO;	// restrict colors
			}
			cmd.Content.m_item[count].m_wHue = wHue;
		}
		count ++;

		addAOSTooltip( pItem );
	}

	if ( ! count )
	{
		return 0;
	}
	int len;
	if ( fCorpseEquip )
	{
		cmd.CorpEquip.m_item[count].m_layer = LAYER_NONE;	// terminator.
		len = sizeof( cmd.CorpEquip ) - sizeof(cmd.CorpEquip.m_item) + ( count * sizeof(cmd.CorpEquip.m_item[0])) + 1;
		cmd.CorpEquip.m_Cmd = XCMD_CorpEquip;
		cmd.CorpEquip.m_len = len;
		cmd.CorpEquip.m_UID = pContainer->GetUID();
	}
	else
	{
		len = sizeof( cmd.Content ) - sizeof(cmd.Content.m_item) + ( count * sizeof(cmd.Content.m_item[0]));
		cmd.Content.m_Cmd = XCMD_Content;
		cmd.Content.m_len = len;
		cmd.Content.m_count = count;
	}

	xSendPkt( &cmd, len );
	return( count );
}



void CClient::addOpenGump( const CObjBase * pContainer, GUMP_TYPE gump )
{
	// NOTE: if pContainer has not already been sent to the client
	//  this will crash client.
	CCommand cmd;
	cmd.ContOpen.m_Cmd = XCMD_ContOpen;
	cmd.ContOpen.m_UID = pContainer->GetUID();
	cmd.ContOpen.m_gump = gump;

	// we automatically get open sound for this,.
	xSendPkt( &cmd, sizeof( cmd.ContOpen ));
}




bool CClient::addContainerSetup( const CItemContainer * pContainer ) // Send Backpack (with items)
{
	ASSERT(pContainer);
	DEBUG_CHECK( ! pContainer->IsWeird());
	ASSERT( pContainer->IsItem());

	// open the conatiner with the proper GUMP.
	CItemBase * pItemDef = pContainer->Item_GetDef();
	GUMP_TYPE gump = pItemDef->IsTypeContainer();
	if ( gump <= GUMP_RESERVED )
	{
		return false;
	}

	addOpenGump( pContainer, gump );
	addContents( pContainer, false, false, false );
	return( true );
}

void CClient::addSeason( SEASON_TYPE season, bool fNormalCursor )
{
	ASSERT(m_pChar);
	if ( m_pChar->IsStatFlag( STATF_DEAD ))	// everything looks like this when dead.
	{
		season = SEASON_Desolate;
	}
	if ( season == m_Env.m_Season )	// the season i saw last.
		return;

	m_Env.m_Season = season;

	CCommand cmd;
   if ( m_Crypt.GetClientVer() > 0x126040 || IsNoCryptVer(1) ) {
	   cmd.Season.m_Cmd = XCMD_Season;
	   cmd.Season.m_season = season;
	   cmd.Season.m_cursor = fNormalCursor ? 1 : 0;
	   xSendPkt( &cmd, sizeof( cmd.Season ));
   } else {
	   cmd.Season2.m_Cmd = XCMD_Season;
	   cmd.Season2.m_season = season;
	   xSendPkt( &cmd, sizeof( cmd.Season2 ));
   }
}

void CClient::addWeather( WEATHER_TYPE weather ) // Send new weather to player
{
	ASSERT( m_pChar );

	if ( g_Cfg.m_fNoWeather )
		return;

	if ( m_pChar->IsStatFlag( STATF_InDoors ))
	{
		// If there is a roof over our head at the moment then stop rain.
		weather = WEATHER_DRY;
	}
	else if ( weather == WEATHER_DEFAULT )
	{
		weather = m_pChar->GetTopSector()->GetWeather();
	}

	if ( weather == m_Env.m_Weather )
		return;

	CCommand cmd;
	cmd.Weather.m_Cmd = XCMD_Weather;
	cmd.Weather.m_type = weather;
	cmd.Weather.m_ext1 = 0x40;
	cmd.Weather.m_unk010 = 0x10;

	// m_ext = seems to control a very gradual fade in and out ?
	switch ( weather )
	{
	case WEATHER_RAIN:	// rain
	case WEATHER_STORM:
	case WEATHER_SNOW:	// snow
		// fix weird client transition problem.
		// May only transition from Dry.
		addWeather( WEATHER_DRY );
		break;
	default:	// dry or cloudy
		cmd.Weather.m_type = WEATHER_DRY;
		cmd.Weather.m_ext1 = 0;
		break;
	}

	m_Env.m_Weather = weather;
	xSendPkt( &cmd, sizeof(cmd.Weather));
}

void CClient::addLight( int iLight )
{
	// NOTE: This could just be a flash of light.
	// Global light level.
	ASSERT(m_pChar);

	if (m_pChar->m_LocalLight)
		iLight = m_pChar->m_LocalLight;

	if ( iLight < LIGHT_BRIGHT )
	{
		iLight = m_pChar->GetLightLevel();
	}

	// Scale light level for non-t2a.
	if ( iLight < LIGHT_BRIGHT )
		iLight = LIGHT_BRIGHT;
	if ( iLight > LIGHT_DARK )
		iLight = LIGHT_DARK;

	if ( IsResDisp( RDS_PRET2A ) )
	{
		iLight = IMULDIV( iLight, LIGHT_DARK_OLD, LIGHT_DARK );
	}

	if ( iLight == m_Env.m_Light )
		return;
	m_Env.m_Light = iLight;

	CCommand cmd;
	cmd.Light.m_Cmd = XCMD_Light;
	cmd.Light.m_level = iLight;
	xSendPkt( &cmd, sizeof( cmd.Light ));
}

void CClient::addArrowQuest( int x, int y )
{
	CCommand cmd;
	cmd.Arrow.m_Cmd = XCMD_Arrow;
	cmd.Arrow.m_Active = ( x && y ) ? 1 : 0;	// 1/0
	cmd.Arrow.m_x = x;
	cmd.Arrow.m_y = y;
	xSendPkt( &cmd, sizeof( cmd.Arrow ));
}

void CClient::addMusic( WORD id )
{
	// Music is ussually appropriate for the region.
	CCommand cmd;
	cmd.PlayMusic.m_Cmd = XCMD_PlayMusic;
	cmd.PlayMusic.m_musicid = id;
	xSendPkt( &cmd, sizeof( cmd.PlayMusic ));
}

bool CClient::addKick( CTextConsole * pSrc, bool fBlock )
{
	// Kick me out.
	ASSERT( pSrc );
	if ( GetAccount() == NULL )
	{
		m_fClosed	= true;
		return( true );
	}

	if ( ! GetAccount()->Kick( pSrc, fBlock ))
		return( false );

	LPCTSTR pszAction = fBlock ? "KICK" : "DISCONNECTED";
	SysMessagef( "You have been %sed by '%s'", (LPCTSTR) pszAction, (LPCTSTR) pSrc->GetName());

	if ( IsConnectTypePacket() )
	{
		CCommand cmd;
		cmd.Kick.m_Cmd = XCMD_Kick;
		cmd.Kick.m_unk1 = 0;	// The kickers uid ?
		xSendPkt( &cmd, sizeof( cmd.Kick ));
	}

	m_fClosed	= true;
	return( true );
}

void CClient::addSound( SOUND_TYPE id, const CObjBaseTemplate * pBase, int iOnce )
{
	// ARGS:
	//  iOnce = 1 = default.

	CPointMap pt;
	if ( pBase )
	{
		pBase = pBase->GetTopLevelObj();
		pt = pBase->GetTopPoint();
	}
	else
	{
		pt = m_pChar->GetTopPoint();
	}

	if ( id <= 0 || id == SOUND_CLEAR )
	{
		// Clear any recurring sounds.
		if ( ! iOnce )
		{
			// Only if far away from the last one.
			if ( ! m_pntSoundRecur.IsValidPoint())
				return;
			if ( m_pntSoundRecur.GetDist( pt ) < UO_MAP_VIEW_SIZE )
				return;
		}

		// Force a clear.
		m_pntSoundRecur.InitPoint();
	}
	else
	{
		// valid sound ?
		// if ( id > SOUND_QTY )
			// return;
		if ( ! iOnce )
		{
			// This sound should repeat.
			if ( ! pBase )
				return;
			if ( m_pntSoundRecur.IsValidPoint())
			{
				addSound( SOUND_CLEAR );
			}
			m_pntSoundRecur = pt;
		}
	}

	CCommand cmd;
	cmd.Sound.m_Cmd = XCMD_Sound;
	cmd.Sound.m_flags = iOnce;
	cmd.Sound.m_id = id;
	cmd.Sound.m_volume = 0;
	cmd.Sound.m_x = pt.m_x;
	cmd.Sound.m_y = pt.m_y;
	cmd.Sound.m_z = pt.m_z;

	xSendPkt( &cmd, sizeof(cmd.Sound));
}

void CClient::addItemDragCancel( BYTE type )
{
	// Sound seems to be automatic ???
	// addSound( 0x051 );
	CCommand cmd;
	cmd.DragCancel.m_Cmd = XCMD_DragCancel;
	cmd.DragCancel.m_type = type;
	xSendPkt( &cmd, sizeof( cmd.DragCancel ));
}

void CClient::addBarkSpeakTable( SPKTAB_TYPE index, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	// The text is really client side.
	// this is just a resource id for it.

	CCommand cmd;
	cmd.SpeakTable.m_Cmd = XCMD_SpeakTable;

	if ( pSrc == NULL || pSrc->IsItem())
	{
		cmd.SpeakTable.m_id = 0xFFFF;
	}
	else	// char id only.
	{
		const CChar * pChar = dynamic_cast <const CChar*>(pSrc);
		ASSERT(pChar);
		cmd.SpeakTable.m_id = pChar->GetDispID();
	}

	cmd.SpeakTable.m_mode = mode;		// 9 = TALKMODE_TYPE
	cmd.SpeakTable.m_wHue = wHue;		// 10-11 = HUE_TYPE.
	cmd.SpeakTable.m_font = font;		// 12-13 = FONT_TYPE
	cmd.SpeakTable.m_index = 500000 + index; // predefined message type (SPKTAB_TYPE)

	int iLen = sizeof(cmd.SpeakTable);
	int iNameLen;
	if ( pSrc == NULL )
	{
		cmd.SpeakTable.m_UID = 0xFFFFFFFF;
		//iNameLen = strcpylen( cmd.SpeakTable.m_charname, "System" );
	}
	else
	{
		cmd.SpeakTable.m_UID = pSrc->GetUID();
		iNameLen = strcpylen( cmd.SpeakTable.m_charname, pSrc->GetName(), sizeof(cmd.SpeakTable.m_charname) );
	}
	memset( cmd.SpeakTable.m_charname+iNameLen, 0, sizeof(cmd.SpeakTable.m_charname)-iNameLen );

	cmd.SpeakTable.m_len = iLen;		// 1-2 = var len size. (50)
	xSendPkt( &cmd, iLen );
}

void CClient::addBarkUNICODE( const NCHAR * pwText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	if ( pwText == NULL )
		return;

	if ( ! IsConnectTypePacket())
	{
		// Need to convert back from unicode !
		//SysMessage(pwText);
		return;
	}

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SYSTEM;
		pSrc = NULL;
	}
/*
	if ( ISINTRESOURCE(pwText))
	{
		// SPKTAB_TYPE
		// This is not really a string but just a resource ID on the client side.
		// addBarkSpeakTable( (SPKTAB_TYPE) GETINTRESOURCE(pwText), pSrc, wHue, mode, font );
		// return;
	}
*/
	CCommand cmd;
	cmd.SpeakUNICODE.m_Cmd = XCMD_SpeakUNICODE;

	// string copy unicode (til null)
	int i=0;
	for ( ; pwText[i] && i < MAX_TALK_BUFFER; i++ )
	{
		cmd.SpeakUNICODE.m_utext[i] = pwText[i];
	}
	cmd.SpeakUNICODE.m_utext[i++] = '\0';	// add for the null

	int len = sizeof(cmd.SpeakUNICODE) + (i*sizeof(NCHAR));
	cmd.SpeakUNICODE.m_len = len;
	cmd.SpeakUNICODE.m_mode = mode;		// mode = range.
	cmd.SpeakUNICODE.m_wHue = wHue;
	cmd.SpeakUNICODE.m_font = font;		// font. 3 = system message just to you !

	lang.GetStrDef(cmd.SpeakUNICODE.m_lang);

	int iNameLen;
	if ( pSrc == NULL )
	{
		cmd.SpeakUNICODE.m_UID = 0xFFFFFFFF;	// 0x01010101;
		iNameLen = strcpylen( cmd.SpeakUNICODE.m_charname, "System" );
	}
	else
	{
		cmd.SpeakUNICODE.m_UID = pSrc->GetUID();
		iNameLen = strcpylen( cmd.SpeakUNICODE.m_charname, pSrc->GetName(), sizeof(cmd.SpeakUNICODE.m_charname) );
	}
	memset( cmd.SpeakUNICODE.m_charname+iNameLen, 0, sizeof(cmd.SpeakUNICODE.m_charname)-iNameLen );

	if ( pSrc == NULL || pSrc->IsItem())
	{
		cmd.SpeakUNICODE.m_id = 0xFFFF;	// 0x0101;
	}
	else	// char id only.
	{
		const CChar * pChar = dynamic_cast <const CChar*>(pSrc);
		ASSERT(pChar);
		cmd.SpeakUNICODE.m_id = pChar->GetDispID();
	}
	xSendPkt( &cmd, len );
}


void CClient::addBarkParse( LPCTSTR pszText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	if ( !pszText )
		return;

	WORD Args[] = { wHue, font, 0 };

	if ( *pszText == '@' )
	{
		pszText++;
		if ( *pszText == '@' ) // @@ = just a @ symbol
			goto bark_default;

		const char *		s	= pszText;
		pszText		= strchr( s, ' ' );

		if ( !pszText )
			return;

/*
		HUE_TYPE	wHue1;
		FONT_TYPE	font1;

		if ( sscanf( s, "%d,%d", &wHue1, &font1 ) == 2 )
		{ wHue	= wHue1; font	= font1; }
		else if ( sscanf( s, ",%d", &font1 ) == 1 )
		{ font	= font1; }
		else if ( sscanf( s, "%d", &wHue1 ) == 1 )
		{ wHue	= wHue1; }
*/

		for ( int i = 0; ( s < pszText ) && ( i < 3 ); )
		{
			if ( *s == ',' ) // default value, skip it
			{
				i++;
				s++;
				continue;
			}
			Args[i] = Exp_GetVal( s );
			i++;

			if ( *s == ',' )
				s++;
			else
				break;	// no more args here!
		}
		pszText++;
		if ( Args[1] > FONT_QTY )
			Args[1]	= FONT_NORMAL;
	}

	if ( Args[2] )
	{
		NCHAR szBuffer[ MAX_TALK_BUFFER ];
		int iLen = CvtSystemToNUNICODE( szBuffer, COUNTOF(szBuffer), pszText, -1 );
		addBarkUNICODE( szBuffer, pSrc, (HUE_TYPE) Args[0], mode, (FONT_TYPE) Args[1], 0 );
	}
	else
bark_default:
		addBark( pszText, pSrc, (HUE_TYPE) Args[0], mode, (FONT_TYPE) Args[1] );
}



void CClient::addBark( LPCTSTR pszText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	if ( pszText == NULL )
		return;

	if ( ! IsConnectTypePacket())
	{
		SysMessage(pszText);
		return;
	}

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SYSTEM;
		pSrc = NULL;
	}

	CCommand cmd;
/*
	if ( ISINTRESOURCE(pszText))
	{
		// SPKTAB_TYPE
		// This is not really a string but just a resource ID on the client side.
		// addBarkSpeakTable( (SPKTAB_TYPE) GETINTRESOURCE(pszText), pSrc, wHue, mode, font );
		// return;
	}
*/
	cmd.Speak.m_Cmd = XCMD_Speak;
	int len = strlen(pszText);
	DEBUG_CHECK( len < MAX_TALK_BUFFER );
	len += sizeof(cmd.Speak);
	cmd.Speak.m_len = len;
	cmd.Speak.m_mode = mode;		// mode = range.
	cmd.Speak.m_wHue = wHue;
	cmd.Speak.m_font = font;		// font. 3 = system message just to you !

	int iNameLen;
	if ( pSrc == NULL )
	{
		cmd.Speak.m_UID = 0xFFFFFFFF;
		iNameLen = strcpylen( cmd.Speak.m_charname, "System" );
	}
	else
	{
		cmd.Speak.m_UID = pSrc->GetUID();
		iNameLen = strcpylen( cmd.Speak.m_charname, pSrc->GetName(), sizeof(cmd.Speak.m_charname) );
	}
	memset( cmd.Speak.m_charname+iNameLen, 0, sizeof(cmd.Speak.m_charname)-iNameLen );

	if ( pSrc == NULL || pSrc->IsItem())
	{
		cmd.Speak.m_id = 0xFFFF; // 0x0101;
	}
	else	// char id only.
	{
		const CChar * pChar = dynamic_cast <const CChar*>(pSrc);
		ASSERT(pChar);
		cmd.Speak.m_id = pChar->GetDispID();
	}
	strcpylen( cmd.Speak.m_text, pszText, MAX_TALK_BUFFER );
	xSendPkt( &cmd, len );
}

void CClient::addObjMessage( LPCTSTR pMsg, const CObjBaseTemplate * pSrc, HUE_TYPE wHue ) // The message when an item is clicked
{
	addBarkParse( pMsg, pSrc, wHue, ( pSrc == m_pChar ) ? TALKMODE_OBJ : TALKMODE_ITEM, FONT_NORMAL );
}

void CClient::addEffect( EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate * pDst, const CObjBaseTemplate * pSrc, BYTE bSpeedSeconds, BYTE bLoop, bool fExplode, DWORD color, DWORD render )
{
	// bSpeedSeconds = seconds = 0=very fast, 7=slow.
	// wHue =

	ASSERT(m_pChar);
	ASSERT( pDst );
	pDst = pDst->GetTopLevelObj();
	CPointMap ptDst = pDst->GetTopPoint();

	CCommand cmd;
	if ( color || render )
		cmd.Effect.m_Cmd = XCMD_EffectEx;
	else
		cmd.Effect.m_Cmd = XCMD_Effect;
	cmd.Effect.m_motion = motion;
	cmd.Effect.m_id = id;
	cmd.Effect.m_UID = pDst->GetUID();

	CPointMap ptSrc;
	if ( pSrc != NULL && motion == EFFECT_BOLT )
	{
		pSrc = pSrc->GetTopLevelObj();
		ptSrc = pSrc->GetTopPoint();
	}
	else
	{
		ptSrc = ptDst;
	}

	cmd.Effect.m_speed = bSpeedSeconds;		// 22= 0=very fast, 7=slow.
	cmd.Effect.m_loop = bLoop;		// 23= 0 is really long.  1 is the shortest., 6 = longer
	cmd.Effect.m_unk = 	0;		// 0x300
	cmd.Effect.m_OneDir = true;		// 26=1=point in single dir else turn.
	cmd.Effect.m_explode = fExplode;	// 27=effects that explode on impact.

	cmd.Effect.m_srcx = ptSrc.m_x;
	cmd.Effect.m_srcy = ptSrc.m_y;
	cmd.Effect.m_srcz = ptSrc.m_z;
	cmd.Effect.m_dstx = ptDst.m_x;
	cmd.Effect.m_dsty = ptDst.m_y;
	cmd.Effect.m_dstz = ptDst.m_z;

	cmd.Effect.m_hue	= color;
	cmd.Effect.m_render	= render;

	switch ( motion )
	{
	case EFFECT_BOLT:	// a targetted bolt
		if ( ! pSrc )
			return;
		cmd.Effect.m_targUID = pDst->GetUID();
		cmd.Effect.m_UID = pSrc->GetUID();	// source
		cmd.Effect.m_OneDir = false;
		cmd.Effect.m_loop = 0;	// Does not apply.
		break;

	case EFFECT_LIGHTNING:	// lightning bolt.
		break;

	case EFFECT_XYZ:	// Stay at current xyz ??? not sure about this.
		break;

	case EFFECT_OBJ:	// effect at single Object.
		break;
	}

	if ( cmd.Effect.m_Cmd == XCMD_EffectEx )
		xSendPkt( &cmd,  sizeof(cmd.Effect)  );
	else
		xSendPkt( &cmd,  sizeof(cmd.Effect) - 8 );
}


void CClient::GetAdjustedItemID( const CChar * pChar, const CItem * pItem, HUE_TYPE & wHue )
{
	// An equipped item.
	ASSERT( pChar );

	if ( m_pChar->IsStatFlag( STATF_Hallucinating ))
	{
		wHue = Calc_GetRandVal( HUE_DYE_HIGH );
	}
	else if ( pChar->IsStatFlag(STATF_Stone))
	{
		wHue = HUE_STONE;
	}
	else
	{
		wHue = pItem->GetHue();
		if (( wHue & HUE_MASK_HI ) > HUE_QTY )
			wHue &= HUE_MASK_LO | HUE_TRANSLUCENT;
		else
			wHue &= HUE_MASK_HI | HUE_TRANSLUCENT;
	}
}

void CClient::GetAdjustedCharID( const CChar * pChar, CREID_TYPE & id, HUE_TYPE & wHue )
{
	// Some clients can't see all creature artwork and colors.
	// try to do the translation here,.

	ASSERT( GetAccount() );
	ASSERT( pChar );

	if ( IsPriv(PRIV_DEBUG))
	{
		id = CREID_MAN;
		wHue = 0;
		return;
	}

	id = pChar->GetDispID();
	CCharBase * pCharDef = pChar->Char_GetDef();

	if ( m_pChar->IsStatFlag( STATF_Hallucinating )) // viewer is Hallucinating.
	{
		if ( pChar != m_pChar )
		{
			// Get a random creature from the artwork.
			id = (CREID_TYPE) Calc_GetRandVal(CREID_EQUIP_GM_ROBE);
			for ( int iTries = 0; iTries < CREID_EQUIP_GM_ROBE; iTries ++ )
			{
				if ( CCharBase::FindCharBase( id ) )
					break;
				id = (CREID_TYPE) ( id + 1 );
				if ( id >= CREID_EQUIP_GM_ROBE )
					id = (CREID_TYPE) 1;
			}
		}

		wHue = Calc_GetRandVal( HUE_DYE_HIGH );
	}
	else
	{
		if ( pChar->IsStatFlag(STATF_Stone))	// turned to stone.
		{
			wHue = HUE_STONE;
		}
		else
		{
			wHue = pChar->GetHue();
			if ( pCharDef && GetResDisp() < pCharDef->GetResLevel() )
				if ( pCharDef->m_ResDispDnHue != HUE_DEFAULT )
					wHue = pCharDef->m_ResDispDnHue;
#if 0
				wHue = pChar->GetHue();
#endif
			// allow transparency and underwear colors
			if (( wHue & HUE_MASK_HI ) > HUE_QTY )
				wHue &= HUE_MASK_LO | HUE_UNDERWEAR | HUE_TRANSLUCENT;
			else
				wHue &= HUE_MASK_HI | HUE_UNDERWEAR | HUE_TRANSLUCENT;
		}
	}

#if 0
	// IF they do not have t2a. (change that to use npc defs)
	if ( IsResDisp( RDS_PRET2A ) )
	{
		switch ( id )
		{
		case CREID_Tera_Warrior:
		case CREID_Tera_Drone:
		case CREID_Tera_Matriarch:
			id = CREID_GIANT_SNAKE;
			break;
		case CREID_Titan:
		case CREID_Cyclops:
			id = CREID_OGRE;
			break;
		case CREID_Giant_Toad:
		case CREID_Bull_Frog:
			id = CREID_GiantRat;
			break;
		case CREID_Ophid_Mage:
		case CREID_Ophid_Warrior:
		case CREID_Ophid_Queen:
			id = CREID_GIANT_SPIDER;
			break;
		case CREID_SEA_Creature:
			id = CREID_SEA_SERP;
			break;
		case CREID_LavaLizard:
			id = CREID_Alligator;
			break;
		case CREID_Ostard_Desert:
		case CREID_Ostard_Frenz:
		case CREID_Ostard_Forest:
			id = CREID_HORSE1;
			break;
		case CREID_VORTEX:
			id = CREID_AIR_ELEM;
			break;
		}
	}
#endif
	if ( pCharDef && GetResDisp() < pCharDef->GetResLevel() )
		id = (CREID_TYPE) pCharDef->m_ResDispDnId;
}

void CClient::addCharMove( const CChar * pChar )
{
	// This char has just moved on screen.
	// or changed in a subtle way like "hidden"
	// NOTE: If i have been turned this will NOT update myself.

	addPause();

	CCommand cmd;
	cmd.CharMove.m_Cmd = XCMD_CharMove;
	cmd.CharMove.m_UID = pChar->GetUID();

	CREID_TYPE id;
	HUE_TYPE wHue;
	GetAdjustedCharID( pChar, id, wHue );
	cmd.CharMove.m_id = id;
	cmd.CharMove.m_wHue = wHue;

	CPointMap pt = pChar->GetTopPoint();
	cmd.CharMove.m_x  = pt.m_x;
	cmd.CharMove.m_y  = pt.m_y;
	cmd.CharMove.m_z = pt.m_z;
	cmd.CharMove.m_dir = pChar->GetDirFlag();
	cmd.CharMove.m_mode = pChar->GetModeFlag( pChar->CanSeeTrue( m_pChar ));
	cmd.CharMove.m_noto = pChar->Noto_GetFlag( m_pChar );

	xSendPkt( &cmd, sizeof(cmd.CharMove));
}

void CClient::addChar( const CChar * pChar )
{
	// Full update about a char.

	addPause();
	// if ( pChar == m_pChar ) m_wWalkCount = -1;

	CCommand cmd;
	cmd.Char.m_Cmd = XCMD_Char;
	cmd.Char.m_UID = pChar->GetUID();

	CREID_TYPE id;
	HUE_TYPE wHue;
	GetAdjustedCharID( pChar, id, wHue );
	cmd.Char.m_id = id;
	cmd.Char.m_wHue = wHue;

	CPointMap pt = pChar->GetTopPoint();
	pt.GetSector()->SetSectorWakeStatus();	// if it can be seen then wake it.

	cmd.Char.m_x = pt.m_x;
	cmd.Char.m_y = pt.m_y;
	cmd.Char.m_z = pt.m_z;
	cmd.Char.m_dir = pChar->GetDirFlag();
	cmd.Char.m_mode = pChar->GetModeFlag( pChar->CanSeeTrue( m_pChar ));
	cmd.Char.m_noto = pChar->Noto_GetFlag( m_pChar );

	int len = ( sizeof( cmd.Char ) - sizeof( cmd.Char.equip ));
	CCommand * pCmd = &cmd;

	bool fLayer[LAYER_HORSE+1];
	memset( fLayer, 0, sizeof(fLayer));

	if ( ! pChar->IsStatFlag( STATF_Sleeping ))
	{
		// extend the current struct for all the equipped items.
		for ( CItem* pItem=pChar->GetContentHead(); pItem!=NULL; pItem=pItem->GetNext())
		{
			LAYER_TYPE layer = pItem->GetEquipLayer();
			if ( ! CItemBase::IsVisibleLayer( layer ))
				continue;
			if ( ! m_pChar->CanSeeItem( pItem ) && m_pChar != pChar )
				continue;

			// Make certain that no more than one of each layer goes out to client....crashes otherwise!!
			if ( fLayer[layer] )
			{
				DEBUG_CHECK( !fLayer[layer]);
				continue;
			}
			fLayer[layer] = true;
			addAOSTooltip( pItem );

			pCmd->Char.equip[0].m_UID = pItem->GetUID();
			pCmd->Char.equip[0].m_layer = layer;

			HUE_TYPE wHue;
			GetAdjustedItemID( pChar, pItem, wHue );
			if ( wHue )
			{
				pCmd->Char.equip[0].m_id = pItem->GetDispID() | 0x8000;	// include wHue.
				pCmd->Char.equip[0].m_wHue = wHue;
				len += sizeof(pCmd->Char.equip[0]);
				pCmd = (CCommand*)(((BYTE*)pCmd)+sizeof(pCmd->Char.equip[0]));
			}
			else
			{
				pCmd->Char.equip[0].m_id = pItem->GetDispID();
				len += sizeof(pCmd->Char.equip[0]) - sizeof(pCmd->Char.equip[0].m_wHue);
				pCmd = (CCommand*)(((BYTE*)pCmd)+sizeof(pCmd->Char.equip[0])-sizeof(pCmd->Char.equip[0].m_wHue));
			}
		}
	}

	pCmd->Char.equip[0].m_UID = 0;	// terminator.
	len += sizeof( DWORD );

	cmd.Char.m_len = len;
	xSendPkt( &cmd, len );

	addAOSTooltip( pChar );
}

void CClient::addItemName( const CItem * pItem )
{
	// NOTE: CanSee() has already been called.
	ASSERT(pItem);

	bool fIdentified = ( IsPriv(PRIV_GM) || pItem->IsAttr( ATTR_IDENTIFIED ));
	LPCTSTR pszNameFull = pItem->GetNameFull( fIdentified );

	TCHAR szName[ MAX_ITEM_NAME_SIZE + 256 ];
	int len = strcpylen( szName, pszNameFull, sizeof(szName) );

	const CContainer* pCont = dynamic_cast<const CContainer*>(pItem);
	if ( pCont != NULL )
	{
		// ??? Corpses show hair as an item !!
		len += sprintf( szName+len, " (%d items)", pCont->GetCount());
	}

	// obviously damaged ?
	else if ( pItem->IsTypeArmorWeapon())
	{
		int iPercent = pItem->Armor_GetRepairPercent();
		if ( iPercent < 50 &&
			( m_pChar->Skill_GetAdjusted( SKILL_ARMSLORE ) / 10 > iPercent ))
		{
			len += sprintf( szName+len, " (%s)", pItem->Armor_GetRepairDesc());
		}
	}

	// Show the priced value
	CItemContainer * pMyCont = dynamic_cast <CItemContainer *>( pItem->GetParent());
	if ( pMyCont != NULL && pMyCont->IsType(IT_EQ_VENDOR_BOX))
	{
		const CItemVendable * pVendItem = dynamic_cast <const CItemVendable *> (pItem);
		if ( pVendItem )
		{
			len += sprintf( szName+len, " (%d gp)",	pVendItem->GetBasePrice());
		}
	}

	HUE_TYPE wHue = HUE_TEXT_DEF;
	const CItemCorpse * pCorpseItem = dynamic_cast <const CItemCorpse *>(pItem);
	if ( pCorpseItem )
	{
		CChar * pCharCorpse = pCorpseItem->m_uidLink.CharFind();
		if ( pCharCorpse )
		{
			wHue = pCharCorpse->Noto_GetHue( m_pChar, true );
		}
	}

	if ( IsPriv( PRIV_GM ))
	{
		if ( pItem->IsAttr(ATTR_INVIS ))
		{
			len += strcpylen( szName+len, " (invis)" );
		}
		// Show the restock count
		if ( pMyCont != NULL && pMyCont->IsType(IT_EQ_VENDOR_BOX) )
		{
			len += sprintf( szName+len, " (%d restock)", pItem->GetContainedLayer());
		}
		switch ( pItem->GetType() )
		{
		case IT_ADVANCE_GATE:
			if ( pItem->m_itAdvanceGate.m_Type )
			{
				CCharBase * pCharDef = CCharBase::FindCharBase( pItem->m_itAdvanceGate.m_Type );
				len += sprintf( szName+len, " (%x=%s)", pItem->m_itAdvanceGate.m_Type, (pCharDef==NULL)?"?": pCharDef->GetTradeName());
			}
			break;
		case IT_SPAWN_CHAR:
		case IT_SPAWN_ITEM:
			{
				len += pItem->Spawn_GetName( szName + len );
			}
			break;

		case IT_TREE:
		case IT_GRASS:
		case IT_ROCK:
		case IT_WATER:
			{
			const CResourceDef * pResDef = g_Cfg.ResourceGetDef( pItem->m_itResource.m_rid_res );
			if ( pResDef)
			{
				len += sprintf( szName+len, " (%s)", pResDef->GetName());
			}
			}
			break;
		}
	}
	if ( IsPriv( PRIV_DEBUG ))
	{
		// Show UID
		len += sprintf( szName+len, " [0%lx]", (DWORD) pItem->GetUID());
	}

	addObjMessage( szName, pItem, wHue );
}

void CClient::addCharName( const CChar * pChar ) // Singleclick text for a character
{
	// Karma wHue codes ?
	ASSERT( pChar );

	HUE_TYPE wHue	= pChar->Noto_GetHue( m_pChar, true );

	TCHAR *pszTemp = Str_GetTemp();

/*
	strcpy( szTemp, pChar->Noto_GetFameTitle());

	const char *	alt	= pChar->GetKeyStr( "NAME.ALT" );

    if ( alt && *alt )
    {
        strcpy( szTemp, alt );
    }
	else
*/
	{
		LPCTSTR prefix = pChar->GetKeyStr( "NAME.PREFIX" );

		if ( ! *prefix )
			prefix = pChar->Noto_GetFameTitle();

		strcpy( pszTemp, prefix );
		strcat( pszTemp, pChar->GetName() );
		strcat( pszTemp, pChar->GetKeyStr( "NAME.SUFFIX" ) );
	}

	if ( ! pChar->IsStatFlag( STATF_Incognito ))
	{
		// Guild abbrev.
		LPCTSTR pAbbrev = pChar->Guild_AbbrevBracket(MEMORY_TOWN);
		if ( pAbbrev )
		{
			strcat( pszTemp, pAbbrev );
		}
		pAbbrev = pChar->Guild_AbbrevBracket(MEMORY_GUILD);
		if ( pAbbrev )
		{
			strcat( pszTemp, pAbbrev );
		}
	}

	bool fAllShow = IsPriv(PRIV_DEBUG|PRIV_ALLSHOW);

	if ( g_Cfg.m_fCharTags || fAllShow )
	{
		if ( pChar->m_pArea && pChar->m_pArea->IsGuarded() && pChar->m_pNPC )
		{
			if ( pChar->IsStatFlag( STATF_Pet ))
				strcat( pszTemp, " [tame]" );
			else
				strcat( pszTemp, " [npc]" );
		}
		if ( pChar->IsStatFlag( STATF_INVUL ) && ! pChar->IsStatFlag( STATF_Incognito ) && ! pChar->IsPriv( PRIV_PRIV_NOSHOW ))
			strcat( pszTemp, " [invul]" );
		if ( pChar->IsStatFlag( STATF_Stone ))
			strcat( pszTemp, " [stone]" );
		else if ( pChar->IsStatFlag( STATF_Freeze ))
			strcat( pszTemp, " [frozen]" );
		if ( pChar->IsStatFlag( STATF_Insubstantial | STATF_Invisible | STATF_Hidden ))
			strcat( pszTemp, " [hidden]" );
		if ( pChar->IsStatFlag( STATF_Sleeping ))
			strcat( pszTemp, " [sleeping]" );
		if ( pChar->IsStatFlag( STATF_Hallucinating ))
			strcat( pszTemp, " [hallu]" );

		if ( fAllShow )
		{
			if ( pChar->IsStatFlag(STATF_Spawned))
			{
				strcat( pszTemp, " [spawn]" );
			}
			if ( IsPriv( PRIV_DEBUG ))
			{
				// Show UID
				sprintf( pszTemp+strlen(pszTemp), " [0%lx]", (DWORD) pChar->GetUID());
			}
		}
	}
	if ( ! fAllShow && pChar->Skill_GetActive() == NPCACT_Napping )
	{
		strcat( pszTemp, " [afk]" );
	}
	if ( pChar->GetPrivLevel() <= PLEVEL_Guest )
	{
		strcat( pszTemp, " [guest]" );
	}
	if ( pChar->IsPriv( PRIV_JAILED ))
	{
		strcat( pszTemp, " [jailed]" );
	}
	if ( pChar->IsDisconnected())
	{
		strcat( pszTemp, " [logout]" );
	}
	if (( fAllShow || pChar == m_pChar ) && pChar->IsStatFlag( STATF_Criminal ))
	{
		strcat( pszTemp, " [criminal]" );
	}
	if ( fAllShow || ( IsPriv(PRIV_GM) && ( g_Cfg.m_wDebugFlags & DEBUGF_NPC_EMOTE )))
	{
		strcat( pszTemp, " [" );
		strcat( pszTemp, pChar->Skill_GetName());
		strcat( pszTemp, "]" );
	}

	addObjMessage( pszTemp, pChar, wHue );
}

void CClient::addPlayerWalkCancel()
{
	// Resync CChar client back to a previous move.
	CCommand cmd;
	cmd.WalkCancel.m_Cmd = XCMD_WalkCancel;
	cmd.WalkCancel.m_count = m_wWalkCount;	// sequence #

	CPointMap pt = m_pChar->GetTopPoint();

	cmd.WalkCancel.m_x = pt.m_x;
	cmd.WalkCancel.m_y = pt.m_y;
	cmd.WalkCancel.m_dir = m_pChar->GetDirFlag();
	cmd.WalkCancel.m_z = pt.m_z;
	xSendPkt( &cmd, sizeof( cmd.WalkCancel ));
	m_wWalkCount = -1;
}

void CClient::addPlayerStart( CChar * pChar )
{
	if ( m_pChar != pChar )	// death option just usese this as a reload.
	{
		// This must be a CONTROL command ?
		CharDisconnect();
		if ( pChar->IsClient())	// not sure why this would happen but take care of it anyhow.
		{
			pChar->GetClient()->CharDisconnect();
			ASSERT(!pChar->IsClient());
		}
		m_pChar = pChar;
		m_pChar->ClientAttach( this );
	}

	ASSERT( m_pChar->IsClient());
	ASSERT( m_pChar->m_pPlayer );
	DEBUG_CHECK( m_pChar->m_pNPC == NULL );

	CItem * pItemChange = m_pChar->LayerFind(LAYER_FLAG_ClientLinger);
	if ( pItemChange != NULL )
	{
		pItemChange->Delete();
	}

	m_Env.SetInvalid();
/*
	CExtData ExtData;
	ExtData.Party_Enable.m_state = 1;
	addExtData( EXTDATA_Party_Enable, &ExtData, sizeof(ExtData.Party_Enable));
*/
			
	CPointMap pt = m_pChar->GetTopPoint();
		
	CCommand cmd;
	cmd.Start.m_Cmd = XCMD_Start;
	cmd.Start.m_UID = m_pChar->GetUID();
	cmd.Start.m_unk_5_8 = 0x00;
	cmd.Start.m_id = m_pChar->GetDispID();
	cmd.Start.m_x = pt.m_x;
	cmd.Start.m_y = pt.m_y;
	cmd.Start.m_z = pt.m_z;
	cmd.Start.m_dir = m_pChar->GetDirFlag();
	cmd.Start.m_unk_18 = 0x00;
	cmd.Start.m_unk_19_22 = 0xffffffff;
	cmd.Start.m_boundX = 0x0000;
	cmd.Start.m_boundY = 0x0000;
	// cmd.Start.m_mode = m_pChar->GetModeFlag();
	 
	bool bMap = pt.m_map > 0;
	
	cmd.Start.m_mode = bMap ? g_MapList.GetX(pt.m_map) : 0x1800;
	cmd.Start.m_boundH = bMap ? g_MapList.GetY(pt.m_map) : 0x1000;
	cmd.Start.m_zero_31 = 0x0000;
	cmd.Start.m_zero_33 = 0x00000000;

/*
	static const BYTE sm_Pkt_Start1[20] =	// no idea what this stuff does.
		"\x00"
		"\x00\x00\x7F\x00"
		"\x00\x00"
		"\x00\x00"
		"\x07\x80"
		"\x09\x60"
		"\x00\x00\x00\x00\x00\x00";

	memcpy( &cmd.Start.m_unk_18, sm_Pkt_Start1, sizeof( sm_Pkt_Start1 ));
	cmd.Start.m_mode = m_pChar->GetModeFlag(); */

	xSendPkt( &cmd, sizeof( cmd.Start ));
	
	ClearTargMode();	// clear death menu mode. etc. ready to walk about. cancel any previos modes
	
	addMap( NULL, true );
	
	addChangeServer();
	
	// Here Mapdiffs
	
	// addEnableFeatures( g_Cfg.m_iFeatures );
	
	addPlayerView( pt, true );
	
	// addChar( pChar );
	// addPause();	// XXX seems to do this.
	
	addRedrawAll();
	addTime( true );

	m_pChar->MoveToChar( pt );	// Make sure we are in active list.
	m_pChar->Update();	
	
	if ( pChar->m_pParty )
	{
		pChar->m_pParty->SendAddList( NULL );
	}

	addSound( SOUND_CLEAR );
	addWeather( WEATHER_DEFAULT );
	addLight();		// Current light level where I am.
	addPlayerWarMode();
	addOptions();
	
//	addSound( SOUND_CLEAR );
//	addWeather( WEATHER_DEFAULT );
//	addLight();		// Current light level where I am.
	
//	addEnableFeatures( g_Cfg.m_iFeatures );
	
//	addRedrawAll();
//	addTime();
	
}

void CClient::addPlayerWarMode()
{
	CCommand cmd;
	cmd.War.m_Cmd = XCMD_War;
	cmd.War.m_warmode = ( m_pChar->IsStatFlag( STATF_War )) ? 1 : 0;
	cmd.War.m_unk2[0] = 0;
	cmd.War.m_unk2[1] = 0x32;	// ?
	cmd.War.m_unk2[2] = 0;
	xSendPkt( &cmd, sizeof( cmd.War ));
}

void CClient::addToolTip( const CObjBase * pObj, LPCTSTR pszText )
{
	if ( pObj == NULL )
		return;
	if ( pObj->IsChar())
		return; // no tips on chars.

	addPause(); // XXX does this, don't know if needed

	CCommand cmd;
	int i = CvtSystemToNUNICODE( cmd.ToolTip.m_utext, MAX_TALK_BUFFER, pszText, -1 );
	int len = ((i + 1) * sizeof(NCHAR)) + ( sizeof(cmd.ToolTip) - sizeof(cmd.ToolTip.m_utext));

	cmd.ToolTip.m_Cmd = XCMD_ToolTip;
	cmd.ToolTip.m_len = len;
	cmd.ToolTip.m_UID = pObj->GetUID();

	xSendPkt( &cmd, len );
}

bool CClient::addBookOpen( CItem * pBook )
{
	// word wrap is done when typed in the client. (it has a var size font)
	ASSERT(pBook);
	DEBUG_CHECK( pBook->IsTypeBook());

	int iPagesNow = 0;
	bool fWritable;
	WORD nPages = 0;
	CGString sTitle;
	CGString sAuthor;

	if ( pBook->IsBookSystem())
	{
		fWritable = false;

		CResourceLock s;
		if ( ! g_Cfg.ResourceLock( s, pBook->m_itBook.m_ResID ))
			return false;

		while ( s.ReadKeyParse())
		{
			switch ( FindTableSorted( s.GetKey(), CItemMessage::sm_szLoadKeys, COUNTOF( CItemMessage::sm_szLoadKeys )-1 ))
			{
			case CIC_AUTHOR:
				sAuthor = s.GetArgStr();
				break;
			case CIC_PAGES:
				nPages = s.GetArgVal();
				break;
			case CIC_TITLE:
				sTitle = s.GetArgStr();
				break;
			}
		}
		if ( ! sTitle.IsEmpty())
		{
			pBook->SetName( sTitle );	// Make sure the book is named.
		}
	}
	else
	{
		// User written book.
		CItemMessage * pMsgItem = dynamic_cast <CItemMessage *> (pBook);
		if ( pMsgItem == NULL )
			return false;

		fWritable = pMsgItem->IsBookWritable() ? true : false;	// Not sealed
		nPages = fWritable ? ( MAX_BOOK_PAGES ) : ( pMsgItem->GetPageCount());	// Max pages.
		sTitle = pMsgItem->GetName();
		sAuthor = (pMsgItem->m_sAuthor.IsEmpty()) ? "unknown" : (LPCTSTR)( pMsgItem->m_sAuthor );

		if ( fWritable )	// For some reason we must send them now.
		{
			iPagesNow = pMsgItem->GetPageCount();
		}
	}

	CCommand cmd;
	if ( m_Crypt.GetClientVer() >= 0x126000 || IsNoCryptVer(1) )
	{
		cmd.BookOpen_v26.m_Cmd = XCMD_BookOpen;
		cmd.BookOpen_v26.m_UID = pBook->GetUID();
		cmd.BookOpen_v26.m_writable = fWritable ? 1 : 0;
		cmd.BookOpen_v26.m_NEWunk1 = fWritable ? 1 : 0;
		cmd.BookOpen_v26.m_pages = nPages;
		strcpy( cmd.BookOpen_v26.m_title, sTitle );
		strcpy( cmd.BookOpen_v26.m_author, sAuthor );
		xSendPkt( &cmd, sizeof( cmd.BookOpen_v26 ));
	}
	else
	{
		cmd.BookOpen_v25.m_Cmd = XCMD_BookOpen;
		cmd.BookOpen_v25.m_UID = pBook->GetUID();
		cmd.BookOpen_v25.m_writable = fWritable ? 1 : 0;
		cmd.BookOpen_v25.m_pages = nPages;
		strcpy( cmd.BookOpen_v25.m_title, sTitle );
		strcpy( cmd.BookOpen_v25.m_author, sAuthor );
		xSendPkt( &cmd, sizeof( cmd.BookOpen_v25 ));
	}

	// We could just send all the pages now if we want.
	if ( iPagesNow )
	{
		if ( iPagesNow>nPages )
			iPagesNow=nPages;
		for ( int i=0; i<iPagesNow; i++ )
		{
			addBookPage( pBook, i+1 );
		}
	}

	return( true );
}

void CClient::addBookPage( const CItem * pBook, int iPage )
{
	// ARGS:
	//  iPage = 1 based page.
	DEBUG_CHECK( iPage > 0 );
	if ( iPage <= 0 )
		return;

	CCommand cmd;
	cmd.BookPage.m_Cmd = XCMD_BookPage;
	cmd.BookPage.m_UID = pBook->GetUID();
	cmd.BookPage.m_pages = 1;	// we can send multiple pages if we wish.
	cmd.BookPage.m_page[0].m_pagenum = iPage;	// 1 based page.

	int lines=0;
	int length=0;

	if ( pBook->IsBookSystem())
	{
		CResourceLock s;
		if ( ! g_Cfg.ResourceLock( s, RESOURCE_ID( RES_BOOK, pBook->m_itBook.m_ResID.GetResIndex(), iPage )))
			return;

		while (s.ReadKey(false))
		{
			lines++;
			length += strcpylen( cmd.BookPage.m_page[0].m_text+length, s.GetKey()) + 1;
		}
	}
	else
	{
		// User written book pages.
		const CItemMessage * pMsgItem = dynamic_cast <const CItemMessage *> (pBook);
		if ( pMsgItem == NULL )
			return;
		iPage --;
		if ( iPage < pMsgItem->GetPageCount())
		{
			// Copy the pages to the book
			LPCTSTR pszText = pMsgItem->GetPageText(iPage);
			if ( pszText )
			{
				while (true)
				{
					TCHAR ch = pszText[ length ];
					if ( ch == '\t' )
					{
						ch = '\0';
						lines++;
					}
					cmd.BookPage.m_page[0].m_text[ length ] = ch;
					if ( pszText[ length ++ ] == '\0' )
					{
						if ( length > 1 ) lines ++;
						break;
					}
				}
			}
		}
	}

	length += sizeof( cmd.BookPage ) - sizeof( cmd.BookPage.m_page[0].m_text );
	cmd.BookPage.m_len = length;
	cmd.BookPage.m_page[0].m_lines = lines;

	xSendPkt( &cmd, length );
}

int CClient::Setup_FillCharList( CEventCharDef * pCharList, const CChar * pCharFirst )
{
	// list available chars for your account that are idle.
	CAccount * pAccount = GetAccount();
	ASSERT( pAccount );
	int j=0;

	if ( pCharFirst && pAccount->IsMyAccountChar( pCharFirst ))
	{
		m_tmSetupCharList[0] = pCharFirst->GetUID();
		strcpylen( pCharList[0].m_charname, pCharFirst->GetName(), sizeof( pCharList[0].m_charname ));
		pCharList[0].m_charpass[0] = '\0';
		j++;
	}

	int iQty = pAccount->m_Chars.GetCharCount();
	for ( int k=0; k<iQty; k++ )
	{
		CGrayUID uid( pAccount->m_Chars.GetChar(k));
		CChar * pChar = uid.CharFind();
		if ( pChar == NULL )
			continue;
		DEBUG_CHECK( pAccount->IsMyAccountChar( pChar ));
		if ( pCharFirst == pChar )
			continue;
		if ( j >= MAX_CHARS_PER_ACCT )
			break;

		// if ( j >= g_Cfg.m_iMaxCharsPerAccount && ! IsPriv(PRIV_GM)) break;

		m_tmSetupCharList[j] = uid;
		strcpylen( pCharList[j].m_charname, pChar->GetName(), sizeof( pCharList[0].m_charname ));
		pCharList[j].m_charpass[0] = '\0';
		j++;
	}

	// always show max count for some stupid reason. (client bug)
	// pad out the rest of the chars.
	for ( ;j<MAX_CHARS_PER_ACCT;j++)
	{
		m_tmSetupCharList[j].InitUID();
		pCharList[j].m_charname[0] = '\0';
		pCharList[j].m_charpass[0] = '\0';
	}

	if ( this->m_Crypt.GetClientVer() >= 0x00300001 || IsNoCryptVer(3) )
		return iQty;
	else
	return( MAX_CHARS_PER_ACCT );
}

void CClient::addCharList3()
{
	// just pop up a list of chars for this account.
	// GM's may switch chars on the fly.

	ASSERT( GetAccount() );

	CCommand cmd;
	cmd.CharList3.m_Cmd = XCMD_CharList3;
	cmd.CharList3.m_len = sizeof( cmd.CharList3 );
	cmd.CharList3.m_count = Setup_FillCharList( cmd.CharList3.m_char, m_pChar );
	cmd.CharList3.m_unk = 0;

	xSendPkt( &cmd, sizeof( cmd.CharList3 ));

	CharDisconnect();	// since there is no undoing this in the client.
	SetTargMode( CLIMODE_SETUP_CHARLIST );
}

void CClient::SetTargMode( CLIMODE_TYPE targmode, LPCTSTR pPrompt )
{
	// ??? Get rid of menu stuff if previous targ mode.
	// Can i close a menu ?
	// Cancel a cursor input.

	if ( GetTargMode() == CLIMODE_TARG_USE_ITEM )
	{
		CItem * pItemUse = m_Targ_UID.ItemFind();
		if ( pItemUse )
		{
			if ( !IsSetEF(EF_Minimize_Triggers) )
			{
				if ( pItemUse->OnTrigger( ITRIG_TARGON_CANCEL, m_pChar ) == TRIGRET_RET_TRUE )
				{
					m_Targ_Mode = targmode;
					if ( targmode != CLIMODE_NORMAL )
						addSysMessage( pPrompt );
					return;
				}
			}
		}
	}

	if ( GetTargMode() == targmode )
		return;

	if ( GetTargMode() != CLIMODE_NORMAL && targmode != CLIMODE_NORMAL )
	{
		// Just clear the old target mode
		addSysMessage( "Previous targeting cancelled." );
	}

	m_Targ_Mode = targmode;

	if ( targmode == CLIMODE_NORMAL )
		addSysMessage( "Targeting cancelled." );
	else
		addSysMessage( pPrompt );
}

void CClient::addPromptConsole( CLIMODE_TYPE targmode, LPCTSTR pPrompt )
{
	SetTargMode( targmode, pPrompt );

	CCommand cmd;
	cmd.Prompt.m_Cmd = XCMD_Prompt;
	cmd.Prompt.m_len = sizeof( cmd.Prompt );
	memset( cmd.Prompt.m_unk3, 0, sizeof(cmd.Prompt.m_unk3));
	cmd.Prompt.m_text[0] = '\0';

	xSendPkt( &cmd, cmd.Prompt.m_len );
}

void CClient::addTarget( CLIMODE_TYPE targmode, LPCTSTR pPrompt, bool fAllowGround, bool fCheckCrime ) // Send targetting cursor to client
{
	// Expect XCMD_Target back.
	// ??? will this be selective for us ? objects only or chars only ? not on the ground (statics) ?

	SetTargMode( targmode, pPrompt );

	CCommand cmd;
	memset( &(cmd.Target), 0, sizeof( cmd.Target ));
	cmd.Target.m_Cmd = XCMD_Target;
	cmd.Target.m_TargType = fAllowGround; // fAllowGround;	// 1=allow xyz, 0=objects only.
	cmd.Target.m_context = targmode ;	// 5=my id code for action.
	cmd.Target.m_fCheckCrime = fCheckCrime; // // Not sure what this is. (m_checkcrimflag?)

	xSendPkt( &cmd, sizeof( cmd.Target ));
}

void CClient::addTargetDeed( const CItem * pDeed )
{
	// Place an item from a deed. preview all the stuff

	ASSERT( m_Targ_UID == pDeed->GetUID());
	ITEMID_TYPE iddef = (ITEMID_TYPE) RES_GET_INDEX( pDeed->m_itDeed.m_Type );
	m_tmUseItem.m_pParent = pDeed->GetParent();	// Cheat Verify.
	addTargetItems( CLIMODE_TARG_USE_ITEM, iddef );
}

bool CClient::addTargetChars( CLIMODE_TYPE mode, CREID_TYPE baseID, bool fNotoCheck )
{
	CCharBase * pBase = CCharBase::FindCharBase( baseID );
	if ( pBase == NULL )
		return( false );

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "Where would you like to summon the '%s'?", (LPCTSTR) pBase->GetTradeName());

	addTarget(mode, pszTemp, true, fNotoCheck);
	return true;
}

bool CClient::addTargetItems( CLIMODE_TYPE targmode, ITEMID_TYPE id )
{
	// Add a list of items to place at target.
	// preview all the stuff

	ASSERT(m_pChar);

	LPCTSTR pszName;
	CItemBase * pItemDef;
	if ( id < ITEMID_TEMPLATE )
	{
		pItemDef = CItemBase::FindItemBase( id );
		if ( pItemDef == NULL )
			return false;
		pszName = pItemDef->GetName();

		if ( pItemDef->IsType(IT_STONE_GUILD) )
		{
			// Check if they are already in a guild first
			CItemStone * pStone = m_pChar->Guild_Find(MEMORY_GUILD);
			if (pStone)
			{
				addSysMessage( "You are already a member of a guild. Resign first!");
				return false;
			}
		}
	}
	else
	{
		pItemDef = NULL;
		pszName = "template";
	}

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "Where would you like to place the %s?", pszName);

	if ( CItemBase::IsID_Multi( id ))	// a multi we get from Multi.mul
	{
		SetTargMode(targmode, pszTemp);

		CCommand cmd;
		cmd.TargetMulti.m_Cmd = XCMD_TargetMulti;
		cmd.TargetMulti.m_fAllowGround = 1;
		cmd.TargetMulti.m_context = targmode ;	// 5=my id code for action.
		memset( cmd.TargetMulti.m_zero6, 0, sizeof(cmd.TargetMulti.m_zero6));
		cmd.TargetMulti.m_id = id - ITEMID_MULTI;

		// Add any extra stuff attached to the multi. preview this.

		memset( cmd.TargetMulti.m_zero20, 0, sizeof(cmd.TargetMulti.m_zero20));

		xSendPkt( &cmd, sizeof( cmd.TargetMulti ));
		return true;
	}

	if ( IsNoCryptVer(1) || m_Crypt.GetClientVer() >= 0x126030 )
	{
		// preview not supported by this ver?
		addTarget(targmode, pszTemp, true);
		return true;
	}

	CCommand cmd;
	cmd.TargetItems.m_Cmd = XCMD_TargetItems;
	cmd.TargetItems.m_fAllowGround = 1;
	cmd.TargetItems.m_code = targmode;
	cmd.TargetItems.m_xOffset = 0;
	cmd.TargetItems.m_yOffset = 0;
	cmd.TargetItems.m_zOffset = 0;

	int iCount;
	CItemBaseMulti * pBaseMulti = dynamic_cast <CItemBaseMulti *>(pItemDef);
	if ( pBaseMulti )
	{
		iCount = pBaseMulti->m_Components.GetCount();
		for ( int i=0; i<iCount; i++ )
		{
			ASSERT( pBaseMulti->m_Components[i].m_id < ITEMID_MULTI );
			cmd.TargetItems.m_item[i].m_id = pBaseMulti->m_Components[i].m_id;
			cmd.TargetItems.m_item[i].m_dx = pBaseMulti->m_Components[i].m_dx;
			cmd.TargetItems.m_item[i].m_dy = pBaseMulti->m_Components[i].m_dy;
			cmd.TargetItems.m_item[i].m_dz = pBaseMulti->m_Components[i].m_dz;
			cmd.TargetItems.m_item[i].m_unk = 0;
		}
	}
	else
	{
		iCount = 1;
		cmd.TargetItems.m_item[0].m_id = id;
		cmd.TargetItems.m_item[0].m_dx = 0;
		cmd.TargetItems.m_item[0].m_dy = 0;
		cmd.TargetItems.m_item[0].m_dz = 0;
		cmd.TargetItems.m_item[0].m_unk = 0;
	}

	cmd.TargetItems.m_count = iCount;
	int len = ( sizeof(cmd.TargetItems) - sizeof(cmd.TargetItems.m_item)) + ( iCount * sizeof(cmd.TargetItems.m_item[0]));
	cmd.TargetItems.m_len = len;

	SetTargMode(targmode, pszTemp);
	xSendPkt( &cmd, len);

	return( true );
}

void CClient::addDyeOption( const CObjBase * pObj )
{
	// Put up the wHue chart for the client.
	// This results in a Event_Item_Dye message. CLIMODE_DYE
	ITEMID_TYPE id;
	if ( pObj->IsItem())
	{
		const CItem * pItem = dynamic_cast <const CItem*> (pObj);
		ASSERT(pItem);
		id = pItem->GetDispID();
	}
	else
	{
		// Get the item equiv for the creature.
		const CChar * pChar = dynamic_cast <const CChar*> (pObj);
		ASSERT(pChar);
		id = pChar->Char_GetDef()->m_trackID;
	}

	CCommand cmd;
	cmd.DyeVat.m_Cmd = XCMD_DyeVat;
	cmd.DyeVat.m_UID = pObj->GetUID();
	cmd.DyeVat.m_zero5 = pObj->GetHue();
	cmd.DyeVat.m_id = id;
	xSendPkt( &cmd, sizeof( cmd.DyeVat ));
	SetTargMode( CLIMODE_DYE );
}

void CClient::addSkillWindow( SKILL_TYPE skill ) // Opens the skills list
{
	// Whos skills do we want to look at ?
	CChar * pChar = m_Prop_UID.CharFind();
	if ( pChar == NULL ) pChar = m_pChar;

	CCommand cmd;
	cmd.Skill.m_Cmd = XCMD_Skill;

	bool fVer12602 = ( IsNoCryptVer(1) || m_Crypt.GetClientVer() >= 0x126020 );

	int len = ( fVer12602 ) ? sizeof(cmd.Skill) : sizeof(cmd.Skill_v261);

	if ( skill >= MAX_SKILL )
	{	// all skills
		if ( !IsSetEF(EF_Minimize_Triggers) )
		{
			CScriptTriggerArgs Args( -1 );
			if ( m_pChar->OnTrigger( CTRIG_UserSkills, pChar, &Args ) == TRIGRET_RET_TRUE )
				return;
		}

		cmd.Skill.m_single = 0;
		int i=0;
		for ( ; i<MAX_SKILL; i++ )
		{
			int iskillval = pChar->Skill_GetBase( (SKILL_TYPE) i);
			if ( fVer12602 )
			{
				cmd.Skill.skills[i].m_index = i+1;
				cmd.Skill.skills[i].m_val = pChar->Skill_GetAdjusted( (SKILL_TYPE) i);
				cmd.Skill.skills[i].m_valraw = iskillval;
				cmd.Skill.skills[i].m_lock = pChar->Skill_GetLock( (SKILL_TYPE) i);
			}
			else
			{
				cmd.Skill_v261.skills[i].m_index = i+1;
				cmd.Skill_v261.skills[i].m_val = iskillval;
			}
		}
		if ( fVer12602 )
		{
			cmd.Skill.skills[i].m_index = 0;	// terminator.
			len += ((MAX_SKILL-1) * sizeof(cmd.Skill.skills[0])) + sizeof(NWORD);
		}
		else
		{
			cmd.Skill_v261.skills[i].m_index = 0;	// terminator.
			len += ((MAX_SKILL-1) * sizeof(cmd.Skill_v261.skills[0])) + sizeof(NWORD);
		}
	}
	else
	{	// Just one skill update.
		if ( !IsSetEF(EF_Minimize_Triggers) )
		{
			CScriptTriggerArgs Args( skill );
			if ( m_pChar->OnTrigger( CTRIG_UserSkills, pChar, &Args ) == TRIGRET_RET_TRUE )
				return;
		}

		if ( skill >= 50 )
			return;

		cmd.Skill.m_single = 0xff;
		int iskillval = pChar->Skill_GetBase( skill );
		if ( fVer12602 )
		{
			cmd.Skill.skills[0].m_index = skill;
			cmd.Skill.skills[0].m_val = pChar->Skill_GetAdjusted( skill );
			cmd.Skill.skills[0].m_valraw = iskillval;
			cmd.Skill.skills[0].m_lock = pChar->Skill_GetLock( skill );
		}
		else
		{
			cmd.Skill_v261.skills[0].m_index = skill;
			cmd.Skill_v261.skills[0].m_val = iskillval;
		}
	}

	cmd.Skill.m_len = len;
	xSendPkt( &cmd, len );
}

void CClient::addPlayerSee( const CPointMap & ptold )
{
	// adjust to my new location.
	// What do I now see here ?

	// What new things do i see (on the ground) ?
	CWorldSearch AreaItems( m_pChar->GetTopPoint(), UO_MAP_VIEW_RADAR );
	AreaItems.SetAllShow( IsPriv( PRIV_ALLSHOW ));
	while (true)
	{
		CItem * pItem = AreaItems.GetItem();
		if ( pItem == NULL )
			break;
		if ( ! CanSee( pItem ))
			continue;

		// I didn't see it before
		if ( ptold.GetDist( pItem->GetTopPoint()) > UO_MAP_VIEW_SIZE )
		{
			addItem_OnGround( pItem );
		}
	}

	// What new people do i see ?
	CWorldSearch AreaChars( m_pChar->GetTopPoint(), UO_MAP_VIEW_SIZE );
	AreaChars.SetAllShow( IsPriv( PRIV_ALLSHOW ));	// show logged out chars?
	DWORD	dSeeChars(0);
	while (true)
	{
		CChar * pChar = AreaChars.GetChar();
		if ( pChar == NULL )
			break;
		if ( m_pChar == pChar )
			continue;	// I saw myself before.
		if ( ! CanSee( pChar ))
			continue;

		if ( ptold.GetDist( pChar->GetTopPoint()) > UO_MAP_VIEW_SIZE )
		{
			if ( dSeeChars < (g_Cfg.m_iMaxCharComplexity * 4))
			{
				dSeeChars++;
				addChar( pChar );
			}
		}
	}
}

void CClient::addPlayerView( const CPointMap & ptold, bool playerStart )
{
	// I moved = Change my point of view. Teleport etc..

	if ( !playerStart )
		addPause();

	CCommand cmd;
	cmd.View.m_Cmd = XCMD_View;
	cmd.View.m_UID = m_pChar->GetUID();

	CREID_TYPE id;
	HUE_TYPE wHue;
	GetAdjustedCharID( m_pChar, id, wHue );
	cmd.View.m_id = id;
	cmd.View.m_zero7 = 0;
	cmd.View.m_wHue = wHue;
	cmd.View.m_mode = m_pChar->GetModeFlag();

	CPointMap pt = m_pChar->GetTopPoint();
	cmd.View.m_x = pt.m_x;
	cmd.View.m_y = pt.m_y;
	cmd.View.m_zero15 = 0;
	cmd.View.m_dir = m_pChar->GetDirFlag();
	cmd.View.m_z = pt.m_z;

	xSendPkt( &cmd, sizeof( cmd.View ));

	// resync this stuff.
	m_wWalkCount = -1;

	if ( ptold == pt )
		return;	// not a real move i guess. might just have been a change in face dir.

	m_Env.SetInvalid();	// Must resend environ stuff.

	// What can i see here ?
	if ( !playerStart )
		addPlayerSee( ptold );
}

void CClient::addReSync(bool bForceMap)
{
	// Reloads the client with all it needs.
	CPointMap ptold;	// invalid value.
	addMap(NULL, bForceMap ? true : false);
	addPlayerView(ptold);
	addChar( m_pChar );
	addLight();		// Current light level where I am.
}

void	CClient::addMap( const CPointMap * pOldP, bool playerStart)
{
	CPointMap pt = m_pChar->GetTopPoint();
	
	if ( !playerStart && pOldP && pOldP->m_map == pt.m_map )
		return;

	CExtData ExtData;
	ExtData.MapChange.m_state = g_MapList.m_mapnum[pt.m_map];
	addExtData( EXTDATA_Map_Change, &ExtData, sizeof(ExtData.MapChange) );
	if ( !playerStart )
	{
		CPointMap	ptold;
		addPlayerView(ptold);
		addChar(m_pChar);
		addLight();		// Current light level where I am.
	}
}

void CClient::addChangeServer()
{
	CPointMap pt = m_pChar->GetTopPoint();

	CCommand cmd;
		
	cmd.ZoneChange.m_Cmd = XCMD_ZoneChange;
	cmd.ZoneChange.m_x = pt.m_x;
	cmd.ZoneChange.m_y = pt.m_y;
	cmd.ZoneChange.m_z = pt.m_z;
	cmd.ZoneChange.m_unk7_zero = 0x00;
	cmd.ZoneChange.m_serv_boundX = 0x0000;
	cmd.ZoneChange.m_serv_boundY = 0x0000;
	
	bool bMap = pt.m_map > 0;
	
	cmd.ZoneChange.m_serv_boundW = bMap ? g_MapList.GetX(pt.m_map+1) : 0x1800;
	cmd.ZoneChange.m_serv_boundH = bMap ? g_MapList.GetY(pt.m_map+1) : 0x1000;
	
	xSend( &cmd, sizeof(cmd.ZoneChange) );
}

void CClient::UpdateStats()
{
	if ( ! m_fUpdateStats )
		return;

	if ( m_fUpdateStats & SF_UPDATE_STATUS )
	{
		addCharStatWindow( m_pChar->GetUID());
		m_fUpdateStats = 0;
	}
	else
	{
		if ( m_fUpdateStats & SF_UPDATE_HITS )
		{
			addHitsUpdate( m_pChar->GetUID() );
			m_fUpdateStats &= ~SF_UPDATE_HITS;
		}
		if ( m_fUpdateStats & SF_UPDATE_MANA )
		{
			addManaUpdate( m_pChar->GetUID() );
			m_fUpdateStats &= ~SF_UPDATE_MANA;
		}

		if ( m_fUpdateStats & SF_UPDATE_STAM )
		{
			addStamUpdate( m_pChar->GetUID() );
			m_fUpdateStats &= ~SF_UPDATE_STAM;
		}
	}
}

void CClient::addCharStatWindow( CGrayUID uid, bool fRequested ) // Opens the status window
{
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		CScriptTriggerArgs	Args( 0, 0, uid.ObjFind() );
		Args.m_iN3	= fRequested;
		if ( m_pChar->OnTrigger( CTRIG_UserStats, pChar, &Args ) == TRIGRET_RET_TRUE )
			return;
	}

	CCommand cmd;
	cmd.Status.m_Cmd = XCMD_Status;
	cmd.Status.m_UID = pChar->GetUID();

	// WARNING: Char names must be <= 30 !!!
	// "kellen the magincia counsel me" > 30 char names !!
	strcpylen( cmd.Status.m_charname, pChar->GetName(), sizeof( cmd.Status.m_charname ));

	// renamable ?
	if ( m_pChar != pChar &&	// can't rename self. it looks weird.
		pChar->NPC_IsOwnedBy( m_pChar ) && // my pet.
		! pChar->Char_GetDef()->GetHireDayWage())	// can't rename hirelings
	{
		cmd.Status.m_perm = 0xFF;	// I can rename them
	}
	else
	{
		cmd.Status.m_perm = 0x00;
	}

	// Dont bother sending the rest of this if not my self.
	cmd.Status.m_ValidStats = 0;

	if ( pChar == m_pChar )
	{
		m_fUpdateStats = false;

		cmd.Status.m_len = sizeof( cmd.Status );
		cmd.Status.m_ValidStats |= 1;	// valid stats
		cmd.Status.m_sex = ( pChar->Char_GetDef()->IsFemale()) ? 1 : 0;

		int iStr		= pChar->Stat_GetAdjusted(STAT_STR);
		if ( iStr < 0 ) iStr = 0;
		cmd.Status.m_str = iStr;

		int iDex = pChar->Stat_GetAdjusted(STAT_DEX);
		if ( iDex < 0 ) iDex = 0;
		cmd.Status.m_dex = iDex;

		int iInt = pChar->Stat_GetAdjusted(STAT_INT);
		if ( iInt < 0 ) iInt = 0;
		cmd.Status.m_int = iInt;

		cmd.Status.m_health	= pChar->Stat_GetVal(STAT_STR);
		cmd.Status.m_maxhealth	= pChar->Stat_GetMax(STAT_STR);
		cmd.Status.m_stam	= pChar->Stat_GetVal(STAT_DEX);
		cmd.Status.m_maxstam	= pChar->Stat_GetMax(STAT_DEX);
		cmd.Status.m_mana	= pChar->Stat_GetVal(STAT_INT);
		cmd.Status.m_maxmana	= pChar->Stat_GetMax(STAT_INT);

		if ( !g_Cfg.m_fPayFromPackOnly )
			cmd.Status.m_gold = pChar->ContentCount( RESOURCE_ID(RES_TYPEDEF,IT_GOLD));	/// ??? optimize this count is too often.
		else
			cmd.Status.m_gold = pChar->GetPackSafe()->ContentCount( RESOURCE_ID(RES_TYPEDEF,IT_GOLD));

		cmd.Status.m_armor = pChar->m_defense + pChar->Char_GetDef()->m_defense;
		cmd.Status.m_weight = pChar->GetTotalWeight() / WEIGHT_UNITS;
	}
	else
	{
		int iMaxHits = max(pChar->Stat_GetMax(STAT_STR),1);
		cmd.Status.m_maxhealth = 100;
		cmd.Status.m_health = (pChar->Stat_GetVal(STAT_STR) * 100) / iMaxHits;
		cmd.Status.m_len = ((BYTE*)&(cmd.Status.m_sex)) - ((BYTE*)&(cmd.Status));
	}
	xSendPkt(&cmd, cmd.Status.m_len);
}

void CClient::addHitsUpdate( CGrayUID uid )
{
/*
		struct	// size = 9	// update some change in stats.
		{
			BYTE m_Cmd;	// 0=0xa1 (str), 0xa2 (int), or 0xa3 (dex)
			NDWORD m_UID;	// 1-4
			NWORD m_max;	// 5-6
			NWORD m_val;	// 7-8
		} StatChng;
*/
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	CCommand cmd;
	cmd.StatChng.m_Cmd = XCMD_StatChngStr;
	cmd.StatChng.m_UID = pChar->GetUID();
	if ( m_pChar != pChar )
	{
		cmd.StatChng.m_max = 50;
		cmd.StatChng.m_val = ( pChar->Stat_GetVal(STAT_STR) * 50 ) / pChar->Stat_GetMax(STAT_STR);
	}
	else
	{
		cmd.StatChng.m_max = pChar->Stat_GetMax(STAT_STR);
		cmd.StatChng.m_val = pChar->Stat_GetVal(STAT_STR);
	}
	xSendPkt( &cmd, sizeof(cmd.StatChng) );

	if (( pChar->m_pParty == NULL ) || ( m_pChar != pChar ))
		return;

	pChar->m_pParty->AddStatsUpdate( pChar, &cmd, sizeof(cmd.StatChng) );
}

void CClient::addManaUpdate( CGrayUID uid )
{
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	CCommand cmd;
	cmd.StatChng.m_Cmd = XCMD_StatChngInt;
	cmd.StatChng.m_UID = pChar->GetUID();
	cmd.StatChng.m_max = pChar->Stat_GetMax(STAT_INT);
	cmd.StatChng.m_val = pChar->Stat_GetVal(STAT_INT);
	xSendPkt( &cmd, sizeof(cmd.StatChng) );

	if ( pChar->m_pParty == NULL )
		return;

	pChar->m_pParty->AddStatsUpdate( pChar, &cmd, sizeof(cmd.StatChng) );
}

void CClient::addStamUpdate( CGrayUID uid )
{
	CChar * pChar = uid.CharFind();
	if ( pChar == NULL )
		return;

	CCommand cmd;
	cmd.StatChng.m_Cmd = XCMD_StatChngDex;
	cmd.StatChng.m_UID = pChar->GetUID();
	cmd.StatChng.m_max = pChar->Stat_GetMax(STAT_DEX);
	cmd.StatChng.m_val = pChar->Stat_GetVal(STAT_DEX);
	xSendPkt( &cmd, sizeof(cmd.StatChng) );

	if ( pChar->m_pParty == NULL )
		return;

	pChar->m_pParty->AddStatsUpdate( pChar, &cmd, sizeof(cmd.StatChng) );
}

void CClient::addSpellbookOpen( CItem * pBook, BYTE offset )
{

	if ( !m_pChar )
		return;

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		CScriptTriggerArgs	Args( 0, 0, pBook );
		if ( m_pChar->OnTrigger( CTRIG_SpellBook, m_pChar, &Args ) == TRIGRET_RET_TRUE )
			return;
	}

	// NOTE: if the spellbook item is not present on the client it will crash.
	// count what spells I have.

	if ( pBook->GetDispID() == ITEMID_SPELLBOOK2 )
	{
		// weird client bug.
		pBook->SetDispID( ITEMID_SPELLBOOK );
		pBook->Update();
		return;
	}

	int count=0;
	int i=SPELL_Clumsy;
	for ( ;i<SPELL_BOOK_QTY;i++ )
	{
		if ( pBook->IsSpellInBook( (SPELL_TYPE) i ))
		{
			count++;
		}
	}

	addPause();	// try to get rid of the double open.

	addOpenGump( pBook, GUMP_OPEN_SPELLBOOK );
	if (!count)
		return;

	// TODO: AoS SpellBook
	if ( false )
	{
		// Handle new AOS spellbook stuff (old packets no longer work)

		CExtData data;
		data.NewSpellbook.m_Unk1 = 1;
		data.NewSpellbook.m_UID = pBook->GetUID();
		data.NewSpellbook.m_Content[0] = pBook->m_itSpellbook.m_spells1;
		data.NewSpellbook.m_Content[1] = pBook->m_itSpellbook.m_spells2;
		data.NewSpellbook.m_ItemId = pBook->GetDispID();
		data.NewSpellbook.m_Offset = offset; // 1 = normal, 101 = necro, 201 = paladin

		addExtData( EXTDATA_NewSpellbook, &data, sizeof( data.NewSpellbook ) );
		return;
	}

	CCommand cmd;
	int len = sizeof( cmd.Content ) - sizeof(cmd.Content.m_item) + ( count * sizeof(cmd.Content.m_item[0]));
	cmd.Content.m_Cmd = XCMD_Content;
	cmd.Content.m_len = len;
	cmd.Content.m_count = count;

	int j=0;
	for ( i=SPELL_Clumsy; i<SPELL_BOOK_QTY; i++ )
		if ( pBook->IsSpellInBook( (SPELL_TYPE) i ))
	{
		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef( (SPELL_TYPE) i );
		ASSERT(pSpellDef);

		cmd.Content.m_item[j].m_UID = UID_F_ITEM + UID_O_INDEX_FREE + i; // just some unused uid.
		cmd.Content.m_item[j].m_id = pSpellDef->m_idScroll;	// scroll id. (0x1F2E)
		cmd.Content.m_item[j].m_zero6 = 0;
		cmd.Content.m_item[j].m_amount = i;
		cmd.Content.m_item[j].m_x = 0x48;	// may not mean anything ?
		cmd.Content.m_item[j].m_y = 0x7D;
		cmd.Content.m_item[j].m_UIDCont = pBook->GetUID();
		cmd.Content.m_item[j].m_wHue = HUE_DEFAULT;
		j++;
	}

	xSendPkt( &cmd, len );
}


void CClient::addCustomSpellbookOpen( CItem * pBook, DWORD gumpID )
{
	const CItemContainer * pContainer = dynamic_cast <CItemContainer *> (pBook);
	CItem	* pItem;
	if ( !pContainer )
		return;

	int count=0;
	for ( pItem=pContainer->GetContentHead(); pItem!=NULL; pItem=pItem->GetNext() )
	{
		if ( !pItem->IsType( IT_SCROLL ) )
			continue;
		count++;
	}

	addPause();	// try to get rid of the double open.
	addOpenGump( pBook, (GUMP_TYPE) gumpID );
	if ( !count )
		return;

	CCommand cmd;
	int len = sizeof( cmd.Content ) - sizeof(cmd.Content.m_item) + ( count * sizeof(cmd.Content.m_item[0]));
	cmd.Content.m_Cmd = XCMD_Content;
	cmd.Content.m_len = len;
	cmd.Content.m_count = count;

	int j=0;
	for ( pItem=pContainer->GetContentHead(); pItem!=NULL; pItem=pItem->GetNext() )
	{
		if ( !pItem->IsType( IT_SCROLL ) )
			continue;

		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef( (SPELL_TYPE) pItem->m_itSpell.m_spell );
		if ( !pSpellDef )
			continue;

		cmd.Content.m_item[j].m_UID	= pItem->GetUID();
		cmd.Content.m_item[j].m_id	= pSpellDef->m_idScroll;	// scroll id. (0x1F2E)
		cmd.Content.m_item[j].m_zero6	= 0;
		cmd.Content.m_item[j].m_amount	= pItem->m_itSpell.m_spell;
		cmd.Content.m_item[j].m_x	= 0x48;	// may not mean anything ?
		cmd.Content.m_item[j].m_y	= 0x7D;
		cmd.Content.m_item[j].m_UIDCont = pBook->GetUID();
		cmd.Content.m_item[j].m_wHue = HUE_DEFAULT;
		j++;
	}

	xSendPkt( &cmd, len );
}

void CClient::addScrollScript( CResourceLock &s, SCROLL_TYPE type, DWORD context, LPCTSTR pszHeader )
{
	CCommand cmd;
	cmd.Scroll.m_Cmd = XCMD_Scroll;
	cmd.Scroll.m_type = type;
	cmd.Scroll.m_context = context;	// for ScrollClose ?

	int length=0;

	if ( pszHeader )
	{
		length = strcpylen( &cmd.Scroll.m_text[0], pszHeader );
		cmd.Scroll.m_text[length++] = 0x0d;
		length += strcpylen( &cmd.Scroll.m_text[length], "  " );
		cmd.Scroll.m_text[length++] = 0x0d;
	}

	while (s.ReadKey(false))
	{
		if ( ! strnicmp( s.GetKey(), ".line", 5 ))	// just a blank line.
		{
			length += strcpylen( &cmd.Scroll.m_text[length], " " );
		}
		else
		{
			length += strcpylen( &cmd.Scroll.m_text[length], s.GetKey());
		}
		cmd.Scroll.m_text[length++] = 0x0d;
	}

	cmd.Scroll.m_lentext = length;
	length += sizeof( cmd.Scroll ) - sizeof( cmd.Scroll.m_text );
	cmd.Scroll.m_len = length;

	xSendPkt( &cmd, length );
}

void CClient::addScrollResource( LPCTSTR pszSec, SCROLL_TYPE type, DWORD scrollID )
{
	//
	// type = 0 = TIPS
	// type = 2 = UPDATES

	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, RES_SCROLL, pszSec ))
		return;
	addScrollScript( s, type, scrollID );
}

void CClient::addVendorClose( const CChar * pVendor )
{
	// Clear the vendor display.
	CCommand cmd;
	cmd.VendorBuy.m_Cmd = XCMD_VendorBuy;
	cmd.VendorBuy.m_len = sizeof( cmd.VendorBuy );
	cmd.VendorBuy.m_UIDVendor = pVendor->GetUID();
	cmd.VendorBuy.m_flag = 0;	// 0x00 = no items following, 0x02 - items following
	xSendPkt( &cmd, sizeof( cmd.VendorBuy ));
}

int CClient::addShopItems(CChar * pVendor, LAYER_TYPE layer, bool bReal)
{
	// Player buying from vendor.
	// Show the Buy menu for the contents of this container
	// RETURN: the number of items in container.
	//   < 0 = error.
	CItemContainer * pContainer = pVendor->GetBank( layer );
	if ( pContainer == NULL )
		return( -1 );

	CItemBase * pResearchItem	= pVendor->GetKeyItemBase( "RESEARCH.ITEM" );

	addItem(pContainer);
	if ( !bReal )
		addPause();
	else
		addContents( pContainer, false, false, true, (pResearchItem != NULL) );

	// add the names and prices for stuff.
	CCommand cmd;
	cmd.VendOpenBuy.m_Cmd = XCMD_VendOpenBuy;
	cmd.VendOpenBuy.m_VendorUID = pContainer->GetUID();
	int len = sizeof( cmd.VendOpenBuy ) - sizeof(cmd.VendOpenBuy.m_item);

	int iConvertFactor = pVendor->NPC_GetVendorMarkup(m_pChar );

	CCommand * pCur = &cmd;
	int count = 0;
	if ( bReal ) for ( CItem* pItem = pContainer->GetContentHead(); pItem!=NULL; pItem = pItem->GetNext())
	{
		if ( ! pItem->GetAmount())
			continue;

		CItemVendable * pVendItem = dynamic_cast <CItemVendable *> (pItem);
		if ( pVendItem == NULL )
			continue;

		if ( pResearchItem )
		{
			// skip items the character has no skill to make
			// notice addContents does the same
			if ( !m_pChar->SkillResourceTest( &(pItem->Item_GetDef()->m_SkillMake), (ITEMID_TYPE) 0 ) )
				continue;
		}

		long lPrice = pVendItem->GetVendorPrice(iConvertFactor);
		if ( ! lPrice )
		{
			pVendItem->IsValidNPCSaleItem();
			// This stuff will show up anyhow. unpriced.
			// continue;
			lPrice = 100000;
		}

		pCur->VendOpenBuy.m_item[0].m_price = lPrice;

		int	lenname	= 0;
		if ( pResearchItem )
		{
			strcpy( pCur->VendOpenBuy.m_item[0].m_text, "Research: " );
			lenname	= 10;
		}
		lenname	+= strcpylen( pCur->VendOpenBuy.m_item[0].m_text + lenname, pVendItem->GetName() );

		pCur->VendOpenBuy.m_item[0].m_len = lenname + 1;
		lenname += sizeof( cmd.VendOpenBuy.m_item[0] );
		len += lenname;
		pCur = (CCommand *)( ((BYTE*) pCur ) + lenname );
		if ( ++count >= MAX_ITEMS_CONT )
			break;
	}
	cmd.VendOpenBuy.m_len = len;
	cmd.VendOpenBuy.m_count = count;
	xSendPkt( &cmd, len );
	return count;
}

bool CClient::addShopMenuBuy( CChar * pVendor )
{
	// Try to buy stuff that the vendor has.
	ASSERT( pVendor );
	if ( ! pVendor->NPC_IsVendor())
		return( false );

	addPause();
	addChar(pVendor);

	int iTotal = 0;
	int iRes = addShopItems(pVendor, LAYER_VENDOR_STOCK);
	if ( iRes < 0 )
		return false;
	iTotal += iRes;
	if ( iTotal <= 0 )
		return false;

		//	since older clients like 2.0.3 will crash without extra packets, let's provide
		//	some empty packets specialy for them
	addShopItems(pVendor, LAYER_VENDOR_EXTRA, false);

	addOpenGump( pVendor, GUMP_VENDOR_RECT );
	addCharStatWindow( m_pChar->GetUID());	// Make sure the gold total has been updated.
	return( true );
}

int CClient::addShopMenuSellFind( CItemContainer * pSearch, CItemContainer * pVend1, CItemContainer * pVend2, int iConvertFactor, CCommand * & pCur )
{
	// What things do you have in your inventory that the vendor would want ?
	// Search sub containers if necessary.
	// RETURN: How many items did we find.

	ASSERT(pSearch);
	int iCount = 0;

	for ( CItem* pItem = pSearch->GetContentHead() ; pItem!=NULL; pItem = pItem->GetNext())
	{
		// We won't buy containers with items in it.
		// But we will look in them.
		CItemContainer* pContItem = dynamic_cast <CItemContainer*>(pItem);
		if ( pContItem != NULL && pContItem->GetCount())
		{
			if ( pContItem->IsSearchable())
			{
				iCount += addShopMenuSellFind( pContItem, pVend1, pVend2, iConvertFactor, pCur );
			}
			continue;
		}

		CItemVendable * pVendItem = dynamic_cast <CItemVendable *> (pItem);
		if ( pVendItem == NULL )
			continue;

		CItemVendable * pItemSell = CChar::NPC_FindVendableItem( pVendItem, pVend1, pVend2 );
		if ( pItemSell == NULL )
			continue;

		pCur->VendOpenSell.m_item[0].m_UID = pVendItem->GetUID();
		pCur->VendOpenSell.m_item[0].m_id = pVendItem->GetDispID();

		HUE_TYPE wHue = pVendItem->GetHue() & HUE_MASK_HI;
		if ( wHue > HUE_QTY )
			wHue &= HUE_MASK_LO;

		pCur->VendOpenSell.m_item[0].m_wHue = wHue;
		pCur->VendOpenSell.m_item[0].m_amount = pVendItem->GetAmount();
		pCur->VendOpenSell.m_item[0].m_price = pItemSell->GetVendorPrice(iConvertFactor);

		int lenname = strcpylen( pCur->VendOpenSell.m_item[0].m_text, pVendItem->GetName());
		pCur->VendOpenSell.m_item[0].m_len = lenname + 1;
		pCur = (CCommand *)( ((BYTE*) pCur ) + lenname + sizeof( pCur->VendOpenSell.m_item[0] ));

		if ( ++iCount >= MAX_ITEMS_CONT )
			break;
	}

	return( iCount );
}

bool CClient::addShopMenuSell( CChar * pVendor )
{
	// Player selling to vendor.
	// What things do you have in your inventory that the vendor would want ?
	// Should end with a returned Event_VendorSell()

	ASSERT(pVendor);
	if ( ! pVendor->NPC_IsVendor())
		return( false );

	int iConvertFactor		= - pVendor->NPC_GetVendorMarkup( m_pChar );

	addPause();
	CItemContainer * pContainer1 = pVendor->GetBank( LAYER_VENDOR_BUYS );
	addItem( pContainer1 );
	CItemContainer * pContainer2 = pVendor->GetBank( LAYER_VENDOR_STOCK );
	addItem( pContainer2 );

	if ( pVendor->IsStatFlag( STATF_Pet ))	// Player vendor.
	{
		pContainer2 = NULL; // no stock
	}

	CCommand cmd;
	cmd.VendOpenSell.m_Cmd = XCMD_VendOpenSell;
	cmd.VendOpenSell.m_UIDVendor = pVendor->GetUID();

	CCommand * pCur = (CCommand *)((BYTE*) &cmd );
	int iCount = addShopMenuSellFind( m_pChar->GetPackSafe(), pContainer1, pContainer2, iConvertFactor, pCur );
	if ( ! iCount )
	{
		pVendor->Speak( "Thou doth posses nothing of interest to me." );
		return( false );
	}

	cmd.VendOpenSell.m_len = (((BYTE*)pCur) - ((BYTE*) &cmd )) + sizeof( cmd.VendOpenSell ) - sizeof(cmd.VendOpenSell.m_item);
	cmd.VendOpenSell.m_count = iCount;

	xSendPkt( &cmd, cmd.VendOpenSell.m_len );
	return( true );
}

void CClient::addBankOpen( CChar * pChar, LAYER_TYPE layer )
{
	// open it up for this pChar.
	ASSERT( pChar );

	CItemContainer * pBankBox = pChar->GetBank(layer);
	ASSERT(pBankBox);
	addItem( pBankBox );	// may crash client if we dont do this.

	if ( pChar != GetChar())
	{
		// xbank verb on others needs this ?
		// addChar( pChar );
	}

	pBankBox->OnOpenEvent( m_pChar, pChar );
	addContainerSetup( pBankBox );
}

void CClient::addMap( CItemMap * pMap )
{
	// Make player drawn maps possible. (m_map_type=0) ???

	if ( pMap == NULL )
	{
blank_map:
		addSysMessage( "This map is blank." );
		return;
	}
	if ( pMap->IsType(IT_MAP_BLANK))
		goto blank_map;

	CRectMap rect;
	rect.SetRect( pMap->m_itMap.m_left,
		pMap->m_itMap.m_top,
		pMap->m_itMap.m_right,
		pMap->m_itMap.m_bottom,
		GetChar()->GetTopPoint().m_map);

	if ( ! rect.IsValid())
		goto blank_map;
	if ( rect.IsRectEmpty())
		goto blank_map;

	DEBUG_CHECK( pMap->IsType(IT_MAP));

	addPause();

	CCommand cmd;
	cmd.MapDisplay.m_Cmd = XCMD_MapDisplay;
	cmd.MapDisplay.m_UID = pMap->GetUID();
	cmd.MapDisplay.m_Gump_Corner = GUMP_MAP_2_NORTH;
	cmd.MapDisplay.m_x_ul = rect.m_left;
	cmd.MapDisplay.m_y_ul = rect.m_top;
	cmd.MapDisplay.m_x_lr = rect.m_right;
	cmd.MapDisplay.m_y_lr = rect.m_bottom;
	cmd.MapDisplay.m_xsize = 0xc8;	// ??? we could make bigger maps ?
	cmd.MapDisplay.m_ysize = 0xc8;
	xSendPkt( &cmd, sizeof( cmd.MapDisplay ));

	addMapMode( pMap, MAP_UNSENT, false );

	// Now show all the pins
	cmd.MapEdit.m_Cmd = XCMD_MapEdit;
	cmd.MapEdit.m_UID = pMap->GetUID();
	cmd.MapEdit.m_Mode = 0x1;	// MAP_PIN?
	cmd.MapEdit.m_Req = 0x00;

	for ( int i=0; i < pMap->m_Pins.GetCount(); i++ )
	{
		cmd.MapEdit.m_pin_x = pMap->m_Pins[i].m_x;
		cmd.MapEdit.m_pin_y = pMap->m_Pins[i].m_y;
		xSendPkt( &cmd, sizeof( cmd.MapEdit ));
	}
}

void CClient::addMapMode( CItemMap * pMap, MAPCMD_TYPE iType, bool fEdit )
{
	// NOTE: MAPMODE_* depends on who is looking. Multi clients could interfere with each other ?

	ASSERT( pMap );
	DEBUG_CHECK( pMap->IsType(IT_MAP) );
	pMap->m_fPlotMode = fEdit;

	CCommand cmd;
	cmd.MapEdit.m_Cmd = XCMD_MapEdit;
	cmd.MapEdit.m_UID = pMap->GetUID();
	cmd.MapEdit.m_Mode = iType;
	cmd.MapEdit.m_Req = fEdit;
	cmd.MapEdit.m_pin_x = 0;
	cmd.MapEdit.m_pin_y = 0;
	xSendPkt( &cmd, sizeof( cmd.MapEdit ));
}

void CClient::addBulletinBoard( const CItemContainer * pBoard )
{
	// Open up the bulletin board and all it's messages
	// Event_BBoardRequest

	ASSERT( pBoard );
	DEBUG_CHECK( pBoard->IsType(IT_BBOARD) );

	addPause();

	CCommand cmd;

	// Give the bboard name.
	cmd.BBoard.m_Cmd = XCMD_BBoard;
	int len = strcpylen( (TCHAR *) cmd.BBoard.m_data, pBoard->GetName(), MAX_ITEM_NAME_SIZE );
	len += sizeof(cmd.BBoard);
	cmd.BBoard.m_len = len;
	cmd.BBoard.m_flag = BBOARDF_NAME;
	cmd.BBoard.m_UID = pBoard->GetUID(); // 4-7 = UID for the bboard.
	xSendPkt( &cmd, len );

	// Send Content messages for all the items on the bboard.
	// Not sure what x,y are here, date/time maybe ?
	addContents( pBoard, false, false, false );

	// The client will now ask for the headers it wants.
}

bool CClient::addBBoardMessage( const CItemContainer * pBoard, BBOARDF_TYPE flag, CGrayUID uidMsg )
{
	ASSERT(pBoard);

	CItemMessage * pMsgItem = dynamic_cast <CItemMessage *> ( uidMsg.ItemFind());
	if ( ! pBoard->IsItemInside( pMsgItem ))
		return( false );

	addPause();

	// Send back the message header and/or body.
	CCommand cmd;
	cmd.BBoard.m_Cmd = XCMD_BBoard;
	cmd.BBoard.m_flag = ( flag == BBOARDF_REQ_FULL ) ? BBOARDF_MSG_BODY : BBOARDF_MSG_HEAD;
	cmd.BBoard.m_UID = pBoard->GetUID();	// 4-7 = UID for the bboard.

	int len = 4;
	PACKDWORD(cmd.BBoard.m_data+0,pMsgItem->GetUID());

	if ( flag == BBOARDF_REQ_HEAD )
	{
		// just the header has this ? (replied to message?)
		PACKDWORD(cmd.BBoard.m_data+4,0);
		len += 4;
	}

	// author name. if it has one.
	if ( pMsgItem->m_sAuthor.IsEmpty())
	{
		cmd.BBoard.m_data[len++] = 0x01;
		cmd.BBoard.m_data[len++] = 0;
	}
	else
	{
		CChar * pChar = pMsgItem->m_uidLink.CharFind();
		if ( pChar == NULL )	// junk it if bad author. (deleted?)
		{
			pMsgItem->Delete();
			return false;
		}
		LPCTSTR pszAuthor = pMsgItem->m_sAuthor;
		if ( IsPriv(PRIV_GM))
		{
			pszAuthor = m_pChar->GetName();
		}
		int lenstr = strlen(pszAuthor) + 1;
		cmd.BBoard.m_data[len++] = lenstr;
		strcpy( (TCHAR*) &cmd.BBoard.m_data[len], pszAuthor);
		len += lenstr;
	}

	// Pad this out with spaces to indent next field.
	int lenstr = strlen( pMsgItem->GetName()) + 1;
	cmd.BBoard.m_data[len++] = lenstr;
	strcpy( (TCHAR*) &cmd.BBoard.m_data[len], pMsgItem->GetName());
	len += lenstr;

	// Get the BBoard message time stamp. m_itBook.m_Time
	CServTime time;
	time = pMsgItem->m_itBook.m_Time;

	TCHAR *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "Day %d", ( g_World.GetGameWorldTime(time) / (24*60)) % 365 );
	lenstr = strlen(pszTemp) + 1;
	cmd.BBoard.m_data[len++] = lenstr;
	strcpy((TCHAR*)&cmd.BBoard.m_data[len], pszTemp);
	len += lenstr;

	if ( flag == BBOARDF_REQ_FULL )
	{
		// request for full message body
		//
		PACKDWORD(&(cmd.BBoard.m_data[len]),0);
		len += 4;

		// Pack the text into seperate lines.
		int lines = pMsgItem->GetPageCount();

		// number of lines.
		PACKWORD(&(cmd.BBoard.m_data[len]),lines);
		len += 2;

		// Now pack all the lines
		for ( int i=0; i<lines; i++ )
		{
			LPCTSTR pszText = pMsgItem->GetPageText(i);
			if ( pszText == NULL )
				continue;
			lenstr = strlen(pszText)+1;
			cmd.BBoard.m_data[len++] = lenstr;
			strcpy( (TCHAR*) &cmd.BBoard.m_data[len], pszText );
			len += lenstr;
		}
	}

	len = sizeof( cmd.BBoard ) - sizeof( cmd.BBoard.m_data ) + len;
	cmd.BBoard.m_len = len;
	xSendPkt( &cmd, len );
	return( true );
}

void CClient::addPing( BYTE bCode )
{
	if ( ! IsConnectTypePacket())
	{
		// Not sure how to ping anything but a game client.
		return;
	}
	// Client never responds to this.
	CCommand cmd;
	cmd.Ping.m_Cmd = XCMD_Ping;
	cmd.Ping.m_bCode = bCode;
	xSendPkt( &cmd, sizeof(cmd.Ping)); // who knows what this does?
}

void CClient::addEnableFeatures( int features )
{
	CCommand cmd;
	cmd.FeaturesEnable.m_Cmd = XCMD_Features;
	cmd.FeaturesEnable.m_enable = features;
	xSendPkt( &cmd, sizeof( cmd.FeaturesEnable ));
}

void CClient::addRedrawAll()
{
	CCommand cmd;
	cmd.ReDrawAll.m_Cmd = XCMD_ReDrawAll;
	xSendPkt( &cmd, sizeof(cmd.ReDrawAll));
}

void CClient::addChatSystemMessage( CHATMSG_TYPE iType, LPCTSTR pszName1, LPCTSTR pszName2, CLanguageID lang )
{
	CCommand cmd;
	cmd.ChatReq.m_Cmd = XCMD_ChatReq;
	cmd.ChatReq.m_subcmd = iType;

	if ( iType >= CHATMSG_PlayerTalk && iType <= CHATMSG_PlayerPrivate) // These need the language stuff
		lang.GetStrDef( cmd.ChatReq.m_lang ); // unicode support: pszLang
	else
		memset( cmd.ChatReq.m_lang, 0, sizeof(cmd.ChatReq.m_lang));

	// Convert internal UTF8 to UNICODE for client.
	// ? If we're sending out player names, prefix name with moderator status

	if ( pszName1 == NULL )
		pszName1 = "";
	int len1 = CvtSystemToNUNICODE( cmd.ChatReq.m_uname, MAX_TALK_BUFFER, pszName1, -1 );

	if ( pszName2 == NULL )
		pszName2 = "";
	int len2 = CvtSystemToNUNICODE( cmd.ChatReq.m_uname+len1+1, MAX_TALK_BUFFER, pszName2, -1 );

	int len = sizeof(cmd.ChatReq) + (len1*2) + (len2*2);
	cmd.ChatReq.m_len = len;
	xSendPkt( &cmd, len );
}

void CClient::addGumpTextDisp( const CObjBase * pObj, GUMP_TYPE gump, LPCTSTR pszName, LPCTSTR pszText )
{
	DEBUG_CHECK( gump < GUMP_QTY );

	// ??? how do we control where exactly the text goes ??

	// display a gump with some text on it.
	int lenname = pszName ? strlen( pszName ) : 0 ;
	lenname++;
	int lentext = pszText ? strlen( pszText ) : 0 ;
	lentext++;

	CCommand cmd;

	int len = ( sizeof(cmd.GumpTextDisp) - 2 ) + lenname + lentext;

	cmd.GumpTextDisp.m_Cmd = XCMD_GumpTextDisp;
	cmd.GumpTextDisp.m_len = len;
	cmd.GumpTextDisp.m_UID = pObj ? ((DWORD)( pObj->GetUID())) : UID_CLEAR;
	cmd.GumpTextDisp.m_gump = gump;
	cmd.GumpTextDisp.m_len_unktext = lenname;
	cmd.GumpTextDisp.m_unk11 = 10;	// ? not HUE_TYPE, not x,
	strcpy( cmd.GumpTextDisp.m_unktext, ( pszName ) ? pszName : "" );

	CCommand * pCmd = (CCommand *)(((BYTE*)(&cmd))+lenname-1);
	strcpy( pCmd->GumpTextDisp.m_text, ( pszText ) ? pszText : "" );
	xSendPkt( &cmd, len );
}

void CClient::addItemMenu( CLIMODE_TYPE mode, const CMenuItem * item, int count, CObjBase * pObj )
{
	// We must set GetTargMode() to show what mode we are in for menu select.
	// Will result in Event_MenuChoice()
	// cmd.ItemMenu.

	if ( ! count )
		return;
	if ( pObj == NULL )
		pObj = m_pChar;

	CCommand cmd;
	cmd.MenuItems.m_Cmd = XCMD_MenuItems;
	cmd.MenuItems.m_UID = pObj->GetUID();
	cmd.MenuItems.m_context = mode;

	int len = sizeof( cmd.MenuItems ) - sizeof( cmd.MenuItems.m_item );
	int lenttitle = item[0].m_sText.GetLength();
	cmd.MenuItems.m_lenname = lenttitle;
	strcpy( cmd.MenuItems.m_name, item[0].m_sText );

	lenttitle --;
	len += lenttitle;
	CCommand * pCmd = (CCommand *)(((BYTE*)&cmd) + lenttitle );
	pCmd->MenuItems.m_count = count;

	// Strings in here and NOT null terminated.
	for ( int i=1; i<=count; i++ )
	{
		int lenitem = item[i].m_sText.GetLength();
		if ( lenitem <= 0 || lenitem >= 256 )
		{
			DEBUG_ERR(("Bad option length %d in menu item %d\n", lenitem, i ));
			continue;
		}

		pCmd->MenuItems.m_item[0].m_id = item[i].m_id; // image next to menu.
		pCmd->MenuItems.m_item[0].m_check = 0;	// check or not ?
		pCmd->MenuItems.m_item[0].m_lentext = lenitem;
		strcpy( pCmd->MenuItems.m_item[0].m_name, item[i].m_sText );

		lenitem += sizeof( cmd.MenuItems.m_item[0] ) - 1;
		pCmd = (CCommand *)(((BYTE*)pCmd) + lenitem );
		len += lenitem;
	}

	cmd.MenuItems.m_len = len;
	xSendPkt( &cmd, len );

	m_tmMenu.m_UID = pObj->GetUID();

	SetTargMode( mode );
}


bool CClient::addWalkCode( EXTDATA_TYPE iType, int iCodes )
{
	// Fill up the walk code buffer.
	// RETURN: true = new codes where sent.

	if ( ! m_Crypt.IsInit() )	// This is not even a game client ! IsConnectTypePacket()
		return false;
	if ( ! IsNoCryptVer(1) )
		if ( m_Crypt.GetClientVer() < 0x126000 )
			return false;
	if ( ! ( g_Cfg.m_wDebugFlags & DEBUGF_WALKCODES ))
		return( false );

	if ( iType == EXTDATA_WalkCode_Add )
	{
		if ( m_Walk_InvalidEchos >= 0 )
			return false;					// they are stuck til they give a valid echo!
		// On a timer tick call this.
		if ( m_Walk_CodeQty >= COUNTOF(m_Walk_LIFO))	// They are appearently not moving fast.
			return false;
	}
	else
	{
		// Fill the buffer at start.
		ASSERT( m_Walk_CodeQty < 0 );
		m_Walk_CodeQty = 0;
	}

	ASSERT( iCodes <= COUNTOF(m_Walk_LIFO));

	// make a new code and send it out
	CExtData ExtData;
	int i;
	for ( i=0; i < iCodes && m_Walk_CodeQty < COUNTOF(m_Walk_LIFO); m_Walk_CodeQty++, i++ )
	{
		DWORD dwCode = 0x88ca0000 + Calc_GetRandVal( 0xffff );
		m_Walk_LIFO[m_Walk_CodeQty] = dwCode;
		ExtData.WalkCode[i] = dwCode;
	}

	addExtData( iType, &ExtData, i*sizeof(DWORD));
	return( true );
}



//---------------------------------------------------------------------
// Login type stuff.

bool CClient::Setup_Start( CChar * pChar ) // Send character startup stuff to player
{
	// Play this char.
	ASSERT( GetAccount() );
	ASSERT( pChar );
	char	*z = Str_GetTemp();

	CharDisconnect();	// I'm already logged in as someone else ?

	g_Log.Event( LOGM_CLIENTS_LOG, "%x:Setup_Start acct='%s', char='%s'\n", m_Socket.GetSocket(), (LPCTSTR) GetAccount()->GetName(), (LPCTSTR) pChar->GetName());

#ifndef _DEBUG
	srand( CWorldClock::GetSystemClock()); // Perform randomize
#endif

	bool fQuickLogIn = false;
	if ( ! pChar->IsDisconnected())
	{
		// The players char is already in game ! Client linger time re-login.
		fQuickLogIn = true;
	}

	addPlayerStart( pChar );
	ASSERT(m_pChar);

	// Gump memory cleanup, we don't want them from logged out players
	m_pChar->Memory_ClearTypes(MEMORY_GUMPRECORD);

	//	gms should login with invul and without allshow flag set
	if ( GetPrivLevel() >= PLEVEL_Counsel )
	{
		if ( IsPriv(PRIV_ALLSHOW) ) ClearPrivFlags(PRIV_ALLSHOW);
		if ( !pChar->IsStatFlag(STATF_INVUL) ) pChar->StatFlag_Set(STATF_INVUL);
	}

	bool	fNoMessages	= false;

	CScriptTriggerArgs	Args( fNoMessages, fQuickLogIn, NULL );

	if ( pChar->OnTrigger( CTRIG_LogIn, pChar, &Args ) == TRIGRET_RET_TRUE )
	{
		m_pChar->ClientDetach();
		pChar->SetDisconnected();
		addLoginErr( LOGIN_ERR_BLOCKED );
		return false;
	}

	fNoMessages	= (Args.m_iN1 != 0);
	fQuickLogIn	= (Args.m_iN2 != 0);

	if ( fQuickLogIn )
	{
		//	Allow this information to be send if needed from @Login trigger
		//addSysMessage( "Welcome Back" );
	}
	else if ( !fNoMessages )
	{
		sprintf(z, g_szServerDescription, g_Cfg.m_sVerName.GetPtr(), g_Serv.m_sServVersion.GetPtr());
		addBark(z, NULL, HUE_YELLOW, TALKMODE_SYSTEM, FONT_NORMAL);

		sprintf(z, (g_Serv.m_Clients.GetCount()==2) ?
			g_Cfg.GetDefaultMsg( DEFMSG_LOGIN_PLAYER ) : g_Cfg.GetDefaultMsg( DEFMSG_LOGIN_PLAYERS ),
			g_Serv.m_Clients.GetCount()-1 );
		addSysMessage(z);

		// Get the intro script.
		addScrollResource(( GetPrivLevel() <= PLEVEL_Guest ) ? "SCROLL_GUEST" : "SCROLL_MOTD", SCROLL_TYPE_UPDATES );

		sprintf(z, "Last logged: %s", GetAccount()->m_TagDefs.GetKeyStr("LastLogged"));
		addSysMessage(z);
	}

	if ( ! g_Cfg.m_fSecure )
	{
		addBark( "WARNING: The world is NOT running in SECURE MODE",
			NULL, HUE_TEXT_DEF, TALKMODE_SYSTEM, FONT_BOLD );
	}
	if ( IsPriv( PRIV_GM_PAGE ))
	{
		sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_GMPAGES), g_World.m_GMPages.GetCount());
		addSysMessage(z);
	}
	if ( g_Cfg.m_fRequireEmail )
	{
		// Have you set your email notification ?
		if ( GetAccount()->m_sEMail.IsEmpty())
		{
			sprintf(z, g_Cfg.GetDefaultMsg( DEFMSG_EMAIL_NOTSET_1 ),
				(LPCTSTR)GetAccount()->GetName(), (LPCTSTR)(GetAccount()->m_sEMail));
			addSysMessage(z);
			addSysMessage(g_Cfg.GetDefaultMsg(DEFMSG_EMAIL_NOTSET_2));

			//set me to "GUEST" mode.
		}
	}
	if ( IsPriv( PRIV_JAILED ))
	{
		m_pChar->Jail( &g_Serv, true, 0 );
	}
	if ( ! fQuickLogIn && m_pChar->m_pArea != NULL &&
		m_pChar->m_pArea->IsGuarded() &&
		! m_pChar->m_pArea->IsFlag( REGION_FLAG_ANNOUNCE ))
	{
      		// this->GetClient()->addBarkSpeakTable( (SPKTAB_TYPE)112, NULL, 0x3b2, TALKMODE_ITEM, FONT_NORMAL );
		CVarDefStr * pVarStr = dynamic_cast <CVarDefStr *>( m_pChar->m_pArea->m_TagDefs.GetKey("GUARDOWNER"));
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_REGION_GUARDSP ),
			( pVarStr ) ? (LPCTSTR) pVarStr->GetValStr() : g_Cfg.GetDefaultMsg( DEFMSG_REGION_GUARDSPT ) );
	}
	if ( g_Serv.m_timeShutdown.IsTimeValid() )
	{
		addBark( "WARNING: The system is scheduled to shutdown soon",
			NULL, HUE_TEXT_DEF, TALKMODE_SYSTEM, FONT_BOLD );
	}

	// Announce you to the world.
	Announce( true );
	m_pChar->Update( this );

	if ( g_Cfg.m_fRequireEmail && GetAccount()->m_sEMail.IsEmpty())
	{
		// Prompt to set this right now !
	}

	// don't login on the water ! (unless i can swim)
	if ( ! m_pChar->Char_GetDef()->Can(CAN_C_SWIM) &&
		! IsPriv(PRIV_GM) &&
		m_pChar->IsSwimming())
	{
		// bring to the nearest shore.
		// drift to nearest shore ?
		int iDist = 1;
		int i;
		for ( i=0; i<20; i++)
		{
			// try diagonal in all directions
			int iDistNew = iDist + 20;
			for ( int iDir = DIR_NE; iDir <= DIR_NW; iDir += 2 )
			{
				if ( m_pChar->MoveToValidSpot( (DIR_TYPE) iDir, iDistNew, iDist ))
				{
					i = 100;	// breakout
					break;
				}
			}
			iDist = iDistNew;
		}
		if ( i < 100 )
		{
			addSysMessage( g_Cfg.GetDefaultMsg( DEFMSG_REGION_WATER_1 ) );
		}
		else
		{
			addSysMessage( g_Cfg.GetDefaultMsg( DEFMSG_REGION_WATER_2 ) );
		}
	}

	DEBUG_TRACE(( "%x:Setup_Start done\n", m_Socket.GetSocket()));
	return true;
}

void CClient::Setup_CreateDialog( const CEvent * pEvent ) // All the character creation stuff
{
	ASSERT( GetAccount());
	if ( m_pChar != NULL )
	{
		// Loggin in as a new player while already on line !
		addSysMessage( g_Cfg.GetDefaultMsg( DEFMSG_ALREADYONLINE ) );
		DEBUG_ERR(( "%x:Setup_CreateDialog acct='%s' already on line!\n", m_Socket.GetSocket(), GetAccount()->GetName()));
		return;
	}

	// Make sure they don't already have too many chars !
	int iMaxChars = ( IsPriv( PRIV_GM )) ? MAX_CHARS_PER_ACCT : ( g_Cfg.m_iMaxCharsPerAccount );
	int iQtyChars = GetAccount()->m_Chars.GetCharCount();
	if ( iQtyChars >= iMaxChars )
	{
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_MAXCHARS ), iQtyChars );
		if ( GetPrivLevel() < PLEVEL_Seer )
		{
			addLoginErr( LOGIN_ERR_OTHER );
			return;
		}
	}

	CChar * pChar = CChar::CreateBasic( CREID_MAN );
	ASSERT(pChar);
	pChar->InitPlayer( pEvent, this );

	g_Log.Event( LOGM_CLIENTS_LOG, "%x:Setup_CreateDialog acct='%s', char='%s'\n",
		m_Socket.GetSocket(), (LPCTSTR)GetAccount()->GetName(), (LPCTSTR)pChar->GetName());

	pChar->OnTrigger(CTRIG_CreatePlayer, pChar, NULL);

	Setup_Start( pChar );
}

bool CClient::Setup_Play( int iSlot ) // After hitting "Play Character" button
{
	// Mode == CLIMODE_SETUP_CHARLIST

	DEBUG_TRACE(( "%x:Setup_Play slot %d\n", m_Socket.GetSocket(), iSlot ));

	if ( ! GetAccount())
		return( false );
	if ( iSlot >= COUNTOF(m_tmSetupCharList))
		return false;

	CChar * pChar = m_tmSetupCharList[ iSlot ].CharFind();
	if ( ! GetAccount()->IsMyAccountChar( pChar ))
		return false;

	return Setup_Start( pChar );
}

DELETE_ERR_TYPE CClient::Setup_Delete( int iSlot ) // Deletion of character
{
	ASSERT( GetAccount() );
	DEBUG_MSG(( "%x:Setup_Delete slot=%d\n", m_Socket.GetSocket(), iSlot ));
	if ( iSlot >= COUNTOF(m_tmSetupCharList))
		return DELETE_ERR_NOT_EXIST;

	CChar * pChar = m_tmSetupCharList[iSlot].CharFind();
	if ( ! GetAccount()->IsMyAccountChar( pChar ))
		return DELETE_ERR_BAD_PASS;

	if ( ! pChar->IsDisconnected())
	{
		return DELETE_ERR_IN_USE;
	}

	// Make sure the char is at least x days old.
	if ( g_Cfg.m_iMinCharDeleteTime &&
		(- g_World.GetTimeDiff( pChar->m_timeCreate )) < g_Cfg.m_iMinCharDeleteTime )
	{
		if ( GetPrivLevel() < PLEVEL_Seer )
		{
			return DELETE_ERR_NOT_OLD_ENOUGH;
		}
	}

	//	Do the scripts allow to delete the char?
	CScriptTriggerArgs	args;
	enum TRIGRET_TYPE	tr;
	r_Call("f_onchar_delete", pChar, &args, NULL, &tr);
	if ( tr == TRIGRET_RET_TRUE )
	{
		return DELETE_ERR_NOT_OLD_ENOUGH;
	}

	// pChar->Delete();
	delete pChar;
	// refill the list.

	CCommand cmd;
	cmd.CharList2.m_Cmd = XCMD_CharList2;
	int len = sizeof( cmd.CharList2 );
	cmd.CharList2.m_len = len;
	cmd.CharList2.m_count = Setup_FillCharList( cmd.CharList2.m_char, GetAccount()->m_uidLastChar.CharFind());
	xSendPkt( &cmd, len );

	return( DELETE_SUCCESS );
}

LOGIN_ERR_TYPE CClient::Setup_ListReq( const char * pszAccName, const char * pszPassword, bool fTest )
{
	// XCMD_CharListReq
	// Gameserver login and request character listing

	if ( GetConnectType() != CONNECT_GAME )	// Not a game connection ?
		return(LOGIN_ERR_OTHER);

	switch ( GetTargMode())
	{
	case CLIMODE_SETUP_RELAY:
		ClearTargMode();
		break;
	}

	CGString sMsg;
	LOGIN_ERR_TYPE lErr = LOGIN_ERR_OTHER;

	lErr = LogIn( pszAccName, pszPassword, sMsg );

	if ( lErr != LOGIN_SUCCESS )
	{
		if ( fTest && lErr != LOGIN_ERR_OTHER )
		{
			if ( ! sMsg.IsEmpty())
			{
				SysMessage( sMsg );
			}
			addLoginErr(lErr);
		}
		return( lErr );
	}

	ASSERT( GetAccount() );

	CChar * pCharLast = GetAccount()->m_uidLastChar.CharFind();
	if ( pCharLast &&
		GetAccount()->IsMyAccountChar( pCharLast ) &&
		GetAccount()->GetPrivLevel() <= PLEVEL_GM &&
		! pCharLast->IsDisconnected())
	{
		// If the last char is lingering then log back into this char instantly.
		// m_iClientLingerTime
		if ( Setup_Start(pCharLast) )
			return LOGIN_SUCCESS;
		return LOGIN_ERR_BLOCKED /*LOGIN_ERR_OTHER*/; //Setup_Start() returns false only when login blocked by Return 1 in @Login
	}
	

	{
		CCommand cmd;
		cmd.FeaturesEnable.m_Cmd = XCMD_Features;
		cmd.FeaturesEnable.m_enable = g_Cfg.m_iFeatures;
		// Here always use xSendPktNow, since this packet has to be separated from the next one
		xSendPktNow( &cmd, sizeof( cmd.FeaturesEnable ));
	}

	// DEBUG_MSG(( "%x:Setup_ListFill\n", m_Socket.GetSocket()));

	CCommand cmd;
	cmd.CharList.m_Cmd = XCMD_CharList;
	int len = sizeof( cmd.CharList ) - sizeof(cmd.CharList.m_start) + ( g_Cfg.m_StartDefs.GetCount() * sizeof(cmd.CharList.m_start[0]));
	cmd.CharList.m_len = len;
	NDWORD *	flags	= (NDWORD*) (&(cmd.CharList.m_Cmd) + len - sizeof(NDWORD));
	*flags	= g_Cfg.m_iFeaturesLogin;

	DEBUG_CHECK( COUNTOF(cmd.CharList.m_char) == MAX_CHARS_PER_ACCT );

	// list chars to your account that may still be logged in !
	// "LASTCHARUID" = list this one first.
	cmd.CharList.m_count = Setup_FillCharList( cmd.CharList.m_char, pCharLast );

	// now list all the starting locations. (just in case we create a new char.)
	// NOTE: New Versions of the client just ignore all this stuff.

	int iCount = g_Cfg.m_StartDefs.GetCount();
	cmd.CharList.m_startcount = iCount;
	for ( int i=0;i<iCount;i++)
	{
		cmd.CharList.m_start[i].m_id = i+1;
		strcpylen( cmd.CharList.m_start[i].m_area, g_Cfg.m_StartDefs[i]->m_sArea, sizeof(cmd.CharList.m_start[i].m_area));
		strcpylen( cmd.CharList.m_start[i].m_name, g_Cfg.m_StartDefs[i]->m_sName, sizeof(cmd.CharList.m_start[i].m_name));
	}


	xSendPkt( &cmd, len );
	m_Targ_Mode = CLIMODE_SETUP_CHARLIST;
	return LOGIN_SUCCESS;
}

LOGIN_ERR_TYPE CClient::LogIn( CAccountRef pAccount, CGString & sMsg )
{
	if ( pAccount == NULL )
		return( LOGIN_ERR_NONE );

	if ( pAccount->IsPriv( PRIV_BLOCKED ))
	{
		sMsg.Format( g_Cfg.GetDefaultMsg( DEFMSG_ACC_BLOCKED ), (LPCTSTR) g_Serv.m_sEMail );
		return( LOGIN_ERR_BLOCKED );
	}

	// CSocketAddress PeerName = m_Socket.GetPeerName();
	// Look for this account already in use.
	CClient * pClientPrev = pAccount->FindClient( this );
	if ( pClientPrev != NULL )
	{
		// Only if it's from a diff ip ?
		ASSERT( pClientPrev != this );

		bool bInUse = false;

		//	different ip - no reconnect
		if ( ! m_PeerName.IsSameIP( pClientPrev->m_PeerName )) bInUse = true;
		else
		{
			//	from same ip - allow reconnect if the old char is lingering out
			CChar *pCharOld = pClientPrev->GetChar();
			if ( pCharOld )
			{
				CItem	*pItem = pCharOld->LayerFind(LAYER_FLAG_ClientLinger);
				if ( !pItem ) bInUse = true;
			}

			if ( !bInUse )
			{
				if ( IsConnectTypePacket() && pClientPrev->IsConnectTypePacket())
				{
					pClientPrev->CharDisconnect();
					pClientPrev->m_fClosed = true;
				}
				else if ( GetConnectType() == pClientPrev->GetConnectType() ) bInUse = true;
			}
		}

		if ( bInUse )
		{
			sMsg = "Account already in use.";
			return( LOGIN_ERR_USED );
		}
	}

	if ( g_Cfg.m_iClientsMax <= 0 )
	{
		// Allow no one but locals on.
		CSocketAddress SockName = m_Socket.GetSockName();
		if ( ! m_PeerName.IsLocalAddr() && SockName.GetAddrIP() != m_PeerName.GetAddrIP() )
		{
			sMsg = g_Cfg.GetDefaultMsg( DEFMSG_SERV_LD );
			return( LOGIN_ERR_BLOCKED );
		}
	}
	if ( g_Cfg.m_iClientsMax <= 1 )
	{
		// Allow no one but Administrator on.
		if ( pAccount->GetPrivLevel() < PLEVEL_Admin )
		{
			sMsg = g_Cfg.GetDefaultMsg( DEFMSG_SERV_AO );
			return( LOGIN_ERR_BLOCKED );
		}
	}
	if ( pAccount->GetPrivLevel() < PLEVEL_GM &&
		g_Serv.m_Clients.GetCount() > g_Cfg.m_iClientsMax  )
	{
		// Give them a polite goodbye.
		sMsg = g_Cfg.GetDefaultMsg( DEFMSG_SERV_FULL );
		return( LOGIN_ERR_BLOCKED );
	}
	//	Do the scripts allow to login this account?
	pAccount->m_Last_IP = m_PeerName;
	CScriptTriggerArgs Args;
	Args.Init(pAccount->GetName());
	enum TRIGRET_TYPE tr;
	r_Call("f_onaccount_login", &g_Serv, &Args, NULL, &tr);
	if ( tr == TRIGRET_RET_TRUE )
	{
		sMsg = g_Cfg.GetDefaultMsg( DEFMSG_ACC_DENIED );
		return (LOGIN_ERR_BLOCKED);
	}

	m_pAccount = pAccount;
	pAccount->OnLogin( this );

	return( LOGIN_SUCCESS );
}

LOGIN_ERR_TYPE CClient::LogIn( LPCTSTR pszAccName, LPCTSTR pszPassword, CGString & sMsg )
{
	// Try to validate this account.
	// Do not send output messages as this might be a console or web page or game client.
	// NOTE: addLoginErr() will get called after this.

	if ( GetAccount()) // already logged in.
		return( LOGIN_SUCCESS );

	TCHAR szTmp[ MAX_NAME_SIZE ];
	int iLen1 = strlen( pszAccName );
	int iLen2 = strlen( pszPassword );
	int iLen3 = Str_GetBare( szTmp, pszAccName, MAX_NAME_SIZE );
	if ( iLen1 == 0 ||
		iLen1 != iLen3 ||
		iLen1 > MAX_NAME_SIZE )	// a corrupt message.
	{
badformat:
		TCHAR szVersion[ 256 ];
		sMsg.Format( g_Cfg.GetDefaultMsg( DEFMSG_ACC_WCLI ), (LPCTSTR) m_Crypt.WriteClientVer( szVersion ));
		return( LOGIN_ERR_OTHER );
	}

	iLen3 = Str_GetBare( szTmp, pszPassword, MAX_NAME_SIZE );
	if ( iLen2 != iLen3 )	// a corrupt message.
		goto badformat;


	TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];
	if ( ! CAccount::NameStrip( szName, pszAccName ) )
	{
		g_Log.Event( LOGL_WARN|LOGM_CHEAT,
			"%x:Attempting to create bad account '%s'" DEBUG_CR,
			this->m_Socket.GetSocket(), pszAccName );
		return LOGIN_ERR_OTHER;
	}


	if ( Str_Check( pszAccName ) || Str_Check( pszPassword ) )
	{
		g_Log.Event( LOGL_WARN|LOGM_CHEAT,
			"%x:Cheater is trying to create account with carriage returns" DEBUG_CR,
			this->m_Socket.GetSocket() );
		return LOGIN_ERR_OTHER;
	}

	bool fGuestAccount = ! strnicmp( pszAccName, "GUEST", 5 );
	if ( fGuestAccount )
	{
		// trying to log in as some sort of guest.
		// Find or create a new guest account.
		TCHAR *pszTemp = Str_GetTemp();
		for ( int i=0; 1; i++ )
		{
			if ( i>=g_Cfg.m_iGuestsMax )
			{
				sMsg = g_Cfg.GetDefaultMsg( DEFMSG_ACC_GUSED );
				return( LOGIN_ERR_BLOCKED );
			}

			sprintf(pszTemp, "GUEST%d", i);
			CAccountRef pAccount = g_Accounts.Account_FindCreate(pszTemp, true );
			ASSERT( pAccount );

			if ( pAccount->FindClient() == NULL )
			{
				pszAccName = pAccount->GetName();
				break;
			}
		}
	}
	else
	{
		if ( pszPassword[0] == '\0' )
		{
			sMsg = g_Cfg.GetDefaultMsg( DEFMSG_ACC_NEEDPASS );
			return( LOGIN_ERR_BAD_PASS );
		}
	}

	bool fAutoCreate = ( g_Serv.m_eAccApp == ACCAPP_Free || g_Serv.m_eAccApp == ACCAPP_GuestAuto || g_Serv.m_eAccApp == ACCAPP_GuestTrial );

	CAccountRef pAccount = g_Accounts.Account_FindCreate(pszAccName, fAutoCreate);
	if ( ! pAccount )
	{
		// No account by this name.
#ifdef _DEBUG
		g_Log.Event( LOGM_CLIENTS_LOG, "%x:ERR Login NO Account '%s', pass='%s'\n", m_Socket.GetSocket(), pszAccName, pszPassword );
#else
		g_Log.Event( LOGM_CLIENTS_LOG, "%x:ERR Login NO Account '%s'\n", m_Socket.GetSocket(), pszAccName );
#endif
		sMsg.Format( g_Cfg.GetDefaultMsg( DEFMSG_ACC_UNK ), pszAccName );
		return( LOGIN_ERR_NONE );
	}

	if ( ! fGuestAccount && ! pAccount->IsPriv( PRIV_BLOCKED ))
	{
		if ( ! pAccount->CheckPassword(pszPassword))
		{
#ifdef _DEBUG
			g_Log.Event( LOGM_CLIENTS_LOG, "%x: '%s' bad pass '%s' != '%s'\n", m_Socket.GetSocket(), (LPCTSTR) pAccount->GetName(), (LPCTSTR) pszPassword, (LPCTSTR) pAccount->GetPassword());
#else
			g_Log.Event( LOGM_CLIENTS_LOG, "%x: '%s' bad password\n", m_Socket.GetSocket(), (LPCTSTR) pAccount->GetName() );
#endif
			sMsg = g_Cfg.GetDefaultMsg( DEFMSG_ACC_BADPASS );
			return( LOGIN_ERR_BAD_PASS );
		}
	}

	return LogIn( pAccount, sMsg );
}


void CClient::addAOSTooltip( const CObjBase * pObj )
{
	// TODO: AoS Support
	return;

#define MAX_OBJPROPS 8

	struct
	{
		int m_LocID;
		char m_Text[MAX_NAME_SIZE];
	} ObjProps[MAX_OBJPROPS];

	char cNameBuf[MAX_NAME_SIZE];
	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int z = 0;
	unsigned int len = 15; // Size of the header

	for ( ; y <= MAX_NAME_SIZE; y++ )
	{
		ObjProps[y].m_LocID = 0;
		strcpy(ObjProps[y].m_Text, "\0");
	}

	CCommand cmd;

	// Set up header
	cmd.AOSTooltip.m_Cmd = XCMD_AOSTooltip;

	cmd.AOSTooltip.m_Unk1 = 1;
	cmd.AOSTooltip.m_Unk2 = 0;
	cmd.AOSTooltip.m_Unk3 = 0;

	cmd.AOSTooltip.m_UID = (DWORD) pObj->GetUID();
	cmd.AOSTooltip.m_ListID = 1;

	if ( pObj->IsItem() )
	{
		const CItem * pItem = dynamic_cast <const CItem *> (pObj);

		// The name
		ObjProps[x].m_LocID = 1041522; // ~1~~2~~3~
		if ( pItem->GetAmount() > 1 )
			sprintf( ObjProps[x++].m_Text, "\t%d %s\t\x20", pItem->GetAmount(), pObj->GetName() );
		else
			sprintf( ObjProps[x++].m_Text, "\t%s\t\x20", pObj->GetName() );

		if ( pItem->IsAttr( ATTR_NEWBIE ) )
			ObjProps[x++].m_LocID = 1038021; // Blessed

		if ( pItem->IsAttr( ATTR_MAGIC ) )
			ObjProps[x++].m_LocID = 3010064; // Magic

		if ( pItem->IsContainer() )
		{
			// Show item count/weight in the container
			const CContainer * pContainer = dynamic_cast <const CContainer *> (pItem);

			ObjProps[x].m_LocID = 1050044; // ~1_COUNT~ items, ~2_WEIGHT~ stones

			sprintf( ObjProps[x++].m_Text, "%d\t%d\t", pContainer->ContentCountAll(), pItem->GetWeight() );
		}

		CItemBase * pItemDef = pItem->Item_GetDef();
		switch ( pItem->GetType() )
		{
		case IT_ARMOR:
		case IT_CLOTHING:
			ObjProps[x].m_LocID = 1060448; // physical resist ~1_val~%
			sprintf( ObjProps[x++].m_Text, "%d", pItem->Armor_GetDefense() );
op_durability:
			ObjProps[x].m_LocID = 1060410; // durability ~1_val~%
			sprintf( ObjProps[x++].m_Text, "%d", pItem->Armor_GetRepairPercent() );
			break;

		case IT_SPELLBOOK:
			ObjProps[x].m_LocID = 1042886; // ~1_NUMBERS_OF_SPELLS~ Spells

			y = 0;
			z = pItem->m_itSpellbook.m_spells1;
			do y += z & 1;
				while (z >>= 1);
			z = pItem->m_itSpellbook.m_spells2;
			do y += z & 1;
				while (z >>= 1);
			z = pItem->m_itSpellbook.m_spells3;
			do y += z & 1;
				while (z >>= 1);

			sprintf( ObjProps[x++].m_Text, "%d", y );
			break;

		case IT_SCROLL:
		case IT_WAND:
			sprintf( ObjProps[0].m_Text, "\t%s\t\x20", pItem->GetType() == IT_SCROLL ? "Scroll" : "Wand" );

			// There is a list of spellnames starting from LocID 1027981
			ObjProps[x++].m_LocID = 1027981 + pItem->m_itSpell.m_spell;

			if ( pItem->GetType() == IT_WAND )
			{
				ObjProps[x].m_LocID = 1060741; // charges: ~1_val~
				sprintf( ObjProps[x++].m_Text, "%d", pItem->m_itWeapon.m_spellcharges );
			}
			break;

		case IT_WEAPON_MACE_SMITH:
op_maceweapon:
			ObjProps[x++].m_LocID = 1061173; // skill required: mace fighting
op_weaponprops:
			ObjProps[x++].m_LocID = pItemDef->GetEquipLayer() == LAYER_HAND2 ? 1061171 : 1061824;

			ObjProps[x].m_LocID = 1061168; // weapon damage ~1_val~ - ~2_val~
			sprintf( ObjProps[x++].m_Text, "%d\t%d", pItemDef->m_attackBase, pItemDef->m_attackBase + pItemDef->m_attackRange );

			ObjProps[x].m_LocID = 1061167; // weapon speed ~1_val~
			sprintf( ObjProps[x++].m_Text, "%d", g_Cfg.Calc_CombatAttackSpeed( m_pChar, (CItem *) pItem ));

			ObjProps[x].m_LocID = 1061170; // strength requirement ~1_val~
			sprintf( ObjProps[x++].m_Text, "%d", pItemDef->m_ttEquippable.m_StrReq );
			goto op_durability;
			break;

		case IT_WEAPON_MACE_SHARP:
			goto op_maceweapon;

		case IT_WEAPON_SWORD:
			ObjProps[x++].m_LocID = 1061172; // skill required: swordsmanship
			goto op_weaponprops;

		case IT_WEAPON_FENCE:
			ObjProps[x++].m_LocID = 1061174; // skill required: fencing
			goto op_weaponprops;

		case IT_WEAPON_BOW:
			ObjProps[x++].m_LocID = 1061175; // skill required: archery
			goto op_weaponprops;
		}
	}
	else
	{
		ASSERT( pObj->IsChar() );

		const CChar * pChar = dynamic_cast <const CChar *> (pObj);

		// The name, the extra spaces fixes some weird unicode problem, dont ask me how or why
		ObjProps[x].m_LocID = 1041522; // ~1~~2~~3~
		sprintf( ObjProps[x++].m_Text, "\t\x20%s\x20\t\x20", pObj->GetName() );

		if ( pChar->IsClient() )
		{
			const CClient * pClient = dynamic_cast <const CClient *> (pChar->GetClient());

			if ( ( pClient->IsPriv( PRIV_GM ) ) && ( !pClient->IsPriv( PRIV_PRIV_NOSHOW ) ) )
			{
				ObjProps[x++].m_LocID = 1018085; // Game Master
				x++;
			}
		}
	}

	CCommand * pCur = &cmd;
	for ( y = 0; ObjProps[y].m_LocID; y++ )
	{
		// We're constructing the packet here

		pCur->AOSTooltip.m_list[0].m_LocID = ObjProps[y].m_LocID;
		pCur->AOSTooltip.m_list[0].m_textlen = 0;
		pCur->AOSTooltip.m_list[0].m_utexthead = 0x20;
		int i = CvtSystemToNUNICODE( pCur->AOSTooltip.m_list[0].m_utext, COUNTOF( ObjProps[y].m_Text ), ObjProps[y].m_Text, -1 );
		pCur->AOSTooltip.m_list[0].m_textlen = (i + 2) * sizeof( NCHAR );
		pCur->AOSTooltip.m_list[0].m_utexttail = 0;
		len += ( i * sizeof( NCHAR ) ) + sizeof( pCur->AOSTooltip.m_list[0] );
		pCur = (CCommand *)( ((BYTE*) pCur ) + ( i * sizeof( NCHAR ) ) + sizeof( pCur->AOSTooltip.m_list[0] ));
	}

	cmd.AOSTooltip.m_len = len;
	xSendPkt( &cmd, len );
}

void CClient::SendPacket( TCHAR * pszKey )
{
	BYTE	*pszTmp = (BYTE *)Str_GetTemp();
	int	iLen;
	DWORD	iVal;

	iLen	= -1;
	while ( *pszKey )
	{
		if ( iLen > SCRIPT_MAX_LINE_LEN - 4 )
		{	// we won't get here because this lenght is enforced in all scripts
			DEBUG_ERR(("SENDPACKET too big.\n"));
			return;
		}
		GETNONWHITESPACE( pszKey );
		if ( 0 ) ;
		else if ( toupper(*pszKey) == 'D' )
		{
			++pszKey;
			NDWORD		iVal;
			iVal		= Exp_GetVal(pszKey);
			pszTmp[++iLen]	= (BYTE) ((iVal >> 24) & 0xFF);
			pszTmp[++iLen]	= (BYTE) ((iVal >> 16) & 0xFF);
			pszTmp[++iLen]	= (BYTE) ((iVal >>  8) & 0xFF);
			pszTmp[++iLen]	= (BYTE) ((iVal      ) & 0xFF);
		}
		else if ( toupper(*pszKey) == 'W' )
		{
			++pszKey;
			NWORD		iVal;
			iVal		= Exp_GetVal(pszKey);
			pszTmp[++iLen]	= (BYTE) ((iVal >>  8) & 0xFF);
			pszTmp[++iLen]	= (BYTE) ((iVal      ) & 0xFF);
		}
		else if ( toupper(*pszKey) == 'S' )
		{
			// This is deprecated and broken anyway..
			DEBUG_ERR(( "S (string) in SENDPACKET is no longer valid, please use <ASC> !\n" ));
			return;

			++pszKey;

			TCHAR	chEnd	= ' ';
			if ( pszKey[0] == '"' )
			{
				chEnd	= '"';
				pszKey++;
			}

			while ( *pszKey && *pszKey != chEnd )
			{
				pszTmp[++iLen]	= *pszKey;
				pszKey++;
			}
		}
		else
		{
			if ( toupper(*pszKey) == 'B' )
				pszKey++;
			int		iVal	= Exp_GetVal(pszKey);
			pszTmp[++iLen]		= (BYTE) iVal;
		}
	}

	xSendReady( (void*) &pszTmp[0], iLen+1 );
}
