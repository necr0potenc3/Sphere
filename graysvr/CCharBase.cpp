//
// CCharBase.cpp
// Copyright Menace Software (www.menasoft.com).
//
#include "graysvr.h"	// predef header.

extern CRaceClassDef g_BaseRaceClass;

/////////////////////////////////////////////////////////////////
// -CCharBase

CCharBase::CCharBase( CREID_TYPE id ) :
	CBaseBaseDef( RESOURCE_ID( RES_CHARDEF, id ))
{
	m_iHireDayWage = 0;
	m_trackID = ITEMID_TRACK_WISP;
	m_soundbase = 0;
	m_defense = 0;
	m_Anims = 0xFFFFFF;
  	m_MaxFood = 15;			// Default value
	m_wBloodHue = 0;
	m_Str = 0;
	m_Dex = 0;
	m_pRaceClass = &g_BaseRaceClass;

	m_iMoveRate = 100;		//	default move rate is 100%

	if ( IsValidDispID(id))
	{
		// in display range.
		m_wDispIndex = id;
	}
	else
	{
		m_wDispIndex = 0;	// must read from SCP file later
	}
	
	m_ResDispDnHue = HUE_DEFAULT;
	m_ResDispDnId = m_wDispIndex;
}

LPCTSTR CCharBase::GetTradeName() const
{
	// From "Bill the carpenter" or "#HUMANMALE the Carpenter",
	// Get "Carpenter"
	LPCTSTR pName = CBaseBaseDef::GetTypeName();
	if ( pName[0] != '#' )
		return( pName );

	LPCTSTR pSpace = strchr( pName, ' ' );
	if ( pSpace == NULL )
		return( pName );

	pSpace++;
	if ( ! strnicmp( pSpace, "the ", 4 ))
		pSpace += 4;
	return( pSpace );
}

void CCharBase::CopyBasic( const CCharBase * pCharDef )
{
	m_trackID = pCharDef->m_trackID;
	m_soundbase = pCharDef->m_soundbase;

	m_wBloodHue = pCharDef->m_wBloodHue;
	m_MaxFood = pCharDef->m_MaxFood;
	m_FoodType = pCharDef->m_FoodType;
	m_Desires = pCharDef->m_Desires;

	m_defense = pCharDef->m_defense;
	m_Anims = pCharDef->m_Anims;

	m_BaseResources = pCharDef->m_BaseResources;
	
	m_ResDispDnHue = pCharDef->m_ResDispDnHue;
	m_ResDispDnId = pCharDef->m_ResDispDnId;

	CBaseBaseDef::CopyBasic( pCharDef );	// This will overwrite the CResourceLink!!
}

bool CCharBase::SetDispID( CREID_TYPE id )
{
	// Setting the "ID" for this.
	if ( id == GetID())
		return true;
	if ( id == GetDispID())
		return true;
	if ( ! IsValidDispID( id ))
	{
		DEBUG_ERR(( "Creating char SetDispID(0%x) > %d" DEBUG_CR, id, CREID_QTY ));
		return false; // must be in display range.
	}

	// Copy the rest of the stuff from the display base.
	// NOTE: This can mess up g_Serv.m_scpChar offset!!
	CCharBase * pCharDef = FindCharBase( id );
	if ( pCharDef == NULL )
	{
		DEBUG_ERR(( "Creating char SetDispID(0%x) BAD" DEBUG_CR, id ));
		return( false );
	}

	CopyBasic( pCharDef );
	return( true );
}

void CCharBase::SetFoodType( LPCTSTR pszFood )
{
  	m_FoodType.Load( pszFood );

  	// Try to determine the real value
	m_MaxFood = 0;
	for ( int i=0; i<m_FoodType.GetCount(); i++ )
	{
		if ( m_MaxFood < m_FoodType[i].GetResQty())
			m_MaxFood = m_FoodType[i].GetResQty();
	}
}

enum CBC_TYPE
{
	CBC_ANIM,
	CBC_ARMOR,
	CBC_AVERSIONS,
	CBC_BLOODCOLOR,
	CBC_DESIRES,
	CBC_DEX,
	CBC_DISPID,
	CBC_FOODTYPE,
	CBC_HIREDAYWAGE,
	CBC_ICON,
	CBC_ID,
	CBC_JOB,
	CBC_MAXFOOD,
	CBC_MOVERATE,
	CBC_RESDISPDNHUE,
	CBC_RESDISPDNID,
	CBC_SOUND,
	CBC_STR,
	CBC_TEVENTS,
	CBC_TSPEECH,
	CBC_QTY,
};

LPCTSTR const CCharBase::sm_szLoadKeys[CBC_QTY+1] =
{
	"ANIM",
	"ARMOR",
	"AVERSIONS",
	"BLOODCOLOR",
	"DESIRES",
	"DEX",
	"DISPID",
	"FOODTYPE",
	"HIREDAYWAGE",
	"ICON",
	"ID",
	"JOB",
	"MAXFOOD",
	"MOVERATE",
	"RESDISPDNHUE",
	"RESDISPDNID",
	"SOUND",
	"STR",
	"TEVENTS",
	"TSPEECH",
	NULL,
};

bool CCharBase::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pChar )
{
	EXC_TRY(("r_WriteVal('%s',,%x)", pszKey, pChar));
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	case CBC_ANIM:
		sVal.FormatHex( m_Anims );
		break;
	case CBC_AVERSIONS:
		{
			TCHAR *pszTmp = Str_GetTemp();
			m_Aversions.WriteKeys(pszTmp);
			sVal = pszTmp;
		}
		break;
	case CBC_BLOODCOLOR:
		sVal.FormatHex( m_wBloodHue );
		break;
	case CBC_ARMOR:
		sVal.FormatVal( m_defense );
		break;
	case CBC_DESIRES:
		{
			TCHAR *pszTmp = Str_GetTemp();
			m_Desires.WriteKeys(pszTmp);
			sVal = pszTmp;
		}
		break;
	case CBC_DEX:
		sVal.FormatVal( m_Dex );
		break;
	case CBC_DISPID:
		sVal = g_Cfg.ResourceGetName( RESOURCE_ID( RES_CHARDEF, GetDispID()));
		break;
	case CBC_FOODTYPE:
		{
			TCHAR *pszTmp = Str_GetTemp();
			m_FoodType.WriteKeys(pszTmp);
			sVal = pszTmp;
		}
		break;
	case CBC_HIREDAYWAGE:
		sVal.FormatVal( m_iHireDayWage );
		break;
	case CBC_ICON:
		sVal.FormatHex( m_trackID );
		break;
	case CBC_JOB:
		sVal = GetTradeName();
		break;
	case CBC_MAXFOOD:
		sVal.FormatVal( m_MaxFood );
		break;
	case CBC_MOVERATE:
		sVal.FormatVal(m_iMoveRate);
		break;
	case CBC_RESDISPDNHUE:
		sVal.FormatHex( m_ResDispDnHue );
		break;
	case CBC_RESDISPDNID:
		sVal = g_Cfg.ResourceGetName( RESOURCE_ID( RES_CHARDEF, m_ResDispDnId));
		break;
	case CBC_SOUND:
		sVal.FormatHex( m_soundbase );
		break;
	case CBC_STR:
		sVal.FormatVal( m_Str );
		break;
	case CBC_TSPEECH:
		m_Speech.WriteResourceRefList( sVal );
		break;
	case CBC_TEVENTS:
		m_TEvents.WriteResourceRefList(sVal);
		break;
	default:
		return( CBaseBaseDef::r_WriteVal( pszKey, sVal ));
	}
	return true;
	EXC_CATCH("CCharBase");
	return false;
}

void CCharBase::r_DumpLoadKeys( CTextConsole * pSrc )
{
	r_DumpKeys(pSrc,sm_szLoadKeys);
	CBaseBaseDef::r_DumpLoadKeys(pSrc);
}

bool CCharBase::r_LoadVal( CScript & s )
{
	LOCKDATA;
	EXC_TRY(("r_LoadVal('%s %s')", s.GetKey(), s.GetArgStr()));
	if ( ! s.HasArgs())
		return( false );
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1 ))
	{
	// compatibility for now
	case CBC_ANIM:
		m_Anims = s.GetArgVal();
		break;
	case CBC_AVERSIONS:
		m_Aversions.Load( s.GetArgStr() );
		break;
	case CBC_BLOODCOLOR:
		m_wBloodHue = s.GetArgVal();
		break;
	case CBC_ARMOR:
		m_defense = s.GetArgVal();
		break;
	case CBC_DESIRES:
		m_Desires.Load( s.GetArgStr() );
		break;
	case CBC_DEX:
		m_Dex = s.GetArgVal();
		break;
	case CBC_DISPID:
		return( false );
	case CBC_FOODTYPE:
		SetFoodType( s.GetArgStr());
		break;
	case CBC_HIREDAYWAGE:
		m_iHireDayWage = s.GetArgVal();
		break;
	case CBC_ICON:
		{
			ITEMID_TYPE id = (ITEMID_TYPE) g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr());
			if ( id < 0 || id >= ITEMID_MULTI )
			{
				return( false );
			}
			m_trackID = id;
		}
		break;
	case CBC_ID:
		{
			return SetDispID( (CREID_TYPE) g_Cfg.ResourceGetIndexType( RES_CHARDEF, s.GetArgStr()));
		}
	case CBC_MAXFOOD:
		m_MaxFood = s.GetArgVal();
		break;
	case CBC_MOVERATE:
		m_iMoveRate = s.GetArgVal();
		break;
	case CBC_RESDISPDNHUE:
		m_ResDispDnHue = (HUE_TYPE) s.GetArgVal();
		break;
	case CBC_RESDISPDNID:
		m_ResDispDnId = g_Cfg.ResourceGetIndexType( RES_CHARDEF, s.GetArgStr());
		break;
	case CBC_SOUND:
		m_soundbase = s.GetArgVal();
		break;
	case CBC_STR:
		m_Str = s.GetArgVal();
		break;
	case CBC_TSPEECH:
		return( m_Speech.r_LoadVal( s, RES_SPEECH ));
	case CBC_TEVENTS:
		return ( m_TEvents.r_LoadVal(s, RES_EVENTS));
	default:
		return( CBaseBaseDef::r_LoadVal( s ));
	}
	return true;
	EXC_CATCH("CCharBase");
	return false;
}

bool CCharBase::r_Load( CScript & s )
{
	// Do a prelim read from the script file.

	CScriptObj::r_Load(s);

	if ( ! IsValidDispID( GetDispID()))
	{
		// Bad display id ? could be an advance gate.
		DEBUG_WARN(( "Char Script '%s' has bad dispid 0%x" DEBUG_CR, GetResourceName(), GetDispID()));
		m_wDispIndex = CREID_MAN;
	}
	if ( m_sName.IsEmpty())
	{
		DEBUG_ERR(( "Char Script '%s' has no name!" DEBUG_CR, GetResourceName()));
		return( false );
	}
	return( true );
}

////////////////////////////////////////////

CCharBase * CCharBase::FindCharBase( CREID_TYPE baseID ) // static
{
	// find it (or near it) if already loaded.
	RESOURCE_ID rid = RESOURCE_ID( RES_CHARDEF, baseID );
	int index = g_Cfg.m_ResHash.FindKey(rid);
	if ( index < 0 )
		return NULL;

	CResourceLink * pBaseLink = STATIC_CAST <CResourceLink *> ( g_Cfg.m_ResHash.GetAt(rid,index));
	ASSERT(pBaseLink);
	CCharBase * pBase = dynamic_cast <CCharBase *> (pBaseLink);
	if ( pBase )
		return( pBase );	// already loaded.

	// create a new base.
	pBase = new CCharBase(baseID);
	ASSERT(pBase);
	pBase->CResourceLink::CopyTransfer(pBaseLink);

	// replace existing one
	g_Cfg.m_ResHash.SetAt(rid, index, pBase);

	// load it's data on demand.
	CResourceLock s;
	if ( !pBase->ResourceLock(s))
		return( NULL );
	if ( !pBase->r_Load(s))
		return( NULL );

	return pBase;
}

