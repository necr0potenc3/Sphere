#ifndef _WIN32
#include "graysvr.h"	// predef header.
#include "exceptions.h"

CSignalAux m_Signal;

void _cdecl	SignalSetError( int signal, const char * text )
{
	m_Signal.signal		= signal;
	m_Signal.text		= text;
	fprintf( stderr, "SignalSetError: %d (%s)" DEBUG_CR, signal, text );
	fflush( stderr );
	// siglongjmp( m_Signal.env, signal );
}

void	_cdecl SignalTrapError()
{
	fprintf( stderr, "CAUGHT!!!" DEBUG_CR );
	fflush( stderr );
	throw CGrayError( LOGL_FATAL, m_Signal.signal, m_Signal.text );
}
#endif
