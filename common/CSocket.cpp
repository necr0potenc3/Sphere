#include "../graysvr/graysvr.h"

bool	 CSocketAddressIP::IsMatchIP( const CSocketAddressIP & ip ) const
{
	BYTE		ip1	[4];
	BYTE		ip2	[4];

	memcpy( ip1, (void*) &ip.s_addr,	4 );
	memcpy( ip2, (void*) &s_addr,		4 );

	for ( int i = 0; i < 4; i++ )
	{
		if ( ip1[i] == 255 || ip2[i] == 255 || ip1[i] == ip2[i] )
			continue;
		return false;
	}
	return true;
}


bool	 CSocketAddressIP::IsSameIP( const CSocketAddressIP & ip ) const
{
	return (ip.s_addr == s_addr);
}
