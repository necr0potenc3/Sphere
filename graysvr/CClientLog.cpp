//
// CClientLog.cpp
// Copyright Menace Software (www.menasoft.com).
//
// Login and low level stuff for the client.
//

#include "graysvr.h"	// predef header.

BYTE CClient::sm_xCompress_Buffer[MAX_BUFFER];	// static
CCompressTree CClient::sm_xComp;

/////////////////////////////////////////////////////////////////
// -CClient stuff.

int CClient::xDeCompress( BYTE * pOutput, const BYTE * pInput, int iLen ) // static
{
	if ( ! sm_xComp.IsLoaded())
	{
		if ( ! sm_xComp.Load())
			return( -1 );
	}
	return( sm_xComp.Decode( pOutput, pInput, iLen ));
}

int CClient::xCompress( BYTE * pOutput, const BYTE * pInput, int iLen ) // static
{
	// The game server will compress the outgoing data to the clients.
	return sm_xComp.Encode( pOutput, pInput, iLen );
}

CLogIP * CClient::GetLogIP() const
{
	// CSocketAddress PeerName = m_Socket.GetPeerName();
	if ( ! m_PeerName.IsValidAddr())	// can't get ip ? why ?
		return( NULL );
	return g_Cfg.FindLogIP( m_PeerName, true );
}

bool	CClient::IsConnecting()
{
	switch ( GetConnectType() )
	{
	case CONNECT_TELNET:
	case CONNECT_HTTP:
	case CONNECT_GAME:
		return false;
	}
	return true;
}


void	CClient::SetConnectType( CONNECT_TYPE iType )
{
	m_iConnectType	= iType;
	if ( iType == CONNECT_GAME )
		UpdateLogIPConnecting( false );
}
	

int	CClient::GetLogIPConnecting() const
{
	CLogIP *	pLogIP	= GetLogIP();
	if ( !pLogIP )	return 0;
	return pLogIP->m_iConnecting;
}


int	CClient::GetLogIPConnected() const
{
	CLogIP *	pLogIP	= GetLogIP();
	if ( !pLogIP )	return 0;
	return pLogIP->m_iConnected;
}


void	CClient::UpdateLogIPConnecting( bool fIncrease )
{
	CLogIP *	pLogIP	= GetLogIP();
	if ( !pLogIP )	return;
	if ( fIncrease )
		pLogIP->m_iConnecting++;
	else if ( pLogIP->m_iConnecting > 0 )
		pLogIP->m_iConnecting--;

}


void	CClient::UpdateLogIPConnected( bool fIncrease )
{
	CLogIP *	pLogIP	= GetLogIP();
	if ( !pLogIP )	return;
	if ( fIncrease )
		pLogIP->m_iConnected++;
	else if ( pLogIP->m_iConnected > 0 )
		pLogIP->m_iConnected--;
}


bool CClient::IsBlockedIP() const
{
	CLogIP * pLogIP;

	for ( int i=0; i < g_Cfg.m_LogIP.GetCount(); i++ )
	{
		pLogIP = g_Cfg.m_LogIP[i];

		if ( pLogIP->IsSameIP( m_PeerName ) )		// checked below
			continue;

		if ( pLogIP->IsMatchIP( m_PeerName ) )
		{
			if ( pLogIP->IsBlocked() )
				return true;
		}
	}

	pLogIP = GetLogIP();
	if ( pLogIP == NULL )
		return( true );
	return( pLogIP->CheckPingBlock( false ));
}

//---------------------------------------------------------------------
// Push world display data to this client only.

bool CClient::addLoginErr( LOGIN_ERR_TYPE code )
{
	// code
	// 0 = no account
	// 1 = account used.
	// 2 = blocked.
	// 3 = no password
	// LOGIN_ERR_OTHER

	if ( code == LOGIN_SUCCESS )
		return true;

	DEBUG_ERR(( "%x:Bad Login %d" DEBUG_CR, m_Socket.GetSocket(), code ));
	CCommand cmd;
	cmd.LogBad.m_Cmd = XCMD_LogBad;
	cmd.LogBad.m_code = code;
	xSendPkt( &cmd, sizeof( cmd.LogBad ));
	xFlush();
	m_fClosed	= true;
	return( false );
}


void CClient::addSysMessage( LPCTSTR pszMsg) // System message (In lower left corner)
{
	addBarkParse( pszMsg, NULL, HUE_TEXT_DEF, TALKMODE_SYSTEM, FONT_NORMAL );
}


void CClient::addWebLaunch( LPCTSTR pPage )
{
	// Direct client to a web page
	ASSERT(pPage);
	if ( pPage[0] == '\0' )
		return;

	//addSysMessage( "Launching your web browser. Please wait...");

	CCommand cmd;
	cmd.Web.m_Cmd = XCMD_Web;
	int iLen = sizeof(cmd.Web) + strlen(pPage);
	cmd.Web.m_len = iLen;
	strcpy( cmd.Web.m_page, pPage );
	xSendPkt( &cmd, iLen );
}

///////////////////////////////////////////////////////////////
// Login server.

bool CClient::addRelay( const CServerDef * pServ )
{
	// Tell the client to play on this server.

	ASSERT(pServ);
	CSocketAddressIP ipAddr = pServ->m_ip;

	if ( ipAddr.IsLocalAddr())	// local server address not yet filled in.
	{
		ipAddr = m_Socket.GetSockName();
		DEBUG_MSG(( "%x:Login_Relay to %s" DEBUG_CR, m_Socket.GetSocket(), ipAddr.GetAddrStr() ));
	}

	// CSocketAddress PeerName = m_Socket.GetPeerName();
	if ( m_PeerName.IsLocalAddr() || m_PeerName.IsSameIP( ipAddr ))	// weird problem with client relaying back to self.
	{
		DEBUG_MSG(( "%x:Login_Relay loopback to server %s" DEBUG_CR, m_Socket.GetSocket(), ipAddr.GetAddrStr() ));
		ipAddr.SetAddrIP( SOCKET_LOCAL_ADDRESS );
	}

	DWORD dwAddr = ipAddr.GetAddrIP();

	CCommand cmd;
	cmd.Relay.m_Cmd = XCMD_Relay;
	cmd.Relay.m_ip[3] = ( dwAddr >> 24 ) & 0xFF;
	cmd.Relay.m_ip[2] = ( dwAddr >> 16 ) & 0xFF;
	cmd.Relay.m_ip[1] = ( dwAddr >> 8  ) & 0xFF;
	cmd.Relay.m_ip[0] = ( dwAddr	   ) & 0xFF;
	cmd.Relay.m_port = pServ->m_ip.GetPort();
	cmd.Relay.m_Account = 0x7f000001; // customer account handshake. (don't bother to check this.)

	xSendPkt( &cmd, sizeof(cmd.Relay));
	xFlush();	// flush b4 we turn into a game server.

	m_Targ_Mode = CLIMODE_SETUP_RELAY;

	// just in case they are on the same machine, change over to the new game encrypt
	m_Crypt.InitFast( UNPACKDWORD( cmd.Relay.m_ip ), CONNECT_GAME ); // Init decryption table
	SetConnectType( m_Crypt.GetConnectType() );
	
	return( true );
}

bool CClient::Login_Relay( int iRelay ) // Relay player to a selected IP
{
	// Client wants to be relayed to another server. XCMD_ServerSelect
	// DEBUG_MSG(( "%x:Login_Relay" DEBUG_CR, GetSocket() ));
	// iRelay = 0 = this local server.

	if ( m_Crypt.GetClientVer() >= 0x126000 || IsNoCryptVer(1) )
	{
		// client list Gives us a 1 based index for some reason.
		iRelay --;
	}

	CServerRef pServ;
	if ( iRelay <= 0 )
	{
		pServ = &g_Serv;	// we always list ourself first.
	}
	else
	{
		iRelay --;
		pServ = g_Cfg.Server_GetDef(iRelay);
		if ( pServ == NULL )
		{
			DEBUG_ERR(( "%x:Login_Relay BAD index! %d" DEBUG_CR, m_Socket.GetSocket(), iRelay ));
			return( false );
		}
	}

	return addRelay( pServ );
}

LOGIN_ERR_TYPE CClient::Login_ServerList( const char * pszAccount, const char * pszPassword )
{
	// XCMD_ServersReq
	// Initial login (Login on "loginserver", new format)
	// If the messages are garbled make sure they are terminated to correct length.

	TCHAR szAccount[MAX_ACCOUNT_NAME_SIZE+3];
	int iLenAccount = Str_GetBare( szAccount, pszAccount, sizeof(szAccount)-1 );
	if ( iLenAccount > MAX_ACCOUNT_NAME_SIZE )
		return( LOGIN_ERR_OTHER );
	if ( iLenAccount != strlen(pszAccount))
		return( LOGIN_ERR_OTHER );

	TCHAR szPassword[MAX_NAME_SIZE+3];
	int iLenPassword = Str_GetBare( szPassword, pszPassword, sizeof( szPassword )-1 );
	if ( iLenPassword > MAX_NAME_SIZE )
		return( LOGIN_ERR_OTHER );
	if ( iLenPassword != strlen(pszPassword))
		return( LOGIN_ERR_OTHER );

	// Make sure the first server matches the GetSockName here
	if ( g_Log.IsLogged( LOGL_TRACE ))
	{
		DEBUG_MSG(( "%x:Login_ServerList to '%s','%s'" DEBUG_CR, m_Socket.GetSocket(), pszAccount, pszPassword ));
	}

	// don't bother logging in yet.
	// Give the server list to everyone.
	// if ( LogIn( pszAccount, pszPassword ) )
	//   return( LOGIN_ERR_BAD_PASS );
	CGString sMsg;
	LOGIN_ERR_TYPE lErr = LOGIN_ERR_OTHER;

	lErr = LogIn( pszAccount, pszPassword, sMsg );
	
	if ( lErr != LOGIN_SUCCESS )
	{
		if (  lErr != LOGIN_ERR_OTHER )
		{
			addLoginErr(lErr);
		}
		return( lErr );
	}

	CCommand cmd;
	cmd.ServerList.m_Cmd = XCMD_ServerList;

	int indexoffset = 1;
	if ( m_Crypt.GetClientVer() >= 0x126000 || IsNoCryptVer(1))
	{
		indexoffset = 2;
	}

	// always list myself first here.
	g_Serv.addToServersList( cmd, indexoffset-1, 0 );

	int j = 1;
	for ( int i=0; j < MAX_SERVERS; i++ )
	{
		CServerRef pServ = g_Cfg.Server_GetDef(i);
		if ( pServ == NULL )
			break;
		if ( g_Cfg.m_iPollServers && ! pServ->IsConnected())
			continue;
		pServ->addToServersList( cmd, i+indexoffset, j );
		j++;
	}

	int iLen = sizeof(cmd.ServerList) - sizeof(cmd.ServerList.m_serv) + ( j * sizeof(cmd.ServerList.m_serv[0]));
	cmd.ServerList.m_len = iLen;
	cmd.ServerList.m_count = j;
	cmd.ServerList.m_unk3 = 0xFF;
	xSendPkt( &cmd, iLen );

	m_Targ_Mode = CLIMODE_SETUP_SERVERS;
	return( LOGIN_SUCCESS );
}

//*****************************************

REGRES_TYPE CClient::OnRxAutoServerRegister( const BYTE * pData, int iLen )
{
	// I got the 0xFFFFFFFF header plus a 0 byte.
	// This means it is an auto server registration packet.

	ASSERT( GetConnectType() == CONNECT_AUTO_SERVER );

	// dump the client on return.

	if ( iLen <= 0  || iLen > SCRIPT_MAX_LINE_LEN )
		return REGRES_RET_INVALID;
	((TCHAR *)(pData))[iLen]='\0';

	// Server registration message.
	// Check to see if this server is already here.
	// CSocketAddress PeerName = m_Socket.GetPeerName();
	if ( ! m_PeerName.IsValidAddr() )
		return REGRES_RET_FAILURE;

	// Create a new entry for this. (so we can get it's name etc.)
	CServerRef pServNew = new CServerDef( NULL, m_PeerName );
	ASSERT( pServNew );
	pServNew->ParseStatus( (LPCTSTR)(pData), true );

	if ( pServNew->GetName()[0] == '\0' )
	{
		// There is no name here ?
		// Might we try to match by IP address ?

		for ( int i=0; true; i++ )
		{
			CServerRef pServTest = g_Cfg.Server_GetDef(i);
			if ( pServTest == NULL )
				break;
			if ( m_PeerName.IsSameIP( pServTest->m_ip ))
			{
				pServTest->ParseStatus( (char*)pData, true );
				delete pServNew;
				return REGRES_RET_OK;
			}
		}

		g_Log.Event( LOGL_WARN|LOGM_ACCOUNTS, "%x:Bad reg info, No server name '%s'" DEBUG_CR, m_Socket.GetSocket(), (LPCTSTR)pData );
		delete pServNew;
		return REGRES_RET_IP_UNK;
	}

	// Look up it's name.
	CThreadLockRef lock( &g_Cfg.m_Servers );
	int index = g_Cfg.m_Servers.FindKey( pServNew->GetName());
	if ( index < 0 )
	{
		// No server by this name
		if ( g_Cfg.m_sMainLogServerDir.IsEmpty())
			return( REGRES_RET_NAME_UNK );

		g_Log.Event( LOGL_EVENT|LOGM_ACCOUNTS, "%x:Adding Server '%s' to list." DEBUG_CR, 
			m_Socket.GetSocket(), (LPCTSTR) pServNew->GetName());

		// only the main login server will add automatically.
		g_Cfg.m_Servers.AddSortKey( pServNew, pServNew->GetName());
		return REGRES_RET_OK;
	}

	CServerRef pServName = g_Cfg.Server_GetDef(index);
	ASSERT( pServName );

	if ( ! pServName->IsSame( pServNew ) &&
		! m_PeerName.IsSameIP( pServName->m_ip ))
	{
		g_Log.Event( LOGL_WARN|LOGM_ACCOUNTS, "%x:Bad regpass '%s'!='%s' for server '%s'" DEBUG_CR, m_Socket.GetSocket(), (LPCTSTR) pServNew->m_sRegisterPassword, (LPCTSTR) pServName->m_sRegisterPassword, (LPCTSTR) pServName->GetName());
		delete pServNew;
		return REGRES_RET_BAD_PASS;
	}

	// It looks like our IP just changed thats all. RegPass checks out.
	pServName->ParseStatus( (LPCTSTR)pData, true );
	return REGRES_RET_OK;
}

bool CClient::OnRxPeerServer( const BYTE * pData, int iLen )
{
	// Answer a question for this peer server.
	// This is on my m_Servers list or is the auto registration server.

	ASSERT( GetConnectType() == CONNECT_PEER_SERVER );

	bool fSuccess = false;
	if ( iLen <= 128 )
	{
		TCHAR szRequest[ 128 ];
		strcpylen( szRequest, (LPCTSTR) pData, sizeof(szRequest) );
		TCHAR * pszCmd = Str_TrimWhitespace( szRequest );
		CGString sVal;
		fSuccess = g_Serv.r_WriteVal( pszCmd, sVal, this );
		if ( fSuccess )
		{
			xSendReady( sVal, sVal.GetLength()+1 );
		}
	}
	return( true );	// don't dump the connection.
}

bool CClient::OnRxConsoleLoginComplete()
{
	ASSERT( GetConnectType() == CONNECT_TELNET );
 	if ( GetPrivLevel() < PLEVEL_Admin )	// this really should not happen.
	{
		SysMessage( "Sorry you don't have admin level access" DEBUG_CR );
		return( false );
	}

	// CSocketAddress PeerName = m_Socket.GetPeerName();
	if ( ! m_PeerName.IsValidAddr())
		return( false );

	SysMessagef( "Welcome to the Remote Admin Console '%s','%s'" DEBUG_CR, GetName(), m_PeerName.GetAddrStr());
	return( true );
}

bool CClient::OnRxConsole( const BYTE * pData, int iLen )
{
	// A special console version of the client. (Not game protocol)
	ASSERT( iLen );
	ASSERT( GetConnectType() == CONNECT_TELNET );

	while ( iLen -- )
	{
		int iRet = OnConsoleKey( m_Targ_Text, *pData++, GetAccount() != NULL );
		if ( ! iRet )
			return( false );
		if ( iRet == 2 )
		{
			if ( GetAccount() == NULL )
			{
				if ( !m_zLogin[0] )
				{
					if ( m_Targ_Text.GetLength() > sizeof(m_zLogin)-1 ) SysMessage("Login?:" DEBUG_CR);
					else
					{
						strcpy(m_zLogin, m_Targ_Text);
						SysMessage("Password?:" DEBUG_CR);
					}
					m_Targ_Text.Empty();
				}
				else
				{
					CGString sMsg;

					CAccountRef pAccount = g_Accounts.Account_Find(m_zLogin);
					if (( pAccount == NULL ) || ( pAccount->GetPrivLevel() < PLEVEL_Admin ))
					{
						SysMessage("This account does not exist or is not privileged to log in via telnet.");
						m_Targ_Text.Empty();
						return false;
					}
					if ( LogIn(m_zLogin, m_Targ_Text, sMsg ) == LOGIN_SUCCESS )
					{
						m_Targ_Text.Empty();
						return OnRxConsoleLoginComplete();
					}
					else if ( ! sMsg.IsEmpty())
					{
						SysMessage( sMsg );
						return false;
					}
					m_Targ_Text.Empty();
				}
				return true;
			}
			else
			{
				iRet = g_Serv.OnConsoleCmd( m_Targ_Text, this );
			}
			if ( ! iRet )
				return( false );
		}
	}

	return( true );
}

bool CClient::OnRxPing( const BYTE * pData, int iLen )
{
	// packet iLen < 4
	// UOMon should work like this.
	// RETURN: true = keep the connection open.

	ASSERT( GetConnectType() == CONNECT_UNK );
	ASSERT( iLen < 4 );
	if ( iLen > 1 )	// this is not a ping. don't know what it is.
		return( false );

	if ( pData[0] == '\x1' || pData[0] == ' ' )
	{
		// enter into remote admin mode. (look for password).
		SetConnectType( CONNECT_TELNET );
		m_zLogin[0] = 0;
		SysMessagef( "Welcome to %s Admin Telnet" DEBUG_CR, (LPCTSTR) g_Serv.GetName());

		if ( g_Cfg.m_fLocalIPAdmin )
		{
			// don't bother logging in if local.

			// CSocketAddress PeerName = m_Socket.GetPeerName();
			if ( ! m_PeerName.IsValidAddr())
				return( false );

			if ( m_PeerName.IsLocalAddr() )
			{
				CAccountRef pAccount = g_Accounts.Account_Find( "Administrator" );
				if ( pAccount == NULL )
					pAccount = g_Accounts.Account_Find( "RemoteAdmin" );
				CGString sMsg;
				LOGIN_ERR_TYPE lErr = LogIn( pAccount, sMsg );
				if ( lErr != LOGIN_SUCCESS )
				{
					if ( lErr != LOGIN_ERR_NONE )
					{
						SysMessage( sMsg );
					}
					return( false );
				}
				return OnRxConsoleLoginComplete();
			}
		}

		SysMessage("Login?:" DEBUG_CR);
		return( true );
	}

	// Answer the ping and drop.
	SetConnectType( CONNECT_PING );
	LPCTSTR pTemp = g_Serv.GetStatusString( pData[0] );
	xSendReady( pTemp, strlen( pTemp )+1 );
	return( false );
}

bool CClient::OnRxWebPageRequest( BYTE * pRequest, int iLen )
{
	// Seems to be a web browser pointing at us ? typical stuff :
	ASSERT( GetConnectType() == CONNECT_HTTP );

	pRequest[iLen] = '\0';

	TCHAR * ppLines[16];
	int iQtyLines = Str_ParseCmds( (TCHAR*) pRequest, ppLines, COUNTOF(ppLines), "\r\n" );
	if ( iQtyLines < 1 )
		return( false );

	// Look for what they want to do with the connection.
	bool fKeepAlive = false;
	CGTime dateIfModifiedSince;
	TCHAR * pszReferer = NULL;
	int iContentLength = 0;
	for ( int j=1; j<iQtyLines; j++ )
	{
		TCHAR * pszArgs = ppLines[j];
		if ( ! strnicmp( pszArgs, "Connection:", 11 ))
		{
			pszArgs += 11;
			if ( strstr( pszArgs, "Keep-Alive" ))
			{
				fKeepAlive = true;
			}
		}
		else if ( ! strnicmp( pszArgs, "Referer:", 8 ))
		{
			pszReferer = pszArgs+8;
		}
		else if ( ! strnicmp( pszArgs, "Content-Length:", 15 ))
		{
			pszArgs += 15;
			iContentLength = Exp_GetSingle(pszArgs);
		}
		else if ( ! strnicmp( pszArgs, "If-Modified-Since:", 18 ))
		{
			// If-Modified-Since: Fri, 17 Dec 1999 14:59:20 GMT\r\n
			pszArgs += 18;
			dateIfModifiedSince.Read( pszArgs );
		}
	}

	TCHAR * ppRequest[4];
	int iQtyArgs = Str_ParseCmds( (TCHAR*) ppLines[0], ppRequest, COUNTOF(ppRequest), " " );
	if ( iQtyArgs < 2 )
		return( false );

	linger llinger;
	llinger.l_onoff = 1;
	llinger.l_linger = 500;	// in mSec
	m_Socket.SetSockOpt( SO_LINGER, (char*)&llinger, sizeof(struct linger));
	BOOL nbool = true;
	m_Socket.SetSockOpt( SO_KEEPALIVE, &nbool, sizeof(BOOL));

	// disable NAGLE algorythm for data compression
	nbool=true;
	m_Socket.SetSockOpt( TCP_NODELAY,&nbool,sizeof(BOOL),IPPROTO_TCP);

	if ( ! memcmp( ppLines[0], "POST", 4 ))
	{
		// web browser is posting data back to the server.
		// IT must be one of our defined pages to handled posted info.
		// Is it one of our defined pages ?

		g_Log.Event( LOGM_HTTP|LOGL_EVENT, "%x:HTTP Page Post '%s'" DEBUG_CR, m_Socket.GetSocket(), (LPCTSTR) ppRequest[1] );

		// POST /--WEBBOT-SELF-- HTTP/1.1
		// Accept: image/gif, image/x-xbitmap, image/jpeg, */*
		// Referer: http://127.0.0.1:2593/spherestatus.htm
		// Accept-Language: en-us
		// Content-Type: application/x-www-form-urlencoded
		// Accept-Encoding: gzip, deflate
		// User-Agent: Mozilla/4.0 (compatible; MSIE 5.0; Windows NT; DigExt)
		// Host: 127.0.0.1:2593
		// Content-Length: 29
		// Connection: Keep-Alive
		// T1=stuff1&B1=Submit&T2=stuff2

		// Is it one of our defined pages ?
		CWebPageDef * pWebPage = g_Cfg.FindWebPage(ppRequest[1]);

		if ( pWebPage == NULL )
		{
			pWebPage = g_Cfg.FindWebPage(pszReferer);
		}
		if ( pWebPage )
		{
			if ( pWebPage->ServPagePost( this, ppRequest[1], ppLines[iQtyLines-1], iContentLength ))
				return fKeepAlive;
		}

		// return( false );
		// just fall through and serv up the error page.
	}
	else
	{
		// Request for a page. try to respond.
		// GET /pagename.htm HTTP/1.1\r\n
		// Accept: image/gif, */*\r\n
		// Accept-Language: en-us\r\n
		// Accept-Encoding: gzip, deflate\r\n
		// If-Modified-Since: Fri, 17 Dec 1999 14:59:20 GMT\r\n
		// User-Agent: Mozilla/4.0 (compatible; MSIE 5.0; Windows NT; DigExt)\r\n
		// Host: localhost:2593\r\n
		// Connection: Keep-Alive\r\n	// keep the connection open ? or "close"
		// \r\n
		//

		g_Log.Event( LOGM_HTTP|LOGL_EVENT, "%x:HTTP Page Request '%s', alive=%d" DEBUG_CR, m_Socket.GetSocket(), (LPCTSTR) ppRequest[1], fKeepAlive );
	}

	if ( CWebPageDef::ServPage( this, ppRequest[1], &dateIfModifiedSince ))
	{
		return fKeepAlive;
	}
	return( false );
}

void CClient::xProcessMsg( bool fGood )
{
	// Done with the current packet.
	// m_bin_msg_len = size of the current packet we are processing.

	if ( ! m_bin_msg_len )	// hmm, nothing to do !
		return;

	if ( ! fGood )	// toss all.
	{
		DEBUG_ERR(( "%s (%x):Bad Msg(%x) Eat %d bytes, prv=0%x, type=%d" DEBUG_CR,
			m_pAccount ? m_pAccount->GetName() : "",
		       	m_Socket.GetSocket(),
			m_bin_ErrMsg,
	         	m_bin.GetDataQty(), m_bin_PrvMsg,
			GetConnectType() ));
		m_bin.Empty();	// eat the buffer.
		if ( GetConnectType() == CONNECT_LOGIN )	// tell them about it.
		{
			addLoginErr( LOGIN_ERR_OTHER );
		}
	}
	else
	{
		m_bin.RemoveDataAmount( m_bin_msg_len );
	}

	m_bin_msg_len = 0;	// done with this packet.
}

bool CClient::xProcessClientSetup( CEvent * pEvent, int iLen )
{
	// If this is a login then try to process the data and figure out what client it is.
	// try to figure out which client version we are talking to.
	// (CEvent::ServersReq) or (CEvent::CharListReq)
	// NOTE: Anything else we get at this point is tossed !

	ASSERT( GetConnectType() == CONNECT_CRYPT );
	ASSERT( !m_Crypt.IsInit());
	ASSERT( iLen );
	
	// Try all client versions on the msg.
	CEvent bincopy;		// in buffer. (from client)
	ASSERT( iLen <= sizeof(bincopy));
	memcpy( bincopy.m_Raw, pEvent->m_Raw, iLen );
	
	if ( !m_Crypt.Init( m_tmSetup.m_dwIP, bincopy.m_Raw, iLen ) )
	{
		DEBUG_MSG(( "%x:Odd login message length %d?" DEBUG_CR, m_Socket.GetSocket(), iLen ));
		addLoginErr( LOGIN_ERR_OTHER );
		return( false );
	}
	
	SetConnectType( m_Crypt.GetConnectType() );
	
	if ( !xCanEncLogin() || (m_Crypt.GetConnectType() == CONNECT_LOGIN && !xCanEncLogin(true)) )
	{
		// g_Log.Event( LOGL_EVENT,"%x:Client Encryption Failed" DEBUG_CR, this->m_Socket.GetSocket() );
		addLoginErr( LOGIN_ERR_OTHER );
		return( false );
	}
	
	// g_Log.Event( LOGL_WARN,"%x:Get Client %d type %x - %x|%x" DEBUG_CR, this->m_Socket.GetSocket(), m_Crypt.GetConnectType(), m_Crypt.GetClientVer(), pEvent->m_Raw[0], bincopy.m_Raw[0] );
	
	// memcpy( bincopy.m_Raw, pEvent->m_Raw, iLen );
	
	// g_Log.Event( LOGL_WARN,"%x:Get Client %d type %x" DEBUG_CR, this->m_Socket.GetSocket(), m_Crypt.GetConnectType(), m_Crypt.GetClientVer() );

	if ( IsBlockedIP())	// we are a blocked ip so i guess it does not matter.
	{
		addLoginErr( LOGIN_ERR_BLOCKED );
		return( false );
	}


	LOGIN_ERR_TYPE lErr = LOGIN_ERR_OTHER;

	// g_Log.Event( LOGL_WARN,"%x:Encryption type %d - %x" DEBUG_CR, this->m_Socket.GetSocket(), m_Crypt.GetEncryptionType(), pEvent->Default.m_Cmd );
	
	m_Crypt.Decrypt( pEvent->m_Raw, bincopy.m_Raw, iLen );
	
	// g_Log.Event( LOGL_WARN,"%x:Encryption type %d - D:%x" DEBUG_CR, this->m_Socket.GetSocket(), m_Crypt.GetEncryptionType(), pEvent->Default.m_Cmd );

	if ( pEvent->Default.m_Cmd == XCMD_ServersReq )
	{
		if ( iLen < sizeof( pEvent->ServersReq ))
			return(false);

		lErr = Login_ServerList( pEvent->ServersReq.m_acctname, pEvent->ServersReq.m_acctpass );
		if ( lErr == LOGIN_SUCCESS )
		{
			CAccountRef pAcc = g_Accounts.Account_Find( pEvent->ServersReq.m_acctname );
			if (pAcc)
			{
				pAcc->m_TagDefs.SetNum("clientversion", m_Crypt.GetClientVer());
			}
			else
			{
				// If i can't set the tag is better to stop login now
				lErr = LOGIN_ERR_OTHER;
			}
		}
	}
	else if ( pEvent->Default.m_Cmd == XCMD_CharListReq )
	{
		if ( iLen < sizeof( pEvent->CharListReq ))
			return(false);

		lErr = Setup_ListReq( pEvent->CharListReq.m_acctname, pEvent->CharListReq.m_acctpass, true );
		if ( lErr == LOGIN_SUCCESS )
		{
			// pass detected client version to the game server to make valid cliver used
			CAccountRef pAcc = g_Accounts.Account_Find(pEvent->CharListReq.m_acctname);
			if (pAcc)
			{
				DWORD tmVer = 1;
				tmVer = (DWORD)pAcc->m_TagDefs.GetKey( "clientversion" )->GetValNum();
				if ( tmVer != 1 )
					m_Crypt.SetClientVerEnum(tmVer, false);
					
				if ( !xCanEncLogin(true) )
					lErr = LOGIN_ERR_OTHER;
			}
			else
			{
				lErr = LOGIN_ERR_OTHER;
			}
		}
	}
	
//	TCHAR * sTemp = Str_GetTemp();
//	g_Log.Event( LOGL_EVENT, "%x:%sclient ClientVersion(%s - %x - %d)" DEBUG_CR, m_Socket.GetSocket(), (GetConnectType() == CONNECT_LOGIN) ? "Login" : "Game", (LPCTSTR) m_Crypt.WriteClientVer(sTemp), m_Crypt.GetClientVer(), iVer );
	
	if ( lErr == LOGIN_ERR_OTHER )	// it never matched any crypt format.
	{
		addLoginErr( lErr );
	}

	return( lErr == LOGIN_SUCCESS );
}

bool CClient::xCanEncLogin(bool bCheckCliver)
{
	if ( !bCheckCliver )
	{
		if ( m_Crypt.GetEncryptionType() == ENC_NONE )
			return ( g_Cfg.m_iUsenocrypt != 0 ); // Server don't want no-crypt clients 
		
		return ( g_Cfg.m_fUsecrypt ); // Server don't want crypt clients
	}
	else
	{
		if ( !g_Serv.m_ClientVersion.GetClientVer() ) // Any Client allowed
			return( true );
		
		// g_Log.Event( LOGL_WARN,"%x:Encryption Version %d - %d" DEBUG_CR, this->m_Socket.GetSocket(), m_Crypt.GetClientVer(), g_Serv.m_ClientVersion.GetClientVer() );
		
		return( m_Crypt.GetClientVer() == g_Serv.m_ClientVersion.GetClientVer() );
	}
}

void CClient::xSendPkt(const CCommand * pCmd, int length)
{
	xSendReady((const void *)( pCmd->m_Raw ), length );
}

void CClient::xSendPktNow( const CCommand * pCmd, int length )
{
	const void *pData = (const void*)pCmd->m_Raw;

	xFlush();	// Flush old packets.
	
	// Add my packet
	DEBUG_CHECK(length);
	if ( ! m_Socket.IsOpen() )
		return;

	if ( GetConnectType() != CONNECT_HTTP )	// acting as a login server to this client.
	{
		if ( length > MAX_BUFFER )
		{
			if ( !m_fClosed ) DEBUG_ERR(( "%x:Client out TOO BIG %d!" DEBUG_CR, m_Socket.GetSocket(), length ));
			m_fClosed	= true;
			return;
		}
		if ( m_bout.GetDataQty() + length > MAX_BUFFER )
		{
			if ( !m_fClosed ) DEBUG_ERR(( "%x:Client out overflow %d+%d!" DEBUG_CR, m_Socket.GetSocket(), m_bout.GetDataQty(), length ));
			m_fClosed	= true;
			return;
		}
	}

//	DEBUG_ERR(("SEND: %x:adding %d bytes for NOW" DEBUG_CR, m_Socket.GetSocket(), length));
	m_bout.AddNewData((const BYTE*) pData, length);
	xFlush();
}

bool CClient::xHasData() const
{
	return ( m_bin.GetDataQty() != 0 );
}

void CClient::xFlush()
{
	// Sends buffered data at once
	// NOTE:
	// Make sure we do not overflow the Sockets Tx buffers!

	int iLen = m_bout.GetDataQty();
	if ( !iLen || !m_Socket.IsOpen() || m_fClosed )
		return;

	m_timeLastSend = CServTime::GetCurrentTime();

	int iLenRet;
	if ( GetConnectType() != CONNECT_GAME )	// acting as a login server to this client.
	{
		iLenRet = m_Socket.Send( m_bout.RemoveDataLock(), iLen );
		if ( iLenRet != SOCKET_ERROR )
		{
			// Tx overflow may be handled gracefully.
			g_Serv.m_Profile.Count( PROFILE_DATA_TX, iLenRet );
			m_bout.RemoveDataAmount(iLenRet);
		}
		else	
		{
			// Assume nothing was transmitted.
Do_Handle_Error:
			int iErrCode = CGSocket::GetLastError();
#ifdef _WIN32
			if ( iErrCode == WSAECONNRESET || iErrCode == WSAECONNABORTED )
			{
				m_fClosed	= true;
				return;
			}
			if ( iErrCode == WSAEWOULDBLOCK )
			{
				// just try back later. or select() will close it for us later.
				return;
			}
#endif
			DEBUG_ERR(( "%x:Tx Error %d" DEBUG_CR, m_Socket.GetSocket(), iErrCode ));
			return;
		}
	}
	else
	{
		// Only the game server does this.
		// This acts as a compression alg. tho it may expand the data some times.

		int iLenComp = xCompress( sm_xCompress_Buffer, m_bout.RemoveDataLock(), iLen );
		ASSERT( iLenComp <= sizeof(sm_xCompress_Buffer));

		// This now works. Thx to Necr0 post and library, i've understood how to do it.
		// ( MD5 server -> client )
		if ( m_Crypt.GetEncryptionType() == ENC_TFISH )
		{
			// g_Log.Event( LOGL_WARN,"%x:Encrypting output" DEBUG_CR, this->m_Socket.GetSocket() );
			m_Crypt.Encrypt( sm_xCompress_Buffer, sm_xCompress_Buffer, iLenComp );
		}
//		DEBUG_ERR(("FLUSH: %x:Send %d bytes as %d" DEBUG_CR, m_Socket.GetSocket(), m_bout.GetDataQty(), iLen ));

		iLenRet = m_Socket.Send( sm_xCompress_Buffer, iLenComp );
		if ( iLenRet != SOCKET_ERROR )
		{
			g_Serv.m_Profile.Count( PROFILE_DATA_TX, iLen );
			m_bout.RemoveDataAmount(iLen);	// must use all of it since we have no idea what was really sent.
		}

		if ( iLenRet != iLenComp )
		{
			// Tx overflow is not allowed here !
			// no idea what effect this would have. assume nothing is sent.
			goto Do_Handle_Error;
		}
	}
}

void CClient::xSend( const void *pData, int length, bool bQueue /* false default */ )
{
	// buffer a packet to client.
	DEBUG_CHECK(length);
	if ( ! m_Socket.IsOpen() )
		return;

	if ( GetConnectType() != CONNECT_HTTP )	// acting as a login server to this client.
	{
		if ( length > MAX_BUFFER )
		{
			if ( !m_fClosed ) DEBUG_ERR(( "%x:Client out TOO BIG %d!" DEBUG_CR, m_Socket.GetSocket(), length ));
			m_fClosed	= true;
			return;
		}
		if ( m_bout.GetDataQty() + length > MAX_BUFFER )
		{
			if ( !m_fClosed ) DEBUG_ERR(( "%x:Client out overflow %d+%d!" DEBUG_CR, m_Socket.GetSocket(), m_bout.GetDataQty(), length ));
			m_fClosed	= true;
			return;
		}
	}

	m_bout.AddNewData( (const BYTE*) pData, length );

	// Vjaka: always flush for new 4.0.* clients support. old code commented
	// do flush only after, since all packets are sent via this anyhow
	if ( GetConnectType() == CONNECT_GAME || GetConnectType() == CONNECT_LOGIN )
	{
		if (m_Crypt.GetClientVer() >= 0x400000 || IsNoCryptVer(4) )
			if ( !bQueue )
				xFlush();
	}
}

void CClient::xSendReady( const void *pData, int length ) // We could send the packet now if we wanted to but wait til we have more.
{
	// We could send the packet now if we wanted to but wait til we have more.
	if ( m_bout.GetDataQty() + length >= MAX_BUFFER )
	{
		xFlush();
	}
//	DEBUG_ERR(("SEND: %x:adding %d bytes" DEBUG_CR, m_Socket.GetSocket(), length));
	xSend( pData, length );
	if ( m_bout.GetDataQty() >= MAX_BUFFER / 2 )	// send only if we have a bunch.
	{
		xFlush();
	}
}

bool CClient::xRecvData() // Receive message from client
{
	// High level Rx from Client.
	// RETURN: false = dump the client.
	CEvent Event;
	int iCountNew = m_Socket.Receive( &Event, sizeof(Event), 0 );

	if ( iCountNew <= 0 )	// I should always get data here.
		return( false ); // this means that the client is gone.

	g_Serv.m_Profile.Count( PROFILE_DATA_RX, iCountNew );

	if ( GetConnectType() == CONNECT_UNK ) // first thing
	{
		// This is the first data we get on a new connection.
		// Figure out what the other side wants.

		DEBUG_CHECK( ! m_Crypt.IsInit());

		if ( iCountNew < 4 )	// just a ping for server info. (maybe, or CONNECT_TELNET?)
		{
			if ( IsBlockedIP())
				return( false );
			return( OnRxPing( Event.m_Raw, iCountNew ));
		}

		if ( iCountNew > 5 )
		{
			if ( Event.m_CryptHeader == 0xFFFFFFFF )
			{
				if ( g_Cfg.m_fReplyPeerConnects )
				{
					// special inter-server type message.
					SetConnectType( CONNECT_AUTO_SERVER );

					if ( IsBlockedIP()) return( false );
					if ( Event.m_Raw[4] == 0 )
					{
						// Server Registration message
						REGRES_TYPE retcode = OnRxAutoServerRegister( &Event.m_Raw[5], iCountNew-5 );
						// Send the code back.
					}
					return false;
				}
			}

			// Is it a HTTP request ?
			// Is it HTTP post ?
			if ( ! memcmp( Event.m_Raw, "POST /", 6 ) ||
				! memcmp( Event.m_Raw, "GET /", 5 ))
			{
				if ( ! g_Cfg.m_fUseHTTP )
					return( false );

				// IsBlockedIP
				SetConnectType( CONNECT_HTTP );	// we are serving web pages to this client.

				CLogIP * pLogIP = GetLogIP();
				if ( pLogIP == NULL )
					return( false );
				if ( pLogIP->CheckPingBlock( false ))
					return( false );

				// We might have an existing account connection.
				// ??? Is this proper to connect this way ?
				m_pAccount = pLogIP->GetAccount();

				return( OnRxWebPageRequest( Event.m_Raw, iCountNew ));
			}
		}

		// Assume it's a normal client log in.
		m_tmSetup.m_dwIP = UNPACKDWORD(Event.m_Raw);
		iCountNew -= sizeof( DWORD );
		SetConnectType( CONNECT_CRYPT );
		if ( iCountNew <= 0 )
		{
			return( true );
		}

		return( xProcessClientSetup( (CEvent*)(Event.m_Raw+4), iCountNew ));
	}

	if ( ! m_Crypt.IsInit())
	{
		// This is not a client connection it seems.
		// Must process the whole thing as one packet right now.

		if ( GetConnectType() == CONNECT_CRYPT )
		{
			// try to figure out which client version we are talking to.
			// (CEvent::ServersReq) or (CEvent::CharListReq)
			return( xProcessClientSetup( &Event, iCountNew ));
		}

		if ( GetConnectType() == CONNECT_HTTP )
		{
			// we are serving web pages to this client.
			return( OnRxWebPageRequest( Event.m_Raw, iCountNew ));
		}
		if ( GetConnectType() == CONNECT_PEER_SERVER )
		{
			if ( g_Cfg.m_fReplyPeerConnects ) return( OnRxPeerServer( Event.m_Raw, iCountNew ));
		}
		if ( GetConnectType() == CONNECT_TELNET )
		{
			// We already logged in or are in the process of logging in.
			return( OnRxConsole( Event.m_Raw, iCountNew ));
		}

		g_Log.Event( LOGM_CLIENTS_LOG, "xRecvData() %x:Junk with non inited Crypt" DEBUG_CR, m_Socket.GetSocket());
		return( false );	// No idea what this junk is.
	}

	// Decrypt the client data.
	// TCP = no missed packets ! If we miss a packet we are screwed !

	m_Crypt.Decrypt( m_bin.AddNewDataLock(iCountNew), Event.m_Raw, iCountNew );
	m_bin.AddNewDataFinish(iCountNew);

	return( true );
}

