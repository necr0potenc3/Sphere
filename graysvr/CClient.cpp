//
// CClient.cpp
// Copyright Menace Software (www.menasoft.com).
//
#include "graysvr.h"	// predef header.
#include "CClient.h"

/////////////////////////////////////////////////////////////////
// -CClient stuff.

CClient::CClient( SOCKET client ) :
	m_Socket( client )
{
	// This may be a web connection or Telnet ?
	m_PeerName		= m_Socket.GetPeerName(); // store ip address
	SetConnectType( CONNECT_UNK );	// don't know what sort of connect this is yet.
	UpdateLogIPConnecting( true );
	UpdateLogIPConnected( true );
	
	m_Crypt.SetClientVer( g_Serv.m_ClientVersion );
	m_dwCompressXORIndex = 0;
	m_pAccount = NULL;

	m_pChar = NULL;
	m_pGMPage = NULL;

	m_timeLogin.Init();
	m_timeLastSend =
	m_timeLastEvent = CServTime::GetCurrentTime();

	m_bin_PrvMsg = XCMD_QTY;

	m_wWalkCount = -1;
	m_iWalkStepCount = 0;
	m_iWalkTimeAvg	= 100;
	m_fPaused = false;
	m_fClosed = false;

	m_Targ_Mode = CLIMODE_SETUP_CONNECTING;
	m_tmSetup.m_dwIP = 0;
	m_tmSetup.m_iConnect = 0;

	memset( m_Walk_LIFO, 0, sizeof( m_Walk_LIFO ));	// Initialize the fast walk killer stuff
	m_Walk_InvalidEchos = 0;
	m_Walk_CodeQty = -1;	// how many valid codes in here ?

	m_Env.SetInvalid();

	g_Serv.ClientsInc();
	g_Serv.m_Clients.InsertHead( this );

	if (   !g_BackTask.m_RegisterIP.IsValidAddr()
		|| !m_PeerName.IsSameIP( g_BackTask.m_RegisterIP ) )
	{
		g_Log.Event( LOGM_CLIENTS_LOG,
			"%x:Client connected [Total:%i] ('%s' %i/%i)" DEBUG_CR,
			m_Socket.GetSocket(), g_Serv.StatGet( SERV_STAT_CLIENTS ),
			(LPCTSTR) m_PeerName.GetAddrStr(),
			GetLogIPConnecting(),
			GetLogIPConnected() );
	}
	else
	{
		g_Log.Event( LOGM_CLIENTS_LOG,
			"%x:Register server client connected ('%s')" DEBUG_CR,
			m_Socket.GetSocket(), m_PeerName.GetAddrStr() );
	}

#ifdef _WIN32
	DWORD lVal = 1;	// 0 =  block
	int iRet = m_Socket.IOCtlSocket( FIONBIO, &lVal );
	DEBUG_CHECK( iRet==0 );
#endif
	if ( g_Serv.m_uSizeMsgMax < MAX_BUFFER )
	{
		// MAX_BUFFER for outgoing buffers ?
	}

	// disable NAGLE algorythm for data compression/coalescing.
	// Send as fast as we can. we handle packing ourselves.
	BOOL nbool=TRUE;
	m_Socket.SetSockOpt( TCP_NODELAY, &nbool, sizeof(BOOL), IPPROTO_TCP );
	DEBUG_CHECK( g_Serv.StatGet( SERV_STAT_CLIENTS ) == g_Serv.m_Clients.GetCount() );
}


CClient::~CClient()
{
	g_Serv.StatDec( SERV_STAT_CLIENTS );
	bool bWasChar;

	if ( GetConnectType() != CONNECT_GAME )
		UpdateLogIPConnecting( false );
	UpdateLogIPConnected( false );

	if (   !g_BackTask.m_RegisterIP.IsValidAddr()
		|| !m_PeerName.IsSameIP( g_BackTask.m_RegisterIP ) )
	{
		g_Log.Event( LOGM_CLIENTS_LOG,
		"%x:Client disconnected [Total:%i] ('%s' %i/%i)" DEBUG_CR,
	       		m_Socket.GetSocket(),
			g_Serv.StatGet(SERV_STAT_CLIENTS),
			(LPCTSTR) m_PeerName.GetAddrStr(),
			GetLogIPConnecting(),
			GetLogIPConnected()
		);
	}
	else
	{
		g_Log.Event( LOGM_CLIENTS_LOG,
			"%x:Register server client disconnected ('%s')." DEBUG_CR,
			m_Socket.GetSocket(), (LPCTSTR) m_PeerName.GetAddrStr() );
	}

	bWasChar = ( m_pChar != NULL );
	CharDisconnect();	// am i a char in game ?
	Cmd_GM_PageClear();

	CAccount * pAccount = GetAccount();
	if ( pAccount )
	{
		pAccount->OnLogout(this, bWasChar);
		m_pAccount = NULL;
	}

	xFlush();

	if ( m_Socket.IsOpen() )
		m_Socket.Close();
}

bool CClient::CanInstantLogOut() const
{
	if ( g_Serv.IsLoading())	// or exiting.
		return( true );
	if ( ! g_Cfg.m_iClientLingerTime )
		return true;
	if ( IsPriv( PRIV_GM ))
		return true;
	if ( m_pChar == NULL )
		return( true );
	if ( m_pChar->IsStatFlag(STATF_DEAD))
		return( true );

	const CRegionWorld * pArea = m_pChar->GetRegion();
	if ( pArea == NULL )
		return( true );
	if ( pArea->IsFlag( REGION_FLAG_INSTA_LOGOUT ))
		return( true );
	return( false );
}

void CClient::CharDisconnect()
{
	// Disconnect the CChar from the client.
	// Even tho the CClient might stay active.
	if ( m_pChar == NULL )
		return;
	int	iLingerTime = g_Cfg.m_iClientLingerTime;

	Announce(false);
	bool fCanInstaLogOut = CanInstantLogOut();

	//	stoned chars cannot logout if they are not privileged of course
	if ( m_pChar->IsStatFlag(STATF_Stone) && ( GetPrivLevel() < PLEVEL_Counsel ))
	{
		iLingerTime = 60*60*TICK_PER_SEC;	// 1 hour of linger time
		fCanInstaLogOut = false;
	}

	//	we are not a client anymore
	if ( IsChatActive() )
		g_Serv.m_Chats.QuitChat(this);
	m_pChar->ClientDetach();	// we are not a client any more.

	CScriptTriggerArgs	args(iLingerTime, fCanInstaLogOut);
	m_pChar->OnTrigger(CTRIG_LogOut, m_pChar, &args);
	iLingerTime = args.m_iN1;
	fCanInstaLogOut = args.m_iN2;

	if ( iLingerTime <= 0 ) fCanInstaLogOut = true;

	// Gump memory cleanup, we don't want them on logged out players
	m_pChar->Memory_ClearTypes(MEMORY_GUMPRECORD);

	// log out immediately ? (test before ClientDetach())
	if ( ! fCanInstaLogOut )
	{
		// become an NPC for a little while
		CItem * pItemChange = CItem::CreateBase( ITEMID_RHAND_POINT_W );
		ASSERT(pItemChange);
		pItemChange->SetType(IT_EQ_CLIENT_LINGER);
		pItemChange->SetTimeout(iLingerTime);
		m_pChar->LayerAdd(pItemChange, LAYER_FLAG_ClientLinger);
	}
	else
	{
		// remove me from other clients screens now.
		m_pChar->SetDisconnected();
	}

	m_pChar = NULL;
}

void CClient::SysMessage( LPCTSTR pszMsg ) const // System message (In lower left corner)
{
	// Diff sorts of clients.
	if ( !pszMsg || !*pszMsg ) return;

	switch ( GetConnectType() )
	{
	case CONNECT_TELNET:
		{
		if ( ISINTRESOURCE(pszMsg) ) return;
		for ( ; *pszMsg != '\0'; pszMsg++ )
		{
			if ( *pszMsg == '\n' )	// translate.
			{
				(const_cast <CClient*>(this))->xSendReady( "\r", 1 );
			}
			(const_cast <CClient*>(this))->xSendReady( pszMsg, 1 );
		}
		}
		return;
	case CONNECT_CRYPT:
	case CONNECT_LOGIN:
	case CONNECT_GAME:
		const_cast <CClient*>(this)->addSysMessage( pszMsg );
		return;

	case CONNECT_HTTP:
		const_cast <CClient*>(this)->m_Targ_Text = pszMsg;
		return;

	// else toss it.
	}
}

void CClient::Announce( bool fArrive ) const
{
	ASSERT( GetChar() != NULL );
	ASSERT( GetChar()->m_pPlayer != NULL );
	if ( GetAccount() == NULL || GetChar() == NULL || GetChar()->m_pPlayer == NULL )
		return;

	// We have logged in or disconnected.
	// Annouce my arrival or departure.
	if ( g_Cfg.m_fArriveDepartMsg )
	{
		TCHAR *pszMsg = Str_GetTemp();
		for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
		{
			if ( pClient == this )
				continue;
			CChar * pChar = pClient->GetChar();
			if ( pChar == NULL )
				continue;
			if ( GetPrivLevel() > pClient->GetPrivLevel())
				continue;
			if ( ! pClient->IsPriv( PRIV_DETAIL|PRIV_HEARALL ))
				continue;
			if ( !*pszMsg )
			{
				const CRegionBase * pRegion = m_pChar->GetTopPoint().GetRegion( REGION_TYPE_AREA );
				sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_ARRDEP_1 ),
					(LPCTSTR) m_pChar->GetName(),
					(fArrive)? g_Cfg.GetDefaultMsg( DEFMSG_ARRDEP_2 ) : g_Cfg.GetDefaultMsg( DEFMSG_ARRDEP_3 ),
					pRegion ? (LPCTSTR) pRegion->GetName() : (LPCTSTR) g_Serv.GetName());
			}
			pClient->SysMessage(pszMsg);
		}
	}

	// re-Start murder decay if arriving

	if ( m_pChar->m_pPlayer->m_wMurders )
	{
		CItem * pMurders = m_pChar->LayerFind(LAYER_FLAG_Murders);
		if ( pMurders )
		{
			if ( fArrive )
			{
				// If the Memory exists, put it in the loop
				pMurders->SetTimeout( pMurders->m_itEqMurderCount.m_Decay_Balance * TICK_PER_SEC );
			}
			else
			{
				// Or save decay point if departing and remove from the loop
				pMurders->m_itEqMurderCount.m_Decay_Balance = pMurders->GetTimerAdjusted();
				pMurders->SetTimeout(-1); // turn off the timer til we log in again.
			}
		}
		else if ( fArrive )
		{
			// If not, see if we need one made
			m_pChar->Noto_Murder();
		}
	}

	if ( m_pChar != NULL )
	{
		m_pAccount->m_uidLastChar = m_pChar->GetUID();
	}
}

////////////////////////////////////////////////////

bool CClient::CanSee( const CObjBaseTemplate * pObj ) const
{
	// Can player see item b
	if ( m_pChar == NULL )
		return( false );
	return( m_pChar->CanSee( pObj ));
}

bool CClient::CanHear( const CObjBaseTemplate * pSrc, TALKMODE_TYPE mode ) const
{
	// can we hear this text or sound.

	if ( ! IsConnectTypePacket())
	{
		if ( GetConnectType() != CONNECT_TELNET )
			return( false );
		if ( mode == TALKMODE_BROADCAST ) // && GetAccount()
			return( true );
		return( false );
	}

	if ( mode == TALKMODE_BROADCAST || pSrc == NULL )
		return( true );
	if ( m_pChar == NULL )
		return( false );

	if ( IsPriv( PRIV_HEARALL ) &&
		pSrc->IsChar() &&
		( mode == TALKMODE_SYSTEM || mode == TALKMODE_SAY || mode == TALKMODE_WHISPER || mode == TALKMODE_YELL ))
	{
		const CChar * pCharSrc = dynamic_cast <const CChar*> ( pSrc );
		ASSERT(pCharSrc);
		if ( pCharSrc && pCharSrc->IsClient())
		{
			if ( pCharSrc->GetPrivLevel() <= GetPrivLevel())
			{
				return( true );
			}
		}
		// Else it does not apply.
	}

	return( m_pChar->CanHear( pSrc, mode ));
}

////////////////////////////////////////////////////

void CClient::addTargetVerb( LPCTSTR pszCmd, LPCTSTR pszArg )
{
	// Target a verb at some object .

	ASSERT(pszCmd);
	GETNONWHITESPACE(pszCmd);
	SKIP_SEPERATORS(pszCmd);

	if ( pszCmd == pszArg )
		pszArg = "";

	// priv here
	PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel( pszCmd );
	if ( ilevel > GetPrivLevel() )
		return;

	m_Targ_Text.Format( "%s%s%s", pszCmd, ( pszArg[0] && pszCmd[0] ) ? " " : "", pszArg );
	TCHAR *pszMsg = Str_GetTemp();
	sprintf(pszMsg, "Select object to set/command '%s'", (LPCTSTR) m_Targ_Text);
	addTarget(CLIMODE_TARG_OBJ_SET, pszMsg);
}


void CClient::addTargetFunction( LPCTSTR pszFunction, bool fAllowGround, bool fCheckCrime )
{
	// Target a verb at some object .
	ASSERT(pszFunction);
	GETNONWHITESPACE(pszFunction);
	SKIP_SEPERATORS(pszFunction);

	/*
	// priv here
	PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel( pszCmd );
	if ( ilevel > GetPrivLevel() )
		return;
	*/

	m_Targ_Text.Format( pszFunction );
	addTarget( CLIMODE_TARG_OBJ_FUNC, "", fAllowGround, fCheckCrime );
}

void CClient::addPromptConsoleFunction( LPCTSTR pszFunction, LPCTSTR pszSysmessage )
{
	// Target a verb at some object .
	ASSERT(pszFunction);
//	GETNONWHITESPACE(pszFunction);
//	SKIP_SEPERATORS(pszFunction);

	m_Targ_Text.Format( pszFunction );
	addPromptConsole( CLIMODE_PROMPT_SCRIPT_VERB, pszSysmessage );
}


enum CLIR_TYPE
{
	CLIR_ACCOUNT,
	CLIR_GMPAGEP,
	CLIR_PARTY,
	CLIR_TARG,
	CLIR_TARGPROP,
	CLIR_TARGPRV,
	CLIR_QTY,
};

LPCTSTR const CClient::sm_szRefKeys[CLIR_QTY+1] =
{
	"ACCOUNT",
	"GMPAGEP",
	"PARTY",
	"TARG",
	"TARGPROP",
	"TARGPRV",
	NULL,
};

bool CClient::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	int i = FindTableHeadSorted( pszKey, sm_szRefKeys, COUNTOF(sm_szRefKeys)-1 );
	if ( i >= 0 )
	{
		pszKey += strlen( sm_szRefKeys[i] );
		SKIP_SEPERATORS(pszKey);
		switch (i)
		{
			case CLIR_ACCOUNT:
				if ( pszKey[-1] != '.' )	// only used as a ref !
					break;
				pRef = GetAccount();
				return( true );
			case CLIR_GMPAGEP:
				pRef = m_pGMPage;
				return( true );
			case CLIR_PARTY:
				if ( this->m_pChar->m_pParty )
				{
					pRef = this->m_pChar->m_pParty;
				}
				else
				{
					pRef = NULL;
				}
				return( true );
			case CLIR_TARG:
				pRef = m_Targ_UID.ObjFind();
				return( true );
			case CLIR_TARGPRV:
				pRef = m_Targ_PrvUID.ObjFind();
				return( true );
			case CLIR_TARGPROP:
				pRef = m_Prop_UID.ObjFind();
				return( true );
		}
	}
	return( CScriptObj::r_GetRef( pszKey, pRef ));
}

LPCTSTR const CClient::sm_szLoadKeys[CC_QTY+1] = // static
{
	"ALLMOVE",
	"ALLSHOW",
	"CLIENTVERSION",
	"DEBUG",
	"DETAIL",
	"GM",
	"HEARALL",
	"PRIVSHOW",
	"REPORTEDCLIVER",
	"TARG",
	"TARGP",
	"TARGPROP",
	"TARGPRV",
	"TARGTXT",
	NULL,
};

LPCTSTR const CClient::sm_szVerbKeys[CV_QTY+1] =	// static
{
	"ADD",
	"ADMIN",
	"ARROWQUEST",
	"BADSPAWN",
	"BANKSELF",
	"CAST",
	"CHARLIST",
	"EVERBTARG",
	"EXTRACT",
	"FLUSH",
	"GMPAGE",
	"GOTARG",
	"HELP",
	"INFO",
	"INFORMATION",
	"LAST",
	"LINK",
	"LOGIN",
	"LOGOUT",
	"MENU",
	"MIDILIST",
	"NUDGE",
	"NUKE",
	"NUKECHAR",
	"ONECLICK",
	"PAGE",
	"REPAIR",
	"RESEND",
	"SAVE",
	"SCROLL",
	"SELF",
	"SENDPACKET",
	"SHOWSKILLS",
	"SKILLMENU",
	"SKILLSELECT",
	"STATIC",
	"SMSG",
	"SMSGU",
	"SUMMON",
	"SYSMESSAGE",
	"SYSMESSAGEUA",
	"TELE",
	"TILE",
	"UNEXTRACT",
	"VERSION",
	"WEBLINK",
	NULL,
};

void CClient::r_DumpLoadKeys( CTextConsole * pSrc )
{
	r_DumpKeys(pSrc,sm_szLoadKeys);
}
void CClient::r_DumpVerbKeys( CTextConsole * pSrc )
{
	r_DumpKeys(pSrc,sm_szVerbKeys);
}
int CClient::r_GetVerbIndex( LPCTSTR pszKey ) // static
{
	return FindTableSorted( pszKey, sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 );
}

bool CClient::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	EXC_TRY(("r_WriteVal('%s',,%x)", pszKey, pSrc));
	int index;

	if ( !strnicmp( "TARGP", pszKey, 5 )
		&& ( pszKey[5] == '\0' || pszKey[5] == '.' ) )
		index	= CC_TARGP;
	else
		index	= FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 );
	switch (index)
	{
		case CC_ALLMOVE:
			sVal.FormatVal( IsPriv( PRIV_ALLMOVE ));
			break;
		case CC_ALLSHOW:
			sVal.FormatVal( IsPriv( PRIV_ALLSHOW ));
			break;
		case CC_CLIENTVERSION:
			{
				TCHAR szVersion[ 128 ];
				sVal = m_Crypt.WriteClientVer( szVersion );
			}
			break;
		case CC_DEBUG:
			sVal.FormatVal( IsPriv( PRIV_DEBUG ));
			break;
		case CC_DETAIL:
			sVal.FormatVal( IsPriv( PRIV_DETAIL ));
			break;
		case CC_GM:	// toggle your GM status on/off
			sVal.FormatVal( IsPriv( PRIV_GM ));
			break;
		case CC_HEARALL:
			sVal.FormatVal( IsPriv( PRIV_HEARALL ));
			break;
		case CC_PRIVSHOW:
			// Show my priv title.
			sVal.FormatVal( ! IsPriv( PRIV_PRIV_NOSHOW ));
			break;
		case CC_REPORTEDCLIVER:
			sVal = m_reportedCliver;
			break;
		case CC_TARG:
			sVal.FormatVal( m_Targ_UID );
			break;
		case CC_TARGP:
			if ( pszKey[5] == '.' )
			{
				return m_Targ_p.r_WriteVal( pszKey+6, sVal );
			}
			sVal = m_Targ_p.WriteUsed();
			break;
		case CC_TARGPROP:
			sVal.FormatVal( m_Prop_UID );
			break;
		case CC_TARGPRV:
			sVal.FormatVal( m_Targ_PrvUID );
			break;
		case CC_TARGTXT:
			sVal = m_Targ_Text;
			break;
		default:
			return( CScriptObj::r_WriteVal( pszKey, sVal, pSrc ));
	}
	return true;
	EXC_CATCH("CClient");
	return false;
}

bool CClient::r_LoadVal( CScript & s )
{
	LOCKDATA;
	EXC_TRY(("r_LoadVal('%s %s')", s.GetKey(), s.GetArgStr()));
	if ( GetAccount() == NULL )
		return( false );

	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 ))
	{
		case CC_ALLMOVE:
			GetAccount()->TogPrivFlags( PRIV_ALLMOVE, s.GetArgStr() );
			addReSync();
			break;
		case CC_ALLSHOW:
			addRemoveAll( true, true );
			GetAccount()->TogPrivFlags( PRIV_ALLSHOW, s.GetArgStr() );
			addReSync();
			break;
		case CC_DEBUG:
			GetAccount()->TogPrivFlags( PRIV_DEBUG, s.GetArgStr() );
			addRemoveAll( true, false );
			addReSync();
			break;
		case CC_DETAIL:
			GetAccount()->TogPrivFlags( PRIV_DETAIL, s.GetArgStr() );
			break;
		case CC_GM: // toggle your GM status on/off
			if ( GetPrivLevel() >= PLEVEL_GM )
			{
				GetAccount()->TogPrivFlags( PRIV_GM, s.GetArgStr() );
			}
			break;
		case CC_HEARALL:
			GetAccount()->TogPrivFlags( PRIV_HEARALL, s.GetArgStr() );
			break;
		case CC_PRIVSHOW:
			// Hide my priv title.
			if ( GetPrivLevel() >= PLEVEL_Counsel )
			{
				if ( ! s.HasArgs())
				{
					GetAccount()->TogPrivFlags( PRIV_PRIV_NOSHOW, NULL );
				}
				else if ( s.GetArgVal() )
				{
					GetAccount()->ClearPrivFlags( PRIV_PRIV_NOSHOW );
				}
				else
				{
					GetAccount()->SetPrivFlags( PRIV_PRIV_NOSHOW );
				}
			}
			break;

		case CC_TARG:
			m_Targ_UID = s.GetArgVal();
			break;
		case CC_TARGP:
			m_Targ_p.Read( s.GetArgRaw());
			if ( m_Targ_p.IsValidPoint() )
			{
				m_Targ_p.ValidatePoint();
				SysMessagef( "Invalid point: %s", s.GetArgStr() );
			}
			break;
		case CC_TARGPROP:
			m_Prop_UID = s.GetArgVal();
			break;
		case CC_TARGPRV:
			m_Targ_PrvUID = s.GetArgVal();
			break;
		default:
			return( false );
	}
	return true;
	EXC_CATCH("CClient");
	return false;
}

bool CClient::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	LOCKDATA;
	EXC_TRY(("r_Verb('%s %s',%x)", s.GetKey(), s.GetArgStr(), pSrc));
	// NOTE: This can be called directly from a RES_WEBPAGE script.
	//  So do not assume we are a game client !
	// NOTE: Mostly called from CChar::r_Verb
	// NOTE: Little security here so watch out for dangerous scripts !

	ASSERT(pSrc);
	LPCTSTR pszKey = s.GetKey();

	if ( s.IsKeyHead( "SET", 3 ))
	{
		PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel( "SET" );
		if ( ilevel > GetPrivLevel() )
			return( false );

		ASSERT( m_pChar );
		addTargetVerb( pszKey+3, s.GetArgRaw());
		return( true );
	}

	if ( toupper( pszKey[0] ) == 'X' )
	{
		PLEVEL_TYPE ilevel = g_Cfg.GetPrivCommandLevel( "SET" );
		if ( ilevel > GetPrivLevel() )
			return( false );

		// Target this command verb on some other object.
		ASSERT( m_pChar );
		addTargetVerb( pszKey+1, s.GetArgRaw());
		return( true );
	}

	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 );
	switch (index)
	{
	case CV_ADD:
		if ( s.HasArgs())
		{
			// FindItemName ???
			TCHAR * pszArgs = s.GetArgStr();
			RESOURCE_ID rid = g_Cfg.ResourceGetID( RES_ITEMDEF, (LPCTSTR&)pszArgs );

			if ( rid.GetResType() == RES_CHARDEF )
			{
				m_Targ_PrvUID.InitUID();
				return Cmd_CreateChar( (CREID_TYPE) rid.GetResIndex(), SPELL_Summon, false );
			}

			ITEMID_TYPE id = (ITEMID_TYPE) rid.GetResIndex();
			return Cmd_CreateItem( id );
		}
		else
		{
			Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, "MENU_ADDITEM"));
		}
		break;
	case CV_ADMIN:
		addGumpDialogAdmin( s.GetArgVal()); // Open the Admin console	;
		break;
	case CV_ARROWQUEST:
		{
			int piVal[2];
			int iQty = Str_ParseCmds( s.GetArgRaw(), piVal, COUNTOF(piVal));
			addArrowQuest( piVal[0], piVal[1] );
		}
		break;
	case CV_BADSPAWN:
		{
			//	Loop the world searching for bad spawns
			for ( int m = 0; m < 256; m++ )
			{
				if ( !g_MapList.m_maps[m] ) continue;

				for ( DWORD d = 0; d < g_MapList.GetSectorQty(m); d++ )
				{
					CSector	*pSector = g_World.GetSector(m, d);
					if ( !pSector ) continue;

					CItem	*pNext;
					CItem	*pItem = STATIC_CAST <CItem*>(pSector->m_Items_Timer.GetHead());
					for ( ; pItem != NULL; pItem = pNext )
					{
						pNext = pItem->GetNext();

						if ( pItem->IsType(IT_SPAWN_ITEM) || pItem->IsType(IT_SPAWN_CHAR) )
						{
							CResourceDef	*pDef = pItem->Spawn_FixDef();
							if ( !pDef )
							{
								RESOURCE_ID_BASE	rid = ( pItem->IsType(IT_SPAWN_ITEM) ? pItem->m_itSpawnItem.m_ItemID : pItem->m_itSpawnChar.m_CharID);

								CPointMap	pt = pItem->GetTopPoint();
								m_pChar->Spell_Teleport(pt, true, false);
								m_pChar->m_Act_Targ = pItem->GetUID();
								SysMessagef("Bad spawn (0%lx, id=%s). Set as ACT", (DWORD)pItem->GetUID(), g_Cfg.ResourceGetName(rid));
								goto endbadspawn;
							}
						}
					}
				}
			}

			SysMessage("There are no found bad spawn points left.");
endbadspawn:;
		}
		break;
	case CV_BANKSELF: // open my own bank
		addBankOpen( m_pChar, (LAYER_TYPE) s.GetArgVal());
		break;
	case CV_CAST:
		if ( IsSetOF( OF_Magic_PreCast ) )
		{
			m_tmSkillMagery.m_Spell = (SPELL_TYPE) g_Cfg.ResourceGetIndexType( RES_SPELL, s.GetArgStr());	// m_atMagery.m_Spell
			m_pChar->m_atMagery.m_Spell = (SPELL_TYPE) g_Cfg.ResourceGetIndexType( RES_SPELL, s.GetArgStr());
			m_Targ_UID = dynamic_cast <CObjBase *>(pSrc)->GetUID();	// default target.
			m_Targ_PrvUID = dynamic_cast <CObjBase *>(pSrc)->GetUID();
			m_pChar->Skill_Start( SKILL_MAGERY );
			break;
		}
		else
			Cmd_Skill_Magery( (SPELL_TYPE) g_Cfg.ResourceGetIndexType( RES_SPELL, s.GetArgStr()), dynamic_cast <CObjBase *>(pSrc));
		break;
	case CV_CHARLIST:
		// ussually just a gm command
		addCharList3();
		break;
	case CV_EVERBTARG:
		m_Targ_Text = s.GetArgStr();
		addPromptConsole( CLIMODE_PROMPT_TARG_VERB, m_Targ_Text.IsEmpty() ? "Enter the verb" : "Enter the text" );
		break;

	case CV_EXTRACT:
		// sort of like EXPORT but for statics.
		// Opposite of the "UNEXTRACT" command

		if ( ! s.HasArgs())
		{
			SysMessage( "Usage: EXTRACT filename.ext code" );
		}
		else
		{
			TCHAR * ppArgs[2];
			Str_ParseCmds( s.GetArgStr(), ppArgs, COUNTOF( ppArgs ));

			m_Targ_Text = ppArgs[0]; // Point at the options, if any
			m_tmTile.m_ptFirst.InitPoint(); // Clear this first
			m_tmTile.m_Code = CV_EXTRACT;	// set extract code.
			m_tmTile.m_id = Exp_GetVal(ppArgs[1]);	// extract id.
			addTarget( CLIMODE_TARG_TILE, "Select area to Extract", true );
		}
		break;

	case CV_UNEXTRACT:
		// Create item from script.
		// Opposite of the "EXTRACT" command
		if ( ! s.HasArgs())
		{
			SysMessage( "Usage: UNEXTRACT filename.ext code" );
		}
		else
		{
			TCHAR * ppArgs[2];
			Str_ParseCmds( s.GetArgStr(), ppArgs, COUNTOF( ppArgs ));

			m_Targ_Text = ppArgs[0]; // Point at the options, if any
			m_tmTile.m_ptFirst.InitPoint(); // Clear this first
			m_tmTile.m_Code = CV_UNEXTRACT;	// set extract code.
			m_tmTile.m_id = Exp_GetVal(ppArgs[1]);	// extract id.

			addTarget( CLIMODE_TARG_UNEXTRACT, "Where to place the extracted multi?", true );
		}
		break;

	case CV_GMPAGE:
		m_Targ_Text = s.GetArgStr();
		addPromptConsole( CLIMODE_PROMPT_GM_PAGE_TEXT, g_Cfg.GetDefaultMsg( DEFMSG_GMPAGE_PROMPT ) );
		break;
	case CV_GOTARG: // go to my (preselected) target.
		{
			ASSERT(m_pChar);
			CObjBase * pObj = m_Targ_UID.ObjFind();
			if ( pObj != NULL )
			{
				CPointMap po = pObj->GetTopLevelObj()->GetTopPoint();
				CPointMap pnt = po;
				pnt.MoveN( DIR_W, 3 );
				WORD wBlockFlags = m_pChar->GetMoveBlockFlags();
				pnt.m_z = g_World.GetHeightPoint( pnt, wBlockFlags );	// ??? Get Area
				m_pChar->m_dirFace = pnt.GetDir( po, m_pChar->m_dirFace ); // Face the player
				m_pChar->Spell_Teleport( pnt, true, false );
			}
		}
		break;
	case CV_HELP:
		if ( ! s.HasArgs()) // if no command, display the main help system dialog.
		{
			Dialog_Setup( CLIMODE_DIALOG, g_Cfg.ResourceGetIDType( RES_DIALOG, IsPriv(PRIV_GM) ? "d_HELPGM" : "d_HELP" ), 0, m_pChar );
		}
		else
		{
			// Below here, we're looking for a command to get help on
			int index = g_Cfg.m_HelpDefs.FindKey( s.GetArgStr());
			if ( index >= 0 )
			{
				CResourceNamed * pHelpDef = STATIC_CAST <CResourceNamed*>(g_Cfg.m_HelpDefs[index]);
				ASSERT(pHelpDef);
				CResourceLock s;
				if ( pHelpDef->ResourceLock( s ))
				{
					addScrollScript( s, SCROLL_TYPE_TIPS, 0, s.GetArgStr());
				}
			}
		}
		break;
	case CV_INFO:
		// We could also get ground tile info.
		addTarget( CLIMODE_TARG_OBJ_INFO, "What would you like info on?", true, false );
		break;
	case CV_INFORMATION:
		SysMessage( g_Serv.GetStatusString( 0x22 ));
		SysMessage( g_Serv.GetStatusString( 0x24 ));
		break;
	case CV_LAST:
		// Fake Previous target.
		if ( GetTargMode() >= CLIMODE_MOUSE_TYPE )
		{
			ASSERT(m_pChar);
			CObjBase * pObj = m_pChar->m_Act_Targ.ObjFind();
			if ( pObj != NULL )
			{
				CEvent Event;
				CPointMap pt = pObj->GetUnkPoint();
				Event.Target.m_context = GetTargMode();
				Event.Target.m_x = pt.m_x;
				Event.Target.m_y = pt.m_y;
				Event.Target.m_z = pt.m_z;
				Event.Target.m_UID = pObj->GetUID();
				Event.Target.m_id = 0;
				Event_Target( &Event );
			}
			break;
		}
		return( false );
	case CV_LINK:	// link doors
		m_Targ_UID.InitUID();
		addTarget( CLIMODE_TARG_LINK, "Select the item to link." );
		break;

	case CV_LOGIN:
		{
			// Try to login with name and password given.
			CLogIP * pLogIP = GetLogIP();
			if ( pLogIP == NULL )
				return( false );
			m_pAccount = NULL;	// this is odd ???
			pLogIP->SetAccount( NULL );

			TCHAR * ppArgs[2];
			Str_ParseCmds( s.GetArgStr(), ppArgs, COUNTOF( ppArgs ));
			LOGIN_ERR_TYPE nCode = LogIn( ppArgs[0], ppArgs[1], m_Targ_Text );

			// Associate this with the CLogIP !
			pLogIP->SetAccount( GetAccount());
		}
		break;

	case CV_LOGOUT:
		{
			// Clear the account and the link to this CLogIP
			CLogIP * pLogIP = GetLogIP();
			if ( pLogIP == NULL )
				return( false );
			pLogIP->InitTimes();
		}
		break;

	case CV_MENU:
		Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, s.GetArgStr()));
		break;
	case CV_MIDILIST:
		{
			int piMidi[64];
			int iQty = Str_ParseCmds( s.GetArgStr(), piMidi, COUNTOF(piMidi));
			if ( iQty > 0 )
			{
				addMusic( piMidi[ Calc_GetRandVal( iQty ) ] );
			}
		}
		break;
	case CV_NUDGE:
		if ( ! s.HasArgs())
		{
			SysMessage( "Usage: NUDGE dx dy dz" );
		}
		else
		{
			m_Targ_Text = s.GetArgRaw();
			m_tmTile.m_ptFirst.InitPoint(); // Clear this first
			m_tmTile.m_Code = CV_NUDGE;
			addTarget( CLIMODE_TARG_TILE, "Select area to Nudge", true );
		}
		break;

	case CV_NUKE:
		m_Targ_Text = s.GetArgRaw();
		m_tmTile.m_ptFirst.InitPoint(); // Clear this first
		m_tmTile.m_Code = CV_NUKE;	// set nuke code.
		addTarget( CLIMODE_TARG_TILE, "Select area to Nuke", true );
		break;
	case CV_NUKECHAR:
		m_Targ_Text = s.GetArgRaw();
		m_tmTile.m_ptFirst.InitPoint(); // Clear this first
		m_tmTile.m_Code = CV_NUKECHAR;	// set nuke code.
		addTarget( CLIMODE_TARG_TILE, "Select area to Nuke Chars", true );
		break;

	case CV_ONECLICK:
		m_Targ_Text = s.GetArgRaw();
		m_pChar->Skill_Start( (m_Targ_Text.IsEmpty()) ? SKILL_NONE : NPCACT_OneClickCmd );
		break;

	case CV_PAGE:
		Cmd_GM_PageCmd( s.GetArgStr());
		break;
	case CV_REPAIR:
		addTarget( CLIMODE_TARG_REPAIR, "What item do you want to repair?" );
		break;
	case CV_FLUSH:
		xFlush();
		break;
	case CV_RESEND:
		addReSync();
		break;
	case CV_SAVE:
		g_World.Save(s.GetArgVal());
		break;
	case CV_SCROLL:
		// put a scroll up.
		addScrollResource( s.GetArgStr(), SCROLL_TYPE_UPDATES );
		break;
	case CV_SENDPACKET:
		SendPacket( s.GetArgStr() );
		break;
	case CV_SELF:
		// Fake self target.
		if ( GetTargMode() >= CLIMODE_MOUSE_TYPE )
		{
			ASSERT(m_pChar);
			CEvent Event;
			Event.Target.m_context = GetTargMode();
			CPointMap pt = m_pChar->GetTopPoint();
			Event.Target.m_x = pt.m_x;
			Event.Target.m_y = pt.m_y;
			Event.Target.m_z = pt.m_z;
			Event.Target.m_UID = m_pChar->GetUID();
			Event.Target.m_id = 0;
			Event_Target(&Event);
			break;
		}
		return( false );
	case CV_SHOWSKILLS:
		addSkillWindow( (SKILL_TYPE) MAX_SKILL ); // Reload the real skills
		break;
	case CV_SKILLMENU:				// Just put up another menu.
		return Cmd_Skill_Menu( g_Cfg.ResourceGetIDType( RES_SKILLMENU, s.GetArgStr()));
	case CV_SKILLSELECT:
		Event_Skill_Use( g_Cfg.FindSkillKey( s.GetArgStr() ) );
		break;
	case CV_STATIC:
		if ( s.HasArgs())
		{
			Cmd_CreateItem( (ITEMID_TYPE) g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()), true );
		}
		break;
	case CV_SUMMON:	// from the spell skill script.
		// m_Targ_PrvUID should already be set.
		return Cmd_CreateChar( (CREID_TYPE) g_Cfg.ResourceGetIndexType( RES_CHARDEF, s.GetArgStr()), SPELL_Summon, true );
	case CV_SMSG:
	case CV_SYSMESSAGE:
		SysMessage( s.GetArgStr() );
		break;
	case CV_SMSGU:
	case CV_SYSMESSAGEUA:
		{
			TCHAR * pszArgs[5];
			int iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, 5 );
			if ( iArgQty > 4 )
			{
				// Font and mode are actually ignored here, but they never made a difference
				// anyway.. I'd like to keep the syntax similar to SAYUA
			 	NCHAR szBuffer[ MAX_TALK_BUFFER ];
				CvtSystemToNUNICODE( szBuffer, COUNTOF(szBuffer), pszArgs[4], -1 );

				addBarkUNICODE( szBuffer, NULL, (HUE_TYPE)Exp_GetVal(pszArgs[0]), TALKMODE_SYSTEM, (FONT_TYPE)0x03, pszArgs[3] );
			}
		}
		break;
	case CV_TELE:
		Cmd_Skill_Magery( SPELL_Teleport, dynamic_cast <CObjBase *>(pSrc));
		break;
	case CV_TILE:
		if ( ! s.HasArgs())
		{
			SysMessage( "Usage: TILE z-height item1 item2 itemX" );
		}
		else
		{
			m_Targ_Text = s.GetArgStr(); // Point at the options
			m_tmTile.m_ptFirst.InitPoint(); // Clear this first
			m_tmTile.m_Code = CV_TILE;
			addTarget( CLIMODE_TARG_TILE, "Pick 1st corner:", true );
		}
		break;
	case CV_VERSION:	// "SHOW VERSION"
		{
			TCHAR * tVersionOut = Str_GetTemp();
			sprintf(tVersionOut, g_szServerDescription, (LPCTSTR)g_Cfg.m_sVerName.GetPtr(), (LPCTSTR)g_Serv.m_sServVersion.GetPtr());
			SysMessage( (LPCTSTR)tVersionOut );
		}
		break;
	case CV_WEBLINK:
		addWebLaunch( s.GetArgStr());
		break;
	default:
		if ( r_LoadVal( s ))
		{
			CGString sVal;
			if ( r_WriteVal( s.GetKey(), sVal, pSrc ))
			{
				SysMessagef( "%s = %s", (LPCTSTR) s.GetKey(), (LPCTSTR) sVal );	// feedback on what we just did.
				return( true );
			}
		}
		return( CScriptObj::r_Verb( s, pSrc ));	// used in the case of web pages to access server level things..
	}
	return true;
	EXC_CATCH("CClient");
	return false;
}

