//
// CCharAct.cpp
//

#include "graysvr.h"	// predef header.
#include "CClient.h"

bool CChar::TeleportToObj( int iType, TCHAR * pszArgs )
{
	// "GONAME", "GOTYPE", "GOCHAR"
	// 0 = object name
	// 1 = char
	// 2 = item type

	DWORD dwUID = m_Act_Targ.GetObjUID() &~ UID_F_ITEM;
	DWORD dwTotal = g_World.GetUIDCount();
	DWORD dwCount = dwTotal-1;

	int iArg;
	if ( iType )
	{
		if ( pszArgs[0] && iType == 1 )
			dwUID = 0;
		iArg = RES_GET_INDEX( Exp_GetVal( pszArgs ));
	}
	else
	{
		// _strupr( pszArgs );
	}

	while ( dwCount-- )
	{
		if ( ++dwUID >= dwTotal )
		{
			dwUID = 1;
		}
		CObjBase * pObj = g_World.FindUID(dwUID);
		if ( pObj == NULL )
			continue;

		switch ( iType )
		{
		case 0:
			{
			MATCH_TYPE match = Str_Match( pszArgs, pObj->GetName());
			if ( match != MATCH_VALID )
				continue;
			}
			break;
		case 1:	// char
			{
				if ( ! pObj->IsChar())
					continue;
				CChar * pChar = dynamic_cast <CChar*>(pObj);
				DEBUG_CHECK( pChar );
				if ( iArg-- > 0 )
					continue;
			}
			break;
		case 2:	// item type
			{
				if ( ! pObj->IsItem())
					continue;
				CItem * pItem = dynamic_cast <CItem*>(pObj);
				ASSERT( pItem );
				if ( ! pItem->IsType( (IT_TYPE) iArg ))
					continue;
			}
			break;
		case 3: // char id
			{
				if ( ! pObj->IsChar())
					continue;
				CChar * pChar = dynamic_cast <CChar*>(pObj);
				DEBUG_CHECK( pChar );
				if ( pChar->GetID() != iArg )
					continue;
			}
			break;
		case 4:	// item id
			{
				if ( ! pObj->IsItem())
					continue;
				CItem * pItem = dynamic_cast <CItem*>(pObj);
				ASSERT( pItem );
				if ( pItem->GetID() != iArg )
					continue;
			}
			break;
		}

		CObjBaseTemplate * pObjTop = pObj->GetTopLevelObj();
		if ( pObjTop->IsChar())
		{
			if ( ! CanDisturb( dynamic_cast<CChar*>(pObjTop)))
				continue;
		}

		if ( pObjTop == this )
			continue;

		m_Act_Targ = pObj->GetUID();
		Spell_Teleport( pObjTop->GetTopPoint(), true, false );
		return( true );
	}
	return( false );
}

bool CChar::TeleportToCli( int iType, int iArgs )
{
	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( ! iType )
		{
			if ( pClient->m_Socket.GetSocket() != iArgs )
				continue;
		}
		CChar * pChar = pClient->GetChar();
		if ( pChar == NULL )
			continue;
		if ( ! CanDisturb( pChar ))
			continue;
		if ( iType )
		{
			if ( iArgs-- )
				continue;
		}
		m_Act_Targ = pChar->GetUID();
		Spell_Teleport( pChar->GetTopPoint(), true, false );
		return( true );
	}
	return( false );
}

void CChar::Jail( CTextConsole * pSrc, bool fSet, int iCell )
{
	if ( fSet )	// set the jailed flag.
	{
		if ( m_pPlayer )	// allow setting of this to offline chars.
		{
			m_pPlayer->GetAccount()->SetPrivFlags( PRIV_JAILED );
		}
		if ( IsClient())
		{
			m_pClient->SetPrivFlags( PRIV_JAILED );
		}
		TCHAR szJailName[ 128 ];
		if ( iCell )
		{
			sprintf( szJailName, "jail%d", iCell );
		}
		else
		{
			strcpy( szJailName, "jail" );
		}
		Spell_Teleport( g_Cfg.GetRegionPoint( szJailName ), true, false );
		SysMessageDefault( DEFMSG_JAILED );
	}
	else	// forgive.
	{
		if ( IsClient())
		{
			if ( ! m_pClient->IsPriv( PRIV_JAILED ))
				return;
			m_pClient->ClearPrivFlags( PRIV_JAILED );
		}
		if ( m_pPlayer )
		{
			m_pPlayer->GetAccount()->ClearPrivFlags( PRIV_JAILED );
		}
		SysMessageDefault( DEFMSG_FORGIVEN );
	}
}

void CChar::AddGoldToPack( int iAmount, CItemContainer * pPack )
{
	// A vendor is giving me gold. put it in my pack or other place.

	if ( pPack == NULL )
		pPack = GetPackSafe();

	while ( iAmount > 0 )
	{
		CItem * pGold = CItem::CreateScript( ITEMID_GOLD_C1, this );

		int iGoldStack = min( iAmount, USHRT_MAX );
		pGold->SetAmount( iGoldStack );

		Sound( pGold->GetDropSound( pPack ));
		pPack->ContentAdd( pGold );
		iAmount -= iGoldStack;
	}
}

void CChar::LayerAdd( CItem * pItem, LAYER_TYPE layer )
{
	// add equipped items.
	// check for item already in that layer ?
	// NOTE: This could be part of the Load as well so it may not truly be being "equipped" at this time.
	// OnTrigger for equip is done by ItemEquip()

	if ( pItem == NULL )
		return;
	if ( pItem->GetParent() == this &&
		pItem->GetEquipLayer() == layer )
	{
		return;
	}

	if ( layer == LAYER_DRAGGING )
	{
		pItem->RemoveSelf();	// remove from where i am before add so UNEQUIP effect takes.
		// NOTE: CanEquipLayer may bounce an item . If it stacks with this we are in trouble !
	}

	// This takes care of any conflicting items in the slot !
	layer = CanEquipLayer( pItem, layer, NULL, false );
	if ( layer == LAYER_NONE )
	{
		// we should not allow non-layered stuff to be put here ?
		// Put in pack instead ?
#ifdef _DEBUG
		if ( g_Log.IsLogged( LOGL_TRACE ))
		{
			DEBUG_MSG(( "ContentAdd id=%s '%s', LAYER_NONE is strange" DEBUG_CR, (LPCTSTR) pItem->GetResourceName(), (LPCTSTR) pItem->GetName()));
		}
#endif
		ItemBounce( pItem );
		return;
	}

	if ( layer == LAYER_SPECIAL )
	{
		if ( pItem->IsType( IT_EQ_TRADE_WINDOW ))
			layer = LAYER_NONE;
	}

	CContainer::ContentAddPrivate( pItem );
	pItem->SetEquipLayer( layer );

	// update flags etc for having equipped this.
	switch ( layer )
	{
		case LAYER_HAND1:
		case LAYER_HAND2:
			// If weapon
			if ( pItem->IsTypeWeapon())
			{
				m_uidWeapon = pItem->GetUID();
				Fight_ResetWeaponSwingTimer();
			}
			else if ( pItem->IsTypeArmor())
			{
				// Shield of some sort.
				m_defense = CalcArmorDefense();
				StatFlag_Set( STATF_HasShield );
				UpdateStatsFlag();
			}
			break;
		case LAYER_SHOES:
		case LAYER_PANTS:
		case LAYER_SHIRT:
		case LAYER_HELM:		// 6
		case LAYER_GLOVES:	// 7
		case LAYER_COLLAR:	// 10 = gorget or necklace.
		case LAYER_HALF_APRON:
		case LAYER_CHEST:	// 13 = armor chest
		case LAYER_TUNIC:	// 17 = jester suit
		case LAYER_ARMS:		// 19 = armor
		case LAYER_CAPE:		// 20 = cape
		case LAYER_ROBE:		// 22 = robe over all.
		case LAYER_SKIRT:
		case LAYER_LEGS:
			// If armor or clothing = change in defense rating.
			m_defense = CalcArmorDefense();
			UpdateStatsFlag();
			break;
	
			// These effects are not magical. (make them spells !)
	
		case LAYER_FLAG_Criminal:
			StatFlag_Set( STATF_Criminal );
			return;
		case LAYER_FLAG_SpiritSpeak:
			StatFlag_Set( STATF_SpiritSpeak );
			return;
		case LAYER_FLAG_Stuck:
			StatFlag_Set( STATF_Freeze );
			break;
	}

	if ( layer != LAYER_DRAGGING )
	{
		switch ( pItem->GetType())
		{
			case IT_EQ_SCRIPT:	// pure script.
				break;
			case IT_EQ_MEMORY_OBJ:
				Memory_UpdateFlags( dynamic_cast <CItemMemory *>(pItem) );
				break;
			case IT_EQ_NPC_SCRIPT:
				NPC_Script_OnTick( dynamic_cast <CItemMessage *>(pItem), false );
				break;
			case IT_EQ_HORSE:
				StatFlag_Set(STATF_OnHorse);
				break;
			case IT_COMM_CRYSTAL:
				StatFlag_Set(STATF_COMM_CRYSTAL);
				break;
		}
	}

	pItem->Update();
}

void CChar::OnRemoveOb( CGObListRec* pObRec )	// Override this = called when removed from list.
{
	// Unequip the item.
	// This may be a delete etc. It can not FAIL !
	CItem * pItem = STATIC_CAST <CItem*>(pObRec);
	ASSERT(pItem);
	DEBUG_CHECK( pItem->IsItemEquipped());

	LAYER_TYPE layer = pItem->GetEquipLayer();
	if ( layer != LAYER_DRAGGING && ! g_Serv.IsLoading())
	{
		pItem->OnTrigger( ITRIG_UNEQUIP, this );
	}

	CContainer::OnRemoveOb( pObRec );

	// remove equipped items effects
	switch ( layer )
	{
		case LAYER_HAND1:
		case LAYER_HAND2:	// other hand = shield
			if ( pItem->IsTypeWeapon())
			{
				m_uidWeapon.InitUID();
				Fight_ResetWeaponSwingTimer();
			}
			else if ( pItem->IsTypeArmor())
			{
				// Shield
				m_defense = CalcArmorDefense();
				StatFlag_Clear( STATF_HasShield );
				UpdateStatsFlag();
			}
			break;
		case LAYER_SHOES:
		case LAYER_PANTS:
		case LAYER_SHIRT:
		case LAYER_HELM:		// 6
		case LAYER_GLOVES:	// 7
		case LAYER_COLLAR:	// 10 = gorget or necklace.
		case LAYER_CHEST:	// 13 = armor chest
		case LAYER_TUNIC:	// 17 = jester suit
		case LAYER_ARMS:		// 19 = armor
		case LAYER_CAPE:		// 20 = cape
		case LAYER_ROBE:		// 22 = robe over all.
		case LAYER_SKIRT:
		case LAYER_LEGS:
			m_defense = CalcArmorDefense();
			UpdateStatsFlag();
			break;
	
		case LAYER_FLAG_Criminal:
			StatFlag_Clear( STATF_Criminal );
			break;
		case LAYER_FLAG_SpiritSpeak:
			StatFlag_Clear( STATF_SpiritSpeak );
			break;
		case LAYER_FLAG_Stuck:
			StatFlag_Clear( STATF_Freeze );
			break;
	}

	// Items with magic effects.
	if ( layer != LAYER_DRAGGING )
	{
		switch ( pItem->GetType())
		{
			case IT_COMM_CRYSTAL:
				if ( ContentFind( RESOURCE_ID( RES_TYPEDEF,IT_COMM_CRYSTAL ), 0, 0 ) == NULL )
				{
					StatFlag_Clear(STATF_COMM_CRYSTAL);
				}
				break;
			case IT_EQ_HORSE:
				StatFlag_Clear(STATF_OnHorse);
				break;
			case IT_EQ_MEMORY_OBJ:
				// Clear the associated flags.
				Memory_UpdateClearTypes( dynamic_cast<CItemMemory*>(pItem), 0xFFFF );
				break;
		}

		// If items are magical then remove effect here.
		Spell_Effect_Remove( pItem );
	}
}

void CChar::DropAll( CItemContainer * pCorpse, WORD wAttr )
{
	// shrunk or died. (or sleeping)
	if ( IsStatFlag( STATF_Conjured ))
		return;	// drop nothing.

	CItemContainer * pPack = GetPack();
	if ( pPack != NULL )
	{
		if ( pCorpse == NULL )
		{
			pPack->ContentsDump( GetTopPoint(), wAttr );
		}
		else
		{
			pPack->ContentsTransfer( pCorpse, true );
		}
	}

	// transfer equipped items to corpse or your pack (if newbie).
	UnEquipAllItems( pCorpse );
}

void CChar::UnEquipAllItems( CItemContainer * pDest )
{
	// We morphed, sleeping, died or became a GM.
	// Pets can be told to "Drop All"
	// drop item that is up in the air as well.

	if ( ! GetCount())
		return;
	CItemContainer * pPack = NULL;

	CItem* pItemNext;
	CItem* pItem=GetContentHead();
	for ( ; pItem!=NULL; pItem=pItemNext )
	{
		pItemNext = pItem->GetNext();
		LAYER_TYPE layer = pItem->GetEquipLayer();
		switch ( layer )
		{
		case LAYER_NONE:
			DEBUG_CHECK( pItem->IsType( IT_EQ_TRADE_WINDOW ));
			pItem->Delete();	// Get rid of any trades.
			continue;
		case LAYER_FLAG_Poison:
		case LAYER_FLAG_Criminal:
		case LAYER_FLAG_Hallucination:
		case LAYER_FLAG_Potion:
		case LAYER_FLAG_Drunk:
		case LAYER_FLAG_Stuck:
		case LAYER_FLAG_PotionUsed:
			if ( IsStatFlag( STATF_DEAD ))
				pItem->Delete();
			continue;
		case LAYER_PACK:
		case LAYER_HORSE:
			continue;
		case LAYER_HAIR:	// leave this.
		case LAYER_BEARD:
			// Copy hair and beard to corpse.
			if ( pDest == NULL )
				continue;
			if ( pDest->IsType(IT_CORPSE))
			{
				CItem * pDupe = CItem::CreateDupeItem( pItem );
				pDest->ContentAdd( pDupe );	// add content
				// Equip layer only matters on a corpse.
				pDupe->SetContainedLayer( layer );
			}
			continue;
		case LAYER_DRAGGING:
			layer = LAYER_NONE;
			break;
		default:
			// can't transfer this to corpse.
			if ( ! CItemBase::IsVisibleLayer( layer ))
				continue;
			break;
		}
		if ( pDest != NULL &&
			! pItem->IsAttr( ATTR_NEWBIE|ATTR_MOVE_NEVER|ATTR_CURSED2|ATTR_BLESSED2 ))
		{	// Move item to dest. (corpse ussually)
			pDest->ContentAdd( pItem );
			if ( pDest->IsType(IT_CORPSE))
			{
				// Equip layer only matters on a corpse.
				pItem->SetContainedLayer( layer );
			}
		}
		else
		{	// Move item to chars' pack.
			if ( pPack == NULL )
				pPack = GetPackSafe();
			pPack->ContentAdd( pItem );
		}
	}
}

void CChar::CancelAllTrades()
{
	// remove all trade windows. client logged out.
	for ( CItem* pItem=GetContentHead(); pItem!=NULL; )
	{
		CItem* pItemNext = pItem->GetNext();
		if ( pItem->IsType( IT_EQ_TRADE_WINDOW ))
		{
			pItem->Delete();
		}
		pItem=pItemNext;
	}
}

void CChar::UpdateDrag( CItem * pItem, CObjBase * pCont, CPointMap * ppt )
{
	// Show the world that I am picking up or putting down this object.
	// NOTE: This makes people disapear.
	CCommand cmd;
	cmd.DragAnim.m_Cmd = XCMD_DragAnim;
	cmd.DragAnim.m_id = pItem->GetDispID();
	cmd.DragAnim.m_unk3 = 0;
	cmd.DragAnim.m_unk5 = 0;
	cmd.DragAnim.m_unk7 = 0;

	CPointMap ptThis = GetTopPoint();

	if ( pCont != NULL )
	{
		// I'm putting an object in a cont..
		CObjBaseTemplate * pObjTop = pCont->GetTopLevelObj();
		if ( pObjTop == this )
			return;	// move stuff in my own pack.

		CPointMap ptTop = pObjTop->GetTopPoint();

		cmd.DragAnim.m_srcUID = GetUID();
		cmd.DragAnim.m_src_x = ptThis.m_x;
		cmd.DragAnim.m_src_y = ptThis.m_y;
		cmd.DragAnim.m_src_x = ptThis.m_z;
		cmd.DragAnim.m_dstUID = pObjTop->GetUID();
		cmd.DragAnim.m_dst_x = ptTop.m_x;
		cmd.DragAnim.m_dst_y = ptTop.m_y;
		cmd.DragAnim.m_dst_z = ptTop.m_z;
	}
	else if ( ppt != NULL )
	{
		// putting on ground.
		cmd.DragAnim.m_srcUID = GetUID();
		cmd.DragAnim.m_src_x = ptThis.m_x;
		cmd.DragAnim.m_src_y = ptThis.m_y;
		cmd.DragAnim.m_src_x = ptThis.m_z;
		cmd.DragAnim.m_dstUID = 0;
		cmd.DragAnim.m_dst_x = ppt->m_x;
		cmd.DragAnim.m_dst_y = ppt->m_y;
		cmd.DragAnim.m_dst_z = ppt->m_z;
	}
	else
	{
		// I'm getting an object from where ever it is.

		// ??? Note: this doesn't work for ground objects !
		CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
		if ( pObjTop == this )
			return;	// move stuff in my own pack.

		CPointMap ptTop = pObjTop->GetTopPoint();

		cmd.DragAnim.m_srcUID = (pObjTop==pItem) ? 0 : (DWORD) pObjTop->GetUID();
		cmd.DragAnim.m_src_x = ptTop.m_x;
		cmd.DragAnim.m_src_y = ptTop.m_y;
		cmd.DragAnim.m_src_z = ptTop.m_z;
		cmd.DragAnim.m_dstUID = 0; // GetUID();
		cmd.DragAnim.m_dst_x = ptThis.m_x;
		cmd.DragAnim.m_dst_y = ptThis.m_y;
		cmd.DragAnim.m_dst_x = ptThis.m_z;
	}

	UpdateCanSee( &cmd, sizeof(cmd.DragAnim), m_pClient );
}


void	CChar::UpdateStatsFlag() const
{
	// Push status change to all who can see us.
	// For Weight, AC, Gold must update all
	// Just flag the stats to be updated later if possible.
	if ( g_Serv.IsLoading() )
		return;

	if ( ! IsClient())
		return;
	GetClient()->addUpdateStatsFlag();
}

// queue updates

void CChar::UpdateHitsFlag()
{
	if ( g_Serv.IsLoading() )
		return;

	m_fHitsUpdate = true;

	if ( ! IsClient())
		return;
	GetClient()->addUpdateHitsFlag();
}

void CChar::UpdateManaFlag() const
{
	if ( g_Serv.IsLoading() )
		return;

	if ( ! IsClient())
		return;
	GetClient()->addUpdateManaFlag();
}

void CChar::UpdateStamFlag() const
{
	if ( g_Serv.IsLoading() )
		return;

	if ( ! IsClient())
		return;
	GetClient()->addStamUpdate( GetUID() );
}

void CChar::UpdateHitsForOthers() const
{
	CCommand cmd;
	cmd.StatChng.m_Cmd = XCMD_StatChngStr;
	cmd.StatChng.m_UID = GetUID();
	cmd.StatChng.m_max = 50;
	cmd.StatChng.m_val = ( (Stat_GetVal( STAT_STR ) * 50) / Stat_GetMax( STAT_STR ));
	UpdateCanSee( &cmd, sizeof(cmd.StatChng), m_pClient );
}

void CChar::UpdateStatVal( STAT_TYPE type, int iChange, int iLimit )
{
	ASSERT( type < STAT_BASE_QTY+1 );	// allow food
	int iVal = Stat_GetVal( type );

	if ( iChange )
	{
		if ( ! iLimit )
		{
			iLimit = Stat_GetMax( type );
		}
		if ( iChange < 0 )
		{
			iVal += iChange;
		}
		else if ( iVal > iLimit )
		{
			iVal -= iChange;
			if ( iVal < iLimit ) iVal = iLimit;
		}
		else
		{
			iVal += iChange;
			if ( iVal > iLimit ) iVal = iLimit;
		}
		if ( iVal < 0 ) iVal = 0;
		Stat_SetVal( type, iVal );
	}

	iLimit = Stat_GetMax(type);
	if ( iLimit < 0 )
		iLimit = 0;

	switch ( type )
	{
	case STAT_STR:
		UpdateHitsFlag();
		break;
	case STAT_INT:
		UpdateManaFlag();
		break;
	case STAT_DEX:
		UpdateStamFlag();
	}

/*
	CCommand cmd;
	cmd.StatChng.m_Cmd = XCMD_StatChngStr + type - STAT_STR;
	cmd.StatChng.m_UID = GetUID();
	cmd.StatChng.m_max = iLimit;
	cmd.StatChng.m_val = iVal;

	if ( type == STAT_STR )	// everyone sees my health
	{
		cmd.StatChng.m_max = 25;
		cmd.StatChng.m_val = (iLimit == 0 ? 0 : (iVal * 25) / iLimit);
		UpdateCanSee( &cmd, sizeof(cmd.StatChng), m_pClient );
		cmd.StatChng.m_max = iLimit;
		cmd.StatChng.m_val = iVal;
	}

	if ( IsClient())	// send this just to me
	{
		m_pClient->xSendPkt( &cmd, sizeof(cmd.StatChng));
	}
*/
}



bool CChar::UpdateAnimate( ANIM_TYPE action, bool fTranslate, bool fBackward, BYTE iFrameDelay )
{
	// NPC or character does a certain Animate
	// Translate the animation based on creature type.
	// ARGS:
	//   fBackward = make the anim go in reverse.
	//   iFrameDelay = in seconds (approx), 0=fastest, 1=slower

	if ( action < 0 || action >= ANIM_QTY )
		return false;
	if ( fBackward && iFrameDelay )	// backwards and delayed just dont work ! = invis
		iFrameDelay = 0;

	if ( fTranslate || IsStatFlag( STATF_OnHorse ))
	{
		CCharBase* pCharDef = Char_GetDef();
		ASSERT(pCharDef);

		CItem * pWeapon = m_uidWeapon.ItemFind();
		if ( pWeapon != NULL && action == ANIM_ATTACK_WEAPON )
		{
			// action depends on weapon type (skill) and 2 Hand type.
			DEBUG_CHECK( pWeapon->IsItemEquipped());
			LAYER_TYPE layer = pWeapon->Item_GetDef()->GetEquipLayer();
			switch ( pWeapon->GetType() )
			{
			case IT_WEAPON_MACE_CROOK:
			case IT_WEAPON_MACE_PICK:
			case IT_WEAPON_MACE_SMITH:	// Can be used for smithing ?
			case IT_WEAPON_MACE_STAFF:
			case IT_WEAPON_MACE_SHARP:	// war axe can be used to cut/chop trees.
				action = ( layer == LAYER_HAND2 ) ?
					ANIM_ATTACK_2H_DOWN :
					ANIM_ATTACK_1H_DOWN;

do_add_style:
				if ( Calc_GetRandVal( 2 ))
				{
					// add some style to the attacks.
					if ( layer == LAYER_HAND2 )
					{
						action = (ANIM_TYPE)( ANIM_ATTACK_2H_DOWN + Calc_GetRandVal(3));
					}
					else
					{
						action = (ANIM_TYPE)( ANIM_ATTACK_1H_WIDE + Calc_GetRandVal(3));
					}
				}
				break;
			case IT_WEAPON_SWORD:
			case IT_WEAPON_AXE:
				action = ( layer == LAYER_HAND2 ) ?
					ANIM_ATTACK_2H_WIDE :
					ANIM_ATTACK_1H_WIDE;
				goto do_add_style;
			case IT_WEAPON_FENCE:
				action = ( layer == LAYER_HAND2 ) ?
					ANIM_ATTACK_2H_JAB :
					ANIM_ATTACK_1H_JAB;
				goto do_add_style;
			case IT_WEAPON_BOW:
				action = ANIM_ATTACK_BOW;
				break;
			case IT_WEAPON_XBOW:
				action = ANIM_ATTACK_XBOW;
				break;
			}
		}

		if ( IsStatFlag( STATF_OnHorse ))	// on horse back.
		{
			// Horse back anims are dif.
			switch ( action )
			{
			case ANIM_WALK_UNARM:
			case ANIM_WALK_ARM:
				action = ANIM_HORSE_RIDE_SLOW;
				break;
			case ANIM_RUN_UNARM:
			case ANIM_RUN_ARMED:
				action = ANIM_HORSE_RIDE_FAST;
				break;
			case ANIM_STAND:
				action = ANIM_HORSE_STAND;
				break;
			case ANIM_FIDGET1:
			case ANIM_FIDGET_YAWN:
				action = ANIM_HORSE_SLAP;
				break;
			case ANIM_STAND_WAR_1H:
			case ANIM_STAND_WAR_2H:
				action = ANIM_HORSE_STAND;
				break;
			case ANIM_ATTACK_1H_WIDE:
			case ANIM_ATTACK_1H_JAB:
			case ANIM_ATTACK_1H_DOWN:
				action = ANIM_HORSE_ATTACK;
				break;
			case ANIM_ATTACK_2H_JAB:
			case ANIM_ATTACK_2H_WIDE:
			case ANIM_ATTACK_2H_DOWN:
				action = ANIM_HORSE_SLAP;
				break;
			case ANIM_WALK_WAR:
				action = ANIM_HORSE_RIDE_SLOW;
				break;
			case ANIM_CAST_DIR:
				action = ANIM_HORSE_ATTACK;
				break;
			case ANIM_CAST_AREA:
				action = ANIM_HORSE_ATTACK_BOW;
				break;
			case ANIM_ATTACK_BOW:
				action = ANIM_HORSE_ATTACK_BOW;
				break;
			case ANIM_ATTACK_XBOW:
				action = ANIM_HORSE_ATTACK_XBOW;
				break;
			case ANIM_GET_HIT:
				action = ANIM_HORSE_SLAP;
				break;
			case ANIM_BLOCK:
				action = ANIM_HORSE_SLAP;
				break;
			case ANIM_ATTACK_UNARM:
				action = ANIM_HORSE_ATTACK;
				break;
			case ANIM_BOW:
			case ANIM_SALUTE:
			case ANIM_EAT:
				action = ANIM_HORSE_ATTACK_XBOW;
				break;
			default:
				action = ANIM_HORSE_STAND;
				break;
			}
		}
		else if ( GetDispID() < CREID_MAN )
		{
			// Animals have certain anims. Monsters have others.

			if ( GetDispID() >= CREID_HORSE1 )
			{
				// All animals have all these anims thankfully
				switch ( action )
				{
				case ANIM_WALK_UNARM:
				case ANIM_WALK_ARM:
				case ANIM_WALK_WAR:
					action = ANIM_ANI_WALK;
					break;
				case ANIM_RUN_UNARM:
				case ANIM_RUN_ARMED:
					action = ANIM_ANI_RUN;
					break;
				case ANIM_STAND:
				case ANIM_STAND_WAR_1H:
				case ANIM_STAND_WAR_2H:
				default:
					action = ANIM_ANI_STAND;
					break;

				case ANIM_FIDGET1:
					action = ANIM_ANI_FIDGET1;
					break;
				case ANIM_FIDGET_YAWN:
					action = ANIM_ANI_FIDGET2;
					break;
				case ANIM_CAST_DIR:
					action = ANIM_ANI_ATTACK1;
					break;
				case ANIM_CAST_AREA:
					action = ANIM_ANI_EAT;
					break;
				case ANIM_GET_HIT:
					action = ANIM_ANI_GETHIT;
					break;

				case ANIM_ATTACK_1H_WIDE:
				case ANIM_ATTACK_1H_JAB:
				case ANIM_ATTACK_1H_DOWN:
				case ANIM_ATTACK_2H_DOWN:
				case ANIM_ATTACK_2H_JAB:
				case ANIM_ATTACK_2H_WIDE:
				case ANIM_ATTACK_BOW:
				case ANIM_ATTACK_XBOW:
				case ANIM_ATTACK_UNARM:
					switch ( Calc_GetRandVal(2))
					{
					case 0: action = ANIM_ANI_ATTACK1; break;
					case 1: action = ANIM_ANI_ATTACK2; break;
					}
					break;

				case ANIM_DIE_BACK:
					action = ANIM_ANI_DIE1;
					break;
				case ANIM_DIE_FORWARD:
					action = ANIM_ANI_DIE2;
					break;
				case ANIM_BLOCK:
				case ANIM_BOW:
				case ANIM_SALUTE:
					action = ANIM_ANI_SLEEP;
					break;
				case ANIM_EAT:
					action = ANIM_ANI_EAT;
					break;
				}

				while ( action != ANIM_WALK_UNARM && ! ( pCharDef->m_Anims & (1<<action)))
				{
					// This anim is not supported. Try to use one that is.
					switch ( action )
					{
					case ANIM_ANI_SLEEP:	// All have this.
						action = ANIM_ANI_EAT;
						break;
					default:
						action = ANIM_WALK_UNARM;
						break;
					}
				}
			}
			else
			{
				// Monsters don't have all the anims.

				switch ( action )
				{
				case ANIM_CAST_DIR:
					action = ANIM_MON_Stomp;
					break;
				case ANIM_CAST_AREA:
					action = ANIM_MON_PILLAGE;
					break;
				case ANIM_DIE_BACK:
					action = ANIM_MON_DIE1;
					break;
				case ANIM_DIE_FORWARD:
					action = ANIM_MON_DIE2;
					break;
				case ANIM_GET_HIT:
					switch ( Calc_GetRandVal(3))
					{
					case 0: action = ANIM_MON_GETHIT; break;
					case 1: action = ANIM_MON_BlockRight; break;
					case 2: action = ANIM_MON_BlockLeft; break;
					}
					break;
				case ANIM_ATTACK_1H_WIDE:
				case ANIM_ATTACK_1H_JAB:
				case ANIM_ATTACK_1H_DOWN:
				case ANIM_ATTACK_2H_DOWN:
				case ANIM_ATTACK_2H_JAB:
				case ANIM_ATTACK_2H_WIDE:
				case ANIM_ATTACK_BOW:
				case ANIM_ATTACK_XBOW:
				case ANIM_ATTACK_UNARM:
					switch ( Calc_GetRandVal(3))
					{
					case 0: action = ANIM_MON_ATTACK1; break;
					case 1: action = ANIM_MON_ATTACK2; break;
					case 2: action = ANIM_MON_ATTACK3; break;
					}
					break;
				default:
					action = ANIM_WALK_UNARM;
					break;
				}
				// NOTE: Available actions depend HEAVILY on creature type !
				// ??? Monsters don't have all anims in common !
				// translate these !
				while ( action != ANIM_WALK_UNARM && ! ( pCharDef->m_Anims & (1<<action)))
				{
					// This anim is not supported. Try to use one that is.
					switch ( action )
					{
					case ANIM_MON_ATTACK1:	// All have this.
						DEBUG_ERR(( "Anim 0%x This is wrong! Invalid SCP file data." DEBUG_CR, GetDispID()));
						action = ANIM_WALK_UNARM;
						break;

					case ANIM_MON_ATTACK2:	// Dolphins, Eagles don't have this.
					case ANIM_MON_ATTACK3:
						action = ANIM_MON_ATTACK1;	// ALL creatures have at least this attack.
						break;
					case ANIM_MON_Cast2:	// Trolls, Spiders, many others don't have this.
						action = ANIM_MON_BlockRight;	// Birds don't have this !
						break;
					case ANIM_MON_BlockRight:
						action = ANIM_MON_BlockLeft;
						break;
					case ANIM_MON_BlockLeft:
						action = ANIM_MON_GETHIT;
						break;
					case ANIM_MON_GETHIT:
						if ( pCharDef->m_Anims & (1<<ANIM_MON_Cast2))
							action = ANIM_MON_Cast2;
						else
							action = ANIM_WALK_UNARM;
						break;

					case ANIM_MON_Stomp:
						action = ANIM_MON_PILLAGE;
						break;
					case ANIM_MON_PILLAGE:
						action = ANIM_MON_ATTACK3;
						break;
					case ANIM_MON_AttackBow:
					case ANIM_MON_AttackXBow:
						action = ANIM_MON_ATTACK3;
						break;
					case ANIM_MON_AttackThrow:
						action = ANIM_MON_AttackXBow;
						break;

					default:
						DEBUG_ERR(( "Anim Unsupported 0%x for 0%x" DEBUG_CR, action, GetDispID()));
						action = ANIM_WALK_UNARM;
						break;
					}
				}
			}
		}
	}

	WORD wRepeat = 1;

	CCommand cmd;
	cmd.CharAction.m_Cmd = XCMD_CharAction;
	cmd.CharAction.m_UID = GetUID();
	cmd.CharAction.m_action = action;
	cmd.CharAction.m_zero7 = 0;
	cmd.CharAction.m_dir = m_dirFace;
	cmd.CharAction.m_repeat = wRepeat;		// 1, repeat count. 0=forever.
	cmd.CharAction.m_backward = fBackward ? 1 : 0;	// 0, backwards (0/1)
	cmd.CharAction.m_repflag = ( wRepeat == 1 ) ? 0 : 1;	// 0=dont repeat. 1=repeat
	cmd.CharAction.m_framedelay = iFrameDelay;	// 1, 0=fastest.

	UpdateCanSee( &cmd, sizeof(cmd.CharAction));
	return( true );
}

void CChar::UpdateMode( CClient * pExcludeClient, bool fFull )
{
	// If character status has been changed
	// (Polymorph, war mode or hide), resend him
	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( pExcludeClient == pClient )
			continue;
		if ( ! pClient->CanSee( this ))
		{
			// In the case of "INVIS" used by GM's we must use this.
			if ( GetDist( pClient->GetChar()) <= UO_MAP_VIEW_SIZE )
			{
				pClient->addObjectRemove( this );
			}
			continue;
		}
		if ( pClient->IsPriv( PRIV_DEBUG ))
			continue;
		if ( fFull )
		{
			pClient->addChar( this );
		}
		else
		{
			pClient->addCharMove( this );
		}
	}
}


void CChar::UpdateMove( CPointMap pold, CClient * pExcludeClient, bool fFull )
{
	// Who now sees this char ?
	// Did they just see him move ?
	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( pClient == pExcludeClient )
			continue;	// no need to see self move.
		if ( pClient == m_pClient && fFull )
		{
			// What do i now see ?
			pClient->addMap((CPointMap*)(pold.IsValidPoint() ? &pold : NULL));
			pClient->addPlayerView( pold );
			continue;
		}
		CChar * pChar = pClient->GetChar();
		if ( pChar == NULL )
			continue;

		bool fCouldSee = ( pold.GetDist( pChar->GetTopPoint()) <= UO_MAP_VIEW_SIZE );

		if ( ! pClient->CanSee( this ))
		{	// can't see me now.
			if ( fCouldSee ) pClient->addObjectRemove( this );
		}
		else if ( fCouldSee )
		{	// They see me move.
			pClient->addCharMove( this );
		}
		else
		{	// first time this client has seen me.
			pClient->addChar( this );
		}
	}
}

void CChar::UpdateDir( DIR_TYPE dir )
{
	if ( dir != m_dirFace && dir < DIR_QTY )
	{
		m_dirFace = dir;	// face victim.
		UpdateMove( GetTopPoint(), NULL, true );
	}
}

void CChar::UpdateDir( const CPointMap & pt )
{
	// Change in direction.
	UpdateDir( GetTopPoint().GetDir( pt ));
}

void CChar::UpdateDir( const CObjBaseTemplate * pObj )
{
	if ( pObj == NULL )
		return;
	pObj = pObj->GetTopLevelObj();
	if ( pObj == this )		// In our own pack.
		return;
	UpdateDir( pObj->GetTopPoint());
}

void CChar::Update( const CClient * pClientExclude ) // If character status has been changed (Polymorph), resend him
{
	// Or I changed looks.
	// I moved or somebody moved me  ?
	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( pClient == pClientExclude )
			continue;
		if ( pClient == m_pClient )
		{
			pClient->addReSync();
		}
		else if ( pClient->CanSee( this ))
		{
			pClient->addChar( this );
		}
	}
}

SOUND_TYPE CChar::SoundChar( CRESND_TYPE type )
{
	SOUND_TYPE id;

	CCharBase* pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	if ( GetDispID() == CREID_BLADES )
	{
		id = pCharDef->m_soundbase; // just one sound
	}
	else if ( GetDispID() >= CREID_MAN )
	{
		id = 0;

		static const SOUND_TYPE sm_Snd_Man_Die[] = { 0x15a, 0x15b, 0x15c, 0x15d };
		static const SOUND_TYPE sm_Snd_Man_Omf[] = { 0x154, 0x155, 0x156, 0x157, 0x158, 0x159 };
		static const SOUND_TYPE sm_Snd_Wom_Die[] = { 0x150, 0x151, 0x152, 0x153 };
		static const SOUND_TYPE sm_Snd_Wom_Omf[] = { 0x14b, 0x14c, 0x14d, 0x14e, 0x14f };

		if ( pCharDef->IsFemale())
		{
			switch ( type )
			{
			case CRESND_GETHIT:
				id = sm_Snd_Wom_Omf[ Calc_GetRandVal( COUNTOF(sm_Snd_Wom_Omf)) ];
				break;
			case CRESND_DIE:
				id = sm_Snd_Wom_Die[ Calc_GetRandVal( COUNTOF(sm_Snd_Wom_Die)) ];
				break;
			}
		}
		else
		{
			switch ( type )
			{
			case CRESND_GETHIT:
				id = sm_Snd_Man_Omf[ Calc_GetRandVal( COUNTOF(sm_Snd_Man_Omf)) ];
				break;
			case CRESND_DIE:
				id = sm_Snd_Man_Die[ Calc_GetRandVal( COUNTOF(sm_Snd_Man_Die)) ];
				break;
			}
		}
	}
	else
	{
		id = pCharDef->m_soundbase + type;
		switch ( pCharDef->m_soundbase )	// some creatures have no base sounds.
		{
		case 128: // old versions
		case 181:
		case 199:
			if ( type <= CRESND_RAND2 )
				id = 0;
			break;
		case 130: // ANIMALS_DEER3
		case 183: // ANIMALS_LLAMA3
		case 201: // ANIMALS_RABBIT3
			if ( type <= CRESND_RAND2 )
				id = 0;
			else
				id -= 2;
			break;
		}
	}

	if ( type == CRESND_HIT )
	{
		CItem * pWeapon = m_uidWeapon.ItemFind();
		if ( pWeapon != NULL )
		{
			DEBUG_CHECK( pWeapon->IsItemEquipped());
			// weapon type strike noise based on type of weapon and how hard hit.

			switch ( pWeapon->GetType() )
			{
			case IT_WEAPON_MACE_CROOK:
			case IT_WEAPON_MACE_PICK:
			case IT_WEAPON_MACE_SMITH:	// Can be used for smithing ?
			case IT_WEAPON_MACE_STAFF:
				// 0x233 = blunt01 (miss?)
				id = 0x233;
				break;
			case IT_WEAPON_MACE_SHARP:	// war axe can be used to cut/chop trees.
				// 0x232 = axe01 swing. (miss?)
				id = 0x232;
				break;
			case IT_WEAPON_SWORD:
			case IT_WEAPON_AXE:
				if ( pWeapon->Item_GetDef()->GetEquipLayer() == LAYER_HAND2 )
				{
					// 0x236 = hvyswrd1 = (heavy strike)
					// 0x237 = hvyswrd4 = (heavy strike)
					id = Calc_GetRandVal( 2 ) ? 0x236 : 0x237;
					break;
				}
			case IT_WEAPON_FENCE:
				// 0x23b = sword1
				// 0x23c = sword7
				id = Calc_GetRandVal( 2 ) ? 0x23b : 0x23c;
				break;
			case IT_WEAPON_BOW:
			case IT_WEAPON_XBOW:
				// 0x234 = xbow ( hit)
				id = 0x234;
				break;
			}
		}
		else if ( id == 0 )
		{
			static const SOUND_TYPE sm_Snd_Hit[] =
			{
				0x135, //= hit01 = (slap)
				0x137, //= hit03 = (hit sand)
				0x13b, //= hit07 = (hit slap)
			};
			id = sm_Snd_Hit[ Calc_GetRandVal( COUNTOF( sm_Snd_Hit )) ];
		}
	}

	if ( id <= 0 )
		return( 0 );
	Sound( id );
	return( id );
}

int CChar::ItemPickup( CItem * pItem, int amount )
{
	// Pickup off the ground or remove my own equipment. etc..
	// This item is now "up in the air"
	// RETURN:
	//  amount we can pick up.
	//	-1 = we cannot pick this up.

	if (( amount < 0 ) || !pItem )
		return -1;
	if ( pItem->GetParent() == this && pItem->GetEquipLayer() == LAYER_HORSE )
		return -1;
	if (( pItem->GetParent() == this ) && ( pItem->GetEquipLayer() == LAYER_DRAGGING ))
		return pItem->GetAmount();
	if ( !CanTouch(pItem) || !CanMove(pItem, true) )
		return -1;

	if ( IsClient() )
	{
		const CItem *		pItemCont	= dynamic_cast <const CItem*> (pItem->GetParent());
		if (	pItemCont
			&&	pItemCont->IsType( IT_EQ_BANK_BOX )
			&&	pItemCont->m_itEqBankBox.m_pntOpen != GetTopPoint() )
		{
			g_Log.Event( LOGL_WARN|LOGM_CHEAT,
			"%x:Cheater '%s' is picking up item from bank box using 3rd party tools to access it\n",
				m_pClient->m_Socket.GetSocket(), (LPCTSTR) m_pClient->GetAccount()->GetName() );
			return -1;
		}
	}

	const CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
	const CChar * pChar = dynamic_cast <const CChar*> (pObjTop);

	if ( pChar != this &&
		pItem->IsAttr(ATTR_OWNED) &&
		pItem->m_uidLink != GetUID() &&
		!IsPriv(PRIV_ALLMOVE|PRIV_GM))
	{
muststeal:
		SysMessageDefault(DEFMSG_STEAL);
		return -1;
	}

	const CItemCorpse * pCorpseItem = dynamic_cast <const CItemCorpse *>(pObjTop);
	if ( pCorpseItem )
	{
		// Taking stuff off someones corpse can be a crime !
		if ( CheckCorpseCrime(pCorpseItem, true, false) )
			SysMessageDefault(DEFMSG_GUARDS);
	}

	int iAmountMax = pItem->GetAmount();
	if ( iAmountMax <= 0 )
		return -1;
	amount = max(1, min(amount, iAmountMax));
	int iItemWeight = ( amount == 1 ) ? pItem->GetWeight() : pItem->Item_GetDef()->GetWeight() * amount;

	// Is it too heavy to even drag ?
	bool fDrop = false;
	if ( GetWeightLoadPercent(GetTotalWeight() + iItemWeight) > 300 )
	{
		SysMessageDefault(DEFMSG_HEAVY);
		if ( pChar != this )
			return -1;
		fDrop = true;	// we can always drop it out of own pack !
	}

	ITRIG_TYPE trigger;
	if ( pChar != NULL )
	{
		if ( ! pChar->NPC_IsOwnedBy( this ))
			goto muststeal;
		trigger = pItem->IsItemEquipped() ? ITRIG_UNEQUIP : ITRIG_PICKUP_PACK;
	}
	else
	{
		trigger = pItem->IsTopLevel() ? ITRIG_PICKUP_GROUND : ITRIG_PICKUP_PACK;
	}

	if ( trigger == ITRIG_PICKUP_GROUND )
	{
		//	bug with taking static/movenever items -or- catching the spell effects
		if ( IsPriv(PRIV_ALLMOVE|PRIV_GM) ) ;
		else if ( pItem->IsAttr(ATTR_STATIC|ATTR_MOVE_NEVER) || pItem->IsType(IT_SPELL) )
			return -1;
	}

	if ( trigger != ITRIG_UNEQUIP )	// unequip is done later.
	{
		CScriptTriggerArgs Args( amount );
		if ( pItem->OnTrigger( trigger, this, &Args ) == TRIGRET_RET_TRUE )
			return( -1 );
		if ( trigger == ITRIG_PICKUP_PACK )
		{
			if ( IsClient() )			// limit fast loot/pick
			{
				long time = g_World.GetCurrentTime().GetTimeRaw();
				if ( time - atoi(m_TagDefs.GetKeyStr("NEXTPACKPICK", true)) < 3 )
					return -1;
				m_TagDefs.SetNum("NEXTPACKPICK", time);
			}

			CItem * pContItem = dynamic_cast <CItem*> ( pItem->GetContainer() );
			if ( pContItem )
			{
				CScriptTriggerArgs Args1(pItem);
				if ( pContItem->OnTrigger(ITRIG_PICKUP_SELF, this, &Args1) == TRIGRET_RET_TRUE )
					return -1;
			}
		}
	}

	if ( pItem->Item_GetDef()->IsStackableType() && amount )
	{
		// Did we only pick up part of it ?
		// part or all of a pile. Only if pilable !
		if ( amount < iAmountMax )
		{
			// create left over item.
			CItem * pItemNew = pItem->UnStackSplit( amount, this );
			pItemNew->SetTimeout( pItem->GetTimerDAdjusted() ); //since this was commented in DupeCopy
		}
	}
	else
	{
		amount = iAmountMax;
	}

	if ( fDrop )
	{
		ItemDrop( pItem, GetTopPoint());
		return( -1 );
	}

	// do the dragging anim for everyone else to see.
	UpdateDrag( pItem );

	// Pick it up.
	pItem->SetDecayTime(-1);	// Kill any decay timer.
	LayerAdd( pItem, LAYER_DRAGGING );

	return( amount );
}

bool CChar::ItemBounce( CItem * pItem )
{
	// We can't put this where we want to
	// So put in my pack if i can. else drop.
	// don't check where this came from !


	if ( pItem == NULL )
		return false;

	CItemContainer * pPack = GetPackSafe();
	if ( pItem->GetParent() == pPack )
		return( true );

	LPCTSTR pszWhere = NULL;
	if ( CanCarry( pItem ))
	{
		// if we can carry it
		pszWhere = g_Cfg.GetDefaultMsg( DEFMSG_BOUNCE_PACK );
		if ( pPack == NULL )
			goto dropit;	// this can happen at load time.

		pPack->ContentAdd( pItem ); // Add it to pack
		Sound( pItem->GetDropSound( pPack ));
	}
	else
	{
dropit:
		if ( ! GetTopPoint().IsValidPoint())
		{
			// NPC is being created and has no valid point yet.
			if (pszWhere)
			{
				DEBUG_ERR(( "No pack to place loot item '%s' for NPC '%s'" DEBUG_CR, pItem->GetResourceName(), GetResourceName()));
			}
			else
			{
				DEBUG_ERR(( "Loot item %s too heavy for NPC %s" DEBUG_CR, pItem->GetResourceName(), GetResourceName()));
			}
			pItem->Delete();
			return false;
		}
		pszWhere = g_Cfg.GetDefaultMsg( DEFMSG_FEET );
		ItemDrop( pItem, GetTopPoint());
	}

	SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_ITEMPLACE ), pItem->GetName(), pszWhere );
	return( true );
}

bool CChar::ItemDrop( CItem * pItem, const CPointMap & pt )
{
	// A char actively drops an item on the ground.
	if ( pItem == NULL )
		return( false );

	CItemBase * pItemDef = pItem->Item_GetDef();
	if (( g_Cfg.m_fFlipDroppedItems || pItemDef->Can(CAN_I_FLIP)) &&
		pItem->IsMovableType() &&
		! pItemDef->IsStackableType())
	{
		// Does this item have a flipped version.
		pItem->SetDispID( pItemDef->GetNextFlipID( pItem->GetDispID()));
	}

	return( pItem->MoveToCheck( pt, this ));
}

bool CChar::ItemEquip( CItem * pItem, CChar * pCharMsg )
{
	// Equip visible stuff. else throw into our pack.
	// Pay no attention to where this came from.
	// Bounce anything in the slot we want to go to. (if possible)
	// NOTE: This can be used from scripts as well to equip memories etc.
	// ASSUME this is ok for me to use. (movable etc)

	if ( pItem == NULL )
		return( false );

	// In theory someone else could be dressing me ?
	if ( pCharMsg == NULL )
	{
		pCharMsg = this;
	}

	if ( pItem->GetParent() == this )
	{
		if ( pItem->GetEquipLayer() != LAYER_DRAGGING )
		{
			// already equipped.
			return( true );
		}
		// do this later to have correct TOPOBJ and CONT in @EquipTest
		// pItem->RemoveSelf();	// remove from where i am before add so UNEQUIP effect takes.
		// NOTE: CanEquipLayer may bounce an item . If it stacks with this we are in trouble !
	}

	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		if ( pItem->OnTrigger( ITRIG_EQUIPTEST, this ) == TRIGRET_RET_TRUE )
		{
			if ( pItem->GetEquipLayer() == LAYER_DRAGGING ) // dragging? else just do nothing
			{
				pItem->RemoveSelf();
				ItemBounce( pItem );
			}
			return( false );
		}
	}

	// strong enough to equip this . etc ?
	// Move stuff already equipped.
   	if ( pItem->GetAmount() > 1 )
		pItem->UnStackSplit( 1, this );
	// remove it from the container so that nothing will be stacked with it if unequipped
	// torch/candle dupe fix
	pItem->RemoveSelf();

	LAYER_TYPE layer = CanEquipLayer( pItem, LAYER_QTY, pCharMsg, false );

	if ( layer == LAYER_NONE )
	{
		ItemBounce( pItem );
		return( false );
	}

	pItem->SetDecayTime(-1);	// Kill any decay timer.
	LayerAdd( pItem, layer );
	if ( ! pItem->IsItemEquipped())	// Equip failed ? (cursed?) Did it just go into pack ?
		return( false );

	if ( pItem->OnTrigger( ITRIG_EQUIP, this ) == TRIGRET_RET_TRUE )
	{
		return( false );
	}
	if ( ! pItem->IsItemEquipped())	// Equip failed ? (cursed?) Did it just go into pack ?
		return( false );

	Spell_Effect_Add( pItem );	// if it has a magic effect.

	if ( CItemBase::IsVisibleLayer(layer))	// visible layer ?
	{
		Sound( 0x057 );
	}

	return( true );
}

void CChar::EatAnim( LPCTSTR pszName, int iQty )
{
	Stat_SetVal( STAT_FOOD, Stat_GetVal(STAT_FOOD) + iQty );

	static const SOUND_TYPE sm_EatSounds[] = { 0x03a, 0x03b, 0x03c };

	Sound( sm_EatSounds[ Calc_GetRandVal( COUNTOF(sm_EatSounds)) ] );
	UpdateAnimate( ANIM_EAT );

	TCHAR *pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_EATSOME), (LPCTSTR)pszName);
	Emote(pszMsg);
}

bool CChar::Reveal( DWORD dwFlags )
{
	// Some outside influence may be revealing us.

	if ( ! IsStatFlag( dwFlags ))	// no effect.
		return( false );

	if (( dwFlags & STATF_Sleeping ) && IsStatFlag( STATF_Sleeping ))
	{
		// Try to wake me up.
		Wake();
	}
	bool fInvis = false;
	if (( dwFlags & STATF_Invisible ) && IsStatFlag( STATF_Invisible  ))
	{
		fInvis = true;
		SetHue( m_prev_Hue );
	}

	ASSERT( !( dwFlags & (STATF_Pet|STATF_Spawned|STATF_SaveParity|STATF_Ridden|STATF_OnHorse)));
	StatFlag_Clear( dwFlags );
	if ( IsStatFlag( STATF_Invisible | STATF_Hidden | STATF_Insubstantial | STATF_Sleeping ))
		return( false );

	if ( fInvis )
	{
		RemoveFromView();	// just the change in wHue requires this .
		Update();
	}
	else
	{
		UpdateMode( NULL, true );
	}

	SysMessageDefault( DEFMSG_HIDING_REVEALED );

	if ( GetDispID() == CREID_CORPSER )
	{
		// Comes out from under the ground.
		UpdateAnimate( ANIM_MON_Stomp, false );
		Sound( 0x221 );
	}

	return( true );
}

void CChar::SpeakUTF8( LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	// Ignore the font argument here !

	if ( IsStatFlag(STATF_Stone))
		return;
	Reveal( STATF_Hidden|STATF_Sleeping );
	if ( mode == TALKMODE_YELL && GetPrivLevel() >= PLEVEL_Counsel && g_Cfg.m_iDistanceYell > 0 )
	{	// Broadcast yell.
		mode = TALKMODE_BROADCAST;	// GM Broadcast (Done if a GM yells something)
	}
	CObjBase::SpeakUTF8( pszText, wHue, mode, m_fonttype, lang );
}
void CChar::SpeakUTF8Ex( const NWORD * pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	// Ignore the font argument here !

	if ( IsStatFlag(STATF_Stone))
		return;
	Reveal( STATF_Hidden|STATF_Sleeping );
	if ( mode == TALKMODE_YELL && GetPrivLevel() >= PLEVEL_Counsel && g_Cfg.m_iDistanceYell > 0 )
	{	// Broadcast yell.
		mode = TALKMODE_BROADCAST;	// GM Broadcast (Done if a GM yells something)
	}
	if ( m_pNPC )
	{
		wHue = m_pNPC->m_SpeechHue;
	}
	CObjBase::SpeakUTF8Ex( pszText, wHue, mode, m_fonttype, lang );
}
void CChar::Speak( LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	// Speak to all clients in the area.
	// Ignore the font argument here !

	if ( IsStatFlag(STATF_Stone))
		return;
	Reveal( STATF_Hidden|STATF_Sleeping );
	if ( mode == TALKMODE_YELL && GetPrivLevel() >= PLEVEL_Counsel && g_Cfg.m_iDistanceYell > 0 )
	{	// Broadcast yell.
		mode = TALKMODE_BROADCAST;	// GM Broadcast (Done if a GM yells something)
	}
	if ( m_pNPC )
	{
		wHue = m_pNPC->m_SpeechHue;
	}
	CObjBase::Speak( pszText, wHue, mode, m_fonttype );
}

CItem * CChar::Make_Figurine( CGrayUID uidOwner, ITEMID_TYPE id )
{
	// Make me into a figurine
	if ( IsDisconnected())	// we are already a figurine !
		return( NULL );
	if ( m_pPlayer )
		return( NULL );

	CCharBase* pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	// turn creature into a figurine.
	CItem * pItem = CItem::CreateScript( ( id == ITEMID_NOTHING ) ? pCharDef->m_trackID : id, this );
	ASSERT(pItem);

	pItem->SetType( IT_FIGURINE );
	pItem->SetName( GetName());
	pItem->SetHue( GetHue());
	pItem->m_itFigurine.m_ID = GetID();	// Base type of creature.
	pItem->m_itFigurine.m_UID = GetUID();
	pItem->m_uidLink = uidOwner;

	if ( IsStatFlag( STATF_Insubstantial ))
	{
		pItem->SetAttr(ATTR_INVIS);
	}

	SoundChar( CRESND_RAND1 );	// Horse winny
	m_atRidden.m_FigurineUID = pItem->GetUID();
	StatFlag_Set( STATF_Ridden );
	Skill_Start( NPCACT_RIDDEN );
	SetDisconnected();

	return( pItem );
}

CItem * CChar::NPC_Shrink()
{
	// This will just kill conjured creatures.
	if ( IsStatFlag( STATF_Conjured ))
	{
		Stat_SetVal( STAT_STR, 0 );
		return( NULL );
	}

	CItem * pItem = Make_Figurine( UID_CLEAR, ITEMID_NOTHING );
	if ( pItem == NULL )
		return( NULL );

	pItem->SetAttr(ATTR_MAGIC);
	pItem->MoveToCheck( GetTopPoint());
	return( pItem );
}

CItem * CChar::Horse_GetMountItem() const
{
	// I am a horse.
	// Get my mount object. (attached to my rider)

	if ( ! IsStatFlag( STATF_Ridden ))
		return( NULL );

	DEBUG_CHECK( Skill_GetActive() == NPCACT_RIDDEN );
	DEBUG_CHECK( m_pNPC );

	CItem * pItem = m_atRidden.m_FigurineUID.ItemFind();
	if ( pItem == NULL ||
		( ! pItem->IsType( IT_FIGURINE ) && ! pItem->IsType( IT_EQ_HORSE )))
	{
		return( NULL );
	}

	DEBUG_CHECK( pItem->m_itFigurine.m_UID == GetUID());
	return( pItem );
}

CChar * CChar::Horse_GetMountChar() const
{
	CItem * pItem = Horse_GetMountItem();
	if ( pItem == NULL )
		return( NULL );
	return( dynamic_cast <CChar*>( pItem->GetTopLevelObj()));
}

const WORD g_Item_Horse_Mounts[][2] = // extern
{
	ITEMID_M_HORSE1,		CREID_HORSE1,
	ITEMID_M_HORSE2,		CREID_HORSE2,
	ITEMID_M_HORSE3,		CREID_HORSE3,
	ITEMID_M_HORSE4,		CREID_HORSE4,
	ITEMID_M_OSTARD_DES,	CREID_Ostard_Desert,	// t2A
	ITEMID_M_OSTARD_Frenz,	CREID_Ostard_Frenz,		// t2A
	ITEMID_M_OSTARD_For,	CREID_Ostard_Forest,	// t2A
	ITEMID_M_LLAMA,			CREID_Llama,			// t2A
	0,0,
};

bool CChar::Horse_Mount( CChar * pHorse ) // Remove horse char and give player a horse item
{
	// RETURN:
	//  true = done mounting so take no more action.
	//  false = we can't mount this so do something else.
	//

	if ( ! CanTouch( pHorse ))
	{
		SysMessageDefault( DEFMSG_MOUNT_DIST );
		return( false );
	}

	ITEMID_TYPE id;
	for ( int i=0; true; i++ )
	{
		if ( i>=COUNTOF(g_Item_Horse_Mounts))
		{
			return( false );
		}
		if ( pHorse->GetDispID() == g_Item_Horse_Mounts[i][1] )
		{
			id = (ITEMID_TYPE) g_Item_Horse_Mounts[i][0];
			break;
		}
	}

	if ( IsStatFlag( STATF_DEAD ) ||	// can't mount horse if dead!
		! IsHuman())	// only humans can ride horses.
	{
		SysMessageDefault( DEFMSG_MOUNT_UNABLE );
		return( false );
	}
	if ( ! pHorse->NPC_IsOwnedBy( this ) || pHorse->m_pPlayer )
	{
		SysMessageDefault( DEFMSG_MOUNT_DONTOWN );
		return( false );
	}

	Horse_UnMount();	// unmount if already on a horse.

	CItem * pItem = pHorse->Make_Figurine( GetUID(), id );
	if (pItem == NULL )
		return( false );

	pItem->SetType( IT_EQ_HORSE );
	pItem->SetTimeout( 10*TICK_PER_SEC );	// give the horse a tick everyone once in a while.
	LayerAdd( pItem, LAYER_HORSE );	// equip the horse item

	return( true );
}

bool CChar::Horse_UnMount() // Get off a horse (Remove horse item and spawn new horse)
{
	if ( ! IsStatFlag( STATF_OnHorse ))
		return( false );

	CItem * pItem = LayerFind( LAYER_HORSE );
	if ( pItem == NULL )
	{
		StatFlag_Clear( STATF_OnHorse );	// flag got out of sync !
		return( false );
	}

	// What creature is the horse item ?
	CChar * pHorse = Use_Figurine( pItem, 0 );
	pItem->Delete();
	return( true );
}

void CChar::OnHearEquip( CChar * pCharSrc, TCHAR * szText )
{
	// Item in my inventory heard something ?

	ASSERT( IsStatFlag(STATF_COMM_CRYSTAL));

	CItem* pItemNext;
	CItem* pItem=GetContentHead();
	for ( ; pItem!=NULL; pItem=pItemNext )
	{
		pItemNext = pItem->GetNext();
		pItem->OnHear( szText, pCharSrc );
	}
}

bool CChar::OnTickEquip( CItem * pItem )
{
	// A timer expired for an item we are carrying.
	// Does it periodically do something ?
	// REUTRN:
	//  false = delete it.

	switch ( pItem->GetEquipLayer())
	{
	case LAYER_FLAG_Wool:
		// This will regen the sheep it was sheered from.
		// Sheared sheep regen wool on a new day.
		if ( GetID() != CREID_Sheep_Sheered )
			return false;

		// Is it a new day ? regen my wool.
		SetID( CREID_Sheep );
		return false;

	case LAYER_FLAG_ClientLinger:
		// remove me from other clients screens.
		DEBUG_CHECK( pItem->IsType( IT_EQ_CLIENT_LINGER ));
		SetDisconnected();
		return( false );

	case LAYER_SPECIAL:
		switch ( pItem->GetType())
		{
		case IT_EQ_SCRIPT:	// pure script.
			break;
		case IT_EQ_NPC_SCRIPT:
			// Make the next action in the script if it is running.
			NPC_Script_OnTick( dynamic_cast <CItemMessage*>( pItem ), false );
			return( true );
		case IT_EQ_MEMORY_OBJ:
			return Memory_OnTick( dynamic_cast <CItemMemory*>( pItem ));
		default:
			DEBUG_CHECK( 0 );	// should be no other types here.
			break;
		}
		break;

	case LAYER_FLAG_Stuck:
		// Only allow me to try to damage the web so often
		// Non-magical. held by something.
		// IT_EQ_STUCK
		pItem->SetTimeout( -1 );
		return( true );

	case LAYER_HORSE:
		// Give my horse a tick. (It is still in the game !)
		// NOTE: What if my horse dies (poisoned?)
		{
			CChar * pHorse = pItem->m_itFigurine.m_UID.CharFind();
			if ( pHorse == NULL )
				return( false );
			pItem->SetTimeout( 10 * TICK_PER_SEC );
			return( pHorse->OnTick());
		}

	case LAYER_FLAG_Murders:
		// decay the murder count.
		DEBUG_CHECK( m_pPlayer );
		if ( ! m_pPlayer || m_pPlayer->m_wMurders <= 0  )
			return( false );

		CScriptTriggerArgs	args;
		args.m_iN1 = m_pPlayer->m_wMurders-1;
		args.m_iN2 = g_Cfg.m_iMurderDecayTime;
		if ( !IsSetEF(EF_Minimize_Triggers) )
		{
			OnTrigger(CTRIG_MurderDecay, this, &args);
			if ( args.m_iN1 < 0 ) args.m_iN1 = 0;
			if ( args.m_iN2 < 1 ) args.m_iN2 = g_Cfg.m_iMurderDecayTime;
		}
		m_pPlayer->m_wMurders = args.m_iN1;
		if ( m_pPlayer->m_wMurders == 0 ) return( false );
		pItem->SetTimeout(args.m_iN2);	// update it's decay time.
		return( true );
	}

	if ( pItem->IsType( IT_SPELL ))
	{
		return Spell_Equip_OnTick(pItem);
	}

	return( pItem->OnTick());
}

bool CChar::SetPoisonCure( int iSkill, bool fExtra )
{
	// Leave the anitdote in your body for a while.
	// iSkill = 0-1000

	CItem * pPoison = LayerFind( LAYER_FLAG_Poison );
	if ( pPoison != NULL )
	{
		// Is it successful ???
		pPoison->Delete();
	}
	if ( fExtra )
	{
		pPoison = LayerFind( LAYER_FLAG_Hallucination );
		if ( pPoison != NULL )
		{
			// Is it successful ???
			pPoison->Delete();
		}
	}
	Update();
	return( true );
}

bool CChar::SetPoison( int iSkill, int iTicks, CChar * pCharSrc )
{
	// SPELL_Poison
	// iSkill = 0-1000 = how bad the poison is
	// iTicks = how long to last.
	// Physical attack of poisoning.

	if ( IsStatFlag( STATF_Conjured ))
	{
		// conjured creatures cannot be poisoned.
		return false;
	}

	CItem * pPoison;
	if ( IsStatFlag( STATF_Poisoned ))
	{
		// strengthen the poison ?
		pPoison = LayerFind( LAYER_FLAG_Poison );
		if ( pPoison)
		{
			pPoison->m_itSpell.m_spellcharges += iTicks;
		}
		return false;
	}

	SysMessage( "You have been poisoned!" );

	// Release if paralyzed ?
	StatFlag_Clear( STATF_Freeze );	// remove paralyze.

	// Might be a physical vs. Magical attack.
	pPoison = Spell_Effect_Create( SPELL_Poison, LAYER_FLAG_Poison, iSkill, (1+Calc_GetRandVal(2))*TICK_PER_SEC, pCharSrc );
	pPoison->m_itSpell.m_spellcharges = iTicks;	// how long to last.
	UpdateStatsFlag();
	return( true );
}

void CChar::Wake()
{
	if ( ! IsStatFlag( STATF_Sleeping ))
		return;
	CItemCorpse * pCorpse = FindMyCorpse();
	if ( pCorpse != NULL )
	{
		RaiseCorpse(pCorpse);
		StatFlag_Clear( STATF_Sleeping );
		Update();	// update light levels etc.
	}
	else
	{
		Stat_SetVal( STAT_STR, 0 );		// Death
	}
}

void CChar::SleepStart( bool fFrontFall )
{
	if ( IsStatFlag( STATF_DEAD | STATF_Sleeping | STATF_Polymorph ))
		return;

	StatFlag_Set( STATF_Sleeping );
	if ( ! MakeCorpse( fFrontFall ))
	{
		SysMessageDefault( DEFMSG_CANTSLEEP );
		return;
	}

	SetID( m_prev_id );
	StatFlag_Clear( STATF_Hidden );

	Update();
}

CItemCorpse * CChar::MakeCorpse( bool fFrontFall )
{
	// some creatures (Elems) have no corpses.
	// IsStatFlag( STATF_DEAD ) might NOT be set. (sleeping)

	bool fLoot = ! IsStatFlag( STATF_Conjured );

	int iDecayTime = -1;	// never default.
	CItemCorpse * pCorpse = NULL;

	if ( fLoot &&
		GetDispID() != CREID_WATER_ELEM &&
		GetDispID() != CREID_AIR_ELEM &&
		GetDispID() != CREID_FIRE_ELEM &&
		GetDispID() != CREID_VORTEX &&
		GetDispID() != CREID_BLADES )
	{
		if ( m_pPlayer )
		{
			Horse_UnMount(); // If i'm conjured then my horse goes with me.
		}

		CItem* pItemCorpse = CItem::CreateScript( ITEMID_CORPSE, this );
		pCorpse = dynamic_cast <CItemCorpse *>(pItemCorpse);
		if ( pCorpse == NULL )	// Weird internal error !
		{
			DEBUG_CHECK(pCorpse);
			pItemCorpse->Delete();
			goto nocorpse;
		}

		TCHAR *pszMsg = Str_GetTemp();
		sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_CORPSE_OF ), (LPCTSTR) GetName());
		pCorpse->SetName(pszMsg);
		pCorpse->SetHue( GetHue());
		pCorpse->SetCorpseType( GetDispID());
		pCorpse->m_itCorpse.m_BaseID = m_prev_id;	// id the corpse type here !
		pCorpse->m_itCorpse.m_facing_dir = m_dirFace;
		pCorpse->SetAttr(ATTR_INVIS);	// Don't display til ready.

		if ( IsStatFlag( STATF_DEAD ))
		{
			pCorpse->m_itCorpse.m_timeDeath = CServTime::GetCurrentTime();	// death time.
			pCorpse->m_itCorpse.m_uidKiller = m_Act_Targ;
			iDecayTime = (m_pPlayer) ?
				g_Cfg.m_iDecay_CorpsePlayer : g_Cfg.m_iDecay_CorpseNPC;
		}
		else	// Sleeping
		{
			pCorpse->m_itCorpse.m_timeDeath.Init();	// Not dead.
			pCorpse->m_itCorpse.m_uidKiller = GetUID();
			iDecayTime = -1;	// never
		}

		if ( IsRespawned())	// not being deleted.
		{
			pCorpse->m_uidLink = GetUID();
		}
	}
	else
	{
nocorpse:
		// Some creatures can never sleep. (not corpse)
		if ( ! IsStatFlag( STATF_DEAD ))
			return( NULL );

		if ( m_pPlayer )
		{
			StatFlag_Clear( STATF_Conjured );
			Horse_UnMount();
		}

		if ( IsHuman())
			return( NULL );	// conjured humans just disapear.

		CItem * pItem = CItem::CreateScript( ITEMID_FX_SPELL_FAIL, this );
		ASSERT(pItem);
		pItem->MoveToDecay( GetTopPoint(), 2*TICK_PER_SEC );
	}

	// can fall forward.
	DIR_TYPE dir = m_dirFace;
	if ( fFrontFall )
	{
		dir = (DIR_TYPE) ( dir | 0x80 );
		if ( pCorpse )
			pCorpse->m_itCorpse.m_facing_dir = dir;
	}

	// Death anim. default is to fall backwards. lie face up.
	CCommand cmd;
	cmd.CharDeath.m_Cmd = XCMD_CharDeath;
	cmd.CharDeath.m_UID = GetUID();	// 1-4
	cmd.CharDeath.m_UIDCorpse = ( pCorpse == NULL ) ? 0 : (DWORD) pCorpse->GetUID(); // 9-12
	cmd.CharDeath.m_DeathFlight = IsStatFlag( STATF_Fly ) ? 1 : 0; 	// Die in flight ?
	cmd.CharDeath.m_Death2Anim = ( dir & 0x80 ) ? 1 : 0; // Fore/Back Death ?

	UpdateCanSee( &cmd, sizeof( cmd.CharDeath ), m_pClient );

	// Move non-newbie contents of the pack to corpse. (before it is displayed)
	if ( fLoot )
	{
		DropAll( pCorpse );
	}
	if ( pCorpse )
	{
		pCorpse->ClrAttr(ATTR_INVIS);	// make visible.
		pCorpse->MoveToDecay( GetTopPoint(), iDecayTime );
	}

	return( pCorpse );
}

bool CChar::RaiseCorpse( CItemCorpse * pCorpse )
{
	// We are creating a char from the current char and the corpse.
	// Move the items from the corpse back onto us.

	// If NPC is disconnected then reconnect them.
	// If the player is off line then don't allow this !!!

	ASSERT(pCorpse);
	if ( pCorpse->GetCount())
	{
		CItemContainer * pPack = GetPackSafe();

		CItem* pItemNext;
		for ( CItem * pItem = pCorpse->GetContentHead(); pItem!=NULL; pItem=pItemNext )
		{
			pItemNext = pItem->GetNext();
			if ( pItem->IsType( IT_HAIR ) ||
				pItem->IsType( IT_BEARD ) ||
				pItem->IsAttr( ATTR_MOVE_NEVER ))
				continue;	// Hair on corpse was copied!
			// Redress if equipped.
			if ( pItem->GetContainedLayer())
				ItemEquip( pItem );	// Equip the item.
			else
				pPack->ContentAdd( pItem );	// Toss into pack.
		}

		// Any items left just dump on the ground.
		pCorpse->ContentsDump( GetTopPoint());
	}

	if ( pCorpse->IsTopLevel() || pCorpse->IsItemInContainer())
	{
		// I should move to where my corpse is just in case.
		m_fClimbUpdated = false; // update climb height
		MoveToChar( pCorpse->GetTopLevelObj()->GetTopPoint());
	}

	// Corpse is now gone. 	// 0x80 = on face.
	Update();
	UpdateDir( (DIR_TYPE)( pCorpse->m_itCorpse.m_facing_dir &~ 0x80 ));
	UpdateAnimate( ( pCorpse->m_itCorpse.m_facing_dir & 0x80 ) ? ANIM_DIE_FORWARD : ANIM_DIE_BACK, true, true, 2 );

	pCorpse->Delete();

	return( true );
}

bool CChar::Death()
{
	// RETURN: false = delete

	if ( IsStatFlag(STATF_DEAD|STATF_INVUL) ) return true;
	if ( m_pNPC )
	{
		//	bugfix: leave no corpse if for some reason creature dies while mounted
		if ( IsStatFlag(STATF_Ridden) )
		{
			StatFlag_Set(STATF_Conjured);
		}
	}

	if ( OnTrigger( CTRIG_Death, this ) == TRIGRET_RET_TRUE )
		return( true );

	if ( IsClient())	// Prevents crashing ?
	{
		GetClient()->addPause();
	}

	// I am dead and we need to give credit for the kill to my attacker(s).
	TCHAR * pszKillStr = Str_GetTemp();
	int iKillStrLen = sprintf( pszKillStr, g_Cfg.GetDefaultMsg( DEFMSG_KILLED_BY ), (m_pPlayer)?'P':'N', GetName() );
	int iKillers = 0;

	// Look through my memories of who i was fighting. (make sure they knew they where fighting me)
	CChar	*pKiller = NULL;
	CItem* pItemNext;
	CItem* pItem=GetContentHead();
	for ( ; pItem!=NULL; pItem=pItemNext )
	{
		pItemNext = pItem->GetNext();

      	if ( pItem->IsType(IT_EQ_TRADE_WINDOW) )
		{
			CItemContainer* pCont = dynamic_cast <CItemContainer*> (pItem);
			if ( pCont )
			{
				pCont->Trade_Delete();
				continue;
			}
		}

		// i was harmed by this killer. noto his as a killer
		if ( pItem->IsMemoryTypes(MEMORY_HARMEDBY|MEMORY_AGGREIVED) )
		{
			CItemMemory * pMemory = STATIC_CAST <CItemMemory *>(pItem);
			pKiller = pMemory->m_uidLink.CharFind();
			if ( pKiller != NULL )
			{
				pKiller->Noto_Kill( this, false );

				iKillStrLen += sprintf(pszKillStr+iKillStrLen, "%s%c'%s'", iKillers ? ", " : "", (pKiller->m_pPlayer)?'P':'N', pKiller->GetName());
				iKillers ++;
			}
			Memory_ClearTypes(pMemory, 0xFFFF);
		}
	}

	//	No aggressor/killer detected. Try detect person last hit me  from the act target
	if ( !pKiller )
	{
		CObjBase	*ob = g_World.FindUID(m_Act_Targ);
		if ( ob ) pKiller = STATIC_CAST <CChar *>(ob);
	}

	if ( pKiller )
	{
		CScriptTriggerArgs args(this);
		pKiller->OnTrigger(CTRIG_Kill, pKiller, &args);
	}

	// record the kill event for posterity.

	iKillStrLen += sprintf( pszKillStr+iKillStrLen, ( iKillers ) ? "." DEBUG_CR : "accident." DEBUG_CR );
	g_Log.Event( ((m_pPlayer) ? LOGL_EVENT:LOGL_TRACE )|LOGM_KILLS, pszKillStr );

	if ( m_pParty )
	{
		m_pParty->SysMessageAll( pszKillStr );
	}

	NPC_PetClearOwners();	// Forgot who owns me. dismount my master if ridden.
	Reveal();
	SoundChar( CRESND_DIE );
	Spell_Dispel(100);		// Get rid of all spell effects.

	// Only players should loose stats upon death.
	if ( m_pPlayer )
	{
		m_pPlayer->m_wDeaths++;
		Noto_Fame( -Stat_GetAdjusted(STAT_FAME)/10 );

		//	experience could go down
		if ( g_Cfg.m_bExperienceSystem && ( g_Cfg.m_iExperienceMode&EXP_MODE_ALLOW_DOWN ))
		{
			ChangeExperience(-((int)m_exp/10));
		}
	}

	// create the corpse item.
	StatFlag_Set(STATF_DEAD);
	StatFlag_Clear(STATF_Stone|STATF_Freeze|STATF_Hidden|STATF_Sleeping);

	CItemCorpse * pCorpse = MakeCorpse(Calc_GetRandVal(2));

	Stat_SetVal( STAT_STR, 0 ); 	// on my way to death.

	if ( m_pPlayer )
	{
		SetHue( HUE_DEFAULT );	// Get all pale.

		LPCTSTR pszGhostName = "c_ghost_man";
		CCharBase	*pCharDefPrev = CCharBase::FindCharBase( m_prev_id );

		if ( pCharDefPrev && pCharDefPrev->IsFemale() )
			pszGhostName = "c_ghost_woman";

		SetID( (CREID_TYPE) g_Cfg.ResourceGetIndexType( RES_CHARDEF, pszGhostName ));
		LayerAdd( CItem::CreateScript( ITEMID_DEATHSHROUD, this ));
		Update();		// show everyone I am now a ghost.

		// Manifest the ghost War mode for ghosts.
		if ( ! IsStatFlag( STATF_War ))
			StatFlag_Set( STATF_Insubstantial );
	}

	//	bugfix: no need to call @DeathCorpse since no corpse is created
	if ( pCorpse )
	{
   		CScriptTriggerArgs Args( pCorpse );
   		OnTrigger( CTRIG_DeathCorpse, this, &Args );
	}

	Skill_Cleanup();

	if ( !IsClient() )
	{
		if ( m_pPlayer || IsRespawned() )
		{
			SetDisconnected();	// Respawn the NPC later
			return( true );
		}

		// Makes no sense to link the corpse to something that is not going to be valid.
		if ( pCorpse && pCorpse->m_uidLink == GetUID())
			pCorpse->m_uidLink.InitUID();
	}
	else return true;
	return false;	// must delete this now !
}


bool CChar::OnFreezeCheck()
{
	// Check why why are held in place.
	// Can we break free ?
	// RETURN: true = held in place.
	DEBUG_CHECK( IsStatFlag( STATF_Stone | STATF_Freeze ));

	CItem * pFlag = LayerFind( LAYER_FLAG_Stuck );
	if ( pFlag == NULL )	// stuck for some other reason i guess.
	{
		SysMessage(( IsStatFlag( STATF_Sleeping )) ?
			g_Cfg.GetDefaultMsg( DEFMSG_UNCONSCIOUS ) :
			g_Cfg.GetDefaultMsg( DEFMSG_FROZEN ) );
	}
	else
	{
		// IT_EQ_STUCK
		CItem * pWeb = pFlag->m_uidLink.ItemFind();
		if ( pWeb == NULL ||
			! pWeb->IsTopLevel() ||
			pWeb->GetTopPoint() != GetTopPoint())
		{
			// Maybe we teleported away ?
			pFlag->Delete();
			return( false );
		}

		// Only allow me to try to damage it once per sec.
		if ( ! pFlag->IsTimerSet())
		{
			return( Use_Obj( pWeb, false ));
		}
	}
	return( ! IsPriv( PRIV_GM ));
}

void CChar::Flip( LPCTSTR pCmd )
{
	UpdateDir( GetDirTurn( m_dirFace, 1 ));
}

CRegionBase * CChar::CanMoveWalkTo( CPointBase & ptDst, bool fCheckChars, bool fCheckOnly, DIR_TYPE dir )
{
	// For both players and NPC's
	// Walk towards this point as best we can.
	// Affect stamina as if we WILL move !
	// RETURN:
	//  ptDst.m_z = the new z
	//  NULL = failed to walk here.
	if ( IsStatFlag( STATF_Freeze | STATF_Stone ) && OnFreezeCheck())
	{
		// NPC's would call here.
		return( NULL );	// can't move.
	}

	if ( !fCheckOnly && Stat_GetVal(STAT_DEX) <= 0 && ! IsStatFlag( STATF_DEAD ) )
	{
		SysMessageDefault( DEFMSG_FATIGUE );
		return( NULL );
	}


	int iWeightLoadPercent = GetWeightLoadPercent( GetTotalWeight());
	if ( !fCheckOnly && iWeightLoadPercent > 200 )
	{
		SysMessageDefault( DEFMSG_OVERLOAD );
		return( NULL );
	}


	// ok to go here ? physical blocking objects ?
	WORD wBlockFlags = 0;
	signed char ClimbHeight;
	CRegionBase * pArea;

	if ( IsSetEF( EF_WalkCheck ) )
		pArea = CheckValidMove_New( ptDst, &wBlockFlags, dir, &ClimbHeight );
	else
		pArea = CheckValidMove( ptDst, &wBlockFlags, dir );

	if ( pArea == NULL )
		return( NULL );

	if ( fCheckOnly )
	{
		if (( g_Cfg.m_iNpcAi&NPC_AI_PATH ) && fCheckChars )	// fast lookup of being able to go through char there
		{
			if ( !IsStatFlag(STATF_DEAD|STATF_Sleeping|STATF_Insubstantial) )
			{
				CWorldSearch AreaChars(ptDst);
				AreaChars.SetAllShow(true);
				while ( true )
				{
					CChar *pChar = AreaChars.GetChar();
					if ( !pChar )
						break;
					if (( pChar == this ) || (pChar->GetTopZ() != ptDst.m_z) || !ptDst.IsSameMap(pChar->GetTopMap()) )
						continue;

					if ( !m_pPlayer )
						return NULL;	// not through non-players
					if ( pChar->IsStatFlag(STATF_DEAD|STATF_Insubstantial) || pChar->IsDisconnected())
						continue;

					//	How much stamina to push past ?
					int iStamReq = g_Cfg.Calc_WalkThroughChar(this, pChar);
					//	cannot push the char
					if ( iStamReq < 0 || Stat_GetVal(STAT_DEX) <= iStamReq )
						return NULL;
				}
			}
		}
		return pArea;
	}

	if ( ! m_pPlayer )
	{
		// Does the NPC want to walk here ?
		if ( !NPC_CheckWalkHere( ptDst, pArea, wBlockFlags ) )
			return( NULL );
	}


	if (	IsStatFlag(STATF_OnHorse)
		&&	( wBlockFlags & CAN_I_ROOF )
		&&	g_Cfg.m_iMountHeight
		&&	!pArea->IsFlag(REGION_FLAG_UNDERGROUND)
		&&	! IsPriv(PRIV_GM) ) // but ok in dungeons
	{
		SysMessageDefault( DEFMSG_MOUNT_CEILING );
		return( NULL );
	}

	// Bump into other creatures ?
	if ( ! IsStatFlag( STATF_DEAD | STATF_Sleeping | STATF_Insubstantial ) && fCheckChars )
	{
		CWorldSearch AreaChars( ptDst );
		AreaChars.SetAllShow( true );	// show logged out chars.
		while (true)
		{
			CChar * pChar = AreaChars.GetChar();
			if ( pChar == NULL )
				break;
			if ( pChar == this )
				continue;
			if ( pChar->GetTopZ() != ptDst.m_z )
				continue;
			if ( ! ptDst.IsSameMap( pChar->GetTopMap()))
				continue;

			if ( ! m_pPlayer )
				return( NULL ); // NPC's can't bump thru.

			if ( pChar->IsStatFlag( STATF_DEAD | STATF_Insubstantial ) ||
				pChar->IsDisconnected())
			{
				if ( CanDisturb(pChar) && pChar->IsStatFlag(STATF_SpiritSpeak))
				{
					SysMessageDefault( DEFMSG_TINGLING );
				}
				continue;
			}

			// How much stamina to push past ?
			int iStamReq = g_Cfg.Calc_WalkThroughChar( this, pChar );

			if ( !IsSetEF(EF_Minimize_Triggers) )
			{
				CScriptTriggerArgs Args( iStamReq );
				TRIGRET_TYPE	iRet	= pChar->OnTrigger( CTRIG_PersonalSpace, this );
				iStamReq	= Args.m_iN1;

				if ( iRet == TRIGRET_RET_TRUE )
					return NULL;
			}

			TCHAR *pszMsg = Str_GetTemp();
			if ( pChar->IsStatFlag( STATF_Invisible ))
			{
				strcpy(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_INVISIBLE));
				continue;
			}
			else if ( pChar->IsStatFlag( STATF_Hidden ))
			{
				// reveal hidden people ?
				sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_HIDING_STUMBLE ), (LPCTSTR) pChar->GetName());
				pChar->Reveal(STATF_Hidden);
			}
			else if ( pChar->IsStatFlag( STATF_Sleeping ))
			{
				sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_STEPON_BODY ), (LPCTSTR) pChar->GetName());
				// ??? damage.
			}
			else if ( iStamReq < 0 || Stat_GetVal(STAT_DEX) < iStamReq + 1 )
			{
				sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_CANTPUSH ), (LPCTSTR) pChar->GetName());
				if ( ! IsPriv( PRIV_GM ))
				{
					SysMessage(pszMsg);
					return NULL;
				}
			}
			else
			{
				// ??? What is the true amount of stamina lost ?
				sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_PUSH ), (LPCTSTR) pChar->GetName());
			}
			SysMessage(pszMsg);

			if ( IsPriv( PRIV_GM ))
			{
				// client deducts stam for us. make sure this is fixed.
				UpdateStatVal( STAT_DEX, 10 );
			}
			else
			{
				if ( iStamReq < 0 )		// this really should not happen.
					iStamReq = 0;
				UpdateStatVal( STAT_DEX, -iStamReq );
			}
			break;
		}
	}


	// decrease stamina if running or overloaded.
	if ( !IsPriv(PRIV_GM) )
	{
		// We are overloaded. reduce our stamina faster.
		// Running acts like an increased load.
		int iStamReq = g_Cfg.Calc_DropStamWhileMoving( this, iWeightLoadPercent );
		if ( iStamReq )
		{
			// Lower avail stamina.
			UpdateStatVal( STAT_DEX, -iStamReq );
		}
	}

	StatFlag_Mod( STATF_InDoors, ( wBlockFlags & CAN_I_ROOF ) || pArea->IsFlag(REGION_FLAG_UNDERGROUND) );

	if ( wBlockFlags & CAN_I_CLIMB )
		m_zClimbHeight = ClimbHeight;
	else
		m_zClimbHeight = 0;

	return( pArea );
}

void CChar::CheckRevealOnMove()
{
	// Are we going to reveal ourselves by moving ?
	if ( IsStatFlag( STATF_Invisible | STATF_Hidden | STATF_Sleeping ))
	{
		// Wake up if sleeping and this is possible.
		bool bReveal(false);

		if ( IsStatFlag( STATF_Fly ) ||
			! IsStatFlag( STATF_Hidden ) ||
			IsStatFlag( STATF_Sleeping ) ||
			! Skill_UseQuick( SKILL_STEALTH, Calc_GetRandVal( 105 )))
			bReveal = true;

		CScriptTriggerArgs Args(bReveal);	// ARGN1 - reveal?
		OnTrigger(CTRIG_StepStealth, this, &Args);
		bReveal = Args.m_iN1 == 1;
		if ( bReveal ) Reveal();
	}
}

bool CChar::CheckLocation( bool fStanding )
{
	// We are at this location
	// what will happen ?
	// RETURN: true = we teleported.

	if ( ! fStanding )
	{
		SKILL_TYPE iSkillActive	= Skill_GetActive();

		if ( g_Cfg.IsSkillFlag( iSkillActive, SKF_IMMOBILE ) )
		{
			Skill_Fail(false);
			return true;
		}

		if ( g_Cfg.IsSkillFlag( iSkillActive, SKF_FIGHT ) )
		{
			// Are we using a skill that is effected by motion ?
			m_atFight.m_fMoved	= 1;
		}
		else switch ( iSkillActive )
		{
		case SKILL_MEDITATION:
		case SKILL_NECROMANCY:
		case SKILL_MAGERY:
			// Skill is broken if we move ?
			break;
		case SKILL_HIDING:	// this should become stealth ?
			break;
		case SKILL_FENCING:
		case SKILL_MACEFIGHTING:
		case SKILL_SWORDSMANSHIP:
		case SKILL_WRESTLING:
			m_atFight.m_fMoved	= 1;
			break;
		case SKILL_ARCHERY:
			m_atFight.m_fMoved	= 1;
			if ( !IsSetOF( OF_Archery_CanMove ) )
			{
				// If we moved and are wielding are in combat and are using a
				// crossbow/bow kind of weapon, then reset the weaponswingtimer.
				Fight_ResetWeaponSwingTimer();
			}
			break;
		}

		// This could get REALLY EXPENSIVE !
		if ( m_pArea->OnRegionTrigger( this, RTRIG_STEP ) == TRIGRET_RET_TRUE )
			return( false );
	}

	bool	fStepCancel	= false;
	CWorldSearch AreaItems( GetTopPoint());
	while (true)
	{
		CItem * pItem = AreaItems.GetItem();
		if ( pItem == NULL )
			break;

		int zdiff = pItem->GetTopZ() - GetTopZ();

		int	height	= pItem->Item_GetDef()->GetHeight();
		if ( height < 3 )
			height	= 3;

		if ( zdiff > height || zdiff < -3 )

		// if ( abs(zdiff) > 3 )
			continue;

		CScriptTriggerArgs Args( (int) fStanding );
		if ( pItem->OnTrigger( ITRIG_STEP, this , &Args ) == TRIGRET_RET_TRUE )
		{
			fStepCancel	= true;
			continue;
		}

		switch ( pItem->GetType() )
		{
		case IT_SHRINE:
			// Resurrect the ghost
			if ( fStanding )
				continue;
			OnSpellEffect( SPELL_Resurrection, this, 1000, pItem );
			return( false );
		case IT_WEB:
			if ( fStanding )
				continue;
			// Caught in a web.
			if ( Use_Item_Web( pItem ))
				return( true );
			continue;
		// case IT_CAMPFIRE:	// does nothing. standing on burning kindling shouldn't hurt us
		case IT_FIRE:
			// fire object hurts us ?
			// pItem->m_itSpell.m_spelllevel = 0-1000 = heat level.
			{
				int iSkillLevel = pItem->m_itSpell.m_spelllevel/2;
				iSkillLevel = iSkillLevel + Calc_GetRandVal(iSkillLevel);
				if ( IsStatFlag( STATF_Fly ))	// run through fire.
				{
					iSkillLevel /= 2;
				}
				OnTakeDamage( g_Cfg.GetSpellEffect( SPELL_Fire_Field, iSkillLevel ), NULL, DAMAGE_FIRE | DAMAGE_GENERAL );
			}
			Sound( 0x15f ); // Fire noise.
			return( false );
		case IT_SPELL:
			{
				SPELL_TYPE Spell = (SPELL_TYPE) RES_GET_INDEX(pItem->m_itSpell.m_spell);
				OnSpellEffect( Spell, pItem->m_uidLink.CharFind(), pItem->m_itSpell.m_spelllevel, pItem );
				const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(Spell);
				if ( pSpellDef )
				{
					Sound( pSpellDef->m_sound);
				}
				/*
				if ( !m_pPlayer && Calc_GetRandVal(3) )
				{
					m_atFlee.m_iStepsMax = 5 + Calc_GetRandVal(10);	// how long should it take to get there.
					m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
					Skill_Start( NPCACT_FLEE );
				}
				*/
			}
			return( false );
		case IT_TRAP:
		case IT_TRAP_ACTIVE:
		// case IT_TRAP_INACTIVE: // reactive it?
			OnTakeDamage( pItem->Use_Trap(), NULL, DAMAGE_HIT_BLUNT | DAMAGE_GENERAL );
			return( false );
		case IT_SWITCH:
			if ( pItem->m_itSwitch.m_fStep )
			{
				Use_Item( pItem );
			}
			return( false );
		case IT_MOONGATE:
		case IT_TELEPAD:
			if ( fStanding )
				continue;
			Use_MoonGate( pItem );
			return( true );
		case IT_SHIP_PLANK:
			// a plank is a teleporter off the ship.
			if ( ! fStanding && ! IsStatFlag( STATF_Fly ))
			{
				// Find some place to go. (in direction of plank)
				if ( MoveToValidSpot( m_dirFace, UO_MAP_VIEW_SIZE, 1 ))
				{
					pItem->SetTimeout( 5*TICK_PER_SEC );	// autoclose it behind us.
					return( true );
				}
			}
			continue;

		case IT_ADVANCE_GATE:
			// Upgrade the player to the skills of the selected NPC script.
			if ( fStanding )
				continue;
			Use_AdvanceGate( pItem );
			break;
		}
	}

	if ( fStepCancel )
		return false;

	if ( fStanding )
		return( false );

	// Check the map teleporters in this CSector. (if any)
	CPointMap pt = GetTopPoint();
	CSector * pSector = pt.GetSector();
	ASSERT(pSector);
	const CTeleport * pTel = pSector->GetTeleport( pt );
	if ( pTel )
	{
		// Only do this for clients
		if ( IsClient() )
		{
			Spell_Teleport( pTel->m_ptDst, true, false, ITEMID_NOTHING );
			return( true );
		}
	}
	return( false );
}

bool CChar::MoveToRegion( CRegionWorld * pNewArea, bool fAllowReject )
{
	// Moving to a new region. or logging out (not in any region)
	// pNewArea == NULL = we are logging out.
	// RETURN:
	//  false = do not allow in this area.

#ifdef _DEBUG
	if ( pNewArea )
	{
		DEBUG_CHECK( pNewArea->IsValid());
	}
	if ( m_pArea )
	{
		DEBUG_CHECK( m_pArea->IsValid());
	}
#endif

	if ( m_pArea == pNewArea )
		return true;

	if ( ! g_Serv.IsLoading())
	{
		if ( fAllowReject && IsPriv( PRIV_GM ))
		{
			fAllowReject = false;
		}

		// Leaving region trigger. (may not be allowed to leave ?)
		if ( m_pArea )
		{
			if ( m_pArea->OnRegionTrigger( this, RTRIG_EXIT ) == TRIGRET_RET_TRUE )
			{
				if ( pNewArea && fAllowReject )
					return false;
			}
		}

		if ( IsClient() && pNewArea )
		{
			if ( pNewArea->IsFlag( REGION_FLAG_ANNOUNCE ) &&
				! pNewArea->IsInside2d( GetTopPoint()))	// new area.
			{
				CVarDefStr * pVarStr = dynamic_cast <CVarDefStr *>( pNewArea->m_TagDefs.GetKey("ANNOUNCEMENT"));
				SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_REGION_ENTER ), ( pVarStr ) ? (LPCTSTR) pVarStr->GetValStr() :
					(LPCTSTR) pNewArea->GetName());
			}

			// Is it guarded ?
			else if ( m_pArea != NULL && ! IsStatFlag( STATF_DEAD ))
			{
				if ( pNewArea->IsGuarded() != m_pArea->IsGuarded())
				{
					if ( pNewArea->IsGuarded() )
					{
                  // this->GetClient()->addBarkSpeakTable( (SPKTAB_TYPE)112, NULL, 0x3b2, TALKMODE_ITEM, FONT_NORMAL );
						CVarDefStr * pVarStr = dynamic_cast <CVarDefStr *>( pNewArea->m_TagDefs.GetKey("GUARDOWNER"));
						SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_REGION_GUARDS_1 ),
							( pVarStr ) ? (LPCTSTR)pVarStr->GetValStr() : g_Cfg.GetDefaultMsg( DEFMSG_REGION_GUARD_ART ) );
					}
					else
					{
                  // this->GetClient()->addBarkSpeakTable( (SPKTAB_TYPE)113, NULL, 0x3b2, TALKMODE_ITEM, FONT_NORMAL );
						CVarDefStr * pVarStr = dynamic_cast <CVarDefStr *>( m_pArea->m_TagDefs.GetKey("GUARDOWNER"));
						SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_REGION_GUARDS_2 ),
							( pVarStr ) ? (LPCTSTR)pVarStr->GetValStr() : g_Cfg.GetDefaultMsg( DEFMSG_REGION_GUARD_ART ) );
					}
				}
				if ( pNewArea->IsFlag(REGION_FLAG_NO_PVP) != m_pArea->IsFlag(REGION_FLAG_NO_PVP))
				{
					SysMessageDefault(( pNewArea->IsFlag(REGION_FLAG_NO_PVP)) ?
						DEFMSG_REGION_PVPSAFE :
						DEFMSG_REGION_PVPNOT );
				}
				if ( pNewArea->IsFlag(REGION_FLAG_SAFE) != m_pArea->IsFlag(REGION_FLAG_SAFE))
				{
					SysMessageDefault(( pNewArea->IsFlag(REGION_FLAG_SAFE)) ?
						DEFMSG_REGION_SAFETYGET :
						DEFMSG_REGION_SAFETYLOSE );
				}
			}
		}

		// Entering region trigger.
		if ( pNewArea )
		{
			if ( pNewArea->OnRegionTrigger( this, RTRIG_ENTER ) == TRIGRET_RET_TRUE )
			{
				if ( m_pArea && fAllowReject )
					return false;
			}
		}
	}

	m_pArea = pNewArea;
	return( true );
}

bool CChar::MoveToChar( CPointMap pt )
{
	// Same as MoveTo
	// This could be us just taking a step or being teleported.
	// Low level: DOES NOT UPDATE DISPLAYS or container flags. (may be offline)
	// This does not check for gravity.
	//

	if ( ! pt.IsCharValid() || !pt.IsValidXY() )
		return false;

	if ( m_pPlayer && ! IsClient())	// moving a logged out client !
	{
		// We cannot put this char in non-disconnect state.
		SetDisconnected();
		pt.GetSector()->m_Chars_Disconnect.InsertHead( this );
		SetUnkPoint( pt );
		return true;
	}

	// Did we step into a new region ?
	CRegionWorld * pAreaNew = dynamic_cast <CRegionWorld *>( pt.GetRegion( REGION_TYPE_MULTI|REGION_TYPE_AREA ));
	if ( ! MoveToRegion( pAreaNew, true ))
		return false;

	DEBUG_CHECK(m_pArea);

	bool fSectorChange	= pt.GetSector()->MoveCharToSector(this);
	bool fMapChange = false;
	CPointMap	prevPt	= GetUnkPoint();

	// Move this item to it's point in the world. (ground or top level)
	SetTopPoint(pt);
	if ( IsClient() && ( prevPt.m_map != pt.m_map ))
		fSectorChange = fMapChange = true;

	if ( fSectorChange && ! g_Serv.IsLoading() )	// there was a change in environment.
	{
		if ( fMapChange )
			GetClient()->addReSync(true);			// a must-have for map change

		CScriptTriggerArgs	Args( prevPt.m_x, prevPt.m_y, prevPt.m_z << 16 | prevPt.m_map );
		OnTrigger(CTRIG_EnvironChange, this, &Args);
	}

#ifdef _DEBUG
	int iRetWeird = CObjBase::IsWeird();
	if ( iRetWeird )
	{
		DEBUG_CHECK( ! iRetWeird );
	}
#endif
	if ( !m_fClimbUpdated )
		FixClimbHeight();

	return true;
}

bool CChar::MoveToValidSpot( DIR_TYPE dir, int iDist, int iDistStart )
{
	// Move from here to a valid spot.
	// ASSUME "here" is not a valid spot. (even if it really is)

	CPointMap pt = GetTopPoint();
	pt.MoveN( dir, iDistStart );
	pt.m_z += PLAYER_HEIGHT;

	WORD wCan = GetMoveBlockFlags();	// CAN_C_SWIM
	for ( int i=0; i<iDist; i++ )
	{
		WORD wBlockFlags = wCan;
		signed char z = g_World.GetHeightPoint( pt, wBlockFlags, true );;
		if ( ! ( wBlockFlags &~ wCan ))
		{
			// we can go here. (maybe)
			if ( Spell_Teleport( pt, true, true, ITEMID_NOTHING ))
				return( true );
		}
		pt.Move( dir );
	}
	return( false );
}

bool CChar::SetPrivLevel( CTextConsole * pSrc, LPCTSTR pszFlags )
{
	// "PRIVSET"
	// Set this char to be a GM etc. (or take this away)
	// NOTE: They can be off-line at the time.

	ASSERT(pSrc);
	ASSERT(pszFlags);

	if ( pSrc->GetPrivLevel() < PLEVEL_Admin )	// Only an admin can do this.
		return( false );
	if ( pSrc->GetPrivLevel() < GetPrivLevel())	// priv is better than mine ?
		return( false );
	if ( pszFlags[0] == '\0' )
		return( false );
	if ( m_pPlayer == NULL )
		return false;

	CAccount * pAccount = m_pPlayer->GetAccount();
	ASSERT(pAccount);

	PLEVEL_TYPE PrivLevel = CAccount::GetPrivLevelText( pszFlags );

	// Remove Previous GM Robe
	ContentConsume( RESOURCE_ID(RES_ITEMDEF,ITEMID_GM_ROBE), INT_MAX );

	if ( PrivLevel >= PLEVEL_Counsel )
	{
		// Give gm robe.
		CItem * pItem = CItem::CreateScript( ITEMID_GM_ROBE, this );
		ASSERT(pItem);
		pItem->SetAttr( ATTR_MOVE_NEVER | ATTR_NEWBIE | ATTR_MAGIC );
		pItem->m_itArmor.m_spelllevel = 1000;

		pAccount->SetPrivFlags( PRIV_GM_PAGE );
		if ( PrivLevel >= PLEVEL_GM )
		{
			pAccount->SetPrivFlags( PRIV_GM );
			pItem->SetHue( HUE_RED );
		}
		else
		{
			pItem->SetHue( HUE_BLUE_NAVY );
		}
		UnEquipAllItems();
		ItemEquip( pItem );
	}
	else
	{
		// Revoke GM status.
		pAccount->ClearPrivFlags( PRIV_GM_PAGE|PRIV_GM );
	}

	if ( pAccount->GetPrivLevel() < PLEVEL_Admin && PrivLevel < PLEVEL_Admin )	// can't demote admin this way.
	{
		pAccount->SetPrivLevel( PrivLevel );
	}

	Update();
	return true;
}

TRIGRET_TYPE CChar::OnTrigger( LPCTSTR pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs )
{
	// Attach some trigger to the cchar. (PC or NPC)
	// RETURN: true = block further action.
	static LPCTSTR const excInfo[] =
	{
		"",
		"body triggers",
		"events",
		"controlled npc triggers",
	};
	const char *zTemp = excInfo[0];
#if !defined( _DEBUG ) && !defined( NO_INTERNAL_EXCEPTIONS )
	try
	{
#endif
		CTRIG_TYPE iAction;
		if ( ISINTRESOURCE(pszTrigName))
		{
			iAction = (CTRIG_TYPE) GETINTRESOURCE(pszTrigName);
			pszTrigName = sm_szTrigName[iAction];
		}
		else
		{
			iAction = (CTRIG_TYPE) FindTableSorted( pszTrigName, sm_szTrigName, COUNTOF(sm_szTrigName)-1 );
		}

		CCharBase* pCharDef = Char_GetDef();
		ASSERT(pCharDef);

		if ( !m_pPlayer )			//	CHARDEF triggers (based on body type)
		{
			zTemp = excInfo[1];
			if ( pCharDef->HasTrigger(iAction) )
			{
				CResourceLock s;
				if ( pCharDef->ResourceLock(s) )
				{
					TRIGRET_TYPE iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
					if (( iRet != TRIGRET_RET_FALSE ) && ( iRet != TRIGRET_RET_DEFAULT ))
						return iRet;
				}
			}
		}

		//
		// Go thru the event blocks for the NPC/PC to do events.
		//
		zTemp = excInfo[2];
		int i;
		int origEvents = m_OEvents.GetCount();
		int curEvents = origEvents;
		for ( i = 0; i < curEvents; i++ )			//	EVENTS (could be modifyed ingame!)
		{
			CResourceLink	*pLink = m_OEvents[i];
			ASSERT(pLink);
			if ( !pLink->HasTrigger(iAction) )
				continue;
			CResourceLock s;
			if ( !pLink->ResourceLock(s) )
				continue;

			TRIGRET_TYPE iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
			if ( iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT )
				return iRet;

			curEvents = m_OEvents.GetCount();
			if ( curEvents < origEvents ) // the event has been deleted, modify the counter for other trigs to work
			{
				i--;
				origEvents = curEvents;
			}
		}

		if ( m_pNPC )								//	TEVENTS (constant events of NPCs)
		{
			zTemp = excInfo[3];
			for ( i=0; i < pCharDef->m_TEvents.GetCount(); i++ )
			{
				CResourceLink * pLink = pCharDef->m_TEvents[i];
				ASSERT(pLink);
				if ( ! pLink->HasTrigger( iAction ))
					continue;
				CResourceLock s;
				if ( ! pLink->ResourceLock( s ))
					continue;
				TRIGRET_TYPE iRet = CScriptObj::OnTriggerScript( s, pszTrigName, pSrc, pArgs );
				if ( iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT )
					return iRet;
			}
			if ( g_Cfg.m_pEventsPetLink && g_Cfg.m_pEventsPetLink->HasTrigger(iAction) )
			{
				CResourceLock s;
				if ( g_Cfg.m_pEventsPetLink->ResourceLock( s ) )
				{
					TRIGRET_TYPE iRet = CScriptObj::OnTriggerScript( s, pszTrigName, pSrc, pArgs );
					if ( iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT )
						return iRet;
				}
			}
		}
#if !defined( _DEBUG ) && !defined( NO_INTERNAL_EXCEPTIONS )
	}
	catch ( CGrayError &e )
	{
		g_Log.CatchEvent( &e, "Char Trigger in %s (%s) (UID=0%lx)", pszTrigName, zTemp, (DWORD) GetUID());
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent( NULL, "Char Trigger in %s (%s) (UID=0%lx)", pszTrigName, zTemp, (DWORD) GetUID());
	}
#endif
	return TRIGRET_RET_DEFAULT;
}



void CChar::OnTickFood()
{
	if ( IsStatFlag( STATF_Conjured ))
		return;	// No need for food.

	if ( IsStatFlag( STATF_Pet ))	// This may be money instead of food
	{
	    	if ( ! NPC_CheckHirelingStatus())
	    		return;
	}
	if ( Stat_GetMax(STAT_FOOD) == 0 )	// No need for food.
  		return;

	long	lFood = Stat_GetVal(STAT_FOOD);
   	if ( Stat_GetVal(STAT_FOOD) > 0 ) lFood--;
	else lFood++;

	CScriptTriggerArgs Args(lFood);	// ARGN1 - new food level
	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		if ( OnTrigger(CTRIG_Hunger, this, &Args) != TRIGRET_RET_TRUE ) goto dohunger;
		else return;
	}
dohunger:
	{
		Stat_SetVal(STAT_FOOD, lFood);

		int  nFoodLevel = Food_GetLevelPercent();
		if ( nFoodLevel < 40 )	// start looking for food at 40%
 		{
	    	// Tell everyone we look hungry.
	    	SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_HUNGER ), (LPCTSTR) Food_GetLevelMessage( false, false ));
			if ( ! m_pPlayer )
			{
			   	ASSERT(m_pNPC);
  				NPC_OnTickFood( nFoodLevel );
			}
		}
	}
}

bool CChar::OnTick()
{
	TIME_PROFILE_INIT;
	if ( IsSetSpecific )
		TIME_PROFILE_START;
	// Assume this is only called 1 time per sec.
	// Get a timer tick when our timer expires.
	// RETURN: false = delete this.
	static LPCTSTR const excInfo[] =
	{
		"",					// 0
		"death",
		"stats ticking",
		"timer expired",
		"npc tick",
		"combat",
		"combat hittry",
		"update stats",
		"skill abort",
		"skill fail",
		"skill cleanup",	// 10
	};
	const char *zTemp = excInfo[0];

#if !defined( _DEBUG ) && !defined( NO_INTERNAL_EXCEPTIONS )
	try
	{
#endif
		int iTimeDiff = - g_World.GetTimeDiff( m_timeLastRegen );
		if ( iTimeDiff == 0 )
			return true;	// ??? I have no idea why this happens
		DEBUG_CHECK( iTimeDiff > 0 );

		if ( iTimeDiff >= TICK_PER_SEC )	// don't bother with < 1 sec times.
		{
			// decay equipped items (spells)
			CItem* pItemNext;
			CItem* pItem=GetContentHead();
			for ( ; pItem!=NULL; pItem=pItemNext )
			{
				pItemNext = pItem->GetNext();
				DWORD	uid = pItem->GetUID();
				try
				{
					// always check the validity of the memory objects. (even before expired)
					if ( pItem->IsType(IT_EQ_MEMORY_OBJ) )
					{
						if ( !pItem->m_uidLink.ObjFind() )
						{
							pItem->Delete();
							continue;
						}
					}

					if ( !pItem->IsTimerSet() ) continue;
					if ( !pItem->IsTimerExpired() ) continue;
					if ( !OnTickEquip(pItem) )
					{
						pItem->Delete();
					}
				}
				catch (CGrayError &e)
				{
					g_Log.CatchEvent(&e, "Char OnTick (UID=0%lx), item (UID=0%lx) decay.", (DWORD)GetUID(), uid);
				}
				catch (...)
				{
					g_Log.CatchEvent(NULL, "Char OnTick (UID=0%lx), item (UID=0%lx) decay.", (DWORD)GetUID(), uid);
				}
			}

			// Players have a silly "always run" flag that gets stuck on.
			if ( IsClient())
			{
				int iTimeLastEvent = -g_World.GetTimeDiff(GetClient()->m_timeLastEvent);
				if ( iTimeLastEvent > TICK_PER_SEC )
				{
					StatFlag_Clear(STATF_Fly);
				}
			}

			// NOTE: Summon flags can kill our hp here. check again.
			if ( Stat_GetVal(STAT_STR) <= 0 )	// We can only die on our own tick.
			{
				zTemp = excInfo[1];
				m_timeLastRegen = CServTime::GetCurrentTime();
				return Death();
			}
			if ( IsStatFlag( STATF_DEAD ) )	// We are dead, don't update anything.
			{
				m_timeLastRegen = CServTime::GetCurrentTime();
				return true;
			}

			const CRaceClassDef * pRace = Char_GetDef()->GetRace();
			ASSERT(pRace);

			zTemp = excInfo[2];
			for ( int i=0; i< STAT_BASE_QTY+1; i++ )	// allow food
			{
				m_Stat[i].m_regen += iTimeDiff;

				int iRate = pRace->GetRegenRate((STAT_TYPE)i);	// in TICK_PER_SEC

				switch ( i )
				{
				case STAT_STR:
					if ( m_pPlayer )
					{
						if ( IsStatFlag(STATF_Fly))
							continue;
					}

					if ( !Stat_GetVal(STAT_FOOD) )
						continue; // iRate += iRate/2;	// much slower with no food.

					// Fast metabolism bonus ?
					ASSERT( Stat_GetVal(STAT_DEX) >= 0 );
					iRate += iRate / (1 + (Stat_GetVal(STAT_DEX)/8));
					break;
				case STAT_DEX:
					break;
				case STAT_INT:
					break;
				}

				if ( m_Stat[i].m_regen < iRate )
					continue;

				m_Stat[i].m_regen = 0;
				if ( i == STAT_FOOD )
				{
					OnTickFood();
				}
				else if ( Stat_GetVal((STAT_TYPE)i) != Stat_GetMax((STAT_TYPE)i))
				{
					UpdateStatVal( (STAT_TYPE) i, 1 );
				}
			}
		}
		else
		{
			// Check this all the time.
			if ( Stat_GetVal(STAT_STR) <= 0 )	// We can only die on our own tick.
			{
				zTemp = excInfo[1];
				return Death();
			}
		}

		if ( IsStatFlag( STATF_DEAD ))
			return true;
		if ( IsDisconnected())	// mounted horses can still get a tick.
		{
			m_timeLastRegen = CServTime::GetCurrentTime();
			return( true );
		}

		DEBUG_CHECK( IsTopLevel());	// not deleted or off line.

		if ( IsTimerExpired() && IsTimerSet())
		{
			zTemp = excInfo[3];
			// My turn to do some action.
			switch ( Skill_Done())
			{
				case -SKTRIG_ABORT:	zTemp = excInfo[8]; Skill_Fail(true); break;	// fail with no message or credit.
				case -SKTRIG_FAIL:	zTemp = excInfo[9]; Skill_Fail(false); break;
				case -SKTRIG_QTY:	zTemp = excInfo[9]; Skill_Cleanup(); break;
			}
			if ( ! m_pPlayer )
			{
				// What to do next ?
		   		ASSERT(m_pNPC);
				g_Serv.m_Profile.Start( PROFILE_NPC_AI );
				zTemp = excInfo[4];
				NPC_OnTickAction();

				//	Some NPC AI actions
				if ( !IsSetOF(OF_Multithreaded) )
				{
					if ( g_Cfg.m_iNpcAi&NPC_AI_FOOD )
						NPC_Food();
					if ( g_Cfg.m_iNpcAi&NPC_AI_EXTRA )
						NPC_AI();
				}
				g_Serv.m_Profile.Start( PROFILE_CHARS );
			}
		}
		else
		{
			// Are we ready to hit ?
			zTemp = excInfo[5];

			if (( iTimeDiff >= TICK_PER_SEC ) &&
				( g_Cfg.m_wDebugFlags & DEBUGF_SKILL_TIME ))
			{
				TCHAR szTemp[ 128 ];
				int len = sprintf( szTemp, "%d", GetTimerAdjusted());
				if ( Fight_IsActive())
				{
					switch (m_atFight.m_War_Swing_State)
					{
					case WAR_SWING_READY:		szTemp[len++] = '|'; break;
					case WAR_SWING_EQUIPPING:	szTemp[len++] = '-'; break;
					case WAR_SWING_SWINGING:	szTemp[len++] = '+'; break;
					default: szTemp[len++] = '?'; break;
					}
				}
				if ( IsStatFlag(STATF_Fly))
				{
					szTemp[len++] = 'F';
				}
				szTemp[len] = '\0';
				UpdateObjMessage( szTemp, szTemp, NULL, HUE_ORANGE, TALKMODE_NOSCROLL );
			}

			// Hit my current target. (if i'm ready)
			zTemp = excInfo[6];
			if ( IsStatFlag( STATF_War ))
			{
				if ( Fight_IsActive())
				{
					if ( m_atFight.m_War_Swing_State == WAR_SWING_READY )
					{
						Fight_HitTry();
					}
				}
				else if ( Skill_GetActive() == SKILL_NONE )
				{
					Fight_AttackNext();
				}
			}
		}

		if ( iTimeDiff >= TICK_PER_SEC )
		{
			// Check location periodically for standing in fire fields, traps, etc.
			CheckLocation( true );
			m_timeLastRegen = CServTime::GetCurrentTime();
		}

		zTemp = excInfo[7];
		if ( IsClient() )
			GetClient()->UpdateStats();

		iTimeDiff = - g_World.GetTimeDiff( m_timeLastHitsUpdate );
		if ( g_Cfg.m_iHitsUpdateRate && ( iTimeDiff >= g_Cfg.m_iHitsUpdateRate ) ) // update hits for all
		{
			if ( m_fHitsUpdate )
			{
				UpdateHitsForOthers();
				m_fHitsUpdate = false;
			}
			m_timeLastHitsUpdate = CServTime::GetCurrentTime();
		}
#if !defined( _DEBUG ) && !defined( NO_INTERNAL_EXCEPTIONS )
	}
	catch ( CGrayError &e )
	{
		g_Log.CatchEvent( &e, "Char OnTick (%s) (UID=0%lx)", zTemp, (DWORD) GetUID());
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent( NULL, "Char OnTick (%s) (UID=0%lx)", zTemp, (DWORD) GetUID());
	}
#endif
	if ( IsSetSpecific )
	{
		TIME_PROFILE_END;
		DEBUG_ERR(("CChar::OnTick(%x) took %d.%d to run\n", GetUID(), TIME_PROFILE_GET_HI, TIME_PROFILE_GET_LO));
	}
	return true;
}

