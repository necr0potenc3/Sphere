#include "graysvr.h"	// predef header.
#include "CClient.h"


enum GUMPCTL_TYPE	// controls we can put in a gump.
{
	GUMPCTL_BUTTON	= 0,		// 7 = X,Y,Down gump,Up gump,pressable(1/0),page,id
	GUMPCTL_CHECKBOX,		// 6 = x,y,gumpcheck,gumpuncheck,starting state,checkid

	GUMPCTL_CHECKERTRANS,		// NEW: x,y,w,h
	GUMPCTL_CROPPEDTEXT,		// 6 = x,y,sx,sy,color?,startindex

	GUMPCTL_DCROPPEDTEXT,
	GUMPCTL_DHTMLGUMP,
	GUMPCTL_DORIGIN,
	GUMPCTL_DTEXT,
	GUMPCTL_DTEXTENTRY,

	GUMPCTL_GROUP,

	GUMPCTL_GUMPPIC,		// 3 = x,y,gumpID	 hue=color// put gumps in the dlg.
	GUMPCTL_GUMPPICTILED,		// x, y, gumpID, w, h, hue=color
	GUMPCTL_HTMLGUMP,		// 7 = x,y,sx,sy, 0 0 0

	// Not really controls but more attributes.
	GUMPCTL_NOCLOSE,		// 0 = not really used
	GUMPCTL_NODISPOSE,		// 0 = not really used  (modal?)
	GUMPCTL_NOMOVE,			// 0 = The gump cannot be moved around.

	GUMPCTL_PAGE,			// 1 = set current page number	// for multi tab dialogs.

	GUMPCTL_RADIO,			// 6 = x,y,gump1,gump2,starting state,id
	GUMPCTL_RESIZEPIC,		// 5 = x,y,gumpback,sx,sy	// can come first if multi page. put up some background gump
	GUMPCTL_TEXT,			// 4 = x,y,color?,startstringindex	// put some text here.
	GUMPCTL_TEXTENTRY,
	GUMPCTL_TILEPIC,		// 3 = x,y,item	// put item tiles in the dlg.
	GUMPCTL_TILEPICHUE,		// NEW: x,y,item,color

	GUMPCTL_TOOLTIP,		// From SE client. tooltip cliloc(1003000)

	GUMPCTL_XMFHTMLGUMP,		// 7 = x,y,sx,sy, cliloc(1003000) hasBack canScroll
	GUMPCTL_XMFHTMLGUMPCOLOR,	// NEW: x,y,w,h ???

	GUMPCTL_QTY,
};


//*******************************************
// -CDialogDef
LPCTSTR const CDialogDef::sm_szLoadKeys[GUMPCTL_QTY+1] =
{
	"button",
	"checkbox",

	"checkertrans",
	"croppedtext",

	"dcroppedtext",
	"dhtmlgump",
	"dorigin",
	"dtext",
	"dtextentry",

	"group",

	"gumppic",
	"gumppictiled",
	"htmlgump",

	"noclose",
	"nodispose",
	"nomove",

	"page",
	"radio",
	"resizepic",
	"text",
	"textentry",
	"tilepic",
	"tilepichue",

	"tooltip",

	"xmfhtmlgump",
	"xmfhtmlgumpcolor",

	NULL,
};


int	CDialogDef::GumpAddText( LPCTSTR pszText )
{
	m_sText[m_iTexts] = pszText;
	m_iTexts++;
	return (m_iTexts-1);
}

#define SKIP_ALL( args )		SKIP_SEPERATORS( args ); GETNONWHITESPACE( args );
#define GET_ABSOLUTE( c )		SKIP_ALL( pszArgs );	int c = Exp_GetSingle( pszArgs );

#define GET_EVAL( c )		SKIP_ALL( pszArgs );	int c = Exp_GetVal( pszArgs );

#define GET_RELATIVE( c, base )								\
	SKIP_ALL( pszArgs ); int c;								\
	if ( *pszArgs == '-' && isspace(pszArgs[1]))				\
		c	= base, ++pszArgs;								\
	else if ( *pszArgs == '+' )								\
		c = base + Exp_GetSingle( ++pszArgs );					\
	else if ( *pszArgs == '-' )								\
		c = base - Exp_GetSingle( ++pszArgs );					\
	else if ( *pszArgs == '*' )								\
		base = c	= base + Exp_GetSingle( ++pszArgs );		\
	else													\
		c = Exp_GetSingle( pszArgs );			
	

bool CDialogDef::r_Verb( CScript & s, CTextConsole * pSrc )	// some command on this object as a target
{
	LOCKDATA;
	EXC_TRY(("r_Verb('%s %s',%x)", s.GetKey(), s.GetArgStr(), pSrc));
	// The first part of the key is GUMPCTL_TYPE
	LPCSTR		pszKey = s.GetKey();

	int index = FindTableHead( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 );
	if ( index < 0 )
	{
		CScriptTriggerArgs Args( s.GetArgRaw() );
		if ( r_Call( s.GetKey(), pSrc, &Args ) )
			return true;
		return( CResourceLink::r_Verb(s,pSrc) );
	}
	
	LPCSTR	pszArgs	= s.GetArgStr();

	switch( index )
	{
		case GUMPCTL_PAGE:
		{
			if ( m_iControls >= COUNTOF(m_sControls)-1 )
				return false;

			GET_ABSOLUTE( page );

			if ( page <= 0 )		return true;

			int	iNewPage;
			if ( m_iPage == 0 || page > m_iPage || page == 0 )
				iNewPage	= page;
			else if ( page == m_iPage  )
				iNewPage	= 1;
			else
				iNewPage	= page + 1;	

			m_sControls[m_iControls].Format( "page %d", iNewPage );
			m_iControls++;
			return true;
		}
		case GUMPCTL_BUTTON:			// 7 = X,Y,Down gump,Up gump,pressable(1/0),page,id
		{
			if ( m_iControls >= COUNTOF(m_sControls)-1 )
				return false;

			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( down );
			GET_ABSOLUTE( up );
			GET_ABSOLUTE( press );
			GET_ABSOLUTE( page );
			GET_ABSOLUTE( id );

			int	iNewPage;
			if ( m_iPage == 0 || page > m_iPage || page == 0 )
				iNewPage	= page;
			else if ( page == m_iPage  )
				iNewPage	= 1;
			else
				iNewPage	= page + 1;	

			m_sControls[m_iControls].Format( "button %d %d %d %d %d %d %d", x, y, down, up, press, iNewPage, id );
			m_iControls++;
			return true;
		}
		case GUMPCTL_GUMPPIC:
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( id );
			SKIP_ALL( pszArgs );

			m_sControls[m_iControls].Format( "gumppic %d %d %d%s%s", x, y, id, *pszArgs ? " hue=" : "", *pszArgs ? pszArgs : "" );
			m_iControls++;
			return true;
		}
		case GUMPCTL_TILEPIC:
		case GUMPCTL_TILEPICHUE:
		{
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( id );
			SKIP_ALL( pszArgs );

			// m_sControls[m_iControls].Format( "tilepic %d %d %d%s%s", x, y, id, *pszArgs ? " " : "", pszArgs );
			// TilePic don't use args, TilePicHue yes :)
			if ( index == GUMPCTL_TILEPIC )
				m_sControls[m_iControls].Format( "tilepic %d %d %d", x, y, id );
			else
				m_sControls[m_iControls].Format( "tilepichue %d %d %d%s%s", x, y, id, *pszArgs ? " " : "", *pszArgs ? pszArgs : "" );

			m_iControls++;
			return true;
		}
		case GUMPCTL_DTEXT:
		{
			if ( m_iControls >= COUNTOF(m_sControls)-1 )
				return false;
			if ( m_iTexts >= COUNTOF(m_sText)-1 )
				return false;

			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( hue );
			SKIP_ALL( pszArgs )
			if ( *pszArgs == '.' )			pszArgs++;

			int	iText	= GumpAddText( *pszArgs ? pszArgs : " " );
			m_sControls[m_iControls].Format( "text %d %d %d %d", x, y, hue, iText );
			m_iControls++;
			return true;
		}
		case GUMPCTL_DCROPPEDTEXT:
		{
			if ( m_iControls >= COUNTOF(m_sControls)-1 )
				return false;
			if ( m_iTexts >= COUNTOF(m_sText)-1 )
				return false;
			
			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( w );
			GET_ABSOLUTE( h );
			GET_ABSOLUTE( hue );
			SKIP_ALL( pszArgs )
			if ( *pszArgs == '.' )			pszArgs++;

			int	iText	= GumpAddText( *pszArgs ? pszArgs : " " );
			m_sControls[m_iControls].Format( "croppedtext %d %d %d %d %d %d", x, y, w, h, hue, iText );
			m_iControls++;
			return true;
		}
		case GUMPCTL_DHTMLGUMP:
		{
			if ( m_iControls >= COUNTOF(m_sControls)-1 )
				return false;
			if ( m_iTexts >= COUNTOF(m_sText)-1 )
				return false;

			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( w );
			GET_ABSOLUTE( h );
			GET_ABSOLUTE( bck );
			GET_ABSOLUTE( options );
			SKIP_ALL( pszArgs )

			int	iText	= GumpAddText( *pszArgs ? pszArgs : " " );
			m_sControls[m_iControls].Format( "htmlgump %d %d %d %d %d %d %d", x, y, w, h, iText, bck, options );
			m_iControls++;
			return true;
		}
		case GUMPCTL_DTEXTENTRY:
		{
			if ( m_iControls >= COUNTOF(m_sControls)-1 )
				return false;
			if ( m_iTexts >= COUNTOF(m_sText)-1 )
				return false;

			GET_RELATIVE( x, m_iOriginX );
			GET_RELATIVE( y, m_iOriginY );
			GET_ABSOLUTE( w );
			GET_ABSOLUTE( h );
			GET_ABSOLUTE( hue );
			GET_ABSOLUTE( id );
			SKIP_ALL( pszArgs )

			int	iText	= GumpAddText( *pszArgs ? pszArgs : " " );
			m_sControls[m_iControls].Format( "textentry %d %d %d %d %d %d %d", x, y, w, h, hue, id, iText );
			m_iControls++;
			return true;
		}
		case GUMPCTL_DORIGIN:
		{
			// GET_RELATIVE( x, m_iOriginX );
			// GET_RELATIVE( y, m_iOriginY );
			// m_iOriginX	= x;
			// m_iOriginY	= y;
			
			SKIP_ALL( pszArgs );
			if ( *pszArgs == '-' && (isspace( pszArgs[1] ) || !pszArgs[1]) )		pszArgs++;
			else  if ( *pszArgs == '*' )	m_iOriginX	+= Exp_GetSingle( ++pszArgs );
			else							m_iOriginX	 = Exp_GetSingle( pszArgs );

			SKIP_ALL( pszArgs );
			if ( *pszArgs == '-' && (isspace( pszArgs[1] ) || !pszArgs[1]) )		pszArgs++;
			else  if ( *pszArgs == '*' )	m_iOriginY	+= Exp_GetSingle( ++pszArgs );
			else							m_iOriginY	= Exp_GetSingle( pszArgs );
			
			return true;
		}
		case GUMPCTL_CROPPEDTEXT:
		case GUMPCTL_TEXT:
		case GUMPCTL_TEXTENTRY:
			break;
		default:
			break;
	}

	if ( m_iControls >= COUNTOF(m_sControls)-1 )
		return false;

	m_sControls[m_iControls].Format( "%s %s", pszKey, pszArgs );
	m_iControls++;
	return true;
	EXC_CATCH("CDialogDef");
	return false;
}


CDialogDef::CDialogDef( RESOURCE_ID rid ) :
	CResourceLink( rid )
{
		m_iControls		= 0;
		m_iTexts		= 0;
		m_pObj		= NULL;
}


bool	CDialogDef::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	if ( !m_pObj )
		return false;
	return m_pObj->r_WriteVal( pszKey, sVal, pSrc );
}


bool		CDialogDef::r_LoadVal( CScript & s )
{
	if ( !m_pObj )
		return false;
	return m_pObj->r_LoadVal( s );
}


bool CDialogDef::GumpSetup( int iPage, CClient * pClient, CObjBase * pObjSrc, LPCTSTR Arguments )
{
	CResourceLock	s;

	m_iControls		= 0;
	m_iTexts		= 0;
	m_pObj			= pObjSrc;
	m_iOriginX		= 0;
	m_iOriginY		= 0;
	m_iPage			= iPage;

	CScriptTriggerArgs	Args	( iPage, 0, pObjSrc );
	Args.m_s1_raw = Args.m_s1 = Arguments;

	// read text first
	if ( g_Cfg.ResourceLock( s, RESOURCE_ID( RES_DIALOG, GetResourceID().GetResIndex(), RES_DIALOG_TEXT ) ) )
	{
		while ( s.ReadKey())
		{
			if ( m_iTexts >= COUNTOF(m_sText)-1 )
				break;
			m_pObj->ParseText( s.GetKeyBuffer(), pClient->GetChar() );
			m_sText[m_iTexts] = s.GetKey();
			m_iTexts++;
		}
	}
	else
	{
		// no gump text?
	}

	
	// read the main dialog
	if ( !ResourceLock( s ) )
		return false;

	if ( !s.ReadKey() )		// read the size.
		return( false );

	// starting x,y location.
	int iSizes[2];

	TCHAR * pszBuf = s.GetKeyBuffer();
	m_pObj->ParseText( pszBuf, pClient->GetChar() );

	Str_ParseCmds( pszBuf, iSizes, COUNTOF(iSizes) );
	m_x		= iSizes[0];
	m_y		= iSizes[1];


	if ( OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pClient->GetChar(), &Args ) == TRIGRET_RET_TRUE )
		return false;

	return true;
}



bool CClient::Dialog_Setup( CLIMODE_TYPE mode, RESOURCE_ID_BASE rid, int iPage, CObjBase * pObj, LPCTSTR Arguments )
{
	if ( pObj == NULL )
		return( false );

	CResourceDef *	pRes	= g_Cfg.ResourceGetDef( rid );
	CDialogDef *	pDlg	= dynamic_cast <CDialogDef*>(pRes);
	if ( !pRes )
	{
		DEBUG_ERR(("Invalid RES_DIALOG." DEBUG_CR ));
		return false;
	}

	if ( !pDlg->GumpSetup( iPage, this, pObj, Arguments ) )
		return false;

	// Now pack it up to send,
	// m_tmGumpDialog.m_ResourceID = rid;
	addGumpDialog( mode, pDlg->m_sControls, pDlg->m_iControls, pDlg->m_sText, pDlg->m_iTexts, pDlg->m_x, pDlg->m_y, pObj, rid );
	return( true );
}




void CClient::addGumpInpVal( bool fCancel, INPVAL_STYLE style,
	DWORD iMaxLength,
	LPCTSTR pszText1,
	LPCTSTR pszText2,
	CObjBase * pObj )
{
	// CLIMODE_INPVAL
	// Should result in Event_GumpInpValRet
	// just input an objects attribute.

	// ARGS:
	// 	m_Targ_UID = pObj->GetUID();
	//  m_Targ_Text = verb

	ASSERT( pObj );

	CCommand cmd;

	cmd.GumpInpVal.m_Cmd = XCMD_GumpInpVal;
	cmd.GumpInpVal.m_len = sizeof(cmd.GumpInpVal);

	cmd.GumpInpVal.m_UID = pObj->GetUID();
	cmd.GumpInpVal.m_context = CLIMODE_INPVAL; // LOWORD( m_tmGumpDialog.m_ResourceID); // CLIMODE_INPVAL;

	cmd.GumpInpVal.m_textlen1 = sizeof(cmd.GumpInpVal.m_text1);
	strcpy( cmd.GumpInpVal.m_text1, pszText1 );

	cmd.GumpInpVal.m_cancel = fCancel ? 1 : 0;
	cmd.GumpInpVal.m_style = (BYTE) style;
	cmd.GumpInpVal.m_mask = iMaxLength;	// !!!
	cmd.GumpInpVal.m_textlen2 = sizeof(cmd.GumpInpVal.m_text2);
	cmd.GumpInpVal.m_text2[0] = '\0';	// clear just in case.

	switch ( style )
	{
	case INPVAL_STYLE_NOEDIT: // None
		break;
	case INPVAL_STYLE_TEXTEDIT: // Text
		sprintf(cmd.GumpInpVal.m_text2,
			"%s (%i chars max)", pszText2, iMaxLength );
		break;
	case INPVAL_STYLE_NUMEDIT: // Numeric
		sprintf(cmd.GumpInpVal.m_text2,
			"%s (0 - %i)", pszText2, iMaxLength );
		break;
	}

	xSendPkt( &cmd, sizeof(cmd.GumpInpVal));

	// m_tmInpVal.m_UID = pObj->GetUID();
	// m_tmInpVal.m_PrvGumpID = m_tmGumpDialog.m_ResourceID;

	m_Targ_UID = pObj->GetUID();
	// m_Targ_Text = verb
	SetTargMode( CLIMODE_INPVAL );
}


void CClient::addGumpDialog( CLIMODE_TYPE mode, const CGString * psControls, int iControls, const CGString * psText, int iTexts, int x, int y, CObjBase * pObj, DWORD rid )
{
	// Add a generic GUMP menu.
	// Should return a Event_GumpDialogRet
	// NOTE: These packets can get rather LARGE.
	// x,y = where on the screen ?

	if ( pObj == NULL )
		pObj = m_pChar;
	int lengthControls=1;
	int i=0;
	for ( ; i < iControls; i++)
	{
		lengthControls += psControls[i].GetLength() + 2;
	}

	int lengthText = lengthControls + 20 + 3;
	for ( i=0; i < iTexts; i++)
	{
		int lentext2 = psText[i].GetLength();
		DEBUG_CHECK( lentext2 < MAX_TALK_BUFFER );
		lengthText += (lentext2*2)+2;
	}

	int	context_mode	= mode;
	if ( mode == CLIMODE_DIALOG && rid != 0 )
	{
		context_mode	= GETINTRESOURCE( rid );
	}

	// Send the fixed length stuff
	CCommand cmd;
	cmd.GumpDialog.m_Cmd = XCMD_GumpDialog;
	cmd.GumpDialog.m_len = lengthText;
	cmd.GumpDialog.m_UID = pObj->GetUID();
	cmd.GumpDialog.m_context = context_mode;
	cmd.GumpDialog.m_x = x;
	cmd.GumpDialog.m_y = y;
	cmd.GumpDialog.m_lenCmds = lengthControls;
	xSend( &cmd, 21, true );

	TCHAR	*pszMsg = Str_GetTemp();
	for ( i=0; i<iControls; i++)
	{
		sprintf(pszMsg, "{%s}", (LPCTSTR) psControls[i]);
		xSend(pszMsg, strlen(pszMsg), true );
	}

	// Pack up the variable length stuff
	BYTE Pkt_gump2[3];
	Pkt_gump2[0] = '\0';
	PACKWORD( &Pkt_gump2[1], iTexts );
	xSend( Pkt_gump2, 3, true);

	// Pack in UNICODE type format.
	for ( i=0; i < iTexts; i++)
	{
		int len1 = psText[i].GetLength();

		NWORD len2;
		len2 = len1;
		xSend( &len2, sizeof(NWORD), true);
		if ( len1 )
		{
			NCHAR szTmp[MAX_TALK_BUFFER];
			int len3 = CvtSystemToNUNICODE( szTmp, COUNTOF(szTmp), psText[i], -1 );
			xSend( szTmp, len2*sizeof(NCHAR), true);
		}
	}
	
	// Send the queued dialog
	xFlush();

	// m_tmGumpDialog.m_UID = pObj->GetUID();

	// SetTargMode( mode );
	
	if ( m_pChar )
	{
		CItemMemory * pMemory = m_pChar->Memory_AddObj(this->GetChar(), MEMORY_GUMPRECORD);
		pMemory->m_itNormal.m_more1 = context_mode;
		pMemory->m_itNormal.m_more2 = pObj->GetUID();
		pMemory->SetName("Gump Memory");

		if (context_mode == CLIMODE_DIALOG_ADMIN)
			pMemory->GetTagDefs()->SetStr( "dialog_name", false, "clientlist");
		else if (context_mode == CLIMODE_DIALOG_HAIR_DYE)
			pMemory->GetTagDefs()->SetStr( "dialog_name", false, "hairdye");
		else if (context_mode == CLIMODE_DIALOG_GUILD)
			pMemory->GetTagDefs()->SetStr( "dialog_name", false, "guild");
		else
		{
			CResourceDef *	pRes = g_Cfg.ResourceGetDef(RESOURCE_ID( RES_DIALOG, context_mode ));
			if ( !pRes )
				pMemory->GetTagDefs()->SetStr( "dialog_name", false, "undef");
			else
			{
				CDialogDef * pDlg = dynamic_cast <CDialogDef*>(pRes);
				if ( !pDlg )
					pMemory->GetTagDefs()->SetStr( "dialog_name", false, "undef");
				else
					pMemory->GetTagDefs()->SetStr( "dialog_name", false, (LPCTSTR) pDlg->GetName());
			}
		}

		if (context_mode == CLIMODE_DIALOG_HAIR_DYE || context_mode == CLIMODE_DIALOG_GUILD )
		{
			pMemory->GetTagDefs()->SetNum( "targ_uid", (DWORD) m_Targ_UID , false);
		}
	}
	
}

bool CClient::addGumpDialogProps( CGrayUID uid )
{
	// put up a prop dialog for the object.
	CObjBase * pObj = uid.ObjFind();
	if ( pObj == NULL )
		return false;
	if ( m_pChar == NULL )
		return( false );
	if ( ! m_pChar->CanTouch( pObj ))	// probably a security issue.
		return( false );

	m_Prop_UID = m_Targ_UID = uid;
	if ( uid.IsChar())
	{
		addSkillWindow( (SKILL_TYPE) MAX_SKILL ); // load the targets skills
	}

	TCHAR *pszMsg = Str_GetTemp();
	strcpy(pszMsg, pObj->IsItem() ? "d_ITEMPROP1" : "d_CHARPROP1" );

	RESOURCE_ID rid = g_Cfg.ResourceGetIDType(RES_DIALOG, pszMsg);
	if ( ! rid.IsValidUID())
		return false;

	Dialog_Setup( CLIMODE_DIALOG, rid, 0, pObj );
	return( true );
}

void CClient::addGumpSpecial(int mode)
{
	if ( mode == 0 )	// worldsave status
	{
		RESOURCE_ID	rid = g_Cfg.ResourceGetIDType(RES_DIALOG, "d_server_save");
		if ( rid.IsValidUID() )
		{
			Dialog_Setup(CLIMODE_DIALOG, rid, 0, m_pChar);
		}
	}
}

void CClient::addGumpDialogAdmin( int iAdPage )
{
	// Alter this routine at your own risk....if anymore
	// bytes get sent to the client, you WILL crash them
	// Take away, but don't add (heh) (actually there's like
	// 100 bytes available, but they get eaten up fast with
	// this gump stuff)

	static LPCTSTR const sm_szGumps[] = // These are on every page and don't change
	{
		"page 0",
		"resizepic 0 0 5120 623 310",
		"resizepic 242 262 5120 170 35",
		"text 230 10 955 0",
		"text 25 30 955 1",
		"text 78 30 955 2",
		"text 195 30 955 3",
		"text 335 30 955 4",
		"text 481 30 955 5"
	};

	CGString sControls[256];
	int iControls = 0;
	while ( iControls<COUNTOF(sm_szGumps))
	{
		sControls[iControls] = sm_szGumps[iControls];
		iControls++;
	}

	static LPCTSTR const sm_szText[] = // These are on every page and don't change
	{
		"Admin Control Panel",
		"Sock",
		"Account",
		"Name",
		"IP Address",
		"Location"
	};

	CGString sText[256];
	int iTexts=0;
	while ( iTexts<COUNTOF(sm_szText))
	{
		sText[iTexts] = sm_szText[iTexts];
		iTexts++;
	}

	// set standard gump arguments
	int iY = 50;
	int iIndex = COUNTOF(sm_szText);

	CClient * pClient = g_Serv.GetClientHead();
	int i=0;
	for ( ; pClient && i<iAdPage * ADMIN_CLIENTS_PER_PAGE; pClient = pClient->GetNext())
	{
		if ( ! m_pChar->CanDisturb( pClient->GetChar()))
			continue;
		i++;
	}

	// Valid max page
	if ( pClient == NULL )
	{
		iAdPage = 0;
		pClient = g_Serv.GetClientHead();
	}

	m_tmGumpAdmin.m_Page = iAdPage;

	for ( i=0; pClient && i<ADMIN_CLIENTS_PER_PAGE; pClient = pClient->GetNext())
	{
		CChar * pChar = pClient->GetChar();
		if ( ! m_pChar->CanDisturb( pChar ))
			continue;

		// Sock buttons
		//	X, Y, Down gump, Up gump, pressable, iPage, id
		sControls[iControls++].Format(
			"button 12 %i 2362 2361 1 1 %i",
			iY + 4, // Y
			901 + i );	// id

		static const int sm_iGumpsSpaces[] = // These are on every page and don't change
		{
			19,		// Text (sock)
			73,		// Text (account name)
			188,	// Text (name)
			328,	// Text (ip)
			474		// Text (location)
		};

		for ( int j=0; j<COUNTOF(sm_iGumpsSpaces); j++ )
		{
			sControls[iControls++].Format(
				"text %i %i 955 %i",
				8 + sm_iGumpsSpaces[j],
				iY,
				iIndex );
			
			iIndex ++; // advance to the next line of text
		}

		iY += 20; // go down a 'line' on the dialog

		// CSocketAddress PeerName = pClient->m_Socket.GetPeerName();

		TCHAR accountName[MAX_NAME_SIZE];
		strcpylen( accountName, pClient->GetName(), 12 );
		// max name size for this dialog....otherwise CRASH!!!, not a big enuf buffer :(

		if ( pClient->IsPriv(PRIV_GM) || pClient->GetPrivLevel() >= PLEVEL_Counsel )
		{
			memmove( accountName+1, accountName, 13 );
			accountName[0] = ( pChar && pChar->IsDND()) ? '*' : '+';
		}

		ASSERT( i<COUNTOF(m_tmGumpAdmin.m_Item));
		if ( pChar != NULL )
		{
			m_tmGumpAdmin.m_Item[i] = pChar->GetUID();

			TCHAR characterName[MAX_NAME_SIZE];
			strcpylen(characterName, pChar->GetName(), 30);
			characterName[12] = 0; // same comment as the accountName...careful with this stuff!
			sText[iTexts++].Format("%x", pClient->m_Socket.GetSocket());
			sText[iTexts++] = accountName;
			sText[iTexts++] = characterName;

			CPointMap pt = pChar->GetTopPoint();
			sText[iTexts++].Format( "%s", pClient->m_PeerName.GetAddrStr()) ;
			sText[iTexts++].Format( "%d,%d,%d [%d]",
				pt.m_x,
				pt.m_y,
				pt.m_z,
				pt.m_map) ;
		}
		else
		{
			m_tmGumpAdmin.m_Item[i] = 0;

			sText[iTexts++].Format( "%03x", pClient->m_Socket.GetSocket());
			sText[iTexts++] = accountName;
			sText[iTexts++] = "N/A";
			sText[iTexts++] = m_PeerName.GetAddrStr();
			sText[iTexts++] = "N/A";
		}
		i++;
	}

	if ( m_tmGumpAdmin.m_Page )	// is there a previous ?
	{
		sControls[iControls++] = "button 253 267 5537 5539 1 0 801"; // p
	}
	if ( pClient )	// is there a next ?
	{
		sControls[iControls++] = "button 385 267 5540 5542 1 0 802"; // n
	}
	
	addGumpDialog( CLIMODE_DIALOG_ADMIN, sControls, iControls, sText, iTexts, 0x05, 0x46 );
}

TRIGRET_TYPE CClient::Dialog_OnButton( RESOURCE_ID_BASE rid, DWORD dwButtonID, CObjBase * pObj, CDialogResponseArgs * pArgs )
{
	// one of the gump dialog buttons was pressed.
	if ( pObj == NULL )		// object is gone ?
		return TRIGRET_ENDIF;

	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, RESOURCE_ID( RES_DIALOG, rid.GetResIndex(), RES_DIALOG_BUTTON )))
	{
		return TRIGRET_ENDIF;
	}

	// Set the auxiliary stuff for INPDLG here
	// m_tmInpVal.m_PrvGumpID	= rid;
	// m_tmInpVal.m_UID		= pObj ? pObj->GetUID() : (CGrayUID) 0;

	int piCmd[3];
	int iArgs;
	while ( s.ReadKeyParse())
	{
		if ( ! s.IsKeyHead( "ON", 2 ))
			continue;

		iArgs = Str_ParseCmds( s.GetArgStr(), piCmd, COUNTOF(piCmd) );
		if ( iArgs == 0 )		continue;

		if ( iArgs == 1 ?
				piCmd[0] != dwButtonID
			 :	( dwButtonID < piCmd[0]  || dwButtonID > piCmd[1] ) )
			continue;

		pArgs->m_iN1	= dwButtonID;		
		return pObj->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, m_pChar, pArgs );
	}

	return( TRIGRET_ENDIF );
}


bool CClient::Dialog_Close( CObjBase * pObj, RESOURCE_ID_BASE rid, int buttonID )
{
	CExtData ExtData;
	ExtData.GumpChange.dialogID		= GETINTRESOURCE( rid );
	ExtData.GumpChange.buttonID		= buttonID;
	addExtData( EXTDATA_GumpChange, &ExtData, sizeof(ExtData.GumpChange) );
	return true;
}




TRIGRET_TYPE CClient::Menu_OnSelect( RESOURCE_ID_BASE rid, int iSelect, CObjBase * pObj ) // Menus for general purpose
{
	// A select was made. so run the script.
	// iSelect = 0 = cancel.

	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, rid ))
	{
		// Can't find the resource ?
		return( TRIGRET_ENDIF );
	}

	if ( pObj == NULL )
		pObj = m_pChar;

	// execute the menu script.
	int i=0;	// 1 based selection.
	while ( s.ReadKeyParse())
	{
		if ( ! s.IsKey( "ON" ))
			continue;

		i++;
		if ( i < iSelect )
			continue;
		if ( i > iSelect )
			break;

		return pObj->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, m_pChar, NULL );
	}

	// No selection ?
	return( TRIGRET_ENDIF );
}

bool CMenuItem::ParseLine( TCHAR * pszArgs, CScriptObj * pObjBase, CTextConsole * pSrc )
{
	TCHAR * pszArgStart = pszArgs;
	while ( _ISCSYM( *pszArgs ))
		pszArgs++;

	if ( *pszArgs )
	{
		*pszArgs = '\0';
		pszArgs++;
		GETNONWHITESPACE( pszArgs );
	}

	// The item id (if we want to have an item type menu) or 0

	if ( strcmp( pszArgStart, "0" ))
	{
		m_id = (ITEMID_TYPE) g_Cfg.ResourceGetIndexType( RES_ITEMDEF, pszArgStart );
		CItemBase * pItemBase = CItemBase::FindItemBase( (ITEMID_TYPE) m_id );
		if ( pItemBase != NULL )
		{
			m_id = pItemBase->GetDispID();
			pObjBase = pItemBase;
		}
		else
		{
			DEBUG_ERR(( "Bad MENU item id '%s'\n", pszArgStart ));
			return( false );	// skip this.
		}
	}
	else
	{
		m_id = 0;
	}

	if ( pObjBase != NULL )
	{
		pObjBase->ParseText( pszArgs, pSrc );
	}
	else
	{
		g_Serv.ParseText( pszArgs, pSrc );
	}

	m_sText = pszArgs;

	if ( m_sText.IsEmpty())
	{
		if ( pObjBase )	// use the objects name by default.
		{
			m_sText = pObjBase->GetName();	
			if ( ! m_sText.IsEmpty())
				return( true );
		}
		DEBUG_ERR(( "Bad MENU item text '%s'\n", pszArgStart ));
	}

	return( !m_sText.IsEmpty() );
}

void CClient::Menu_Setup( RESOURCE_ID_BASE rid, CObjBase * pObj )
{
	// Menus for general purpose
	// Expect Event_MenuChoice() back.

	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, rid ))
	{
		return;
	}

	if ( pObj == NULL )
	{
		pObj = m_pChar;
	}

	s.ReadKey();	// get title for the menu.
	pObj->ParseText( s.GetKeyBuffer(), m_pChar );

	CMenuItem item[MAX_MENU_ITEMS];
	item[0].m_sText = s.GetKey();
	// item[0].m_id = rid.m_internalrid;	// general context id

	int i=0;
	while (s.ReadKeyParse())
	{
		if ( ! s.IsKey( "ON" ))
			continue;
		if ( ++i >= COUNTOF( item ))
			break;
		if ( ! item[i].ParseLine( s.GetArgRaw(), pObj, m_pChar ))
		{
			i--;			
		}
	}

	m_tmMenu.m_ResourceID = rid;

	addItemMenu( CLIMODE_MENU, item, i, pObj );
}


