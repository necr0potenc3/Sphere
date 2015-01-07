//
// CObjBase.cpp
// Copyright Menace Software (www.menasoft.com).
// base classes.
//
#include "graysvr.h"	// predef header.

#if defined(_DEBUG)
bool CObjBaseTemplate::IsValidContainer() const	// am i really in the container i say i am ?
{
	if ( IsDisconnected())	// must also be at top level !
	{
		// There are several places a disconnected item can be.
		if ( GetParent() == NULL )
			return( true );
		if ( IsChar())
		{
			return( dynamic_cast <CCharsDisconnectList*>( GetParent()) != NULL );
		}
		else
		{
			return( false );	// not valid for item to be disconnected. (except at create time)
		}
	}
	if ( IsItem())
	{
		if ( dynamic_cast <const CItem*>(this) == NULL )
			return( false );
	}
	else
	{
		if ( dynamic_cast <const CChar*>(this) == NULL )
			return( false );
	}
	if ( IsTopLevel())
	{
		if ( IsChar())
		{
			return( dynamic_cast <CCharsActiveList*>( GetParent()) != NULL );
		}
		else
		{
			return( dynamic_cast <CItemsList*>( GetParent()) != NULL );
		}
	}
	if ( IsItemEquipped())
	{
		if ( IsChar())
		{
			return( false );
		}
		else
		{
			return( dynamic_cast <CChar*>( GetParent()) != NULL );
		}
	}
	if ( IsItemInContainer())
	{
		if ( IsChar())
		{
			return( false );
		}
		else
		{
			return( dynamic_cast <CItemContainer*>( GetParent()) != NULL );
		}
	}
	return( false );	// no flags !?
}
#endif

bool GetDeltaStr( CPointMap & pt, TCHAR * pszDir )
{
	TCHAR * ppCmd[3];
	int iQty = Str_ParseCmds( pszDir, ppCmd, COUNTOF(ppCmd));

	if (iQty == 0)
		return( false );

	TCHAR chDir = toupper( ppCmd[0][0] );
	int iTmp = Exp_GetVal( ppCmd[1] );

	if ( isdigit( chDir ) || chDir == '-' )
	{
		pt.m_x += Exp_GetVal( ppCmd[0] );
		pt.m_y += iTmp;
		pt.m_z += Exp_GetVal( ppCmd[2] );
	}
	else	// a direction by name.
	{
		if ( iTmp == 0 )
			iTmp = 1;
		DIR_TYPE eDir = GetDirStr( ppCmd[0] );
		if ( eDir >= DIR_QTY )
			return( false );
		pt.MoveN( eDir, iTmp );
	}

	return( true );
}

/////////////////////////////////////////////////////////////////
// -CObjBase stuff
// Either a player, npc or item.

CObjBase::CObjBase( bool fItem )
{
	sm_iCount ++;
	m_wHue=HUE_DEFAULT;
	m_timeout.Init();

	//	Init some global variables
	m_ModAr = 0;

	if ( g_Serv.IsLoading())
	{
		// Don't do this yet if we are loading. UID will be set later.
		// Just say if this is an item or not.
		CObjBaseTemplate::SetUID( fItem ? ( UID_O_DISCONNECT | UID_F_ITEM | UID_O_INDEX_MASK ) : ( UID_O_DISCONNECT | UID_O_INDEX_MASK ));
	}
	else
	{
		// Find a free UID slot for this.
		SetUID( UID_CLEAR, fItem );
		ASSERT(IsValidUID());
		SetContainerFlags(UID_O_DISCONNECT);	// it is no place for now
	}

	// Put in the idle list by default. (til placed in the world)
	g_World.m_ObjNew.InsertHead( this );
}

CObjBase::~CObjBase()
{
	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		CChar	*pChar = dynamic_cast <CChar*> (this);
		CItem	*pItem = dynamic_cast <CItem*> (this);

		if ( pChar )
			pChar->OnTrigger(CTRIG_Destroy, &g_Serv);
		if ( pItem )
			pItem->OnTrigger(ITRIG_DESTROY, &g_Serv);
	}

	sm_iCount --;
	ASSERT( IsDisconnected());

	// free up the UID slot.
	SetUID( UID_UNUSED, false );
}

bool CObjBase::IsContainer() const
{
	// Simple test if object is a container.
	return( dynamic_cast <const CContainer*>(this) != NULL );
}

int CObjBase::IsWeird() const
{
	int iResultCode = CObjBaseTemplate::IsWeird();
	if ( iResultCode )
	{
		return( iResultCode );
	}
	if ( ! g_Serv.IsLoading())
	{
		if ( GetUID().ObjFind() != this )	// make sure it's linked both ways correctly.
		{
			return( 0x3201 );
		}
	}
	return( 0 );
}

void CObjBase::SetUID( DWORD dwIndex, bool fItem )
{
	// Move the serial number,
	// This is possibly dangerous if conflict arrises.

	dwIndex &= UID_O_INDEX_MASK;	// Make sure no flags in here.
	if ( IsValidUID())	// i already have a uid ?
	{
		if ( ! dwIndex )
			return;	// The point was just to make sure it was located.
		// remove the old UID.
		g_World.FreeUID( ((DWORD)GetUID()) & UID_O_INDEX_MASK );
	}

	if ( dwIndex != UID_O_INDEX_MASK )	// just wanted to remove it
	{
		dwIndex = g_World.AllocUID( dwIndex, this );
	}

	if ( fItem ) dwIndex |= UID_F_ITEM;

	CObjBaseTemplate::SetUID( dwIndex );
}

bool CObjBase::SetNamePool( LPCTSTR pszName )
{
	ASSERT(pszName);

	// Parse out the name from the name pool ?
	if ( pszName[0] == '#' )
	{
		pszName ++;
		TCHAR *pszTmp = Str_GetTemp();
		strcpy( pszTmp, pszName );

		TCHAR * ppTitles[2];
		Str_ParseCmds( pszTmp, ppTitles, COUNTOF(ppTitles));

		CResourceLock s;
		if ( ! g_Cfg.ResourceLock( s, RES_NAMES, ppTitles[0] ))
		{
failout:
			DEBUG_ERR(( "Name pool '%s' could not be found\n", ppTitles[0] ));
			CObjBase::SetName( ppTitles[0] );
			return false;
		}

		// Pick a random name.
		if ( ! s.ReadKey())
			goto failout;
		int iCount = Calc_GetRandVal( atoi( s.GetKey())) + 1;
		while ( iCount-- )
		{
			if ( ! s.ReadKey())
				goto failout;
		}

		return CObjBaseTemplate::SetName( s.GetKey());
	}

	// NOTE: Name must be <= MAX_NAME_SIZE
	TCHAR szTmp[ MAX_ITEM_NAME_SIZE + 1 ];
	int len = strlen( pszName );
	if ( len >= MAX_ITEM_NAME_SIZE )
	{
		strcpylen( szTmp, pszName, MAX_ITEM_NAME_SIZE );
		pszName = szTmp;
	}

	// Can't be a dupe name with type ?
	LPCTSTR pszTypeName = Base_GetDef()->GetTypeName();
	if ( ! strcmpi( pszTypeName, pszName ))
		pszName = "";

	return CObjBaseTemplate::SetName( pszName );
}

bool CObjBase::MoveNearObj( const CObjBaseTemplate * pObj, int iSteps, WORD wCan )
{
	ASSERT( pObj );
	if ( pObj->IsDisconnected())	// nothing is "near" a disconnected item.
	{
		// DEBUG_CHECK(! pObj->IsDisconnected() );
		return( false );
	}

	pObj = pObj->GetTopLevelObj();
	MoveNear( pObj->GetTopPoint(), iSteps, wCan );
	return( true );
}

void CObjBase::r_WriteSafe( CScript & s )
{
	// Write an object with some fault protection.

	CGrayUID uid;
	try
	{
		uid = GetUID();
		if ( ! g_Cfg.m_fSaveGarbageCollect )
		{
			if ( g_World.FixObj( this ))
				return;
		}
		r_Write(s);
	}
	catch ( CGrayError &e )
	{
		g_Log.CatchEvent( &e, "Write Object 0%x:: ", (DWORD) uid, uid.ObjFind()->GetName() );
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent( NULL, "Write Object 0%x", (DWORD) uid );
	}
}

void CObjBase::SetTimeout( int iDelayInTicks )
{
	// Set delay in TICK_PER_SEC of a sec. -1 = never.
	if ( iDelayInTicks < 0 )
		m_timeout.Init();
	else
		m_timeout = CServTime::GetCurrentTime() + iDelayInTicks;
}

void CObjBase::Sound( SOUND_TYPE id, int iOnce ) const // Play sound effect for player
{
	// play for everyone near by.

	if ( id <= 0 )
		return;

	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( ! pClient->CanHear( this, TALKMODE_OBJ ))
			continue;
		pClient->addSound( id, this, iOnce );
	}
}

void CObjBase::Effect( EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBase * pSource, BYTE bSpeedSeconds, BYTE bLoop, bool fExplode, DWORD color, DWORD render ) const
{
	// show for everyone near by.
	//
	// bSpeedSeconds
	// bLoop
	// fExplode

	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( ! pClient->CanSee( this ))
			continue;
		pClient->addEffect( motion, id, this, pSource, bSpeedSeconds, bLoop, fExplode, color, render );
	}
}


void CObjBase::Emote( LPCTSTR pText, CClient * pClientExclude, bool fForcePossessive )
{
	// IF this is not the top level object then it might be possessive ?

	// "*You see NAME blah*" or "*You blah*"
	// fPosessive = "*You see NAME's blah*" or "*Your blah*"

	CObjBase * pObjTop = STATIC_CAST <CObjBase*>( GetTopLevelObj());
	ASSERT(pObjTop);

	TCHAR *pszThem = Str_GetTemp();
	TCHAR *pszYou = Str_GetTemp();

	if ( pObjTop->IsChar())
	{
		// Someone has this equipped.

		if ( pObjTop != this )
		{
			sprintf(pszThem, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_1 ), (LPCTSTR) pObjTop->GetName(), (LPCTSTR) GetName(), (LPCTSTR) pText);
			sprintf(pszYou, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_2 ), (LPCTSTR) GetName(), (LPCTSTR) pText);
		}
		else if ( fForcePossessive )
		{
			// ex. "You see joes poor shot ruin an arrow"
			sprintf(pszThem, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_3 ), (LPCTSTR) GetName(), (LPCTSTR) pText);
			sprintf(pszYou, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_4 ), pText);
		}
		else
		{
			sprintf(pszThem, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_5 ), (LPCTSTR) GetName(), (LPCTSTR) pText);
			sprintf(pszYou, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_6 ), pText);
		}
	}
	else
	{
		// Top level is an item. Article ?
		sprintf(pszThem, g_Cfg.GetDefaultMsg( DEFMSG_EMOTE_7 ), (LPCTSTR) GetName(), (LPCTSTR) pText);
		strcpy(pszYou, pszThem);
	}

	pObjTop->UpdateObjMessage(pszThem, pszYou, pClientExclude, HUE_RED, TALKMODE_EMOTE);
}

void CObjBase::Speak( LPCTSTR pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	g_World.Speak( this, pText, wHue, mode, font );
}

void CObjBase::SpeakUTF8( LPCTSTR pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	// convert UTF8 to UNICODE.
	NCHAR szBuffer[ MAX_TALK_BUFFER ];
	int iLen = CvtSystemToNUNICODE( szBuffer, COUNTOF(szBuffer), pText, -1 );
	g_World.SpeakUNICODE( this, szBuffer, wHue, mode, font, lang );
}
void CObjBase::SpeakUTF8Ex( const NWORD * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	g_World.SpeakUNICODE( this, pText, wHue, mode, font, lang );
}
void CObjBase::MoveNear( CPointMap pt, int iSteps, WORD wCan )
{
	// Move to nearby this other object.
	// Actually move it within +/- iSteps

	DIR_TYPE dir = (DIR_TYPE) Calc_GetRandVal( DIR_QTY );
	for (; iSteps > 0 ; iSteps-- )
	{
		// Move to the right or left?
		CPointBase pTest = pt;	// Save this so we can go back to it if we hit a blocking object.
		pt.Move( dir );
		dir = GetDirTurn( dir, Calc_GetRandVal(3)-1 );	// stagger ?
		// Put the item at the correct Z point
		WORD wBlockRet = wCan;
		if ( IsSetEF( EF_WalkCheck ) )
			pt.m_z = g_World.GetHeightPoint_New( pt, wBlockRet, true );
		else
			pt.m_z = g_World.GetHeightPoint( pt, wBlockRet, true );
		if ( wBlockRet &~ wCan )
		{
			// Hit a block, so go back to the previous valid position
			pt = pTest;
			break;	// stopped
		}
	}

	MoveTo( pt );
}

void CObjBase::UpdateObjMessage( LPCTSTR pTextThem, LPCTSTR pTextYou, CClient * pClientExclude, HUE_TYPE wHue, TALKMODE_TYPE mode ) const
{
	// Show everyone a msg coming form this object.

	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( pClient == pClientExclude )
			continue;
		if ( ! pClient->CanSee( this ))
			continue;

		pClient->addBarkParse(( pClient->GetChar() == this )? pTextYou : pTextThem, this, wHue, mode, FONT_NORMAL );
	}
}

void CObjBase::UpdateCanSee( const CCommand * pCmd, int iLen, CClient * pClientExclude ) const
{
	// Send this update message to everyone who can see this.
	// NOTE: Need not be a top level object. CanSee() will calc that.
	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( pClient == pClientExclude )
			continue;
		if ( ! pClient->CanSee( this ))
			continue;
		pClient->xSendPkt( pCmd, iLen );
	}
}

TRIGRET_TYPE CObjBase::OnHearTrigger( CResourceLock & s, LPCTSTR pszCmd, CChar * pSrc, TALKMODE_TYPE & mode )
{
	// Check all the keys in this script section.
	// look for pattern or partial trigger matches.
	// RETURN:
	//  TRIGRET_ENDIF = no match.
	//  TRIGRET_DEFAULT = found match but it had no RETURN
	CScriptTriggerArgs Args( pszCmd );
	Args.m_iN1	= mode;

	bool fMatch = false;

	while ( s.ReadKeyParse())
	{
		if ( s.IsKeyHead("ON",2))
		{
			// Look for some key word.
			_strupr( s.GetArgStr());
			if ( Str_Match( s.GetArgStr(), pszCmd ) == MATCH_VALID )
				fMatch = true;
			continue;
		}

		if ( ! fMatch )
			continue;	// look for the next "ON" section.

		TRIGRET_TYPE iRet = CObjBase::OnTriggerRun( s, TRIGRUN_SECTION_EXEC, pSrc, &Args );
		if ( iRet != TRIGRET_RET_FALSE )
			return( iRet );

		fMatch = false;
	}

	mode	= (TALKMODE_TYPE) Args.m_iN1;
	return( TRIGRET_ENDIF );	// continue looking.
}

bool CObjBase::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	if ( ! strnicmp( pszKey, "SECTOR.", 7 ))
	{
		pszKey += 7;
		pRef = GetTopLevelObj()->GetTopSector();
		return( true );
	}
	if ( ! strnicmp( pszKey, "TYPEDEF.", 8 ))
	{
		pszKey += 8;
		pRef = Base_GetDef();
		return( true );
	}
	if ( ! strnicmp( pszKey, "TOPOBJ.", 7 ))
	{
		pszKey += 7;
		pRef = dynamic_cast <CObjBase*>( GetTopLevelObj());
		return( true );
	}
	return( CScriptObj::r_GetRef( pszKey, pRef ));
}

enum OBC_TYPE
{
	OC_CANSEE,
	OC_CANSEELOS,
	OC_COLOR,
	OC_COMPLEXITY,
	OC_DISTANCE,
	OC_EVENTS,
	OC_ISCHAR,
	OC_ISEVENT,
	OC_ISITEM,
	OC_ISNEARTYPE,
	OC_MAP,
	OC_MODAC,
	OC_MODAR,
	OC_NAME,
	OC_P,
	OC_SERIAL,
	OC_TAG,
	OC_TAG0,
	OC_TAGAT,
	OC_TAGCOUNT,
	OC_TIMER,
	OC_TIMERD,
	OC_UID,
	OC_VERSION,
	OC_WEIGHT,
	OC_Z,
	OC_QTY,
};

LPCTSTR const CObjBase::sm_szLoadKeys[OC_QTY+1] =
{
	"CanSee",
	"CanSeeLOS",
	"COLOR",
	"COMPLEXITY",
	"Distance",	// distance to the source.
	"EVENTS",
	"IsChar",
	"ISEVENT",
	"IsItem",
	"IsNearType",
	"MAP",
	"MODAC",
	"MODAR",
	"NAME",
	"P",
	"SERIAL",	// same as UID
	"TAG",
	"TAG0",
	"TAGAT",
	"TAGCOUNT",
 	"TIMER",
	"TIMERD",
	"UID",
	"VERSION",
	"Weight",
	"Z",
	NULL,
};

void CObjBase::r_DumpLoadKeys( CTextConsole * pSrc )
{
	r_DumpKeys(pSrc,sm_szLoadKeys);
	CScriptObj::r_DumpLoadKeys(pSrc);
}



bool CObjBase::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	EXC_TRY(("r_WriteVal('%s',,%x)", pszKey, pSrc));
	LPCSTR	pszArgs	= NULL;
	DEBUG_CHECK(pSrc);

	int index = FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	if ( index < 0 )
	{
		// RES_FUNCTION call
		// Is it a function returning a value ? Parse args ?
		if ( ( pszArgs = strchr( pszKey, ' ' ) ) )
		{
			pszArgs++;
			SKIP_SEPERATORS(pszArgs);
		}

		CScriptTriggerArgs Args( pszArgs ? pszArgs : "" );
		if ( r_Call( pszKey, pSrc, &Args, &sVal ) )
			return true;

		// Just try to default to something reasonable ?
		// Even though we have not really specified it correctly !
/*
		// SECTOR. ?
		if ( ! IsDisconnected() && GetTopLevelObj()->GetTopPoint().IsValidPoint() )
		{

			if ( GetTopLevelObj()->GetTopSector()->r_WriteVal( pszKey, sVal, pSrc ))
				return( true );
		}

*/

		// WORLD. ?
		if ( g_World.r_WriteVal( pszKey, sVal, pSrc ))
			return( true );


		// TYPEDEF. ?
		if ( Base_GetDef()->r_WriteVal( pszKey, sVal, pSrc ))
			return( true );

		return(	CScriptObj::r_WriteVal( pszKey, sVal, pSrc ));
	}

	bool	fZero	= false;
	switch (index)
	{
	case OC_CANSEE:
		{
			CChar * pChar = pSrc->GetChar();
			if ( pChar == NULL )
				return( false );
			sVal.FormatVal( pChar->CanSee( this ));
		}
		break;
	case OC_CANSEELOS:
		{
			CChar * pChar = pSrc->GetChar();
			if ( pChar == NULL )
				return( false );
			sVal.FormatVal( pChar->CanSeeLOS( this ));
		}
		break;
	case OC_COLOR:
		sVal.FormatHex( GetHue());
		break;
	case OC_COMPLEXITY:
		{
		if ( IsDisconnected() || !GetTopLevelObj()->GetTopPoint().IsValidPoint() )
			return false;
		return GetTopLevelObj()->GetTopSector()->r_WriteVal( pszKey, sVal, pSrc );
		}
	case OC_DISTANCE:
		{
			pszKey	+= 8;
			SKIP_SEPERATORS( pszKey ); GETNONWHITESPACE( pszKey );
			CObjBase *	pObj	= pSrc->GetChar();

			CObjBase	* pThis	= this;
			if ( !IsTopLevel() )
				pThis	= dynamic_cast <CObjBase*>( GetTopLevelObj() );
			if ( !pThis )
				return false;

			if ( *pszKey )
			{
				CPointMap	pt	= g_Cfg.GetRegionPoint( pszKey );

				if ( pt.IsValidPoint() )
				{
					if ( !pThis->GetTopPoint().IsValidPoint() )
						return false;
					else
						sVal.FormatVal( pThis->GetTopDist( pt ) );
					return true;
				}

				CGrayUID	uid			= Exp_GetVal( pszKey );
				SKIP_SEPERATORS( pszKey ); GETNONWHITESPACE( pszKey );
				pObj	= uid.ObjFind();
			}

			if ( pObj && !pObj->IsTopLevel() )
					pObj	= dynamic_cast <CObjBase*>( pObj->GetTopLevelObj() );
			if ( !pObj )
				return false;
			sVal.FormatVal( pThis->GetDist( pObj ) );
		}
		break;
	case OC_EVENTS:
		m_OEvents.WriteResourceRefList( sVal );
		break;
	case OC_ISCHAR:
		sVal.FormatVal( IsChar());
		break;
	case OC_ISEVENT:
		if ( pszKey[7] != '.' )
			return( false );
		pszKey += 8;
		sVal = ( m_OEvents.FindResourceName(RES_EVENTS, pszKey) >= 0 ) ? "1" : "0";
		return true;
	case OC_ISITEM:
		sVal.FormatVal( IsItem());
		break;
	case OC_ISNEARTYPE:
		{
			bool	fP	= false;
			pszKey	+= 10;
			if ( !strnicmp( pszKey, ".P", 2 ) )
			{
				fP	= true;
				pszKey	+=2;
			}
			SKIP_SEPERATORS( pszKey );

			if ( !GetTopPoint().IsValidPoint() )
				sVal.FormatVal( 0 );
			else
			{
				int		iType		= Exp_GetVal( pszKey );
				int		iDistance;
				SKIP_SEPERATORS( pszKey );
				if ( !*pszKey )
					iDistance	= 0;
				else
					iDistance	= Exp_GetVal( pszKey );
				if ( fP )
				{
					CPointMap	pt	= g_World.FindItemTypeNearby(
						GetTopPoint(), (IT_TYPE) GETINTRESOURCE( iType ), iDistance );
					if ( !pt.IsValidPoint() )
						sVal.FormatVal( 0 );
					else
						sVal	= pt.WriteUsed();
				}
				else
					sVal.FormatVal( g_World.IsItemTypeNear(
						GetTopPoint(),
						(IT_TYPE) GETINTRESOURCE( iType ),
						iDistance ) );

					// (IT_TYPE) g_Cfg.ResourceGetIndexType( RES_TYPEDEF, s.GetArgStr() )
			}
			return true;
		}
		break;

	case OC_MAP:
		sVal.FormatVal( GetUnkPoint().m_map);
		break;
	case OC_MODAR:
	case OC_MODAC:
		sVal.FormatVal( m_ModAr );
		break;
	case OC_NAME:
		sVal = GetName();
		break;
	case OC_P:
		if ( pszKey[1] == '.' )
		{
			return( GetUnkPoint().r_WriteVal( pszKey+2, sVal ));
		}
		sVal = GetUnkPoint().WriteUsed();
		break;
	case OC_TAG0:
		fZero	= true;
		pszKey++;
	case OC_TAG:			// "TAG" = get/set a local tag.
		{
			if ( pszKey[3] != '.' )
				return( false );
			pszKey += 4;

			CVarDefBase *	pVarKey	= m_TagDefs.GetKey( pszKey );
			if ( !pVarKey )
				sVal	= Base_GetDef()->m_TagDefs.GetKeyStr( pszKey, fZero );
			else
				sVal = pVarKey->GetValStr();
		}
		return( true );
	case OC_TIMER:
		sVal.FormatVal( GetTimerAdjusted());
		break;
	case OC_TIMERD:
		sVal.FormatVal( GetTimerDAdjusted() );
		break;
	case OC_UID:
		if ( pszKey[3] == '.' )
			return(	CScriptObj::r_WriteVal( pszKey, sVal, pSrc ) );
	case OC_SERIAL:
		sVal.FormatHex( GetUID());
		break;
	case OC_VERSION:
		sVal = g_Serv.m_sServVersion;
		break;
	case OC_WEIGHT:
		sVal.FormatVal( GetWeight());
		break;
	case OC_Z:
		sVal.FormatVal( GetUnkZ());
		break;
	case OC_TAGAT:
		{
 			pszKey += 5;	// eat the 'TAGAT'
 			if ( *pszKey == '.' )	// do we have an argument?
 			{
 				SKIP_SEPERATORS( pszKey );
 				int iQty = Exp_GetVal( pszKey );
				if ( iQty < 0 || iQty >= m_TagDefs.GetCount() )
 					return( false );	// tryig to get non-existant tag

 				CVarDefBase * pTagAt = m_TagDefs.GetAt( iQty );
 				if ( !pTagAt )
 					return( false );	// tryig to get non-existant tag

 				SKIP_SEPERATORS( pszKey );
 				if ( ! *pszKey )
 				{
 					sVal.Format( "%s=%s", (LPCTSTR) pTagAt->GetKey(), (LPCTSTR) pTagAt->GetValStr() );
 					return( true );
 				}
 				if ( strnicmp( pszKey, "KEY", 3 ))	// key?
 				{
 					sVal = (LPCTSTR) pTagAt->GetKey();
 					return( true );
 				}
 				if ( strnicmp( pszKey, "VAL", 3 ))	// val?
 				{
 					sVal = pTagAt->GetValStr();
 					return( true );
 				}
 			}

		return( false );
		}
		break;
	case OC_TAGCOUNT:
		sVal.FormatVal( m_TagDefs.GetCount() );
		break;
	default:
		DEBUG_CHECK(0);
		return( false );
	}
	return true;
	EXC_CATCH("CObjBase");
	return false;
}

bool CObjBase::r_LoadVal( CScript & s )
{
	LOCKDATA;
	// load the basic stuff.
	EXC_TRY(("r_LoadVal('%s %s')", s.GetKey(), s.GetArgStr()));
	// we're using FindTableSorted so we must do this here.
	// Using FindTableHeadSorted instead would result in keywords
	// starting with "P" not working, for instance :)

	if ( s.IsKeyHead( "TAG.", 4 ))
	{
		bool fQuoted = false;
		m_TagDefs.SetStr( s.GetKey()+4, fQuoted, s.GetArgStr( &fQuoted ), false );
		return( true );
	}
	if ( s.IsKeyHead( "TAG0.", 5 ))
	{
		bool fQuoted = false;
		m_TagDefs.SetStr( s.GetKey()+5, fQuoted, s.GetArgStr( &fQuoted ), true );
		return( true );
	}

	int index = FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	if ( index < 0 )
	{
		return( CScriptObj::r_LoadVal(s));
	}

	switch ( index )
	{
	case OC_COLOR:
		if ( ! strcmpi( s.GetArgStr(), "match_shirt" ) ||
			! strcmpi( s.GetArgStr(), "match_hair" ))
		{
			CChar * pChar = dynamic_cast <CChar*>(GetTopLevelObj());
			if ( pChar )
			{
				CItem * pHair = pChar->LayerFind( ! strcmpi( s.GetArgStr()+6, "shirt" ) ? LAYER_SHIRT : LAYER_HAIR );
				if ( pHair )
				{
					m_wHue = pHair->GetHue();
					break;
				}
			}
			m_wHue = HUE_GRAY;
			break;
		}
		RemoveFromView();
		m_wHue = (HUE_TYPE) s.GetArgVal();
		Update();
		break;
	case OC_EVENTS:
		return( m_OEvents.r_LoadVal( s, RES_EVENTS ));
	case OC_MAP:
		// Move to another map
		if ( ! IsTopLevel())
			return( false );
		{
			CPointMap pt = GetTopPoint();
			pt.m_map = s.GetArgVal();

			//	is the desired mapplane allowed?
			if ( !g_MapList.m_maps[pt.m_map] )
				return false;

			MoveTo(pt);
		}
		break;
	case OC_MODAR:
	case OC_MODAC:
		{
			m_ModAr = s.GetArgVal();
			CChar * pChar = dynamic_cast <CChar*>(GetTopLevelObj());
			if ( pChar && pChar->IsChar() )
			{
				pChar->m_defense = pChar->CalcArmorDefense();
				pChar->UpdateStatsFlag();
			}
		}
		break;
	case OC_NAME:
		SetName( s.GetArgStr());
		break;
	case OC_P:	// Must set the point via the CItem or CChar methods.
		return( false );
	case OC_TIMER:
		SetTimeout( s.GetArgVal() * TICK_PER_SEC );
		break;
	case OC_TIMERD:
		SetTimeout( s.GetArgVal());
		break;
	case OC_UID:
	case OC_SERIAL:
		DEBUG_CHECK( IsDisconnected());
		DEBUG_CHECK( g_Serv.IsLoading());
		DEBUG_CHECK( ! IsValidUID());
		// Don't set container flags through this.
		SetUID( s.GetArgVal(), (dynamic_cast <CItem*>(this)) ? true : false );
		break;
	default:
		DEBUG_CHECK(0);
		return( false );
	}
	return true;
	EXC_CATCH("CObjBase");
	return false;
}

void CObjBase::r_Write( CScript & s )
{
	s.WriteKeyHex( "SERIAL", GetUID());
	if ( IsIndividualName())
		s.WriteKey( "NAME", GetIndividualName());
	if ( m_wHue != HUE_DEFAULT )
		s.WriteKeyHex( "COLOR", GetHue());
	if ( m_timeout.IsTimeValid() )
		s.WriteKeyVal( "TIMER", GetTimerAdjusted());
	m_TagDefs.r_WriteTags( s );
	m_OEvents.r_Write( s, "EVENTS" );
	if ( m_ModAr ) s.WriteKeyVal( "MODAR", m_ModAr );
}

enum OV_TYPE
{
	OV_DAMAGE,
	OV_DCLICK,
	OV_DIALOG,
	OV_DIALOGCLOSE,
	OV_EDIT,
	OV_EFFECT,
	OV_EMOTE,
	OV_FIX,
	OV_FIXWEIGHT,
	OV_FLIP,
	OV_FOLLOW,
	OV_INFO,
	OV_INPDLG,
	OV_MENU,
	OV_MESSAGE,
	OV_MESSAGEUA,
	OV_MOVE,
	OV_MOVENEAR,
	OV_MOVETO,
	OV_MSG,
	OV_NEWDUPE,
	OV_NEWITEM,
	OV_NEWNPC,
	OV_NUDGEDOWN,
	OV_NUDGEUP,
	OV_P,	// protect this from use.
	OV_PROMPTCONSOLE,
	OV_REMOVE,
	OV_REMOVEFROMVIEW,
	OV_SAY,
	OV_SAYU,
	OV_SAYUA,
	OV_SDIALOG,
	OV_SOUND,
	OV_SPELLEFFECT,
	OV_TAGLIST,
	OV_TARGET,
	OV_TRY,
	OV_TRYP,
	OV_TRYSRC,
	OV_UID,
	OV_UPDATE,
	OV_UPDATEX,
	OV_USEITEM,
	OV_Z,
	OV_QTY,
};

LPCTSTR const CObjBase::sm_szVerbKeys[OV_QTY+1] =
{
	"DAMAGE",
	"DCLICK",
	"DIALOG",
	"DIALOGCLOSE",
	"EDIT",
	"EFFECT",
	"EMOTE",
	"FIX",
	"FIXWEIGHT",
	"FLIP",
	"FOLLOW",
	"INFO",
	"INPDLG",
	"MENU",
	"MESSAGE",
	"MESSAGEUA",
	"MOVE",
	"MOVENEAR",
	"MOVETO",
	"MSG",
	"NEWDUPE",
	"NEWITEM",
	"NEWNPC",
	"NUDGEDOWN",
	"NUDGEUP",
	"P",	// protect this from use.
	"PROMPTCONSOLE",
	"REMOVE",
	"REMOVEFROMVIEW",
	"SAY",
	"SAYU",
	"SAYUA",
	"SDIALOG",
	"SOUND",
	"SPELLEFFECT",
	"TAGLIST",
	"TARGET",
	"TRY",
	"TRYP",
	"TRYSRC",
	"UID",
	"UPDATE",
	"UPDATEX",
	"USEITEM",
	"Z",
	NULL,
};

void CObjBase::r_DumpVerbKeys( CTextConsole * pSrc )
{
	r_DumpKeys(pSrc,sm_szVerbKeys);
	CScriptObj::r_DumpVerbKeys(pSrc);
}

bool CObjBase::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	LOCKDATA;
	EXC_TRY(("r_Verb('%s %s',%x)", s.GetKey(), s.GetArgStr(), pSrc));
	ASSERT(pSrc);
	int	index;
	LPCTSTR	pszKey		= s.GetKey();

	if ( !strnicmp(pszKey, "CLEARTAGS", 9) )
	{
		pszKey = s.GetArgStr();
		SKIP_SEPERATORS(pszKey);
		m_TagDefs.ClearKeys(pszKey);
		return true;
	}

	CScriptTriggerArgs Args( s.GetArgRaw() );
	if ( r_Call( pszKey, pSrc, &Args ) )
		return true;

	if ( !strnicmp( pszKey, "SERV.NEWDUPE", 12 ))
		index	= OV_NEWDUPE;
	else if ( !strnicmp( pszKey, "SERV.NEWITEM", 12 ))
		index	= OV_NEWITEM;
	else if ( !strnicmp( pszKey, "SERV.NEWNPC", 11 ))
		index	= OV_NEWNPC;
	else if ( !strnicmp( pszKey, "TARGET", 6 ) )
		index	= OV_TARGET;
	else
		index = FindTableSorted( pszKey, sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 );

	if ( index < 0 )
	{
		// Not found.
		return( CScriptObj::r_Verb( s, pSrc ));
	}

	CChar * pCharSrc = pSrc->GetChar();
	CClient * pClientSrc = (pCharSrc && pCharSrc->IsClient()) ? (pCharSrc->GetClient()) : NULL ;

	switch (index)
	{
		case OV_DAMAGE:	//	"Amount,SourceFlags,SourceCharUid" = do me some damage.
			{
				int piCmd[3];
				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd));
				if ( iArgQty < 1 )
					return( false );
				if ( iArgQty > 2 )	// Give it a new source char UID
				{
					CObjBaseTemplate * pObj = CGrayUID( piCmd[2] ).ObjFind();
					if ( pObj )
					{
						pObj = pObj->GetTopLevelObj();
					}
					pCharSrc = dynamic_cast <CChar*>(pObj);
				}
				OnTakeDamage( piCmd[0],
					pCharSrc,
					( iArgQty > 1 ) ? piCmd[1] : ( DAMAGE_HIT_BLUNT | DAMAGE_GENERAL ));
			}
			break;

		case OV_EDIT:
			{
				// Put up a list of items in the container. (if it is a container)
				if ( pClientSrc == NULL )
					return( false );
				pClientSrc->m_Targ_Text = s.GetArgStr();
				pClientSrc->Cmd_EditItem( this, -1 );
			}
			break;
		case OV_EFFECT: // some visual effect.
			{
				int piCmd[7];
				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd));
				if ( iArgQty < 2 )
					return( false );
				CObjBase *	pThis	= this;
				if ( piCmd[0] == -1 )
				{
					if ( pCharSrc )
					{
						piCmd[0]	= EFFECT_BOLT;
						pThis		= pCharSrc;
						pCharSrc	= dynamic_cast <CChar*>(this);
					}

				}
				pThis->Effect( (EFFECT_TYPE) piCmd[0], (ITEMID_TYPE) RES_GET_INDEX(piCmd[1]),
					pCharSrc,
					(iArgQty>=3)? piCmd[2] : 5,		// BYTE bSpeedSeconds = 5,
					(iArgQty>=4)? piCmd[3] : 1,		// BYTE bLoop = 1,
					(iArgQty>=5)? piCmd[4] : false,	// bool fExplode = false
					(iArgQty>=6)? piCmd[5] : 0,		// hue
					(iArgQty>=7)? piCmd[6] : 0		// render mode
					);
			}
			break;
		case OV_EMOTE:
			Emote( s.GetArgStr());
			break;
		case OV_FIXWEIGHT:
			//FixWeight();
			break;
		case OV_FLIP:
			Flip( s.GetArgStr());
			break;
		case OV_FOLLOW:
			// Follow this item.
			if ( pCharSrc )
			{
				// I want to follow this person about.
				pCharSrc->Memory_AddObjTypes( this, MEMORY_FOLLOW );
			}
			break;
		case OV_INPDLG:
			// "INPDLG" verb maxchars
			// else assume it was a property button.
			{
				if ( pClientSrc == NULL )
					return( false );

				TCHAR *Arg_ppCmd[2];		// Maximum parameters in one line
				int iQty = Str_ParseCmds( s.GetArgStr(), Arg_ppCmd, COUNTOF( Arg_ppCmd ));

				CGString sOrgValue;
				if ( ! r_WriteVal( Arg_ppCmd[0], sOrgValue, pSrc ))
					sOrgValue = ".";

				pClientSrc->m_Targ_Text = Arg_ppCmd[0];	// The attribute we want to edit.

				int iMaxLength = iQty > 1 ? atoi(Arg_ppCmd[1]) : 1;

				CGString sPrompt;
				sPrompt.Format( "%s (# = default)", (LPCTSTR) Arg_ppCmd[0] );
				pClientSrc->addGumpInpVal( true, INPVAL_STYLE_TEXTEDIT,
					iMaxLength,	sPrompt, sOrgValue, this );
			}
			break;
		case OV_MENU:
			{
				if ( pClientSrc == NULL )
					return( false );
				pClientSrc->Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, s.GetArgStr()), this );
			}
			break;
/*			pThis->Effect( (EFFECT_TYPE) piCmd[0], (ITEMID_TYPE) RES_GET_INDEX(piCmd[1]),
				pCharSrc,
				(iArgQty>=3)? piCmd[2] : 5,		// BYTE bSpeedSeconds = 5,
				(iArgQty>=4)? piCmd[3] : 1,		// BYTE bLoop = 1,
				(iArgQty>=5)? piCmd[4] : false,	// bool fExplode = false
				(iArgQty>=6)? piCmd[5] : 0,		// hue
				(iArgQty>=7)? piCmd[6] : 0		// render mode
				);
		}
*/			break;
		case OV_MESSAGE:	//put info message (for pSrc client only) over item.
		case OV_MSG:
			{
				if ( pCharSrc == NULL )
					UpdateObjMessage(s.GetArgStr(), s.GetArgStr(), NULL, HUE_TEXT_DEF, IsChar() ? TALKMODE_EMOTE : TALKMODE_ITEM);
				else
					pCharSrc->ObjMessage(s.GetArgStr(), this);
			}
			break;
		case OV_MESSAGEUA:
			{
				if ( pClientSrc == NULL )
					return( false );

				TCHAR * pszArgs[5];
				NCHAR ncBuffer[ MAX_TALK_BUFFER ];

				int iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, 5 );
				if ( iArgQty < 4 )
					break;

				CvtSystemToNUNICODE( ncBuffer, COUNTOF( ncBuffer ), pszArgs[4], -1 );
				pClientSrc->addBarkUNICODE( ncBuffer, this,
					(HUE_TYPE) ( pszArgs[0][0] ? Exp_GetVal(pszArgs[0]) : HUE_TEXT_DEF ),
					(TALKMODE_TYPE) ( pszArgs[1][0] ? Exp_GetVal(pszArgs[1]) : TALKMODE_SAY ),
					(FONT_TYPE) ( pszArgs[2][0] ? Exp_GetVal(pszArgs[2]) : FONT_NORMAL ),
					CLanguageID(pszArgs[3]));
				break;
			}
		case OV_MOVE:
			// move without restriction. east,west,etc. (?up,down,)
			if ( IsTopLevel())
			{
				CPointMap pt = GetTopPoint();
				if ( ! GetDeltaStr( pt, s.GetArgStr()))
					return( false );
				MoveTo( pt );
				Update();
			}
			break;
		case OV_MOVENEAR:
			{
				CObjBase *	pObjNear;
				int piCmd[4];

				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd) );
				if ( iArgQty == 0 )
					return false;
				if ( iArgQty < 3 )
					piCmd[2]	= 1;
				if ( iArgQty < 2 )
					piCmd[1]	= 1;

				CGrayUID	uid	= piCmd[0];
				pObjNear	= uid.ObjFind();
				if ( !pObjNear )
					return false;
				MoveNearObj( pObjNear, piCmd[1] );
				if ( piCmd[2] )
					Update();
			}
			break;
		case OV_NEWDUPE:	// uid
			{
				CGrayUID uid( s.GetArgVal());
				CObjBase * pObj = uid.ObjFind();
				g_World.m_uidNew = uid;	// for last target stuff. (trigger stuff)
				CScript script( "DUPE" );
				return pObj->r_Verb( script, pSrc );
			}
		case OV_NEWITEM:	// just add an item but don't put it anyplace yet..
			{
				TCHAR	*ppCmd[3];
				int iQty = Str_ParseCmds(s.GetArgRaw(), ppCmd, COUNTOF(ppCmd), ",");
				if ( iQty == 0 ) return false;

				CItem * pItem = CItem::CreateHeader(ppCmd[0], NULL, false, pSrc->GetChar());
				if ( !pItem )
				{
					g_World.m_uidNew = (DWORD)0;
					return false;
				}
				if ( ppCmd[1] ) pItem->SetAmount(Exp_GetVal(ppCmd[1]));
				if ( ppCmd[2] ) pItem->LoadSetContainer(Exp_GetVal(ppCmd[2]), (LAYER_TYPE)GetUnkZ());
				g_World.m_uidNew = pItem->GetUID();	// for last target stuff. (trigger stuff)
			}
			break;
		case OV_NEWNPC:
			{
				CREID_TYPE id = (CREID_TYPE) g_Cfg.ResourceGetIndexType( RES_CHARDEF, s.GetArgRaw());
				CChar * pChar = CChar::CreateNPC( id );
				if ( !pChar )
				{
					g_World.m_uidNew = (DWORD)0;
					return false;
				}
				pChar->MoveNearObj( this, 1 );
				pChar->Update();
				g_World.m_uidNew = pChar->GetUID();	// for last target stuff. (trigger stuff)
			}
			break;
		case OV_NUDGEDOWN:
			if ( IsTopLevel())
			{
				int zdiff = s.GetArgVal();
				SetTopZ( GetTopZ() - ( zdiff ? zdiff : 1 ));
				Update();
			}
			break;
		case OV_NUDGEUP:
			if ( IsTopLevel())
			{
				int zdiff = s.GetArgVal();
				SetTopZ( GetTopZ() + ( zdiff ? zdiff : 1 ));
				Update();
			}
			break;
		case OV_MOVETO:
		case OV_P:
			MoveTo( g_Cfg.GetRegionPoint( s.GetArgStr()));
			break;
		case OV_PROMPTCONSOLE:
			{
				if ( pClientSrc == NULL )
					return( false );

				TCHAR * pszArgs[2];

				int iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, 2 );
				if ( iArgQty == 0 )
					break;
					
				pClientSrc->addPromptConsoleFunction( pszArgs[0], pszArgs[1] );
			}
			break;
		case OV_INFO:
			if ( ! pClientSrc )
				return( false );
			return pClientSrc->addGumpDialogProps( GetUID());

		case OV_REMOVE:	//remove this object now.
			Delete();
			return( true );
		case OV_REMOVEFROMVIEW:
			RemoveFromView();	// remove this item from all clients.
			return( true );
		case OV_SAY: //speak so everyone can here
			Speak( s.GetArgStr());
			break;

		case OV_SAYU:
			// Speak in unicode from the UTF8 system format.
			SpeakUTF8( s.GetArgStr(), HUE_TEXT_DEF, TALKMODE_SAY, FONT_NORMAL );
			break;

		case OV_SAYUA:
			// This can have full args. SAYUA Color, Mode, Font, Lang, Text Text
			{
				TCHAR * pszArgs[5];
				int iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, 5 );
				if ( iArgQty < 4 )
					break;

				SpeakUTF8( pszArgs[4],
					(HUE_TYPE) ( pszArgs[0][0] ? Exp_GetVal(pszArgs[0]) : HUE_TEXT_DEF ),
					(TALKMODE_TYPE) ( pszArgs[1][0] ? Exp_GetVal(pszArgs[1]) : TALKMODE_SAY ),
					(FONT_TYPE) ( pszArgs[2][0] ? Exp_GetVal(pszArgs[2]) : FONT_NORMAL ),
					CLanguageID(pszArgs[3]));
			}
			break;

		case OV_SOUND:
			{
				int piCmd[2];
				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd));
				Sound( piCmd[0], ( iArgQty > 1 ) ? piCmd[1] : 1 );
			}
			break;
		case OV_SPELLEFFECT:	// spell, strength, noresist
			{
				int piCmd[4];
				int iArgs = Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd));
				CItem		* pItemSrc	= NULL;
				switch( iArgs )
				{
				case 4:
					{
						CGrayUID	uid	= (DWORD) piCmd[3];
						pItemSrc	= uid.ItemFind();
					}
				case 3:
					if ( piCmd[2] == 1 )
					{
						pCharSrc = dynamic_cast <CChar*> (this);
					}
					else
					{
						CGrayUID	uid	= (DWORD) piCmd[2];
						pCharSrc	= uid.CharFind();
					}
					break;
				default:
					break;
				}
				OnSpellEffect( (SPELL_TYPE) RES_GET_INDEX( piCmd[0] ), pCharSrc, piCmd[1], pItemSrc );
			}
			break;
		case OV_TAGLIST:
			m_TagDefs.DumpKeys(pSrc, "TAG.");
			break;
		case OV_TARGET:
			{
				if ( pClientSrc == NULL )
					return( false );
				pszKey	+= 6;
				bool		fGround		= false;
				bool		fCrim		= false;
				bool		fFunction	= false;
				char		low = tolower(*pszKey);

				while (( low >= 'a' ) && ( low <= 'z' ))
				{
					if ( low == 'f' )
						fFunction = true;
					else if ( low == 'g' )
						fGround = true;
					else if ( low == 'w' )
						fCrim = true;

					low = tolower(*(++pszKey));
				}

				pClientSrc->m_Targ_UID = GetUID();
				pClientSrc->m_tmUseItem.m_pParent = GetParent();	// Cheat Verify

				if ( fFunction )
					pClientSrc->addTargetFunction( s.GetArgStr(), fGround, fCrim );
				else
					pClientSrc->addTarget( CLIMODE_TARG_USE_ITEM, s.GetArgStr(), fGround, fCrim );
			}
			break;
		case OV_DIALOG:
		case OV_SDIALOG:
			{
				if ( pClientSrc == NULL )
					return( false );

				TCHAR *	Arg_ppCmd[3];		// Maximum parameters in one line
				int iQty = Str_ParseCmds( s.GetArgStr(), Arg_ppCmd, COUNTOF( Arg_ppCmd ));

				if ( iQty < 1 )
					return( false );

				if ( index == OV_SDIALOG )
				{
					bool cancontinue = true;
					int context = GETINTRESOURCE( (DWORD) g_Cfg.ResourceGetIDType( RES_DIALOG, Arg_ppCmd[0] ) );

					if ( pClientSrc->GetChar()->Memory_FindTypes(MEMORY_GUMPRECORD) != NULL )
					{
						CItem * pCheckItem = pClientSrc->GetChar()->GetContentHead();

						for ( ; pCheckItem!=NULL; pCheckItem=pCheckItem->GetNext())
						{
							if ( ! pCheckItem->IsMemoryTypes(MEMORY_GUMPRECORD))
								continue;

							CItemMemory * pCheckMemory = dynamic_cast <CItemMemory *>( pCheckItem );

							if ( pCheckMemory )
							{
								if ( pCheckMemory->m_itNormal.m_more1 == context )
								{
									// We cannot call this gump
									cancontinue = false;
									break;
								}
							}
						}
					}

					if ( !cancontinue )
						return( false );	
				}

				pClientSrc->Dialog_Setup( CLIMODE_DIALOG, g_Cfg.ResourceGetIDType( RES_DIALOG, Arg_ppCmd[0] ),
					iQty > 1 ? Exp_GetVal( Arg_ppCmd[1]) : 0, this, Arg_ppCmd[2] );
			}
			break;
		case OV_DIALOGCLOSE:
			{
				if ( pClientSrc == NULL )
					return( false );
				TCHAR *	Arg_ppCmd[2];		// Maximum parameters in one line
				int iQty = Str_ParseCmds( s.GetArgStr(), Arg_ppCmd, COUNTOF( Arg_ppCmd ));

				if ( iQty < 1 )
					return false;

				pClientSrc->Dialog_Close( this,
					g_Cfg.ResourceGetIDType( RES_DIALOG, Arg_ppCmd[0] ),
					iQty > 1 ? Exp_GetVal( Arg_ppCmd[1]) : 0 );
			}
			break;
		case OV_TRYP:
			{
				int iMinPriv = s.GetArgVal();

				if ( iMinPriv >= PLEVEL_QTY )
				{
					pSrc->SysMessagef("The %s property can't be changed.", (LPCTSTR) s.GetArgStr());
					return false;
				}

				if ( pSrc->GetPrivLevel() < iMinPriv )
				{
					pSrc->SysMessagef( "You lack the privilege to change the %s property.", (LPCTSTR) s.GetArgStr());
					return false;
				}

				// do this verb only if we can touch it.
				if ( pSrc->GetPrivLevel() <= PLEVEL_Counsel )
				{
					if ( pCharSrc == NULL || !pCharSrc->CanTouch(this) )
					{
						pSrc->SysMessagef("Can't touch %s object %s", (LPCTSTR)s.GetArgStr(), (LPCTSTR)GetName());
						return false;
					}
				}
			}
			// no break here, TRYP only does extra checks
		case OV_TRY:
			{
				LPCTSTR pszVerb = s.GetArgStr();
				CScript script( pszVerb );
				if ( !r_Verb(script, pSrc) )
				{
					DEBUG_ERR(( "Can't try %s object %s (0%x)\n", (LPCTSTR) pszVerb, (LPCTSTR) GetName(), (DWORD)GetUID()));
					return( false );
				}
			}
			return( true );
		case OV_TRYSRC:
			{
				CGrayUID NewSrc = s.GetArgVal();
				LPCTSTR pszVerb = s.GetArgStr();

				CTextConsole * pNewSrc = NewSrc.CharFind();

				if ((!NewSrc.IsValidUID()) || (!pNewSrc))
				{
					DEBUG_ERR(( "Can't trysrc %s object %s (0%x): invalid src uid 0%x\n", (LPCTSTR)pszVerb, (LPCTSTR) GetName(), (DWORD)GetUID(), NewSrc.GetObjUID()));
					return false;
				}

				CScript script( pszVerb );
				if (!r_Verb(script, pNewSrc))
				{
					DEBUG_ERR(( "Can't trysrc %s object %s (0%x) with src %s (0%x)\n", (LPCTSTR) pszVerb, (LPCTSTR) GetName(), (DWORD)GetUID(), NewSrc.GetObjUID(), (DWORD)pNewSrc->GetName()));
					return false;
				}
			}
			return true;

		case OV_UID:
			// block anyone from ever doing this.
			if ( pSrc )
			{
				pSrc->SysMessage( "Setting the UID this way is not allowed" );
			}
			return( false );

		case OV_UPDATE:
			Update();
			break;
		case OV_UPDATEX:
			// Some things like equipped items need to be removed before they can be updated !
			RemoveFromView();
			Update();
			break;
		case OV_DCLICK:
			if ( ! pCharSrc )
				return( false );
			if ( s.HasArgs() )
			{
				CGrayUID uid = s.GetArgVal();

				if (( ! uid.ObjFind()) || ( ! this->IsChar() ))
					return( false );

				CChar *pChar = dynamic_cast <CChar *> (this);

				return pChar->Use_Obj( uid.ObjFind(), true, true );
			}
			else
				return pCharSrc->Use_Obj( this, true, true );

		case OV_USEITEM:
			if ( ! pCharSrc )
				return( false );
			if ( s.HasArgs() )
			{
				CGrayUID uid = s.GetArgVal();

				if (( ! uid.ObjFind()) || ( ! this->IsChar() ))
					return( false );

				CChar *pChar = dynamic_cast <CChar *> (this);

				return pChar->Use_Obj( uid.ObjFind(), false, true );
			}
			else
				return pCharSrc->Use_Obj( this, false, true );

		case OV_FIX:
			s.GetArgStr()[0] = '\0';
		case OV_Z:	//	ussually in "SETZ" form
			if ( IsItemEquipped())
				return( false );
			if ( s.HasArgs())
			{
				SetUnkZ( s.GetArgVal());
			}
			else if ( IsTopLevel())
			{
				WORD wBlockFlags = CAN_C_WALK;
				if ( IsSetEF( EF_WalkCheck ) )
					SetTopZ( g_World.GetHeightPoint_New( GetTopPoint(), wBlockFlags, true));
				else
					SetTopZ( g_World.GetHeightPoint( GetTopPoint(), wBlockFlags, true ));
			}
			Update();
			break;

		default:
			return( false );
	}
	return true;
	EXC_CATCH("CObjBase");
	return false;
}

void CObjBase::RemoveFromView( CClient * pClientExclude )
{
	// Remove this item from all clients.
	// In a destructor this can do funny things.

	if ( IsDisconnected())
		return;	// not in the world.

	CObjBaseTemplate * pObjTop = GetTopLevelObj();

	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( pClientExclude == pClient )
			continue;
		CChar * pChar = pClient->GetChar();
		if ( pChar == NULL )
			continue;
		if ( pChar->GetTopDist( pObjTop ) > UO_MAP_VIEW_RADAR )
			continue;
		pClient->addObjectRemove( this );
	}
}

void CObjBase::DeletePrepare()
{
	// Prepare to delete.
	RemoveFromView();
	RemoveSelf();	// Must remove early or else virtuals will fail.
	ASSERT( GetParent() == NULL );
	ASSERT( IsDisconnected());	// It is no place in the world.
}

void CObjBase::Delete()
{
	DeletePrepare();
	ASSERT( IsDisconnected());	// It is no place in the world.
	g_World.m_ObjDelete.InsertHead(this);
}




TRIGRET_TYPE CObjBase::Spell_OnTrigger( SPELL_TYPE spell, SPTRIG_TYPE stage, CChar * pSrc, CScriptTriggerArgs * pArgs )
{
	CSpellDef	* pSpellDef = g_Cfg.GetSpellDef( spell );
	if ( !pSpellDef )
		return TRIGRET_RET_TRUE;

	if ( pSpellDef->HasTrigger( stage ) )
	{
		// RES_SKILL
		CResourceLock s;
		if ( pSpellDef->ResourceLock( s ))
		{
			return CScriptObj::OnTriggerScript( s, CSpellDef::sm_szTrigName[stage], pSrc, pArgs );
		}
	}
	return TRIGRET_RET_DEFAULT;
}

