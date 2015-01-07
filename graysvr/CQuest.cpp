//
// cQuest.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.
#include "CClient.h"

//*****************************************************************
// -CCharRefArray

int CCharRefArray::FindChar( const CChar * pChar ) const
{
	if ( pChar == NULL )
	{
		return( -1 );
	}
	CGrayUID uid( pChar->GetUID());
	int iQty = m_uidCharArray.GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		if ( uid == m_uidCharArray[i] )
			return( i );
	}
	return( -1 );
}

int CCharRefArray::AttachChar( const CChar * pChar )
{
	int i = FindChar( pChar );
	if ( i >= 0 )
		return( i );
	return m_uidCharArray.Add( pChar->GetUID());
}

void CCharRefArray::DetachChar( int i )
{
	m_uidCharArray.RemoveAt(i);
}

int CCharRefArray::DetachChar( const CChar * pChar )
{
	int i = FindChar( pChar );
	if ( i < 0 )
		return( -1 );
	DetachChar( i );
	return( i );
}

void CCharRefArray::DeleteChars()
{
	int iQty = m_uidCharArray.GetCount();
	for ( int k=iQty-1; k >= 0; --k )
	{
		CChar * pChar = m_uidCharArray[k].CharFind();
		if ( pChar )
		{
			delete pChar;	//
		}
	}
	m_uidCharArray.RemoveAll();
}


void CCharRefArray::WritePartyChars( CScript & s )
{
	int iQty = m_uidCharArray.GetCount();
	for ( int j=0; j<iQty; j++ )	// write out links to all my chars
	{
		s.WriteKeyHex( "CHARUID", m_uidCharArray[j] );
	}
}

//*****************************************************************
// -CPartyDef

CPartyDef::CPartyDef( CChar * pChar1, CChar *pChar2 )
{
	// pChar1 = the master.
	ASSERT(pChar1);
	ASSERT(pChar2);
	pChar1->m_pParty = this;
	pChar2->m_pParty = this;
	AttachChar(pChar1);
	AttachChar(pChar2);
	SendAddList( NULL );	// send full list to all.
	m_sName.Format("Party_0%x", (DWORD)pChar1->GetUID());
}

// ---------------------------------------------------------
int CPartyDef::AttachChar( CChar * pChar )
{
	// RETURN:
	//  index of the char in the group. -1 = not in group.
	int i = m_Chars.AttachChar( pChar );
	pChar->SetKeyNum("PARTY_CANLOOTME", 0);
	return( i );
}

int CPartyDef::DetachChar( CChar * pChar )
{
	// RETURN:
	//  index of the char in the group. -1 = not in group.
	int i = m_Chars.DetachChar( pChar );
	// TODO: Party: If not in the group, why i should delete the key?
	if ( i != -1 )
	{
		CVarDefBase * sTempVal = pChar->GetTagDefs()->GetKey("PARTY_CANLOOTME");
		if ( sTempVal )
			pChar->DeleteKey("PARTY_CANLOOTME");

		sTempVal = pChar->GetTagDefs()->GetKey("PARTY_LASTINVITE");
		if ( sTempVal )
			pChar->DeleteKey("PARTY_LASTINVITE");
	}
	return( i );
}

void CPartyDef::SetLootFlag( CChar * pChar, bool fSet )
{
	ASSERT( pChar );
	int i = m_Chars.FindChar( pChar );
	pChar->SetKeyNum("PARTY_CANLOOTME", fSet);
	SysMessageChar( pChar, fSet ? g_Cfg.GetDefaultMsg( DEFMSG_PARTY_LOOT_ALLOW ) :
									g_Cfg.GetDefaultMsg( DEFMSG_PARTY_LOOT_BLOCK ));
}

bool CPartyDef::GetLootFlag( const CChar * pChar )
{
	int i = m_Chars.FindChar( pChar );
	return (atoi(pChar->GetKeyStr("PARTY_CANLOOTME")) != 0);
}

// ---------------------------------------------------------
int CPartyDef::CraftAddList( CExtData * pExtData )
{
	pExtData->Party_Msg_Data.m_code = PARTYMSG_Add;

	int iQty;
	iQty = m_Chars.GetCharCount();
	
	if ( !iQty )
		return( -1 );

	for ( int i = 0; i < iQty; i++ )
	{
		pExtData->Party_Msg_Data.m_uids[i] = m_Chars.GetChar(i);
	}

	pExtData->Party_Msg_Data.m_Qty = iQty;

	return( (iQty*sizeof(DWORD)) + 2 );
}

int CPartyDef::CraftEmptyList( CExtData * pExtData, CChar * pChar )
{
	pExtData->Party_Msg_Data.m_code = PARTYMSG_Remove;
	pExtData->Party_Msg_Data.m_uids[0] = pChar->GetUID();
	pExtData->Party_Msg_Data.m_Qty = 0;

	return( (sizeof(DWORD)) + 2 );
}

int CPartyDef::CraftRemoveList( CExtData * pExtData, CChar * pChar )
{
	pExtData->Party_Msg_Data.m_code = PARTYMSG_Remove;
	pExtData->Party_Msg_Data.m_uids[0] = pChar->GetUID();

	int iQty;
	iQty = m_Chars.GetCharCount();
	
	if ( !iQty )
	{
		return( -1 );
	}

	int x = 1;
	for ( int i = 0; i < iQty; i++ )
	{
		if ( (m_Chars.GetChar(i)) && ( pChar->GetUID() != m_Chars.GetChar(i) ) )
		{
			pExtData->Party_Msg_Data.m_uids[x] = m_Chars.GetChar(i);
			x++;
		}
	}

	pExtData->Party_Msg_Data.m_Qty = x;

	return( (x*sizeof(DWORD)) + 2 );
}

int CPartyDef::CraftMessage( CExtData * pExtData, CChar * pFrom, const NCHAR * pText, int len )
{
	pExtData->Party_Msg_Rsp.m_code = PARTYMSG_Msg;
	pExtData->Party_Msg_Rsp.m_UID = pFrom->GetUID();

	if ( len > MAX_TALK_BUFFER )
		len = MAX_TALK_BUFFER;
	
	memcpy( pExtData->Party_Msg_Rsp.m_msg, pText, len );

	return( len + 5 );
}
int CPartyDef::CraftMessageFromChar( CExtData * pExtData, CChar * pFrom, LPCTSTR pText )
{
	if ( pText == NULL )
		return( -1 );

	NCHAR szBuffer[ MAX_TALK_BUFFER ];
	int len = CvtSystemToNUNICODE( szBuffer, COUNTOF(szBuffer), pText, -1 );

	if ( len > 0 )
		return CraftMessage( pExtData, pFrom, (const NCHAR *) &szBuffer, len );

	return( -1 );
}

// ---------------------------------------------------------
void CPartyDef::AddStatsUpdate( CChar * pChar, CCommand * cmd, int iLen )
{
	int iQty;
	iQty = m_Chars.GetCharCount();

	if ( !iQty )
		return;

	for ( int i = 0; i < iQty; i++ )
	{
		CChar * pCharNow = m_Chars.GetChar(i).CharFind();
		if ( pCharNow && pCharNow != pChar )
		{
			if ( pCharNow->CanSee( pChar ) && pCharNow->IsClient() )
			{
				pCharNow->GetClient()->xSendPkt(cmd, iLen);
			}
		}
	}
}
// ---------------------------------------------------------
void CPartyDef::SysMessageStatic( CChar * pChar, LPCTSTR pText )
{
	// SysMessage to a member/or not with [PARTY]:
	ASSERT(pChar);
	if ( pChar->IsClient() )
	{
		CClient * pClient = pChar->GetClient();
		ASSERT(pClient);
		TCHAR * pCharTemp = Str_GetTemp();
		sprintf(pCharTemp, "[PARTY]: %s", pText);
		pClient->addBark(pCharTemp, NULL, HUE_GREEN_LIGHT, TALKMODE_SYSTEM, FONT_NORMAL);
	}
}

void CPartyDef::SysMessageAll( LPCTSTR pText )
{
	// SysMessage to all members of the party.
	int iQty = m_Chars.GetCharCount();
	for ( int i=0; i<iQty; i++ )
	{
		CChar * pChar = m_Chars.GetChar(i).CharFind();
		SysMessageChar( pChar, pText );
	}
}


// ---------------------------------------------------------
bool CPartyDef::SendMemberMsg( CChar * pCharDest, const CExtData * pExtData, int iLen )
{
	if ( pCharDest == NULL )
	{
		SendAll( pExtData, iLen );
		return( true );
	}

	// TODO: Party: ReOrganize safe checks:
	//					- If pCharDest->m_pParty is null, return false
	//					- If pCharDest->m_pParty != this, return (Detach(pCharDest) == -1) (and set it null ?)
	//					- If pCharDest->m_pParty == this AND pCharDest NOT IN this, return true

	// Weirdness check.
	if ( pCharDest->m_pParty != this )
	{
		if ( DetachChar( pCharDest ) >= 0 )	// this is bad!
			return( false );
		return( true );
	}
	else if ( ! m_Chars.IsCharIn( pCharDest ))
	{
		pCharDest->m_pParty = NULL;
		return( true );
	}

	if ( pCharDest->IsClient())
	{
		CClient * pClient = pCharDest->GetClient();
		ASSERT(pClient);
		pClient->addExtData( EXTDATA_Party_Msg, pExtData, iLen );
		if ( pExtData->Party_Msg_Data.m_code == PARTYMSG_Remove )
		{
			pClient->addReSync();
		}
	}

	return( true );
}

void CPartyDef::SendAll( const CExtData * pExtData, int iLen )
{
	// Send this to all members of the party.
	int iQty = m_Chars.GetCharCount();
	for ( int i = 0; i < iQty; i++ )
	{
		CChar * pChar = m_Chars.GetChar(i).CharFind();
		ASSERT(pChar);
		if ( ! SendMemberMsg( pChar, pExtData, iLen ))
		{
			iQty--;
			i--;
		}
	}
}

// ---------------------------------------------------------
bool CPartyDef::SendAddList( CChar * pCharDest )
{
	CExtData extdata;
	int iLen = CraftAddList(&extdata);
	
	if (iLen != -1)
	{
		if ( pCharDest )
		{
			SendMemberMsg(pCharDest, &extdata, iLen);
		}
		else
		{
			SendAll(&extdata, iLen);
		}
	}

	return ( iLen != -1 );
}

bool CPartyDef::SendRemoveList( CChar * pCharRemove, bool bFor )
{
	CExtData ExtData;
	int iLen;

	if ( bFor )
	{
		iLen = CraftEmptyList( &ExtData, pCharRemove );
	}
	else
	{
		iLen = CraftRemoveList( &ExtData, pCharRemove );
	}

	if ( iLen == -1 )
		return( false );

	if ( bFor ) 
	{
		SendMemberMsg(pCharRemove, &ExtData, iLen);
	}
	else
	{
		SendAll( &ExtData, iLen );
	}

	return( true );
}

// ---------------------------------------------------------
bool CPartyDef::MessageEvent( CGrayUID uidDst, CGrayUID uidSrc, const NCHAR * pText, int ilenmsg )
{
	if ( pText == NULL )
		return( false );

	if ( uidDst && !IsInParty( uidDst.CharFind() ) )
		return( false );

	CChar * pFrom = uidSrc.CharFind();
	CChar * pTo = NULL;
	if ( uidDst != (DWORD) 0 )
		pTo = uidDst.CharFind();

	TCHAR * szText = Str_GetTemp();
	CvtNUNICODEToSystem( szText, MAX_TALK_BUFFER, pText, MAX_TALK_BUFFER );

	if ( g_Log.IsLoggedMask( LOGM_PLAYER_SPEAK ))
	{
		g_Log.Event( LOGM_PLAYER_SPEAK, "%x:'%s' Says '%s' in party to '%s'\n", pFrom->GetClient()->m_Socket.GetSocket(), pFrom->GetName(), szText, pTo ? pTo->GetName() : "all" );
	}

	CExtData extData;
	int iLen = CraftMessage( &extData, pFrom, pText, ilenmsg );
	if ( iLen == -1 )
		return( false );

	if ( pTo )
		SendMemberMsg( pTo, &extData, iLen );
	else
		SendAll( &extData, iLen );

	return( true );
}

/*
bool CPartyDef::MessageMember( CGrayUID uidTo, CGrayUID uidSrc, const NCHAR * pText, int ilenmsg )
{
	// Message to a single members of the party.
	if ( pText == NULL )
		return false;
	if ( g_Log.IsLoggedMask( LOGM_PLAYER_SPEAK ))
	{
		// g_Log.Event( LOGM_PLAYER_SPEAK, "%x:'%s' Says '%s' mode=%d\n", m_Socket.GetSocket(), m_pChar->GetName(), szText, mode );
	}

	CChar * pChar = uidTo.CharFind();
	if ( pChar == NULL )
		return( false );

	CExtData ExtData;
	ExtData.Party_Msg_Rsp.m_code = PARTYMSG_Msg;
	ExtData.Party_Msg_Rsp.m_UID = uidSrc;
	if ( ilenmsg > MAX_TALK_BUFFER )
		ilenmsg = MAX_TALK_BUFFER;
	memcpy( ExtData.Party_Msg_Rsp.m_msg, pText, ilenmsg );

	return SendMemberMsg( pChar, &ExtData, ilenmsg+5 );
}

void CPartyDef::MessageAll( CGrayUID uidSrc, const NCHAR * pText, int ilenmsg )
{
	// Message to all members of the party.

	if ( pText == NULL )
		return;

	if ( g_Log.IsLoggedMask( LOGM_PLAYER_SPEAK ))
	{
		// g_Log.Event( LOGM_PLAYER_SPEAK, "%x:'%s' Says '%s' mode=%d\n", m_Socket.GetSocket(), m_pChar->GetName(), szText, mode );
	}

	CExtData ExtData;
	ExtData.Party_Msg_Rsp.m_code = PARTYMSG_Msg;
	ExtData.Party_Msg_Rsp.m_UID = uidSrc;
	if ( ilenmsg > MAX_TALK_BUFFER )
		ilenmsg = MAX_TALK_BUFFER;
	memcpy( ExtData.Party_Msg_Rsp.m_msg, pText, ilenmsg );

	SendAll( &ExtData, ilenmsg+5 );
}
*/
// ---------------------------------------------------------
void CPartyDef::AcceptMember( CChar * pChar )
{
	// This person has accepted to be part of the party.
	ASSERT(pChar);

	// SendAddList( pChar->GetUID(), NULL );	// tell all that there is a new member.

	pChar->m_pParty = this;
	AttachChar(pChar);

	// "You have been added to the party"
	// NOTE: We don't have to send the full party to everyone. just the new guy.
	// SendAddList( UID_CLEAR, pChar );
	// SendAddList( pChar->GetUID(), NULL );
	// SendAddList( UID_CLEAR, NULL );

	SendAddList( NULL );
	// else
	//	throwerror !!
}


bool CPartyDef::RemoveMember( CGrayUID uidRemove, CGrayUID uidCommand )
{
	// ARGS:
	//  uid = Who is being removed.
	//  uidAct = who removed this person. (Only the master or self can remove)
	//
	// NOTE: remove of the master will cause the party to disband.

	if ( ! m_Chars.GetCharCount())
	{
		return( false );
	}

	CGrayUID uidMaster = GetMaster();
	if ( uidRemove != uidCommand && uidCommand != uidMaster )
	{
		return( false );
	}

	CChar * pCharRemove = uidRemove.CharFind();

	if ( pCharRemove == NULL )
	{
		return( false );
	}

	if ( !IsInParty(pCharRemove) )
	{
		return( false );
	}

	if ( uidRemove == uidMaster )
	{
		return( Disband( uidMaster ));
	}

	LPCTSTR pszForceMsg = ( (DWORD) uidCommand != (DWORD) uidRemove ) ? g_Cfg.GetDefaultMsg( DEFMSG_PARTY_PART_1 ) : g_Cfg.GetDefaultMsg( DEFMSG_PARTY_PART_2 );

	{
		// Tell the kicked person they are out
		TCHAR *pszMsg = Str_GetTemp();
		sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_PARTY_LEAVE_2 ), pszForceMsg );
		SysMessageChar(pCharRemove, pszMsg);
		// Remove it
		SendRemoveList( pCharRemove, true );
		DetachChar( pCharRemove );
		pCharRemove->m_pParty = NULL;
	}

	if ( m_Chars.GetCharCount() <= 1 )
	{
		// Disband the party
		// "The last person has left the party"
		return( Disband( uidMaster ) );
	}
	else
	{
		SendRemoveList( pCharRemove, false );
		// Tell the others he is gone
		TCHAR *pszMsg = Str_GetTemp();
		sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_PARTY_LEAVE_1 ), (LPCTSTR) pCharRemove->GetName(), (LPCTSTR) pszForceMsg);
		SysMessageAll(pszMsg);
	}

	return( true );
}

bool CPartyDef::Disband( CGrayUID uidMaster )
{
	// Make sure i am the master.
	if ( ! m_Chars.GetCharCount())
	{
		return( false );
	}

	if ( GetMaster() != uidMaster )
	{
		return( false );
	}

	SysMessageAll(g_Cfg.GetDefaultMsg( DEFMSG_PARTY_DISBANDED ));

	int iQty = m_Chars.GetCharCount();
	ASSERT(iQty);
	for ( int i=0; i<iQty; i++ )
	{
		CChar * pChar = m_Chars.GetChar(i).CharFind();
		if ( pChar == NULL )
			continue;
		SendRemoveList( pChar, true );
		pChar->m_pParty = NULL;
	}

	delete this;	// should remove itself from the world list.
	return( true );
}

// ---------------------------------------------------------
bool CPartyDef::DeclineEvent( CChar * pCharDecline, CGrayUID uidInviter )	// static
{
	// This should happen after a timeout as well.
	// " You notify %s that you do not wish to join the party"

	// TODO: Party: Send a notice to, only if the prop is setted
	ASSERT( pCharDecline );

	CChar * pCharInviter = uidInviter.CharFind();
	if ( pCharInviter == NULL )
	{
		return( false );
	}

	if ( uidInviter == pCharDecline->GetUID() )
	{
		return( false );
	}

	CVarDefBase * sTempVal = pCharInviter->GetTagDefs()->GetKey("PARTY_LASTINVITE");
	if ( !sTempVal )
		return( false );

	if ((DWORD)sTempVal->GetValNum() != (DWORD)pCharDecline->GetUID())
		return( false );

	// Remove the key
	pCharInviter->DeleteKey("PARTY_LASTINVITE");

	TCHAR * sTemp = Str_GetTemp();
	sprintf(sTemp, "You notify %s that you do not wish to join the party", (LPCTSTR) pCharInviter->GetName() );
	CPartyDef::SysMessageStatic( pCharDecline, sTemp );
	sTemp = Str_GetTemp();
	sprintf(sTemp, "%s do not wish to join your party", (LPCTSTR) pCharInviter->GetName() );
	CPartyDef::SysMessageStatic( pCharInviter, sTemp );

	return( true );
}

bool CPartyDef::AcceptEvent( CChar * pCharAccept, CGrayUID uidInviter, bool bForced )	// static
{
	// We are accepting the invite to join a party
	// Check to see if the invite is genuine. ??? No security Here !!!

	// Party master is only one that can add ! GetChar(0)

	ASSERT( pCharAccept );

	CChar * pCharInviter = uidInviter.CharFind();
	if ( pCharInviter == NULL )
	{
		return( false );
	}
	if ( pCharInviter == pCharAccept )
	{
		return( false );
	}
	if ( !pCharInviter->IsClient() || !pCharAccept->IsClient() )
	{
		return( false );
	}

	CPartyDef * pParty = pCharInviter->m_pParty;
	if ( !bForced )
	{
		CVarDefBase * sTempVal = pCharInviter->GetTagDefs()->GetKey("PARTY_LASTINVITE");
		if ( !sTempVal )
			return( false );

		if ((DWORD)sTempVal->GetValNum() != (DWORD)pCharAccept->GetUID())
			return( false );

		// Remove the key
		pCharInviter->DeleteKey("PARTY_LASTINVITE");

		if ( !pCharInviter->CanSee( pCharAccept ) )
			return( false );
	}

	if ( pCharAccept->m_pParty != NULL )	// Aready in a party !
	{
		if ( pParty == pCharAccept->m_pParty )	// already in this party
			return( true );

		if ( bForced )
		{
			pCharAccept->m_pParty->RemoveMember( pCharAccept->GetUID(), pCharAccept->GetUID() );
			DEBUG_CHECK(pCharAccept->m_pParty == NULL );
			pCharAccept->m_pParty = NULL;
		}
		else
		{
			return( false );
		}
	}

	TCHAR *pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_PARTY_JOINED ), (LPCTSTR) pCharAccept->GetName());

	if ( pParty == NULL )
	{
		// create the party now.
		pParty = new CPartyDef( pCharInviter, pCharAccept );
		ASSERT(pParty);
		g_World.m_Parties.InsertHead( pParty );
		pParty->SysMessageChar(pCharInviter, pszMsg);
	}
	else
	{
		if ( !bForced )
		{
			if ( !pParty->IsPartyMaster(pCharInviter) )
				return( false );

			if ( pParty->IsPartyFull() )
				return( false );
		}
		// Just add to existing party. Only the party master can do this !
		pParty->SysMessageAll(pszMsg);	// tell everyone already in the party about this.
		pParty->AcceptMember( pCharAccept );
	}

	pParty->SysMessageChar( pCharInviter, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_ADDED) );
	return( true );
}

// ---------------------------------------------------------

enum PDV_TYPE
{
	PDV_ADDMEMBER,
	PDV_ADDMEMBERFORCED,
	PDV_CLEARTAGS,
	PDV_DISBAND,
	PDV_MASTER,
	PDV_MESSAGE,
	PDV_REMOVEMEMBER,
	PDV_SYSMESSAGE,
	PDV_TAGLIST,
	PDV_QTY,
};

LPCTSTR const CPartyDef::sm_szVerbKeys[PDV_QTY+1] =
{
	"ADDMEMBER",
	"ADDMEMBERFORCED",
	"CLEARTAGS",
	"DISBAND",
	"MASTER",
	"MESSAGE",
	"REMOVEMEMBER",
	"SYSMESSAGE",
	"TAGLIST",
	NULL,
};

enum PDC_TYPE
{
	PDC_MASTER,
	PDC_MEMBERS,
	PDC_TAG,
	PDC_TAG0,
	PDC_TAGAT,
	PDC_TAGCOUNT,
	PDC_QTY,
};

LPCTSTR const CPartyDef::sm_szLoadKeys[PDC_QTY+1] =
{
	"MASTER",
	"MEMBERS",
	"TAG",
	"TAG0",
	"TAGAT",
	"TAGCOUNT",
	NULL,
};

enum PDCM_TYPE
{
	PDCM_ISMASTER,
	PDCM_ISLOOTABLE,
	PDCM_QTY,
};

LPCTSTR const CPartyDef::sm_szLoadKeysM[PDCM_QTY+1] =
{
	"ISMASTER",
	"ISLOOTABLE",
	NULL,
};

void CPartyDef::r_DumpLoadKeys( CTextConsole * pSrc )
{ 
	r_DumpKeys(pSrc, sm_szLoadKeys);
}

void CPartyDef::r_DumpVerbKeys( CTextConsole * pSrc )
{ 
	r_DumpKeys(pSrc, sm_szVerbKeys); 
}

bool CPartyDef::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef ) 
{
	return( false );
}

bool CPartyDef::r_LoadVal( CScript & s )
{ 
	LOCKDATA;
	EXC_TRY(("r_LoadVal('%s %s')", s.GetKey(), s.GetArgStr()));
	switch (FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case PDC_TAG0:
		{
			bool fQuoted = false;
			m_TagDefs.SetStr( s.GetKey()+ 5, fQuoted, s.GetArgStr( &fQuoted ), true );
			return( true );
		}
		case PDC_TAG:
		{
			bool fQuoted = false;
			m_TagDefs.SetStr( s.GetKey()+ 4, fQuoted, s.GetArgStr( &fQuoted ));
			return( true );
		}
		default:
			DEBUG_CHECK(0);
			return( false );
	}
	return( true );
	EXC_CATCH("CPartyDef");
	return( false );
}

bool CPartyDef::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	LOCKDATA;
	EXC_TRY(("r_WriteVal('%s',,%x)", pszKey, pSrc));

	if (!strnicmp("MEMBER.",pszKey,7))
	{
		LPCTSTR pszCmd = pszKey+7;

		int nNumber = Exp_GetVal(pszCmd);
		SKIP_SEPERATORS(pszCmd);

		if ( nNumber < 0 || nNumber > m_Chars.GetCharCount() )
			return( false );

		CChar * pMember = m_Chars.GetChar(nNumber).CharFind();

		if ( pMember == NULL )
			return( false );

		sVal.FormatVal(0);

		switch ( FindTableSorted( pszCmd, sm_szLoadKeysM, COUNTOF( sm_szLoadKeysM )-1 ))
		{
			case PDCM_ISMASTER:
			{
				sVal.FormatVal( IsPartyMaster( pMember ) );
				break;
			}
			case PDCM_ISLOOTABLE:
			{
				sVal.FormatVal( GetLootFlag( pMember ) );
				break;		
			}
			default:
			{
				return( pMember->r_WriteVal(pszKey,sVal,pSrc) );
			}
		}
		return( true );
	}

	bool fZero = false;
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
		case PDC_MASTER:
		{
			pszKey += 6;
			SKIP_SEPERATORS(pszKey);

			CChar *pRef = GetMaster().CharFind();
			if ( pRef )
			{
				return( pRef->r_WriteVal(pszKey,sVal,pSrc) );
			}
			return( false );
		}
		case PDC_MEMBERS:
		{
			sVal.FormatVal(m_Chars.GetCharCount());
			return( true );
		}
		case PDC_TAG0:
			fZero	= true;
			pszKey++;
		case PDC_TAG:
		{
			if ( pszKey[3] != '.' )
				return( false );
			pszKey += 4;
			sVal = m_TagDefs.GetKeyStr( pszKey, fZero );
			return( true );
		}
		case PDC_TAGAT:
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
		case PDC_TAGCOUNT:
		{
			sVal.FormatVal( m_TagDefs.GetCount() );
			break;
		}
		default:
			DEBUG_CHECK(0);
			return( false );
	}

	return( true );
	EXC_CATCH("CPartyDef");
	return( false );
}

bool CPartyDef::r_Verb( CScript & s, CTextConsole * pSrc )
{ 
	LOCKDATA;
	EXC_TRY(("r_Verb('%s %s',%x)", s.GetKey(), s.GetArgStr(), pSrc));
	ASSERT(pSrc);

	if (!strnicmp("MEMBER.",s.GetKey(),7))
	{
		LPCTSTR pszKey = s.GetKey();
		pszKey += 7;

		int nNumber = Exp_GetVal(pszKey);
		SKIP_SEPERATORS(pszKey);

		if ( nNumber < 0 || nNumber > m_Chars.GetCharCount() )
			return( false );

		CChar * pMember = m_Chars.GetChar(nNumber).CharFind();

		if ( pMember == NULL )
			return( false );

		CScript s_Script(pszKey, s.GetArgRaw());

		return( pMember->r_Verb(s_Script,pSrc) );
	}

	int iIndex = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys)-1 );

	switch ( iIndex )
	{
		case PDV_ADDMEMBER:
		case PDV_ADDMEMBERFORCED:
		{
			bool bForced = ( iIndex == PDV_ADDMEMBERFORCED);
			CGrayUID toAdd = (DWORD) s.GetArgVal();
			CChar * pCharAdd = toAdd.CharFind();

			if (( !pCharAdd ) && ( IsInParty( pCharAdd ) ))
			{
				return( false );
			}

			if ( !bForced )
			{
				(GetMaster().CharFind())->SetKeyNum("PARTY_LASTINVITE", (DWORD) toAdd);
			}
			
			return( CPartyDef::AcceptEvent( pCharAdd, GetMaster(), bForced ) );
		}
		case PDV_CLEARTAGS:
		{
			LPCTSTR pszArg = s.GetArgStr();
			SKIP_SEPERATORS(pszArg);
			m_TagDefs.ClearKeys(pszArg);
			return( true );
		}
		case PDV_DISBAND:
		{
			return( Disband( GetMaster() ) );
		}
		case PDV_MASTER:
		{
			LPCTSTR pszKey = s.GetKey();
			pszKey += 6;
			SKIP_SEPERATORS(pszKey);

			CChar * pMaster = GetMaster().CharFind();

			if ( pMaster == NULL )
				return( false );
			
			CScript s_Script(pszKey, s.GetArgRaw());
			return( pMaster->r_Verb(s_Script,pSrc) );
		}
		case PDV_MESSAGE:
		{
			break;
		}
		case PDV_REMOVEMEMBER:
		{
			CGrayUID toRemove;
			LPCTSTR pszArg = s.GetArgStr();
			if ( *pszArg == '@' )
			{
				pszArg++;
				int nMember = Exp_GetVal(pszArg);
				if ( nMember < 0 || nMember > m_Chars.GetCharCount() )
					return( false );

				toRemove = m_Chars.GetChar(nMember);
			}
			else
			{
				toRemove = (DWORD) s.GetArgVal();
			}

			if ( toRemove != (DWORD) 0 )
				return( RemoveMember( toRemove, GetMaster() ) );

			return( false );
		}
		case PDV_SYSMESSAGE:
		{
			CGrayUID toSysmessage;
			LPCTSTR pszArg = s.GetArgStr();
			TCHAR *pUid = Str_GetTemp();
			int x(0);

			if ( *pszArg == '@' )
			{
				pszArg++;
				if ( *pszArg != '@' )
				{
					LPCTSTR __pszArg = pszArg;
					while ( *pszArg != ' ' ) { pszArg++; x++; }
					strcpylen(pUid, __pszArg, ++x);

					int nMember = Exp_GetVal(pUid);
					if ( nMember < 0 || nMember > m_Chars.GetCharCount() )
						return( false );

					toSysmessage = m_Chars.GetChar(nMember);
				}
			}
			else
			{
				LPCTSTR __pszArg = pszArg;
				while ( *pszArg != ' ' ) { pszArg++; x++; }
				strcpylen(pUid, __pszArg, ++x);

				toSysmessage = (DWORD) Exp_GetVal(pUid);
			}

			SKIP_SEPERATORS( pszArg );

			if ( toSysmessage != (DWORD) 0 )
			{
				CChar * pSend = toSysmessage.CharFind();
				SysMessageChar( pSend, pszArg );
			}
			else
			{
				SysMessageAll( pszArg );
			}

			break;
		}
		case PDV_TAGLIST:
		{
			m_TagDefs.DumpKeys( pSrc, "TAG." );
			return( true );
		}
		default:
			DEBUG_CHECK(0);
			return( false );
	}
	return( true );
	EXC_CATCH("CPartyDef");
	return( false );
}

bool CPartyDef::r_Load( CScript & s )
{ 
	return( false ); 
}