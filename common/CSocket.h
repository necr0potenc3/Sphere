// CSocket.h

#ifndef _INC_CSOCKET_H
#define _INC_CSOCKET_H
#pragma once

#include "common.h"

#ifdef _WIN32
#include <winsock.h>
typedef int socklen_t;
#else	// else assume LINUX

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>

// Compatibility stuff.
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR    (-1)
#define SOCKET			int
#define TCP_NODELAY		0x0001

#endif	// _WIN32

struct CSocketAddressIP : public in_addr
{
	// Just the ip address. Not the port.
#define SOCKET_LOCAL_ADDRESS 0x0100007f
	// INADDR_ANY              (u_long)0x00000000
	// INADDR_LOOPBACK         0x7f000001
	// INADDR_BROADCAST        (u_long)0xffffffff
	// INADDR_NONE             0xffffffff

	DWORD GetAddrIP() const
	{
		return( s_addr );
	}
	void SetAddrIP( DWORD dwIP )
	{
		s_addr = dwIP;
	}
	LPCTSTR GetAddrStr() const
	{
		return inet_ntoa( *this );
	}
	void SetAddrStr( LPCTSTR pszIP )
	{
		// NOTE: This must be in 1.2.3.4 format.
		s_addr = inet_addr( pszIP );
	}
	bool IsValidAddr() const
	{
		// 0 and 0xffffffff=INADDR_NONE
		return( s_addr != INADDR_ANY && s_addr != INADDR_BROADCAST );
	}
	bool IsLocalAddr() const
	{
		return( s_addr == 0 || s_addr == SOCKET_LOCAL_ADDRESS );
	}

	bool IsSameIP( const CSocketAddressIP & ip ) const;
	bool IsMatchIP( const CSocketAddressIP & ip ) const;

	struct hostent * GetHostStruct() const
	{
		// try to reverse lookup a name for this IP address.
		// NOTE: This is a blocking call !!!!
		return gethostbyaddr((char *)&s_addr,sizeof(s_addr),AF_INET);
	}
	bool SetHostStruct( const struct hostent * pHost )
	{
		// Set the ip from the address name we looked up.
		if ( pHost == NULL ||
			pHost->h_addr_list == NULL ||
			pHost->h_addr == NULL )	// can't resolve the address.
		{
			return( false );
		}
		SetAddrIP( *((DWORD*)( pHost->h_addr ))); // 0.1.2.3
		return true;
	}

	bool SetHostStr( LPCTSTR pszHostName )
	{
		// try to resolve the host name with DNS for the true ip address.
		if ( pszHostName[0] == '\0' )
			return( false );
		if ( isdigit( pszHostName[0] ))
		{
			SetAddrStr( pszHostName ); // 0.1.2.3
			return( true );
		}
		// NOTE: This is a blocking call !!!!
		return SetHostStruct( gethostbyname( pszHostName ));
	}
	bool GetHostStr( TCHAR * pszHostName, int iLenMax )
	{
		// try to resolve the host name with DNS for the true ip address.
		// NOTE: This is a blocking call !!!!
		ASSERT(pszHostName);
		struct hostent * pHost = GetHostStruct();
		if ( pHost )
		{
			strcpy( pszHostName, pHost->h_name );
			return( true );
		}
		else
		{
			// See CGSocket::GetLastError();
			strcpy( pszHostName, GetAddrStr() );
			return( false );
		}
	}
	bool operator==( CSocketAddressIP ip ) const
	{
		return( IsSameIP( ip ) );
	}
	CSocketAddressIP()
	{
		s_addr = INADDR_BROADCAST;
	}
	CSocketAddressIP( DWORD dwIP )
	{
		s_addr = dwIP;
	}
};

struct CSocketAddress : public CSocketAddressIP
{
	// IP plus port.
	// similar to sockaddr_in but without the waste.
	// use this instead.
private:
	WORD m_port;
public:

	// Just the port.
	WORD GetPort() const
	{
		return( m_port );
	}
	void SetPort( WORD wPort )
	{
		m_port = wPort;
	}
	void SetPortStr( LPCTSTR pszPort )
	{
		m_port = (WORD) atoi( pszPort );
	}
	bool SetPortExtStr( TCHAR * pszIP )
	{
		// assume the port is at the end of the line.
		TCHAR * pszPort = strchr( pszIP, ',' );
		if ( pszPort == NULL )
		{
			pszPort = strchr( pszIP, ':' );
			if ( pszPort == NULL )
				return( false );
		}

		SetPortStr( pszPort + 1 );
		*pszPort = '\0';
		return( true );
	}

	// Port and address together.
	void SetAddrPortStr( LPCTSTR pszIP )
	{
		TCHAR szIP[256];
		strncpy( szIP, pszIP, sizeof(szIP));
		SetPortExtStr( szIP );
		SetAddrStr( szIP );
	}
	bool SetHostPortStr( LPCTSTR pszIP )
	{
		// NOTE: This is a blocking call !!!!
		TCHAR szIP[256];
		strncpy( szIP, pszIP, sizeof(szIP));
		SetPortExtStr( szIP );
		return SetHostStr( szIP );
	}

	bool operator==( const CSocketAddress & SockAddr ) const
	{
		return( GetAddrIP() == SockAddr.GetAddrIP() && GetPort() == SockAddr.GetPort() );
	}

	CSocketAddress( in_addr dwIP, WORD uPort )
	{
		s_addr = dwIP.s_addr;
		m_port = uPort;
	}
	CSocketAddress( CSocketAddressIP ip, WORD uPort )
	{
		s_addr = ip.GetAddrIP();
		m_port = uPort;
	}
	CSocketAddress( DWORD dwIP, WORD uPort )
	{
		s_addr = dwIP;
		m_port = uPort;
	}
	CSocketAddress()
	{
		// s_addr = INADDR_BROADCAST;
		m_port = 0;
	}

	// compare to sockaddr_in

	struct sockaddr_in GetAddrPort() const
	{
		struct sockaddr_in SockAddrIn;
		SockAddrIn.sin_family = AF_INET;
		SockAddrIn.sin_addr.s_addr = s_addr;
		SockAddrIn.sin_port = htons(m_port);
		return( SockAddrIn );
	}
	void SetAddrPort( const struct sockaddr_in & SockAddrIn )
	{
		s_addr = SockAddrIn.sin_addr.s_addr;
		m_port = ntohs( SockAddrIn.sin_port );
	}
	CSocketAddress & operator = ( const struct sockaddr_in & SockAddrIn )
	{
		SetAddrPort(SockAddrIn);
		return( *this );
	}
	bool operator==( const struct sockaddr_in & SockAddrIn ) const
	{
		return( GetAddrIP() == SockAddrIn.sin_addr.s_addr && GetPort() == ntohs( SockAddrIn.sin_port ) );
	}
	CSocketAddress( const sockaddr_in & SockAddrIn )
	{
		SetAddrPort( SockAddrIn );
	}
};

class CGSocket
{
private:
	SOCKET  m_hSocket;	// socket connect handle
private:
	void Clear()
	{
		// Transfer the socket someplace else.
		m_hSocket = INVALID_SOCKET;
	}
public:
	CGSocket()
	{
		Clear();
	}
	CGSocket( SOCKET socket )	// accept case.
	{
		m_hSocket = socket;
	}
	static int GetLastError();
	bool IsOpen() const
	{
		return( m_hSocket != INVALID_SOCKET );
	}
	SOCKET GetSocket() const
	{
		return( m_hSocket );
	}

	bool Create()
	{
		ASSERT( ! IsOpen());
		m_hSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		return( IsOpen());
	}
	int Bind( struct sockaddr_in * pSockAddrIn )
	{
		return( bind( m_hSocket, (struct sockaddr *) pSockAddrIn, sizeof(*pSockAddrIn)));
	}
	int Bind( const CSocketAddress & SockAddr )
	{
		struct sockaddr_in SockAddrIn = SockAddr.GetAddrPort();
		if ( SockAddr.IsLocalAddr())
		{
			SockAddrIn.sin_addr.s_addr = INADDR_ANY;	// use all addresses.
		}
		return( Bind( &SockAddrIn ));
	}
	int Bind( WORD uPort )
	{
		struct sockaddr_in SockAddrIn;
		SockAddrIn.sin_family = AF_INET;
		SockAddrIn.sin_addr.s_addr = INADDR_ANY;
		SockAddrIn.sin_port = htons( uPort );
		return( Bind( &SockAddrIn ));
	}
	int Listen( int iMaxBacklogConnections = SOMAXCONN )
	{
		return( listen( m_hSocket, iMaxBacklogConnections ));
	}
	int Connect( struct sockaddr_in * pSockAddrIn )
	{
		// RETURN: 0 = success, else SOCKET_ERROR
		return( connect( m_hSocket, (struct sockaddr*) pSockAddrIn, sizeof(*pSockAddrIn)));
	}
	int Connect( const CSocketAddress & SockAddr )
	{
		struct sockaddr_in SockAddrIn = SockAddr.GetAddrPort();
		return( Connect( &SockAddrIn ));
	}
	int Connect( const struct in_addr ip, WORD wPort )
	{
		CSocketAddress SockAddr( ip.s_addr, wPort );
		return( Connect( SockAddr ));
	}
	int Connect( LPCTSTR pszHostName, WORD wPort )
	{
		CSocketAddress SockAddr;
		SockAddr.SetHostStr( pszHostName );
		SockAddr.SetPort( wPort );
		return( Connect( SockAddr ));
	}
	SOCKET Accept( struct sockaddr_in * pSockAddrIn ) const
	{
		int len = sizeof( struct sockaddr_in );
		return( accept( m_hSocket, (struct sockaddr*) pSockAddrIn, (socklen_t*)&len ));
	}
	SOCKET Accept( CSocketAddress & SockAddr ) const
	{
		// RETURN: Error = hSocketClient < 0 || hSocketClient == INVALID_SOCKET 
		struct sockaddr_in SockAddrIn;
		SOCKET hSocket = Accept( &SockAddrIn );
		SockAddr.SetAddrPort( SockAddrIn );
		return( hSocket );
	}
	int Send( const void * pData, int len ) const
	{
		// RETURN: length sent
		return( send( m_hSocket, (char*) pData, len, 0 ));
	}
	int Receive( void * pData, int len, int flags = 0 )
	{
		// RETURN: length, <= 0 is closed or error.
		// flags = MSG_PEEK or MSG_OOB
		return( recv( m_hSocket, (char*) pData, len, flags ));
	}

	int GetSockName( struct sockaddr_in * pSockAddrIn ) const
	{
		// Get the address of the near end. (us)
		// RETURN: 0 = success
		int len = sizeof( *pSockAddrIn );
		return( getsockname( m_hSocket, (struct sockaddr *) pSockAddrIn, (socklen_t*)&len ));
	}
	CSocketAddress GetSockName() const
	{
		struct sockaddr_in SockAddrIn;
		int iRet = GetSockName( &SockAddrIn );
		if ( iRet )
		{
			return( CSocketAddress( INADDR_BROADCAST, 0 ));	// invalid.
		}
		else
		{
			return( CSocketAddress( SockAddrIn ));
		}
	}

	int GetPeerName( struct sockaddr_in * pSockAddrIn ) const
	{
		// Get the address of the far end.
		// RETURN: 0 = success
		int len = sizeof( *pSockAddrIn );
		return( getpeername( m_hSocket, (struct sockaddr *) pSockAddrIn, (socklen_t*)&len ));
	}
	
	CSocketAddress GetPeerName( ) const
	{
		struct sockaddr_in SockAddrIn;
		int iRet = GetPeerName( &SockAddrIn );
		if ( iRet )
		{
			return( CSocketAddress( INADDR_BROADCAST, 0 ));	// invalid.
		}
		else
		{
			return( CSocketAddress( SockAddrIn ));
		}
	}

	int SetSockOpt( int nOptionName, const void* optval, int optlen, int nLevel = SOL_SOCKET ) const
	{
		// level = SOL_SOCKET and IPPROTO_TCP.
		return( setsockopt( m_hSocket, nLevel, nOptionName, (const char FAR *) optval, optlen ));
	}
	int GetSockOpt( int nOptionName, void* optval, int * poptlen, int nLevel = SOL_SOCKET ) const
	{
		return( getsockopt( m_hSocket, nLevel, nOptionName, (char FAR *) optval, (socklen_t*)poptlen ));
	}

	bool IsReadReady( int sec = 0, int usec = 0 ) const
	{
		if ( ! IsOpen())
			return( false );

		fd_set fds;
		FD_ZERO( &fds );
		FD_SET( m_hSocket, &fds );
		timeval tv;
		tv.tv_sec  = sec;
		tv.tv_usec = usec;
		int result = select( m_hSocket+1, &fds, 0, 0, &tv );
		if (result == 0 || result == SOCKET_ERROR)
		{
		    // m_lastError = WSAGetLastError();
			return false;
		}
		return true;
	}

#ifdef _WIN32
	int IOCtlSocket( long icmd, DWORD * pdwArgs )
	{
		return ioctlsocket( m_hSocket, icmd, pdwArgs );
	}
#else
	int IOCtlSocket( long icmd, int iVal )	// LINUX ?
	{
		return fcntl( m_hSocket, icmd, iVal );
	}
#endif

	void Close()
	{
		if ( ! IsOpen())
			return;
		shutdown(m_hSocket, 2);
#ifdef _WIN32
		closesocket( m_hSocket );
#else
		close(m_hSocket);		// LINUX i assume. SD_BOTH
#endif
		Clear();
	}
	~CGSocket()
	{
		Close();
	}
};

inline int CGSocket::GetLastError()
{
#ifdef _WIN32
	return( WSAGetLastError() );
#else
	return( h_errno );	// WSAGetLastError()
#endif
}

#endif // _INC_CSOCKET_H
