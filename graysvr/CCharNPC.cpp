//
// CCharNPC.CPP
// Copyright Menace Software (www.menasoft.com).
//
// Actions specific to an NPC.
//

#include "graysvr.h"	// predef header.
#include "CClient.h"

enum CPC_TYPE	// Player char.
{
	CPC_ACCOUNT,
	CPC_DEATHS,
	CPC_KILLS,
	CPC_LASTUSED,	// m_timeLastUsed
	CPC_PROFILE,
	CPC_SKILLCLASS,
	CPC_SKILLLOCK,
	CPC_QTY,
};

LPCTSTR const CCharPlayer::sm_szLoadKeys[CPC_QTY+1] =
{
	"ACCOUNT",
	"DEATHS",
	"KILLS",
	"LASTUSED",
	"PROFILE",
	"SKILLCLASS",
	"SKILLLOCK",
	NULL,
};

enum CNC_TYPE
{
	CNC_ACTPRI,
	CNC_HOMEDIST,
	CNC_NEED,
	CNC_NEEDNAME,
	CNC_NPC,
	CNC_SPEECH,
	CNC_SPEECHCOLOR,
	CNC_VENDCAP,
	CNC_VENDGOLD,
	CNC_QTY,
};

LPCTSTR const CCharNPC::sm_szLoadKeys[CNC_QTY+1] =
{
	"ACTPRI",
	"HOMEDIST",
	"NEED",
	"NEEDNAME",
	"NPC",
	"SPEECH",
	"SPEECHCOLOR",
	"VENDCAP",
	"VENDGOLD",
	NULL,
};

void CChar::ClearNPC()
{
	if ( m_pNPC == NULL )
		return;

	delete m_pNPC;
	m_pNPC = NULL;
}

void CChar::ClearPlayer()
{
	if ( m_pPlayer == NULL )
		return;

	// unlink me from my account.
	if ( g_Serv.m_iModeCode != SERVMODE_Exiting )
	{
		if ( m_pPlayer->m_pAccount )
			DEBUG_WARN(( "Player delete '%s' name '%s'" DEBUG_CR,
				(LPCTSTR) m_pPlayer->GetAccount()->GetName(), (LPCTSTR) GetName()));
		else
			DEBUG_WARN(( "Player delete from account name '%s'" DEBUG_CR,
				(LPCTSTR) GetName()));
	}

	// Is this valid ?
	m_pPlayer->GetAccount()->DetachChar( this );
	delete m_pPlayer;
	m_pPlayer = NULL;
}

bool CChar::SetPlayerAccount( CAccount * pAccount )
{
	// Set up the char as a Player.
	if ( pAccount == NULL )
	{
		DEBUG_ERR(( "SetPlayerAccount '%s' NULL" DEBUG_CR, (LPCTSTR) GetName()));
		return false;
	}

	// ClearNPC(); // allow to be both NPC and player ?
	if ( m_pPlayer != NULL )
	{
		if ( m_pPlayer->GetAccount() == pAccount )
			return( true );
		// ??? Move to another account ?
		DEBUG_ERR(( "SetPlayerAccount '%s' already set '%s' != '%s' !" DEBUG_CR, (LPCTSTR) GetName(), (LPCTSTR) m_pPlayer->GetAccount()->GetName(), (LPCTSTR) pAccount->GetName()));
		return( false );
	}

	m_pPlayer = new CCharPlayer( this, pAccount );

	ASSERT(m_pPlayer);
	pAccount->AttachChar( this );
	return( true );
}



bool CChar::SetPlayerAccount( LPCTSTR pszAccName )
{
	CAccountRef pAccount = g_Accounts.Account_FindCreate( pszAccName, g_Serv.m_eAccApp == ACCAPP_Free );
	if ( pAccount == NULL )
	{
		DEBUG_ERR(( "SetPlayerAccount '%s' can't find '%s'!" DEBUG_CR, GetName(), pszAccName ));
		return false;
	}
	return( SetPlayerAccount( pAccount ));
}



bool CChar::SetNPCBrain( NPCBRAIN_TYPE NPCBrain )
{
	// Set up the char as an NPC
	if ( NPCBrain == NPCBRAIN_NONE || IsClient())
	{
		DEBUG_ERR(( "SetNPCBrain NULL" DEBUG_CR ));
		return false;
	}
	if ( m_pPlayer != NULL )
	{
		DEBUG_ERR(( "SetNPCBrain to Player '%s'" DEBUG_CR, m_pPlayer->GetAccount() ));
		return false;
	}
	if ( m_pNPC == NULL )
	{
		m_pNPC = new CCharNPC( this, NPCBrain );
	}
	else
	{
		// just replace existing brain.
		m_pNPC->m_Brain = NPCBrain;
	}
	return( true );
}

//////////////////////////
// -CCharPlayer

CCharPlayer::CCharPlayer( CChar * pChar, CAccount * pAccount ) :
	m_pAccount( pAccount )
{
	ASSERT(pAccount);
	m_wDeaths = 0;
	m_wMurders = 0;
	memset( m_SkillLock, 0, sizeof( m_SkillLock ));
	SetSkillClass( pChar, RESOURCE_ID( RES_SKILLCLASS ));
}

CCharPlayer::~CCharPlayer()
{
}

bool CCharPlayer::SetSkillClass( CChar * pChar, RESOURCE_ID rid )
{
	ASSERT(pChar);
	CResourceDef * pDef = g_Cfg.ResourceGetDef(rid);
	if ( pDef == NULL )
		return( false );
	DEBUG_CHECK( rid.GetResType() == RES_SKILLCLASS );
	CSkillClassDef* pLink = STATIC_CAST <CSkillClassDef*>(pDef);
	if ( pLink == NULL )
		return( false );
	if ( pLink == GetSkillClass())
		return( true );

	// Remove any previous skillclass from the Events block.
	int i = pChar->m_OEvents.FindResourceType( RES_SKILLCLASS );
	if ( i >= 0 )
	{
		pChar->m_OEvents.RemoveAt(i);
	}

	m_SkillClass.SetRef(pLink);

	// set it as m_Events block as well.
	pChar->m_OEvents.Add( pLink );
	return( true );
}

CSkillClassDef * CCharPlayer::GetSkillClass() const
{
	// This should always return NON-NULL.

	CResourceLink * pLink = m_SkillClass.GetRef();
	if ( pLink == NULL )
		return( NULL );
	return( STATIC_CAST <CSkillClassDef *>(pLink));	
}

SKILL_TYPE CCharPlayer::Skill_GetLockType( LPCTSTR pszKey ) const
{
	// only players can have skill locks.

	TCHAR szTmpKey[128];
	strcpylen( szTmpKey, pszKey, sizeof(szTmpKey) );

	TCHAR * ppArgs[2];
	int i = Str_ParseCmds( szTmpKey, ppArgs, COUNTOF(ppArgs), ".[]" );
	if ( i <= 0 )
		return( SKILL_NONE );

	if ( isdigit( ppArgs[1][0] ))
	{
		i = ATOI( ppArgs[1] );
	}
	else
	{
		i = g_Cfg.FindSkillKey( ppArgs[1] );
	}
	if ( i >= MAX_SKILL )
		return( SKILL_NONE );
	return( (SKILL_TYPE) i );
}

void CCharPlayer::r_DumpLoadKeys( CTextConsole * pSrc )
{
	CScriptObj::r_DumpKeys(pSrc,sm_szLoadKeys);
}

bool CCharPlayer::r_WriteVal( CChar * pChar, LPCTSTR pszKey, CGString & sVal )
{
	EXC_TRY(("r_WriteVal(%x,'%s',)", pChar, pszKey));
	ASSERT(pChar);
	ASSERT( GetAccount());

	if ( !strnicmp(pszKey, "SKILLCLASS.", 11) )
	{
		pszKey += 11;
		return GetSkillClass()->r_WriteVal(pszKey, sVal, pChar);
	}
	else if ( !strnicmp(pszKey, "GUILD", 5) )
	{
		pszKey += 5;
		if ( *pszKey == 0 )
		{
			CItemStone *pMyGuild = pChar->Guild_Find(MEMORY_GUILD);
			if ( pMyGuild ) sVal.FormatVal((DWORD)pMyGuild->GetUID());
			else sVal.FormatVal(0);
			return true;
		}
		else if ( *pszKey == '.' )
		{
			pszKey += 1;
			CItemStone *pMyGuild = pChar->Guild_Find(MEMORY_GUILD);
			if ( pMyGuild ) return pMyGuild->r_WriteVal(pszKey, sVal, pChar);
		}
		return false;
	}

	switch ( FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case CPC_SKILLLOCK:
		{
			// "SkillLock[alchemy]"
			SKILL_TYPE skill = Skill_GetLockType( pszKey );
			if ( skill <= SKILL_NONE )
				return( false );
			sVal.FormatVal( Skill_GetLock( skill ));
		}
		return( true );
	case CPC_ACCOUNT:
		sVal = GetAccount()->GetName();
		return( true );
	case CPC_DEATHS:
		sVal.FormatVal( m_wDeaths );
		return( true );
	case CPC_KILLS:
		sVal.FormatVal( m_wMurders );
		return( true );
	case CPC_LASTUSED:
		sVal.FormatVal( - g_World.GetTimeDiff( m_timeLastUsed ) / TICK_PER_SEC );
		return( true );
	case CPC_PROFILE:
		{
			TCHAR szLine[SCRIPT_MAX_LINE_LEN-16];
			Str_MakeUnFiltered( szLine, m_sProfile, sizeof(szLine));
			sVal = szLine;
		}
		return( true );
	case CPC_SKILLCLASS:
		sVal = GetSkillClass()->GetResourceName();
		return( true );

	default:
		if ( FindTableSorted( pszKey, CCharNPC::sm_szLoadKeys, COUNTOF( CCharNPC::sm_szLoadKeys )-1 ) >= 0 )
		{
			sVal = "0";
			return( true );
		}
		return( false );
	}
	EXC_CATCH("CCharPlayer");
	return false;
}

bool CCharPlayer::r_LoadVal( CChar * pChar, CScript &s )
{
	LOCKDATA;
	EXC_TRY(("r_LoadVal('%s %s')", s.GetKey(), s.GetArgStr()));
	LPCTSTR const sm_szLockStates[] =
	{
		"UP",
		"DOWN",
		"LOCK",
	};

	LPCTSTR pszKey = s.GetKey();
	LPCTSTR pszArgs = s.GetArgStr();
	if ( !strnicmp(pszKey, "GMPAGE", 6) )		//	GM pages
	{
		CGMPage	*pPage;
		pszKey += 6;
		if ( *pszKey == '.' )						//	SERV.GMPAGE.*
		{
			SKIP_SEPERATORS(pszKey);
			int iQty = Exp_GetVal(pszKey);
			if (( iQty < 0 ) || ( iQty >= g_World.m_GMPages.GetCount() )) return false;
			SKIP_SEPERATORS(pszKey);
			pPage = STATIC_CAST <CGMPage*> (g_World.m_GMPages.GetAt(iQty));
			if ( !pPage ) return false;

			if ( !strnicmp(pszKey, "HANDLE", 6) )
			{
				CChar *ppChar = pChar;

				if ( *pszArgs ) ppChar = STATIC_CAST <CChar*> (g_World.FindUID(s.GetArgVal()));
				if ( !pChar ) return false;
				CClient *pClient = pChar->GetClient();
				if ( !pClient ) return false;

				pClient->m_pGMPage = pPage;
				pPage->SetGMHandler(pClient);
			}
			else if ( !strnicmp(pszKey, "DELETE", 6) )
			{
				pPage->RemoveSelf();
			}
			else if ( pPage->FindGMHandler() )
			{
				CClient	*pClient = pChar->GetClient();
				if ( pClient ) pClient->Cmd_GM_PageCmd(pszKey);
			}
			else return false;
			return true;
		}
		return false;
	}

	ASSERT(pChar);
	switch ( FindTableHeadSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case CPC_SKILLLOCK:
		{
			SKILL_TYPE skill = Skill_GetLockType( s.GetKey());
			if ( skill <= SKILL_NONE )
				return( false );
			int bState;
			if ( isdigit( s.GetArgRaw()[0] ))
			{
				bState = atoi( s.GetArgRaw());
			}
			else
			{
				bState = FindTable( s.GetArgStr(), sm_szLockStates, COUNTOF( sm_szLockStates ));
			}
			if ( bState < SKILLLOCK_UP || bState > SKILLLOCK_LOCK )
				return( false );
			Skill_SetLock( skill, (SKILLLOCK_TYPE) bState );
		}
		return true;
	case CPC_DEATHS:
		m_wDeaths = s.GetArgVal();
		return true;
	case CPC_KILLS:
		m_wMurders = s.GetArgVal();
		return true;
	case CPC_LASTUSED:
		m_timeLastUsed = CServTime::GetCurrentTime() - ( s.GetArgVal() * TICK_PER_SEC );
		return( true );
	case CPC_PROFILE:
		m_sProfile = Str_MakeFiltered( s.GetArgStr());
		return( true );
	case CPC_SKILLCLASS:
		return SetSkillClass( pChar, g_Cfg.ResourceGetIDType( RES_SKILLCLASS, s.GetArgStr()));
	default:
		// Just ignore any NPC type stuff.
		if ( FindTableSorted( s.GetKey(), CCharNPC::sm_szLoadKeys, COUNTOF( CCharNPC::sm_szLoadKeys )-1 ) >= 0 )
		{
			return( true );
		}
		return( false );
	}
	EXC_CATCH("CCharPlayer");
	return false;
}

void CCharPlayer::r_WriteChar( CChar * pChar, CScript & s ) 
{
	ASSERT(pChar);
	ASSERT(m_pAccount);

	if ( pChar->IsClient())
	{
		// periodically update this. (just in case we are logged in forever hehe.)
		//
		m_timeLastUsed = CServTime::GetCurrentTime();
	}

	s.WriteKey( "ACCOUNT", GetAccount()->GetName());

	if ( m_wDeaths )
		s.WriteKeyVal( "DEATHS", m_wDeaths );
	if ( m_wMurders )
		s.WriteKeyVal( "KILLS", m_wMurders );
	if ( GetSkillClass()->GetResourceID().GetResIndex() )
		s.WriteKey( "SKILLCLASS", GetSkillClass()->GetResourceName());
	if ( ! m_sProfile.IsEmpty())
	{
		TCHAR szLine[SCRIPT_MAX_LINE_LEN-16];
		Str_MakeUnFiltered( szLine, m_sProfile, sizeof(szLine));
		s.WriteKey( "PROFILE", szLine );
	}

	for ( int j=0;j<MAX_SKILL;j++)	// Don't write all lock states!
	{
		if ( ! m_SkillLock[j] )
			continue;
		TCHAR szTemp[128];
		sprintf( szTemp, "SkillLock[%d]", j );	// smaller storage space.
		s.WriteKeyVal( szTemp, m_SkillLock[j] );
	}
}

LPCTSTR const CCharPlayer::sm_szVerbKeys[] =
{
	"EMAIL",
	"KICK",
	"PASSWORD",
	NULL,
};

bool CChar::Player_OnVerb( CScript &s, CTextConsole * pSrc ) // Execute command from script
{
	ASSERT( m_pPlayer );

	switch ( FindTableSorted( s.GetKey(), CCharPlayer::sm_szVerbKeys, COUNTOF(CCharPlayer::sm_szVerbKeys)-1 ))
	{
	case 0:	// "EMAIL"
		// Sets the email for the players account.
		m_pPlayer->GetAccount()->r_Verb( s, pSrc );
		break;
	case 1: // "KICK" = kick and block the account
		if ( ! IsClient())
			return false;
		return( m_pClient->addKick( pSrc ));
	case 2:	// "PASSWORD"
		// Set/Clear the password
		if ( pSrc != this )
		{
			if ( pSrc->GetPrivLevel() <= GetPrivLevel() ||
				pSrc->GetPrivLevel() < PLEVEL_Admin )
			{
				pSrc->SysMessage( "You are not privileged to do this." );
				return( false );
			}
		}
		if ( ! s.HasArgs())
		{
			m_pPlayer->GetAccount()->ClearPassword();
			SysMessage( "Password has been cleared." );
			SysMessage( "Log out, then back in to set the new password." );
			g_Log.Event( LOGM_ACCOUNTS|LOGL_EVENT, "Account '%s', password cleared", (LPCTSTR) m_pPlayer->GetAccount()->GetName());
		}
		else
		{
			m_pPlayer->GetAccount()->SetPassword( s.GetArgStr());
			SysMessage( "Password has been set" );
			g_Log.Event( LOGM_ACCOUNTS|LOGL_EVENT, "Account '%s', password set to '%s'", (LPCTSTR) m_pPlayer->GetAccount()->GetName(), (LPCTSTR) s.GetArgStr());
		}
		break;

	default:
		return( false );
	}

	return( true );
}

//////////////////////////
// -CCharNPC

CCharNPC::CCharNPC( CChar * pChar, NPCBRAIN_TYPE NPCBrain )
{
	m_Brain = NPCBrain;
	m_Home_Dist_Wander = SHRT_MAX;	// as far as i want.
	m_Act_Motivation = 0;
	m_SpeechHue = HUE_TEXT_DEF;
	memset(m_nextX, 0, sizeof(m_nextX));
	memset(m_nextY, 0, sizeof(m_nextY));
}

CCharNPC::~CCharNPC()
{
}

void CCharNPC::r_DumpLoadKeys( CTextConsole * pSrc )
{
	CScriptObj::r_DumpKeys(pSrc,sm_szLoadKeys);
}

bool CCharNPC::r_LoadVal( CChar * pChar, CScript &s )
{
	LOCKDATA;
	EXC_TRY(("r_LoadVal('%s %s')", s.GetKey(), s.GetArgStr()));
	ASSERT(pChar);
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case CNC_ACTPRI:
		m_Act_Motivation = s.GetArgVal();
		break;
	case CNC_NPC:
		m_Brain = (NPCBRAIN_TYPE) s.GetArgVal();
		break;
	case CNC_HOMEDIST:
		if ( ! pChar->m_ptHome.IsValidPoint())
		{
			pChar->m_ptHome = pChar->GetTopPoint();
		}
		m_Home_Dist_Wander = s.GetArgVal();
		break;
	case CNC_NEED:
	case CNC_NEEDNAME:
		{
			TCHAR * pTmp = s.GetArgRaw();
			m_Need.Load(pTmp);
		}
		break;
	case CNC_SPEECH:
		return( m_Speech.r_LoadVal( s, RES_SPEECH ));
	case CNC_SPEECHCOLOR:
		m_SpeechHue = s.GetArgVal();
		break;

	case CNC_VENDCAP:
		{
			CItemContainer * pBank = pChar->GetBank();
			ASSERT(pBank);
			pBank->m_itEqBankBox.m_Check_Restock = s.GetArgVal();
		}
		break;
	case CNC_VENDGOLD:
		{
			CItemContainer * pBank = pChar->GetBank();
			ASSERT(pBank);
			pBank->m_itEqBankBox.m_Check_Amount = s.GetArgVal();
		}
		break;

	default:
		// Just ignore any player type stuff.
		if ( FindTableHeadSorted( s.GetKey(), CCharPlayer::sm_szLoadKeys, COUNTOF( CCharPlayer::sm_szLoadKeys )-1 ) >= 0 )
			return( true );
		return(false );
	}
	return true;
	EXC_CATCH("CCharNPC");
	return false;
}

bool CCharNPC::r_WriteVal( CChar * pChar, LPCTSTR pszKey, CGString & sVal )
{
	EXC_TRY(("r_WriteVal(%x,'%s',)", pChar, pszKey));
	ASSERT(pChar);

	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case CNC_ACTPRI:
		sVal.FormatVal( m_Act_Motivation );
		break;
	case CNC_NPC:
		sVal.FormatVal( m_Brain );
		break;
	case CNC_HOMEDIST:
		sVal.FormatVal( m_Home_Dist_Wander );
		break;
	case CNC_NEED:
		{
			TCHAR *pszTmp = Str_GetTemp();
			m_Need.WriteKey( pszTmp );
			sVal = pszTmp;
		}
		break;
	case CNC_NEEDNAME:
		{
			TCHAR *pszTmp = Str_GetTemp();
			m_Need.WriteNameSingle( pszTmp );
			sVal = pszTmp;
		}
		break;
	case CNC_SPEECH:
		m_Speech.WriteResourceRefList( sVal );
		break;
	case CNC_SPEECHCOLOR:
		sVal.FormatVal( m_SpeechHue );
		break;

	case CNC_VENDCAP:
		{
			CItemContainer * pBank = pChar->GetBank();
			ASSERT(pBank);
			sVal.FormatVal( pBank->m_itEqBankBox.m_Check_Restock );
		}
		break;
	case CNC_VENDGOLD:
		{
			CItemContainer * pBank = pChar->GetBank();
			ASSERT(pBank);
			sVal.FormatVal( pBank->m_itEqBankBox.m_Check_Amount );
		}
		break;
	default:
		if ( FindTableHeadSorted( pszKey, CCharPlayer::sm_szLoadKeys, COUNTOF( CCharPlayer::sm_szLoadKeys )-1 ) >= 0 )
		{
			sVal = "0";
			return( true );
		}
		if ( FindTableSorted( pszKey, CClient::sm_szLoadKeys, CC_QTY ) >= 0 )
		{
			sVal = "0";
			return( true );
		}
		return(false );
	}
	return true;
	EXC_CATCH("CCharNPC");
	return false;
}

void CCharNPC::r_WriteChar( CChar * pChar, CScript & s )
{
	ASSERT(pChar);
	// This says we are an NPC.
	s.WriteKeyVal( "NPC", m_Brain );

	if ( m_Home_Dist_Wander < SHRT_MAX )
		s.WriteKeyVal( "HOMEDIST", m_Home_Dist_Wander );
	if ( m_Act_Motivation )
		s.WriteKeyHex( "ACTPRI", m_Act_Motivation );

	m_Speech.r_Write( s, "SPEECH" );

	if ( m_SpeechHue != HUE_TEXT_DEF )
	{
		s.WriteKeyVal( "SPEECHCOLOR", m_SpeechHue );
	}

	if ( m_Need.GetResourceID().IsValidUID())
	{
		TCHAR *pszTmp = Str_GetTemp();
		m_Need.WriteKey( pszTmp );
		s.WriteKey( "NEED", pszTmp );
	}
}



