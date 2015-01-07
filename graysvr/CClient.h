//
// CClient.h
//

#ifndef _INC_CCLIENT_H
#define _INC_CCLIENT_H
#pragma once

enum CV_TYPE
{
	CV_ADD,
	CV_ADMIN,
	CV_ARROWQUEST,
	CV_BADSPAWN,
	CV_BANKSELF,
	CV_CAST,
	CV_CHARLIST,
	CV_EVERBTARG,
	CV_EXTRACT,
	CV_FLUSH,
	CV_GMPAGE,
	CV_GOTARG,
	CV_HELP,
	CV_INFO,
	CV_INFORMATION,
	CV_LAST,
	CV_LINK,
	CV_LOGIN,
	CV_LOGOUT,
	CV_MENU,
	CV_MIDILIST,
	CV_NUDGE,
	CV_NUKE,
	CV_NUKECHAR,
	CV_ONECLICK,
	CV_PAGE,
	CV_REPAIR,
	CV_RESEND,
	CV_SAVE,
	CV_SCROLL,
	CV_SELF,
	CV_SENDPACKET,
	CV_SHOWSKILLS,
	CV_SKILLMENU,
	CV_SKILLSELECT,
	CV_STATIC,
	CV_SMSG,
	CV_SMSGU,
	CV_SUMMON,
	CV_SYSMESSAGE,
	CV_SYSMESSAGEUA,
	CV_TELE,
	CV_TILE,
	CV_UNEXTRACT,
	CV_VERSION,
	CV_WEBLINK,
	CV_QTY,
};

enum CC_TYPE
{
	CC_ALLMOVE,
	CC_ALLSHOW,
	CC_CLIENTVERSION,
	CC_DEBUG,
	CC_DETAIL,
	CC_GM,				// (R/W)
	CC_HEARALL,
	CC_PRIVSHOW,
	CC_REPORTEDCLIVER,
	CC_TARG,
	CC_TARGP,
	CC_TARGPROP,
	CC_TARGPRV,
	CC_TARGTXT,
	CC_QTY,
};

class CPartyDef : public CGObListRec, public CScriptObj
{
	// a list of characters in the party.
	#define MAX_CHAR_IN_PARTY 10

public:
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szLoadKeysM[];

protected:
	DECLARE_MEM_DYNAMIC;
private:
	CCharRefArray m_Chars;
	CGString m_sName;
	CVarDefArray m_TagDefs;

private:
	bool SendMemberMsg( CChar * pCharDest, const CExtData * pExtData, int iLen );
	void SendAll( const CExtData * pExtData, int iLen );
	// Packet Crafting
	int CraftAddList( CExtData * pExtData );
	int CraftEmptyList( CExtData * pExtData, CChar * pChar );
	int CraftRemoveList( CExtData * pExtData, CChar * pChar );
	int CraftMessage( CExtData * pExtData, CChar * pFrom, const NCHAR * pText, int len );
	int CraftMessageFromChar( CExtData * pExtData, CChar * pFrom, LPCTSTR pText );
	// List manipulation
	int AttachChar( CChar * pChar );
	int DetachChar( CChar * pChar );

public:
	CPartyDef( CChar * pCharInvite, CChar * pCharAccept );

	static bool AcceptEvent( CChar * pCharAccept, CGrayUID uidInviter, bool bForced = false );
	static bool DeclineEvent( CChar * pCharDecline, CGrayUID uidInviter );
	static void SysMessageStatic( CChar * pChar, LPCTSTR pText );

	bool IsPartyFull() const
	{
		return( m_Chars.GetCharCount() >= MAX_CHAR_IN_PARTY );
	}
	bool IsInParty( const CChar * pChar ) const
	{
		int i = m_Chars.FindChar( pChar );
		return( i >= 0 );
	}
	bool IsPartyMaster( const CChar * pChar ) const
	{
		int i = m_Chars.FindChar( pChar );
		return( i == 0 );
	}

	CGrayUID GetMaster() 
	{ 
		return( m_Chars.GetChar(0) ); 
	}

	
	// Refresh status for party members
	void AddStatsUpdate( CChar * pChar, CCommand * cmd, int iLen );
	// List sending wrappers
	bool SendRemoveList( CChar * pCharRemove, bool bFor );
	bool SendAddList( CChar * pCharDest );
	// Party message sending wrappers
	bool MessageEvent( CGrayUID uidDst, CGrayUID uidSrc, const NCHAR * pText, int ilenmsg );
	// void MessageAll( CGrayUID uidSrc, const NCHAR * pText, int ilenmsg );
	// bool MessageMember( CGrayUID uidDst, CGrayUID uidSrc, const NCHAR * pText, int ilenmsg );
	// Sysmessage sending wrappers
	void SysMessageAll( LPCTSTR pText );
	void SysMessageChar( CChar * pChar, LPCTSTR pText ) { CPartyDef::SysMessageStatic( pChar, pText); };

	// Commands
	bool Disband( CGrayUID uidMaster );
	bool RemoveMember( CGrayUID uidRemove, CGrayUID uidCommand );
	void AcceptMember( CChar * pChar );
	void SetLootFlag( CChar * pChar, bool fSet );
	bool GetLootFlag( const CChar * pChar );
	
	// -------------------------------

	LPCTSTR GetName() const { return (LPCTSTR)m_sName; }
	void r_DumpLoadKeys( CTextConsole * pSrc );
	void r_DumpVerbKeys( CTextConsole * pSrc );
	bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script
	bool r_LoadVal( CScript & s );
	bool r_Load( CScript & s );
};

#endif	// _INC_CCLIENT_H
