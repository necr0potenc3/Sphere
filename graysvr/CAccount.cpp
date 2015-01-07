//
// CAccount.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.
#include "../common/CMD5.h"

extern "C"
{
void globalstartsymbol()	// put this here as just the starting offset.
{
}
const int globalstartdata = 0xffffffff;
}

//**********************************************************************
// -CAccounts

bool CAccounts::Account_Load( LPCTSTR pszNameRaw, CScript & s, bool fChanges )
{
	// NOTE: Account names should not have spaces !!!
	// ARGS:
	//   fChanges = false = trap duplicates

	if ( !strcmpi(pszNameRaw, "ACCOUNT") )
	{
		// get rid of the header.
		pszNameRaw = s.GetArgStr();
		if ( !strcmpi(pszNameRaw, "ACCOUNT") )
			return false;
	}

	TCHAR szName[MAX_ACCOUNT_NAME_SIZE];
	if ( !CAccount::NameStrip(szName, pszNameRaw) )
	{
		if ( !fChanges )
		{
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name" DEBUG_CR, pszNameRaw);
			return false;
		}
	}
	m_fLoading = true;

	CAccountRef pAccount = Account_Find(szName);
	if ( pAccount )
	{
		// Look for existing duplicate ?
		if ( !fChanges )
		{
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Account '%s': duplicate name" DEBUG_CR, pszNameRaw);
			return false;
		}
	}
	else
	{
		pAccount = new CAccount(szName);
		ASSERT(pAccount);
	}
	pAccount->r_Load(s);

	m_fLoading = false;

	return true;
}

bool CAccounts::Account_LoadAll( bool fChanges, bool fClearChanges )
{
	// Load the accounts file. (at start up)
	LPCTSTR pszBaseDir;
	LPCTSTR pszBaseName;
	char	*z = Str_GetTemp();

	if ( g_Cfg.m_sAcctBaseDir.IsEmpty() ) pszBaseDir = g_Cfg.m_sWorldBaseDir;
	else pszBaseDir = g_Cfg.m_sAcctBaseDir;

	if ( fChanges ) pszBaseName = GRAY_FILE "acct"; // Open script file
	else pszBaseName = GRAY_FILE "accu";

	strcpy(z, pszBaseDir);
	strcat(z, pszBaseName);

	CScript s;
	if ( ! s.Open(z, OF_READ|OF_TEXT| ( fChanges ? OF_NONCRIT : 0) ))
	{
		if ( !fChanges )
		{
			if ( Account_LoadAll(true) )	// if we have changes then we are ok.
				return true;

											//	auto-creating account files
			if ( !Account_SaveAll() )
				g_Log.Event(LOGL_FATAL|LOGM_INIT, "Can't open account file '%s'" DEBUG_CR, s.GetFilePath());
			else
				return true;
		}
		return false;
	}

	if ( fClearChanges )
	{
		ASSERT( fChanges );
		// empty the changes file.
		s.Close();
		s.Open(NULL, OF_WRITE|OF_TEXT);
		s.WriteString( "// Accounts are periodically moved to the " GRAY_FILE "accu" GRAY_SCRIPT " file." DEBUG_CR
			"// All account changes should be made here." DEBUG_CR
			"// Use the /ACCOUNT UPDATE command to force accounts to update." DEBUG_CR
			);
		return true;
	}

	CScriptFileContext ScriptContext(&s);
	while (s.FindNextSection())
	{
		Account_Load(s.GetKey(), s, fChanges);
	}

	if ( !fChanges ) Account_LoadAll(true);

	return true;
}

bool CAccounts::Account_SaveAll()
{
	EXC_TRY(("Account_SaveAll()"));
	// Look for changes FIRST.
	Account_LoadAll(true);

	LPCTSTR pszBaseDir;

	if ( g_Cfg.m_sAcctBaseDir.IsEmpty() ) pszBaseDir = g_Cfg.m_sWorldBaseDir;
	else pszBaseDir = g_Cfg.m_sAcctBaseDir;

	CScript s;
	if ( !CWorld::OpenScriptBackup(s, pszBaseDir, "accu", g_World.m_iSaveCountID) )
		return false;

	s.Printf( "\\\\ %s %s accounts file" DEBUG_CR
		"\\\\ NOTE: This file cannot be edited while the server is running." DEBUG_CR
		"\\\\ Any file changes must be made to " GRAY_FILE "accu" GRAY_SCRIPT ". This is read in at save time." DEBUG_CR,
		(LPCTSTR)g_Cfg.m_sVerName.GetPtr(), g_Serv.GetName());

	CThreadLockRef lock( &m_Accounts );
	for ( int i=0; i < m_Accounts.GetCount(); i++ )
	{
		CAccountRef pAccount = Account_Get(i);
		if ( pAccount ) pAccount->r_Write(s, false);
	}

	Account_LoadAll(true, true);	// clear the change file now.
	return true;
	EXC_CATCH("CAccounts");
	return false;
}

CAccountRef CAccounts::Account_FindChat( LPCTSTR pszChatName )
{
	// IS this new chat name already used ?
	CThreadLockRef lock(&m_Accounts);
	for ( int i=0; i < m_Accounts.GetCount(); i++ )
	{
		CAccountRef pAccount = Account_Get(i);
		if ( pAccount )
			if ( !pAccount->m_sChatName.CompareNoCase(pszChatName) )
				return pAccount;
	}
	return NULL;
}

CAccountRef CAccounts::Account_Find( LPCTSTR pszName )
{
	TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];

	if ( !CAccount::NameStrip(szName, pszName) )
		return( NULL );

	CThreadLockRef lock( &m_Accounts );
	int i = m_Accounts.FindKey(szName);
	if ( i >= 0 )
		return Account_Get(i);

	return NULL;
}

CAccountRef CAccounts::Account_FindCreate( LPCTSTR pszName, bool fAutoCreate )
{
	// Find an account by this name.
	// Create one in some circumstances.

	CAccountRef pAccount = Account_Find( pszName );
	if ( pAccount )
		return( pAccount );

	if ( fAutoCreate )	// Create if not found.
	{
		bool fGuest = ( g_Serv.m_eAccApp == ACCAPP_GuestAuto || g_Serv.m_eAccApp == ACCAPP_GuestTrial || ! strnicmp( pszName, "GUEST", 5 ));
		TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];

		// NOTE: Account EOF fix
		if ( CAccount::NameStrip( szName, pszName ) )
		{
			return new CAccount( szName, fGuest );
		}
		else
		{
			g_Log.Event( LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name" DEBUG_CR, pszName );
			return false;
		}
	}

	return( NULL );
}

bool CAccounts::Account_Delete( CAccount * pAccount )
{
	m_Accounts.DeleteOb( pAccount );
	return true;
}
void CAccounts::Account_Add( CAccount * pAccount )
{
	m_Accounts.AddSortKey(pAccount,pAccount->GetName());
}

CAccountRef CAccounts::Account_Get( int index )
{
	if ( index >= m_Accounts.GetCount())
		return( NULL );
	return( CAccountRef( STATIC_CAST <CAccount *>( m_Accounts[index])));
}

bool CAccounts::Cmd_AddNew( CTextConsole * pSrc, bool fMail, LPCTSTR pszName, LPCTSTR pszArg )
{
	CAccountRef pAccount = Account_Find( pszName );
	if ( pAccount != NULL )
	{
		pSrc->SysMessagef( "Account '%s' already exists" DEBUG_CR, (LPCTSTR) pszName );
		return false;
	}
	
	TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];

	// NOTE: Account EOF fix
	if ( !CAccount::NameStrip( szName, pszName ) )
	{
		g_Log.Event( LOGL_ERROR|LOGM_INIT, "Account '%s': BAD name" DEBUG_CR, pszName );
		return false;
	}

	pAccount = new CAccount(szName);
	ASSERT(pAccount);
	pAccount->m_dateFirstConnect = pAccount->m_dateLastConnect = CGTime::GetCurrentTime();

	if ( ! fMail )
	{
		pAccount->SetPassword( pszArg );
		return( true );
	}

	// Send them email with the new password.

	return( true );
}

enum VACS_TYPE
{
	VACS_ADD,
	VACS_ADDMAIL,
	VACS_BLOCKED,
	VACS_HELP,
	VACS_JAILED,
	VACS_UNUSED,
	VACS_UPDATE,
	VACS_QTY,
};

LPCTSTR const CAccounts::sm_szVerbKeys[] =	// CAccounts:: // account group verbs.
{
	"ADD",
	"ADDMAIL",
	"BLOCKED",
	"HELP",
	"JAILED",
	"UNUSED",
	"UPDATE",
	NULL,
};

bool CAccounts::Cmd_ListUnused(CTextConsole * pSrc, LPCTSTR pszDays, LPCTSTR pszVerb, LPCTSTR pszArgs, DWORD dwMask)
{
	// do something to all the unused accounts.
	int iDaysTest = Exp_GetVal(pszDays);

	CGTime datetime = CGTime::GetCurrentTime();
	int iDaysCur = datetime.GetDaysTotal();

	int		iCountOrig	= Account_GetCount();
	int		iCountCheck	= iCountOrig;
	int		iCount		= 0;
	for ( int i=0; true; i++ )
	{
		if ( Account_GetCount() < iCountCheck )
		{
			iCountCheck--;
			i--;
		}
		CAccountRef pAccount = Account_Get(i);
		if ( !pAccount ) break;

		int iDaysAcc = pAccount->m_dateLastConnect.GetDaysTotal();
		if ( ! iDaysAcc )
		{
			// account has never been used ? (get create date instead)
			iDaysAcc = pAccount->m_dateFirstConnect.GetDaysTotal();
		}

		if ( iDaysCur - iDaysAcc < iDaysTest ) continue;
		if ( dwMask && !pAccount->IsPriv(dwMask) ) continue;

		iCount ++;
		if ( pszVerb == NULL || pszVerb[0] == '\0' )
		{
			// just list stuff about the account.
			CScript script("SHOW LASTCONNECTDATE");
			pAccount->r_Verb( script, pSrc );
			continue;
		}

		// can't delete unused priv accounts this way.
		if ( ! strcmpi( pszVerb, "DELETE" ) && pAccount->GetPrivLevel() > PLEVEL_Player )
		{
			iCount--;
			pSrc->SysMessagef( "Can't Delete PrivLevel %d Account '%s' this way." DEBUG_CR,
				pAccount->GetPrivLevel(), (LPCTSTR) pAccount->GetName() );
		}
		else
		{
			CScript script( pszVerb, pszArgs );
			pAccount->r_Verb( script, pSrc );
		}
	}

	pSrc->SysMessagef( "Matched %d of %d accounts unused for %d days" DEBUG_CR,
		iCount, iCountOrig, iDaysTest );

	if ( pszVerb && ! strcmpi(pszVerb, "DELETE") )
	{
		int	iDeleted = iCountOrig - Account_GetCount();

		if ( iDeleted < iCount )
			pSrc->SysMessagef("%d deleted, %d cleared of characters (must try to delete again)" DEBUG_CR,
				iDeleted, iCount - iDeleted );
		else if ( iDeleted > 0 )
			pSrc->SysMessagef("All %d unused accounts deleted." DEBUG_CR, iDeleted);
		else
			pSrc->SysMessagef("No accounts deleted." DEBUG_CR, iDeleted);
	}

	return true;
}

bool CAccounts::Account_OnCmd( TCHAR * pszArgs, CTextConsole * pSrc )
{
	// Modify the accounts on line. "ACCOUNT"
	ASSERT( pSrc );

	if ( pSrc->GetPrivLevel() < PLEVEL_Admin )
		return( false );

	TCHAR * ppCmd[5];
	int iQty = Str_ParseCmds( pszArgs, ppCmd, COUNTOF( ppCmd ));

	VACS_TYPE index;
	if ( iQty <= 0 ||
		ppCmd[0] == NULL ||
		ppCmd[0][0] == '\0' )
	{
		index = VACS_HELP;
	}
	else
	{
		index = (VACS_TYPE) FindTableSorted( ppCmd[0], sm_szVerbKeys, COUNTOF( sm_szVerbKeys )-1 );
	}

	static LPCTSTR const sm_pszCmds[] =
	{
		// "/ACCOUNT LIST" DEBUG_CR,
		"/ACCOUNT UPDATE" DEBUG_CR,
		"/ACCOUNT UNUSED days [command]" DEBUG_CR,
		"/ACCOUNT ADD name password",
		"/ACCOUNT name BLOCK 0/1" DEBUG_CR,
		"/ACCOUNT name JAIL 0/1" DEBUG_CR,
		"/ACCOUNT name DELETE = delete all chars and the account" DEBUG_CR,
		"/ACCOUNT name PLEVEL x = set account priv level" DEBUG_CR,
		"/ACCOUNT name EXPORT filename" DEBUG_CR,
		// "/ACCOUNT name DELETE x = delete this char",
		// "/ACCOUNT name SUMMON x = force this char to be on line and here.",
		// "/ACCOUNT name DISCONNECT = log out a summoned char.",
	};

	switch (index)
	{
	case VACS_ADD:
	case VACS_ADDMAIL:
		return Cmd_AddNew( pSrc, index == VACS_ADDMAIL, ppCmd[1], ppCmd[2] );

	case VACS_BLOCKED:
		return Cmd_ListUnused(pSrc, ppCmd[1], ppCmd[2], ppCmd[3], PRIV_BLOCKED);

	case VACS_HELP:
		{
			for ( int i=0; i<COUNTOF(sm_pszCmds); i++ )
			{
				pSrc->SysMessage( sm_pszCmds[i] );
			}
		}
		return true;
	case VACS_JAILED:
		return Cmd_ListUnused(pSrc, ppCmd[1], ppCmd[2], ppCmd[3], PRIV_JAILED);

	case VACS_UPDATE:
		Account_SaveAll();
		return true;

	case VACS_UNUSED:
		return Cmd_ListUnused( pSrc, ppCmd[1], ppCmd[2], ppCmd[3] );
	}

	// Must be a valid account ?

	CAccountRef pAccount = Account_Find( ppCmd[0] );
	if ( pAccount == NULL )
	{
		pSrc->SysMessagef( "Account '%s' does not exist" DEBUG_CR, ppCmd[0] );
		return false;
	}

	if ( !ppCmd[1] || !ppCmd[1][0] )
	{
		CClient	*pClient = pAccount->FindClient();

		char	*z = Str_GetTemp();
		sprintf(z, "Account '%s': PLEVEL:%d, BLOCK:%d, IP:%s, CONNECTED:%s, ONLINE:%s" DEBUG_CR,
			pAccount->GetName(), pAccount->GetPrivLevel(), pAccount->IsPriv(PRIV_BLOCKED),
			pAccount->m_Last_IP.GetAddrStr(), pAccount->m_dateLastConnect.Format(NULL),
			( pClient ? ( pClient->GetChar() ? pClient->GetChar()->GetName() : "<not logged>" ) : "no" ));
		pSrc->SysMessage(z);
		return true;
	}
	else
	{
		CScript script( ppCmd[1], ppCmd[2] );
		return pAccount->r_Verb( script, pSrc );
	}
}

//**********************************************************************
// -CAccount

int CAccount::NameStrip( TCHAR * pszNameOut, LPCTSTR pszNameInp ) // static
{
	// allow just basic chars. No spaces, only numbers, letters and underbar. -+. and single quotes ?
	int len = Str_GetBare( pszNameOut, pszNameInp, MAX_ACCOUNT_NAME_SIZE, " !\"#$%&()*,/:;<=>?@[\\]^{|}~" ); // "'`-.+" // seems people are using these.
	if ( g_Cfg.IsObscene( pszNameOut ))
	{
		// DEBUG_ERR(( "Obscene account '%s' ignored." DEBUG_CR, pszNameInp ));
		return 0;
	}
	else if( !strcmpi( pszNameOut, "eof" ) )
	{
		// DEBUG_ERR(( "EOF Account ignored: %s" DEBUG_CR, pszNameInp ));
		return 0;
	}

	return len;
}

static LPCTSTR const sm_szPrivLevels[ PLEVEL_QTY+1 ] =
{
	"Guest",		// 0 = This is just a guest account. (cannot PK)
	"Player",		// 1 = Player or NPC.
	"Counsel",		// 2 = Can travel and give advice.
	"Seer",			// 3 = Can make things and NPC's but not directly bother players.
	"GM",			// 4 = GM command clearance
	"Dev",			// 5 = Not bothererd by GM's
	"Admin",		// 6 = Can switch in and out of gm mode. assign GM's
	"Owner",		// 7 = Highest allowed level.
	NULL,
};

PLEVEL_TYPE CAccount::GetPrivLevelText( LPCTSTR pszFlags ) // static
{
	int level = FindTable( pszFlags, sm_szPrivLevels, COUNTOF(sm_szPrivLevels)-1 );
	if ( level >= 0 )
	{
		return( (PLEVEL_TYPE) level );
	}

	if ( ! strnicmp( pszFlags, "CO", 2 ))
		return PLEVEL_Counsel;

	level = Exp_GetVal( pszFlags );
	if ( level < 0 || level > PLEVEL_QTY )
		return( PLEVEL_Player );

	return( (PLEVEL_TYPE) level );
}

CAccount::CAccount( LPCTSTR pszName, bool fGuest )
{
	// Make sure the name is in valid format.
	// Assume the pszName has been stripped of all just !

	g_Serv.StatInc( SERV_STAT_ACCOUNTS );

	TCHAR szName[ MAX_ACCOUNT_NAME_SIZE ];
	CAccount::NameStrip( szName, pszName );
	m_sName = szName;

	ASSERT( ! m_sName.IsEmpty());
	ASSERT( ! ISWHITESPACE( m_sName[0] ));

	if ( !strnicmp(m_sName, "GUEST", 5) || fGuest )
		SetPrivLevel(PLEVEL_Guest);
	else
		SetPrivLevel(PLEVEL_Player);
	
	m_ResDisp = RDS_PRET2A;
	
	m_PrivFlags = PRIV_DETAIL;	// Set on by default for everyone.
	m_Total_Connect_Time = 0;
	m_Last_Connect_Time = 0;
	// Add myself to the list.
	g_Accounts.Account_Add( this );
}

void CAccount::DeleteChars()
{
	CClient * pClient = FindClient();
	if ( pClient )
	{	// we have no choice but to kick them.
		pClient->m_fClosed	= true;
	}

	// Now track down all my disconnected chars !
	if ( ! g_Serv.IsLoading())
	{
		m_Chars.DeleteChars();
	}
}


CAccount::~CAccount()
{
	// We should go track down and delete all the chars and clients that use this account !

	g_Serv.StatDec( SERV_STAT_ACCOUNTS );

	DEBUG_WARN(( "Account delete '%s'" DEBUG_CR, GetName() ));
	DeleteChars();
}


void CAccount::SetPrivLevel( PLEVEL_TYPE plevel )
{
	m_PrivLevel = plevel;	// PLEVEL_Counsel
}

CClient * CAccount::FindClient( const CClient * pExclude ) const
{
	// Is the account logged in.
	if ( this == NULL )
		return( NULL );	// this might be possible.
	CClient * pClient = g_Serv.GetClientHead();
	for ( ; pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( pClient == pExclude )
			continue;
		if ( this == pClient->GetAccount())
			break;
	}
	return( pClient );
}

bool CAccount::IsMyAccountChar( const CChar * pChar ) const
{
	// this char is mine ?
	if ( pChar == NULL )
		return( false );
	if ( pChar->m_pPlayer == NULL )
		return( false );
	return(	pChar->m_pPlayer->GetAccount() == this );
}

int CAccount::DetachChar( CChar * pChar )
{
	// unlink the CChar from this CAccount.
	ASSERT( pChar );
	ASSERT( IsMyAccountChar( pChar ));

	if ( m_uidLastChar == pChar->GetUID())
	{
		m_uidLastChar.InitUID();
	}

	return( m_Chars.DetachChar( pChar ));
}

int CAccount::AttachChar( CChar * pChar )
{
	// link the char to this account.
	ASSERT(pChar);
	ASSERT( IsMyAccountChar( pChar ));

	// is it already linked ?
	int i = m_Chars.AttachChar( pChar );
	if ( i >= 0 )
	{
		int iQty = m_Chars.GetCharCount();
		if ( iQty > MAX_CHARS_PER_ACCT )
		{
			g_Log.Event( LOGM_ACCOUNTS|LOGL_ERROR, "Account '%s' has %d characters" DEBUG_CR, (LPCTSTR) GetName(), iQty );
		}
	}

	return( i );
}

void CAccount::TogPrivFlags( WORD wPrivFlags, LPCTSTR pszArgs )
{
	// s.GetArgFlag
	// No args = toggle the flag.
	// 1 = set the flag.
	// 0 = clear the flag.

	if ( pszArgs == NULL || pszArgs[0] == '\0' )	// toggle.
	{
		m_PrivFlags ^= wPrivFlags;
	}
	else if ( Exp_GetVal( pszArgs ))
	{
		m_PrivFlags |= wPrivFlags;
	}
	else
	{
		m_PrivFlags &= ~wPrivFlags;
	}
}

void CAccount::OnLogin( CClient * pClient )
{
	// The account just logged in.

	ASSERT(pClient);
	pClient->m_timeLogin = CServTime::GetCurrentTime();	// g_World clock of login time. "LASTCONNECTTIME"

	// CSocketAddress PeerName = pClient->m_Socket.GetPeerName();

	if ( GetPrivLevel() >= PLEVEL_Counsel )	// ON by default.
	{
		m_PrivFlags |= PRIV_GM_PAGE;
	}

	// Get the real world time/date.
	CGTime datetime = CGTime::GetCurrentTime();

	if ( !m_Total_Connect_Time )	// first time - save first ip and timestamp
	{
		m_First_IP = pClient->m_PeerName;
		m_dateFirstConnect = datetime;
	}

	m_Last_IP = pClient->m_PeerName;
	// LASTCONNECTDATE= date / time of this connect.
	m_TagDefs.SetStr("LastLogged", false, m_dateLastConnect.Format(NULL));
	m_dateLastConnect = datetime;

	if ( pClient->GetConnectType() == CONNECT_TELNET )
	{
		// link the admin client.
		g_Serv.m_iAdminClients++;
	}
	if ( GetPrivLevel() <= PLEVEL_Guest )
	{
		g_Serv.m_nGuestsCur ++;
	}

	g_Log.Event( LOGM_CLIENTS_LOG, "%x:Login '%s'" DEBUG_CR, pClient->m_Socket.GetSocket(), (LPCTSTR) GetName());
	g_Log.FireEvent( LOGEVENT_ClientLogin, pClient->m_Socket.GetSocket());
}

void CAccount::OnLogout(CClient *pClient, bool bWasChar)
{
	// CClient is disconnecting from this CAccount.
	ASSERT(pClient);

	if ( pClient->GetConnectType() == CONNECT_TELNET )// unlink the admin client.
		g_Serv.m_iAdminClients --;

	if ( GetPrivLevel() <= PLEVEL_Guest )			// Free up a guest slot.
		g_Serv.m_nGuestsCur--;	
													// calculate total game time. skip this calculation in
													// case if it was login type packet. it has the same type,
													// so we should check whatever player is attached to a char
	if ( pClient->IsConnectTypePacket() && bWasChar )
	{
		m_Last_Connect_Time = ( -g_World.GetTimeDiff(pClient->m_timeLogin) ) / ( TICK_PER_SEC * 60 );
		if ( m_Last_Connect_Time < 0 )
			m_Last_Connect_Time = 0;

		m_Total_Connect_Time += m_Last_Connect_Time;
	}

	g_Log.FireEvent(LOGEVENT_ClientDetach, pClient->m_Socket.GetSocket());
}

bool CAccount::Kick( CTextConsole * pSrc, bool fBlock )
{
	if ( GetPrivLevel() >= pSrc->GetPrivLevel())
	{
		pSrc->SysMessageDefault(DEFMSG_ACC_PRIV);
		return( false );
	}

	if ( fBlock )
	{
		SetPrivFlags( PRIV_BLOCKED );
		pSrc->SysMessagef( g_Cfg.GetDefaultMsg(DEFMSG_ACC_BLOCK), (LPCTSTR) GetName() );
	}

	LPCTSTR pszAction = fBlock ? "KICK" : "DISCONNECTED";

	char *z = Str_GetTemp();
	sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_ACC_KICK), GetName(), pszAction, pSrc->GetName());
	g_Log.Event(LOGL_EVENT|LOGM_GM_CMDS, "%s" DEBUG_CR, z);
	
	return true;
}

bool CAccount::CheckPassword( LPCTSTR pszPassword )
{
	// RETURN:
	//  false = failure.
	ASSERT(!IsPriv( PRIV_BLOCKED ));
	ASSERT(pszPassword);

	if ( m_sCurPassword.IsEmpty())
	{
		// First guy in sets the password.
		// check the account in use first.
		if ( *pszPassword == '\0' )
			return( false );

		SetPassword( pszPassword );
		return( true );
	}

	// Get the password.
	if( g_Cfg.m_fMd5Passwords )
	{
		// Hash the Password provided by the user and compare the hashes
		char digest[33];
		CMD5::fastDigest( digest, pszPassword );

		if( !strcmpi( digest, GetPassword() ) )
			return( true );
	}
	else
	{
		if ( ! strcmpi( GetPassword(), pszPassword ) )
			return( true );
	}

	if ( ! m_sNewPassword.IsEmpty() && ! strcmpi( GetNewPassword(), pszPassword ))
	{
		// using the new password.
		// kill the old password.
		SetPassword( m_sNewPassword );
		m_sNewPassword.Empty();
		return( true );
	}

	return( false );	// failure.
}

void CAccount::SetPassword( LPCTSTR pszPassword )
{
    if ( Str_Check( pszPassword ) )
        return;

	if( g_Cfg.m_fMd5Passwords )
	{
		char digest[33];

		// Auto-Hash if not loading or reloading
		if( !g_Accounts.m_fLoading )
		{
			CMD5::fastDigest( digest, pszPassword );
			m_sCurPassword = digest;
		}

		// Automatically convert the password into a hash if it's coming from the files
		// and shorter or longer than 32 characters
		else
		{
			if( strlen( pszPassword ) == 32 )
			{
				m_sCurPassword = pszPassword;
			}
			else
			{
				// Autoconverted a Password on Load. Print
				g_Log.Event( LOGM_INIT|LOGL_EVENT, "MD5: Converted password for '%s'.\n", GetName() );
				CMD5::fastDigest( digest, pszPassword );
				m_sCurPassword = digest;
			}
		}
	}
	else
	{
		m_sCurPassword = pszPassword;

		// limit to 16 chars.
		if ( m_sCurPassword.GetLength() > MAX_ACCOUNT_PASSWORD_ENTER )
		{
			m_sCurPassword.SetLength( MAX_ACCOUNT_PASSWORD_ENTER );
		}
	}
}

void CAccount::SetNewPassword( LPCTSTR pszPassword )
{
	if ( !pszPassword || !pszPassword[0] )		// no password given, auto-generate password
	{
		static char const passwdChars[] = "ABCDEFGHJKLMNPQRTUVWXYZ2346789";
		int	len = strlen(passwdChars);
		int charsCnt = Calc_GetRandVal(4) + 6;	// 6 - 10 chars
		if ( charsCnt > MAX_ACCOUNT_PASSWORD_ENTER-1 )
			charsCnt = MAX_ACCOUNT_PASSWORD_ENTER-1;
		char	szTmp[MAX_ACCOUNT_PASSWORD_ENTER + 1];

		for ( int i = 0; i < charsCnt; i++ )
			szTmp[i] = passwdChars[Calc_GetRandVal(len)];

		szTmp[charsCnt] = 0;
		m_sNewPassword = szTmp;
		return;
	}

	m_sNewPassword = pszPassword;
	if ( m_sNewPassword.GetLength() > MAX_ACCOUNT_PASSWORD_ENTER )
		m_sNewPassword.SetLength(MAX_ACCOUNT_PASSWORD_ENTER);
}

bool CAccount::SetEmailAddress( LPCTSTR pszEmail, bool fBlockHost )
{
	if ( !pszEmail || fBlockHost )
		return false;

	GETNONWHITESPACE(pszEmail);
	if ( !g_Cfg.IsValidEmailAddressFormat(pszEmail) )
		return false;

	m_sEMail = pszEmail;
	return true;
}

enum AC_TYPE
{
	AC_ACCOUNT,
	AC_BLOCK,
	AC_CHARS,
	AC_CHARUID,
	AC_CHATNAME,
	AC_EMAIL,
	AC_FIRSTCONNECTDATE,
	AC_FIRSTIP,
	AC_GUEST,
	AC_JAIL,
	AC_LANG,
	AC_LASTCHARUID,
	AC_LASTCONNECTDATE,
	AC_LASTCONNECTTIME,
	AC_LASTIP,
	AC_NAME,
	AC_NEWPASSWORD,
	AC_PASSWORD,
	AC_PLEVEL,
	AC_PRIV,
	AC_RESDISP,
	AC_TAG,
	AC_TAG0,
	AC_TAGCOUNT,
	AC_TOTALCONNECTTIME,
	AC_QTY,
};

LPCTSTR const CAccount::sm_szLoadKeys[AC_QTY+1] = // static
{
	"ACCOUNT",
	"BLOCK",
	"CHARS",
	"CHARUID",
	"CHATNAME",
	"EMAIL",
	"FIRSTCONNECTDATE",
	"FIRSTIP",
	"GUEST",
	"JAIL",
	"LANG",
	"LASTCHARUID",
	"LASTCONNECTDATE",
	"LASTCONNECTTIME",
	"LASTIP",
	"NAME",
	"NEWPASSWORD",
	"PASSWORD",
	"PLEVEL",
	"PRIV",
	"RESDISP",
	"TAG",
	"TAG0",
	"TAGCOUNT",
	"TOTALCONNECTTIME",
	NULL,
};

void CAccount::r_DumpLoadKeys( CTextConsole * pSrc )
{
	r_DumpKeys(pSrc,sm_szLoadKeys);
}
void CAccount::r_DumpVerbKeys( CTextConsole * pSrc )
{
	r_DumpKeys(pSrc,sm_szVerbKeys);
}

bool CAccount::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	if ( ! strnicmp( pszKey, "CHAR.", 5 ))
	{
		// How many chars.
		pszKey += 5;
		int i=Exp_GetVal(pszKey);
		if ( i>=0 && i<m_Chars.GetCharCount())
		{
			pRef = m_Chars.GetChar(i).CharFind();
		}
		SKIP_SEPERATORS(pszKey);
		return( true );
	}
	return( CScriptObj::r_GetRef( pszKey, pRef ));
}

bool CAccount::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	EXC_TRY(("r_WriteVal('%s',,%x)", pszKey, pSrc));
	if ( ! pSrc )
	{
		return( false );
	}

	bool	fZero	= false;

	switch ( FindTableHeadSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case AC_NAME:
	case AC_ACCOUNT:
		sVal = m_sName;
		break;
	case AC_CHARS:
		sVal.FormatVal( m_Chars.GetCharCount());
		break;
	case AC_BLOCK:
		sVal.FormatVal( IsPriv( PRIV_BLOCKED ));
		break;
	case AC_CHATNAME:
		sVal = m_sChatName;
		break;
	case AC_EMAIL:
		sVal = m_sEMail;
		break;
	case AC_TAGCOUNT:
		sVal.FormatVal( m_TagDefs.GetCount() );
		break;
	case AC_FIRSTCONNECTDATE:
		sVal = m_dateFirstConnect.Format(NULL);
		break;
	case AC_FIRSTIP:
		sVal = m_First_IP.GetAddrStr();
		break;
	case AC_GUEST:
		sVal.FormatVal( GetPrivLevel() == PLEVEL_Guest );
		break;
	case AC_JAIL:
		sVal.FormatVal( IsPriv( PRIV_JAILED ));
		break;
	case AC_LANG:
		sVal = m_lang.GetStr();
		break;
	case AC_LASTCHARUID:
		sVal.FormatHex( m_uidLastChar );
		break;
	case AC_LASTCONNECTDATE:
		sVal = m_dateLastConnect.Format(NULL);
		break;
	case AC_LASTCONNECTTIME:
		sVal.FormatVal( m_Last_Connect_Time );
		break;
	case AC_LASTIP:
		sVal = m_Last_IP.GetAddrStr();
		break;
	case AC_PLEVEL:
		sVal.FormatVal( m_PrivLevel );
		break;
	case AC_NEWPASSWORD:
		if ( pSrc->GetPrivLevel() < PLEVEL_Admin ||
			pSrc->GetPrivLevel() < GetPrivLevel())	// can't see accounts higher than you
		{
			return( false );
		}
		sVal = GetNewPassword();
		break;
	case AC_PASSWORD:
		if ( pSrc->GetPrivLevel() < PLEVEL_Admin ||
			pSrc->GetPrivLevel() < GetPrivLevel())	// can't see accounts higher than you
		{
			return( false );
		}
		sVal = GetPassword();
		break;
	case AC_PRIV:
		sVal.FormatHex( m_PrivFlags );
		break;
	case AC_RESDISP:
		sVal.FormatVal( m_ResDisp );
		break;
	case AC_TAG0:
		fZero	= true;
		pszKey++;
	case AC_TAG:			// "TAG" = get/set a local tag.
		{
			if ( pszKey[3] != '.' )
				return( false );
			pszKey += 4;
			sVal = m_TagDefs.GetKeyStr(pszKey, fZero );
			return( true );
		}
	case AC_TOTALCONNECTTIME:
		sVal.FormatVal( m_Total_Connect_Time );
		break;

	default:
		return( CScriptObj::r_WriteVal( pszKey, sVal, pSrc ));
	}
	return true;
	EXC_CATCH("CAccount");
	return false;
}

bool CAccount::r_LoadVal( CScript & s )
{
	LOCKDATA;
	EXC_TRY(("r_LoadVal('%s %s')", s.GetKey(), s.GetArgStr()));
	int i = FindTableHeadSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 );
	if ( i < 0 )
	{
		return( false );
	}

	switch ( i )
	{
	case AC_ACCOUNT: // can't be set this way.
	case AC_NAME: // can't be set this way.
		return( false );
	case AC_BLOCK:
		if ( ! s.HasArgs() || s.GetArgVal())
		{
			SetPrivFlags( PRIV_BLOCKED );
		}
		else
		{
			ClearPrivFlags( PRIV_BLOCKED );
		}
		break;
	case AC_CHARUID:
		// just ignore this ? chars are loaded later !
		if ( ! g_Serv.IsLoading())
		{
			CGrayUID uid( s.GetArgVal());
			CChar * pChar = uid.CharFind();
			if (pChar == NULL)
			{
				DEBUG_ERR(( "Invalid CHARUID 0%x for account '%s'" DEBUG_CR, (DWORD) uid, (LPCTSTR) GetName()));
				return( false );
			}
			if ( ! IsMyAccountChar( pChar ))
			{
				DEBUG_ERR(( "CHARUID 0%x (%s) not attached to account '%s'" DEBUG_CR, (DWORD) uid, (LPCTSTR) pChar->GetName(), (LPCTSTR) GetName()));
				return( false );
			}
			AttachChar(pChar);
		}
		break;
	case AC_CHATNAME:
		m_sChatName = s.GetArgStr();
		break;
	case AC_EMAIL:
		m_sEMail = s.GetArgStr();
		break;
	case AC_FIRSTCONNECTDATE:
		m_dateFirstConnect.Read( s.GetArgStr());
		break;
	case AC_FIRSTIP:
		m_First_IP.SetAddrStr( s.GetArgStr());
		break;
	case AC_GUEST:
		if ( ! s.HasArgs() || s.GetArgVal())
		{
			SetPrivLevel( PLEVEL_Guest );
		}
		else
		{
			if ( GetPrivLevel() == PLEVEL_Guest )
				SetPrivLevel( PLEVEL_Player );
		}
		break;
	case AC_JAIL:
		if ( ! s.HasArgs() || s.GetArgVal())
		{
			SetPrivFlags( PRIV_JAILED );
		}
		else
		{
			ClearPrivFlags( PRIV_JAILED );
		}
		break;
	case AC_LANG:
		m_lang.Set( s.GetArgStr());
		break;
	case AC_LASTCHARUID:
		m_uidLastChar = s.GetArgVal();
		break;
	case AC_LASTCONNECTDATE:
		m_dateLastConnect.Read( s.GetArgStr());
		break;
	case AC_LASTCONNECTTIME:
		// Previous total amount of time in game
		m_Last_Connect_Time = s.GetArgVal();
		break;
	case AC_LASTIP:
		m_Last_IP.SetAddrStr( s.GetArgStr());
		break;
	case AC_PLEVEL:
		SetPrivLevel( GetPrivLevelText( s.GetArgRaw()));
		break;
	case AC_NEWPASSWORD:
		SetNewPassword( s.GetArgStr());
		break;
	case AC_PASSWORD:
		SetPassword( s.GetArgStr() );
		break;
	case AC_PRIV:
		m_PrivFlags = s.GetArgVal();
		if ( m_PrivFlags & PRIV_UNUSED )
		{
			g_Log.EventError("Fixing PRIV field (0%x) for account %s have not supported flags set (caught by mask 0%x)." DEBUG_CR, m_PrivFlags, GetName(), (WORD)PRIV_UNUSED);
			m_PrivFlags &= ~PRIV_UNUSED;
		}
		break;
	case AC_RESDISP:
		{
			if ( !SetResDisp(s.GetArgVal()) )
				m_ResDisp = RDS_T2A;
		}
		break;
	case AC_TAG0:
		{
			bool fQuoted = false;
			m_TagDefs.SetStr( s.GetKey()+ 5, fQuoted, s.GetArgStr( &fQuoted ), true );
		}
		return( true );
	case AC_TAG:
		{
			bool fQuoted = false;
			m_TagDefs.SetStr( s.GetKey()+ 4, fQuoted, s.GetArgStr( &fQuoted ));
		}
		return( true );

	case AC_TOTALCONNECTTIME:
		// Previous total amount of time in game
		m_Total_Connect_Time = s.GetArgVal();
		break;

	default:
		DEBUG_CHECK(0);
		return( false );
	}
	return true;
	EXC_CATCH("CAccount");
	return false;
}

void CAccount::r_Write( CScript & s, bool fTitle )
{
	if ( GetPrivLevel() >= PLEVEL_QTY )
		return;

	// NOTE: This should have "ACCOUNT" section title !

	if ( fTitle )
	{
		s.WriteSection( "ACCOUNT %s", (LPCTSTR) m_sName );
	}
	else
	{
		s.WriteSection( m_sName );
	}

	if ( GetPrivLevel() != PLEVEL_Player )
	{
		s.WriteKey( "PLEVEL", sm_szPrivLevels[ GetPrivLevel() ] );
	}
	if ( m_PrivFlags != PRIV_DETAIL )
	{
		s.WriteKeyHex( "PRIV", m_PrivFlags &~( PRIV_BLOCKED | PRIV_JAILED ));
	}
	if ( !IsResDisp( RDS_PRET2A ) )
	{
		s.WriteKeyVal( "RESDISP", GetResDisp() );
	}
	if ( IsPriv( PRIV_JAILED ))
	{
		s.WriteKeyVal( "JAIL", 1 );
	}
	if ( IsPriv( PRIV_BLOCKED ))
	{
		s.WriteKeyVal( "BLOCK", 1 );
	}
	if ( ! m_sCurPassword.IsEmpty())
	{
		s.WriteKey( "PASSWORD", GetPassword() );
	}
	if ( ! m_sNewPassword.IsEmpty())
	{
		s.WriteKey( "NEWPASSWORD", GetNewPassword() );
	}
	if ( m_Total_Connect_Time )
	{
		s.WriteKeyVal( "TOTALCONNECTTIME", m_Total_Connect_Time );
	}
	if ( m_Last_Connect_Time )
	{
		s.WriteKeyVal( "LASTCONNECTTIME", m_Last_Connect_Time );
	}
	if ( m_uidLastChar.IsValidUID())
	{
		s.WriteKeyHex( "LASTCHARUID", m_uidLastChar );
	}

	m_Chars.WritePartyChars(s);

	if ( ! m_sEMail.IsEmpty())
	{
		s.WriteKey( "EMAIL", m_sEMail );
	}
	if ( m_dateFirstConnect.IsTimeValid())
	{
		s.WriteKey( "FIRSTCONNECTDATE", m_dateFirstConnect.Format(NULL));
	}
	if ( m_First_IP.IsValidAddr() )
	{
		s.WriteKey( "FIRSTIP", m_First_IP.GetAddrStr());
	}

	if ( m_dateLastConnect.IsTimeValid())
	{
		s.WriteKey( "LASTCONNECTDATE", m_dateLastConnect.Format(NULL));
	}
	if ( m_Last_IP.IsValidAddr() )
	{
		s.WriteKey( "LASTIP", m_Last_IP.GetAddrStr());
	}
	if ( ! m_sChatName.IsEmpty())
	{
		s.WriteKey( "CHATNAME", (LPCTSTR) m_sChatName );
	}
	if ( m_lang.IsDef())
	{
		s.WriteKey( "LANG", m_lang.GetStr());
	}

	m_TagDefs.r_WriteTags( s );
}

enum AV_TYPE
{
	AV_BLOCK,
	AV_DELETE,
	AV_EMAIL,
	AV_EXPORT,
	AV_KICK,
	AV_TAGLIST,
};

LPCTSTR const CAccount::sm_szVerbKeys[] =
{
	"BLOCK",
	"DELETE",
	"EMAIL",
	"EXPORT",
	"KICK",
	"TAGLIST",
	NULL,
};

bool CAccount::r_Verb( CScript &s, CTextConsole * pSrc )
{
	LOCKDATA;
	EXC_TRY(("r_Verb('%s %s',%x)", s.GetKey(), s.GetArgStr(), pSrc));
	ASSERT(pSrc);
	if ( pSrc->GetPrivLevel() < PLEVEL_Admin ||
		pSrc->GetPrivLevel() < GetPrivLevel())	// can't change accounts higher than you in any way
	{
		// Priv Level too low
		return( false );
	}

	const char *pszKey = s.GetKey();
	if ( !strnicmp(pszKey, "CLEARTAGS", 9) )
	{
		pszKey = s.GetArgStr();
		SKIP_SEPERATORS(pszKey);
		m_TagDefs.ClearKeys(pszKey);
		return true;
	}

	int i = FindTableSorted( s.GetKey(), sm_szVerbKeys, COUNTOF( sm_szVerbKeys )-1 );
	if ( i < 0 )
	{
		return( CScriptObj::r_Verb( s, pSrc ));
	}

	switch ( i )
	{
	case AV_DELETE: // "DELETE"
		{
			char *z = Str_GetTemp();

			sprintf(z, "Account %s deleted." DEBUG_CR, m_sName);
			if ( !g_Accounts.Account_Delete(this) )
				sprintf(z, "Characters from account %s has been deleted." DEBUG_CR, m_sName);

			g_Log.EventStr(0, z);
			if ( pSrc != &g_Serv )
				pSrc->SysMessage(z);

			return true;
		}
	case AV_EMAIL:
		// Enter the email address , but filtered.
		if ( s.HasArgs())
		{
			if ( ! SetEmailAddress( s.GetArgStr(), true ) || Str_Check(s.GetArgStr()) )
			{
				pSrc->SysMessagef( g_Cfg.GetDefaultMsg(DEFMSG_ACC_EMAIL_FAIL), (LPCTSTR) s.GetArgStr());
				return false;
			}
		}
		pSrc->SysMessagef( g_Cfg.GetDefaultMsg(DEFMSG_ACC_EMAIL_SUCCESS),
			(LPCTSTR) GetName(), (LPCTSTR) m_sEMail );
		return( true );

	case AV_BLOCK:
		if ( ! s.HasArgs() || s.GetArgVal())
		{
			SetPrivFlags(PRIV_BLOCKED);
		}
		else
		{
			ClearPrivFlags(PRIV_BLOCKED);
			return true;
		}
		// fall through

	case AV_KICK:
		// if they are online right now then kick them!
		{
			CClient * pClient = FindClient();
			if ( pClient )
			{
				pClient->addKick( pSrc, i == AV_BLOCK );
			}
			else
			{
				Kick( pSrc, i == AV_BLOCK );
			}
		}
		return( true );

	case AV_EXPORT:
		// Export this account out to this file name.
		// Write out the account info too ?
		{
			CScript sOut;
			if ( ! sOut.Open( s.GetArgStr(), OF_WRITE|OF_CREATE|OF_TEXT ))
				return( false );

			r_Write( sOut, true );

			// now write out all the chars.
			int iQty = m_Chars.GetCharCount();
			for ( int i=0; i<iQty; i++ )
			{
				CGrayUID uid = m_Chars.GetChar(i);
				CChar * pChar = uid.CharFind();
				if ( pChar == NULL )
					continue;
				pChar->r_WriteSafe( sOut );
			}

			sOut.WriteSection( "EOF" );
		}
		return( true );

	case AV_TAGLIST:
		m_TagDefs.DumpKeys( pSrc, "TAG." );
		return( true );
	}
	EXC_CATCH("CAccount");
	return false;
}
