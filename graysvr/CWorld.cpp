//
// CWorld.CPP
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.
#include "../common/grayver.h"	// sphere version

#if !defined( _WIN32 )
#include <sys/time.h>
#endif

static const SOUND_TYPE sm_Sounds_Ghost[] =
{
	SOUND_GHOST_1,
	SOUND_GHOST_2,
	SOUND_GHOST_3,
	SOUND_GHOST_4,
	SOUND_GHOST_5,
};

//////////////////////////////////////////////////////////////////
// -CWorldSearch

CWorldSearch::CWorldSearch( const CPointMap & pt, int iDist ) :
	m_pt( pt ),
	m_iDist( iDist )
{
	// define a search of the world.
	m_fAllShow = false;
	m_pObj = m_pObjNext = NULL;
	m_fInertToggle = false;

	m_pSectorBase = m_pSector = pt.GetSector();

	m_rectSector.SetRect( 
		pt.m_x - iDist,
		pt.m_y - iDist,
		pt.m_x + iDist + 1,
		pt.m_y + iDist + 1,
		pt.m_map);

	// Get upper left of search rect.
	m_iSectorCur = 0;
}

bool CWorldSearch::GetNextSector()
{
	// Move search into nearby CSector(s) if necessary

	if ( ! m_iDist )
		return( false );

	while (true)
	{
		m_pSector = m_rectSector.GetSector(m_iSectorCur++);
		if ( m_pSector == NULL )
			return( false );	// done searching.
		if ( m_pSectorBase == m_pSector )
			continue;	// same as base.
		m_pObj = NULL;	// start at head of next Sector.
		return( true );
	}

	return( false );	// done searching.
}

CItem * CWorldSearch::GetItem()
{
	while (true)
	{
		if ( m_pObj == NULL )
		{
			m_fInertToggle = false;
			m_pObj = STATIC_CAST <CObjBase*> ( m_pSector->m_Items_Inert.GetHead());
		}
		else
		{
			m_pObj = m_pObjNext;
		}
		if ( m_pObj == NULL )
		{
			if ( ! m_fInertToggle )
			{
				m_fInertToggle = true;
				m_pObj = STATIC_CAST <CObjBase*> ( m_pSector->m_Items_Timer.GetHead());
				if ( m_pObj != NULL )
					goto jumpover;
			}
			if ( GetNextSector())
				continue;
			return( NULL );
		}

jumpover:
		m_pObjNext = m_pObj->GetNext();
		if ( m_fAllShow )
		{
			if ( m_pt.GetDistBase( m_pObj->GetTopPoint()) <= m_iDist )
				return( STATIC_CAST <CItem *> ( m_pObj ));
		}
		else
		{
			if ( m_pt.GetDist( m_pObj->GetTopPoint()) <= m_iDist )
				return( STATIC_CAST <CItem *> ( m_pObj ));
		}
	}
}

CChar * CWorldSearch::GetChar()
{
	while (true)
	{
		if ( m_pObj == NULL )
		{
			m_fInertToggle = false;
			m_pObj = STATIC_CAST <CObjBase*> ( m_pSector->m_Chars_Active.GetHead());
		}
		else
		{
			m_pObj = m_pObjNext;
		}
		if ( m_pObj == NULL )
		{
			if ( ! m_fInertToggle && m_fAllShow )
			{
				m_fInertToggle = true;
				m_pObj = STATIC_CAST <CObjBase*> ( m_pSector->m_Chars_Disconnect.GetHead());
				if ( m_pObj != NULL )
					goto jumpover;
			}
			if ( GetNextSector())
				continue;
			return( NULL );
		}

jumpover:
		m_pObjNext = m_pObj->GetNext();
		if ( m_fAllShow )
		{
			if ( m_pt.GetDistBase( m_pObj->GetTopPoint()) <= m_iDist )
				return( STATIC_CAST <CChar *> ( m_pObj ));
		}
		else
		{
			if ( m_pt.GetDist( m_pObj->GetTopPoint()) <= m_iDist )
				return( STATIC_CAST <CChar *> ( m_pObj ));
		}
	}
}

//////////////////////////////////////////////////////////////////
// -CWorldThread

CWorldThread::CWorldThread()
{
	m_fSaveParity = false;		// has the sector been saved relative to the char entering it ?
	m_iUIDIndexLast = 1;

	m_FreeUIDs = (DWORD*)calloc(FREE_UIDS_SIZE, sizeof(DWORD));
	m_FreeOffset = -1;
}

CWorldThread::~CWorldThread()
{
	CloseAllUIDs();
}

void CWorldThread::CloseAllUIDs()
{
	m_ObjDelete.DeleteAll();	// clean up our delete list.
	m_ObjNew.DeleteAll();
	m_UIDs.RemoveAll();

	if ( m_FreeUIDs )
	{
		free(m_FreeUIDs);
		m_FreeUIDs = NULL;
	}
	m_FreeOffset = -1;
}

bool CWorldThread::IsSaving() const
{
	return (m_FileWorld.IsFileOpen() && m_FileWorld.IsWriteMode());
}

DWORD CWorldThread::GetUIDCount() const
{
	return m_UIDs.GetCount();
}

CObjBase *CWorldThread::FindUID(DWORD dwIndex) const
{
	if ( !dwIndex || dwIndex >= GetUIDCount() )
		return NULL;
	if ( m_UIDs[ dwIndex ] == UID_PLACE_HOLDER )	// unusable for now. (background save is going on)
		return NULL;
	return m_UIDs[dwIndex];
}

void CWorldThread::FreeUID(DWORD dwIndex)
{
	// Can't free up the UID til after the save !
	m_UIDs[dwIndex] = ( IsSaving()) ? UID_PLACE_HOLDER : NULL;
}

DWORD CWorldThread::AllocUID( DWORD dwIndex, CObjBase * pObj )
{
	DWORD dwCountTotal = GetUIDCount();

	if ( !dwIndex )					// auto-select tbe suitable hole
	{
		if ( !dwCountTotal )		// if the uids array is empty - increase it.
		{
			dwIndex = 1;
			goto setcount;
		}

		if (( m_FreeOffset >= 0 ) && ( m_FreeOffset < FREE_UIDS_SIZE ) && m_FreeUIDs )
		{
			//	We do have a free uid's array. Use it if possible to determine the first free element
			for ( ; m_FreeUIDs[m_FreeOffset]; m_FreeOffset++ )
			{
				//	yes, that's a free slot
				if ( !m_UIDs[m_FreeUIDs[m_FreeOffset]] )
				{
					dwIndex = m_FreeUIDs[m_FreeOffset++];
					goto successalloc;
				}
			}
		}
		m_FreeOffset = -1;	// mark array invalid, since it does not contain any empty slots
							// use default allocation for a while, till the next garbage collection
		DWORD dwCount = dwCountTotal - 1;
		dwIndex = m_iUIDIndexLast;
		while ( m_UIDs[dwIndex] != NULL )
		{
			if ( ! -- dwIndex )
			{
				dwIndex = dwCountTotal - 1;
			}
			if ( ! -- dwCount )
			{
				dwIndex = dwCountTotal;
				goto setcount;
			}
		}
	}
	else if ( dwIndex >= dwCountTotal )
	{
setcount:
		// We have run out of free UID's !!! Grow the array
		m_UIDs.SetCount(( dwIndex + 0x1000 ) &~ 0xFFF );
	}

successalloc:
	m_iUIDIndexLast = dwIndex; // start from here next time so we have even distribution of allocation.
	CObjBase	*pObjPrv = m_UIDs[dwIndex];
	if ( pObjPrv )
	{
		//NOTE: We cannot use Delete() in here because the UID will
		//	still be assigned til the async cleanup time. Delete() will not work here!
		DEBUG_ERR(( "UID conflict delete 0%lx, '%s'" DEBUG_CR, dwIndex, (LPCTSTR) pObjPrv->GetName()));
		delete pObjPrv;
	}
	m_UIDs[dwIndex] = pObj;
	return dwIndex;
}

bool CWorldThread::LoadThreadInit()
{
	if ( GetUIDCount())
		return( true );
	m_UIDs.SetCount( 8*1024 );	// start count. (will grow as needed)
	return( false );
}

void CWorldThread::SaveThreadClose()
{
	for ( int i=1; i<GetUIDCount(); i++ )
	{
		if ( m_UIDs[i] == UID_PLACE_HOLDER )
			m_UIDs[i] = NULL;
	}
	m_FileData.Close();
	m_FileWorld.Close();
	m_FilePlayers.Close();
}

int CWorldThread::FixObjTry( CObjBase * pObj, int iUID )
{
	// RETURN: 0 = success.
	if ( iUID )
	{
		if (( pObj->GetUID() & UID_O_INDEX_MASK ) != iUID )
		{
			// Miss linked in the UID table !!! BAD
			// Hopefully it was just not linked at all. else How the hell should i clean this up ???
			DEBUG_ERR(( "UID 0%x, '%s', Mislinked" DEBUG_CR, iUID, (LPCTSTR) pObj->GetName()));
			return( 0x7101 );
		}
	}
	return pObj->FixWeirdness();
}

int CWorldThread::FixObj( CObjBase * pObj, int iUID )
{
	// Attempt to fix problems with this item.
	// Ignore any children it may have for now.
	// RETURN: 0 = success.
	//

	int iResultCode;

	try
	{
		iResultCode = FixObjTry(pObj,iUID);
	}
	catch ( CGrayError &e )	// catch all
	{
		g_Log.CatchEvent( &e, "FixObj" );
		iResultCode = 0xFFFF;	// bad mem ?
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent(NULL, "FixObj" );
		iResultCode = 0xFFFF;	// bad mem ?
	}

	if ( ! iResultCode )
		return( 0 );

#ifdef _DEBUG
	CItem * pItem = dynamic_cast <CItem*>(pObj);
	CChar * pChar = dynamic_cast <CChar*>(pObj);
#endif

	try
	{
		iUID = pObj->GetUID();

		// is it a real error ?
		if ( pObj->IsItem())
		{
			CItem * pItem = dynamic_cast <CItem*>(pObj);
			if ( pItem
				&& (   pItem->IsType(IT_EQ_MEMORY_OBJ)
					|| pItem->IsType(IT_EQ_MEMORY_OBJ) ) ) // maybe change to horse memory type
			{
				pObj->Delete();
				return iResultCode;
			}
		}

		if ( iResultCode == 0x1203 || iResultCode == 0x1103 )
		{
			CChar * pChar = dynamic_cast <CChar*>(pObj);
			DEBUG_ERR(( "RIDDEN NPC NOT ACTING AS SUCH: UID=0%x, id=0%x '%s', Invalid code=%0x" DEBUG_CR, (DWORD) pChar->GetUID(), pChar->GetBaseID(), pChar->GetName(), iResultCode ));
			pChar ->Skill_Start( NPCACT_RIDDEN );
		}
		else
		{
			DEBUG_ERR(( "UID=0%x, id=0%x '%s', Invalid code=%0x" DEBUG_CR, iUID, pObj->GetBaseID(), (LPCTSTR) pObj->GetName(), iResultCode ));
			pObj->Delete();
		}
	}
	catch ( CGrayError &e )	// catch all
	{
		g_Log.CatchEvent( &e, "UID=0%x, Asserted cleanup", iUID );
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent( NULL, "UID=0%x, Asserted cleanup", iUID );
	}
	return( iResultCode );
}

void CWorldThread::GarbageCollection_New()
{
	EXC_TRY(("GarbageCollection_New()"));
	// Clean up new objects that are never placed.
	if ( m_ObjNew.GetCount())
	{
		g_Log.Event( LOGL_ERROR, "%d Lost object deleted" DEBUG_CR, m_ObjNew.GetCount());
		m_ObjNew.DeleteAll();
	}
	m_ObjDelete.DeleteAll();	// clean up our delete list.
	EXC_CATCH("CWorldThread");
}

void CWorldThread::GarbageCollection_UIDs()
{
	// Go through the m_ppUIDs looking for Objects without links to reality.
	// This can take a while.

	GarbageCollection_New();

	int iCount = 0;
	for ( int i = 1; i < GetUIDCount(); i++ )
	{
		try
		{
			CObjBase * pObj = m_UIDs[i];
			if ( pObj == NULL || pObj == UID_PLACE_HOLDER )
				continue;	// not used.

			// Look for anomolies and fix them (that might mean delete it.)
			int iResultCode = FixObj( pObj, i );
			if ( iResultCode )
			{
				// Do an immediate delete here instead of Delete()
				delete pObj;
				FreeUID(i);	// Get rid of junk uid if all fails..
				continue;
			}

			if (! (iCount & 0x1FF ))
			{
				g_Log.FireEvent( LOGEVENT_GarbageStatus, IMULDIV(iCount, 100, GetUIDCount()));
				g_Serv.PrintPercent( iCount, GetUIDCount());
			}
			iCount ++;
		}
		catch ( CGrayError &e )
		{
			g_Log.CatchEvent( &e, "GarbageCollection_UIDs" );
		}
		catch(...)
		{
			g_Log.CatchEvent( NULL, "GarbageCollection_UIDs" );
		}
	}

	GarbageCollection_New();

	if ( iCount != CObjBase::sm_iCount )	// All objects must be accounted for.
	{
		g_Log.Event( LOGL_ERROR, "Object memory leak %d!=%d" DEBUG_CR, iCount, CObjBase::sm_iCount );
	}
	else
	{
		g_Log.Event( LOGL_EVENT, "%d Objects accounted for" DEBUG_CR, iCount );
	}

	if ( m_FreeUIDs )	// new UID engine - search for empty holes and store it in a huge array
	{					// the size of the array should be enough even for huge shards
						// to survive till next garbage collection
		memset(m_FreeUIDs, 0, FREE_UIDS_SIZE*sizeof(DWORD));
		m_FreeOffset = -1;

		for ( DWORD d = 1; d < GetUIDCount(); d++ )
		{
			CObjBase	*pObj = m_UIDs[d];

			if ( !pObj )
			{
				if ( m_FreeOffset >= ( FREE_UIDS_SIZE - 1 ))
					break;

				m_FreeUIDs[m_FreeOffset++] = d;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////
// -CWorldClock

DWORD CWorldClock::GetSystemClock() // static
{
	// CLOCKS_PER_SEC is the base units. clock_t shuld be the type but use DWORD instead.,
#ifdef _WIN32
	return( clock());
#else
	// LINUX
	// clock_t times(struct tms *buf); ?
    struct timeval tv;
    gettimeofday(&tv, NULL);
    DWORD TempTime;
    TempTime = ((((tv.tv_sec - 912818000) * CLOCKS_PER_SEC) +
		tv.tv_usec / CLOCKS_PER_SEC));
	return (TempTime);
#endif
}

void CWorldClock::InitTime( long lTimeBase )
{
	m_Clock_PrevSys = GetSystemClock();
	m_timeClock.InitTime(lTimeBase);
}

void CWorldClock::Init()
{
	m_Clock_PrevSys = GetSystemClock();
	m_timeClock.Init();
}

bool CWorldClock::Advance()
{
	DWORD Clock_Sys = GetSystemClock();	// get the system time.

	int iTimeSysDiff = Clock_Sys - m_Clock_PrevSys;
	iTimeSysDiff = IMULDIV( TICK_PER_SEC, iTimeSysDiff, CLOCKS_PER_SEC );

	if ( !iTimeSysDiff )
		return false;
	else if ( iTimeSysDiff < 0 )	// assume this will happen sometimes.
	{
		// This is normal. for daylight savings etc.

		DEBUG_ERR(( "WARNING:system clock 0%xh overflow - recycle" DEBUG_CR, Clock_Sys ));
		m_Clock_PrevSys = Clock_Sys;
		// just wait til next cycle and we should be ok
		return false;
	}

	m_Clock_PrevSys = Clock_Sys;

	CServTime Clock_New = m_timeClock + iTimeSysDiff;

	// CServTime is signed !
	// NOTE: This will overflow after 7 or so years of run time !
	if ( Clock_New <= m_timeClock )	// should not happen! (overflow)
	{
		if ( m_timeClock == Clock_New )
		{
			// Weird. This should never happen, but i guess it is harmless  ?!
			g_Log.Event( LOGL_CRIT, "Clock corruption?" );
			return false;
		}

		// Someone has probably messed with the "TIME" value.
		// Very critical !
		g_Log.Event( LOGL_CRIT, "Clock overflow, reset from 0%x to 0%x" DEBUG_CR, m_timeClock, Clock_New );
		m_timeClock = Clock_New;	// this may cause may strange things.
		return false;
	}

	m_timeClock = Clock_New;
	return( true );
}

//////////////////////////////////////////////////////////////////
// -CWorld

CWorld::CWorld()
{
	m_iSaveCountID = 0;
	m_iSaveStage = 0;
	m_timeSector.Init();
	m_timeRespawn.Init();
	m_timeStartup.Init();

	m_Sectors = NULL;
	m_SectorsQty = 0;
}

void CWorld::Init()
{
	EXC_TRY(("Init()"));
	if ( m_Sectors )	//	disable changes on-a-fly
	{
//		g_Log.EventError("Re-allocating sectors storage space on-a-fly disabled." DEBUG_CR);
		return;
	}

	g_MapList.Init();
	//	initialize all sectors
	int	sectors = 0;
	int m = 0;
	for ( m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] ) continue;
		sectors += g_MapList.GetSectorQty(m);
	}

	m_Sectors = (CSector**)malloc(sectors * sizeof(CSector*));

	for ( m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] ) continue;

		g_Log.Event(LOGM_INIT, "Allocating %d sectors for map %d..." DEBUG_CR, g_MapList.GetSectorQty(m), m);
		for ( int s = 0; s < g_MapList.GetSectorQty(m); s++ )
		{
			CSector	*pSector = new CSector;

			if ( !pSector )
			{
				g_Log.EventError("Sector #%d for map %d creation failed (memory exceeded?)" DEBUG_CR, s, m);
				continue;
			}
			pSector->Init(s, m);
			m_Sectors[m_SectorsQty++] = pSector;
		}
	}
	EXC_CATCH("CWorld");
}

CWorld::~CWorld()
{
	Close();
}

///////////////////////////////////////////////
// Loading and Saving.

void CWorld::GetBackupName( CGString & sArchive, LPCTSTR pszBaseDir, TCHAR chType, int iSaveCount ) // static
{
	int iCount = iSaveCount;
	int iGroup = 0;
	for ( ; iGroup<g_Cfg.m_iSaveBackupLevels; iGroup++ )
	{
		if ( iCount & 0x7 )
			break;
		iCount >>= 3;
	}
	sArchive.Format( "%s" GRAY_FILE "b%d%d%c%s",
		pszBaseDir,
		iGroup, iCount&0x07,
		chType,
		GRAY_SCRIPT );
}

bool CWorld::OpenScriptBackup( CScript & s, LPCTSTR pszBaseDir, LPCTSTR pszBaseName, int iSaveCount ) // static
{
	ASSERT(pszBaseName);

	CGString sArchive;
	GetBackupName( sArchive, pszBaseDir, pszBaseName[0], iSaveCount );

	// remove possible previous archive of same name
	remove( sArchive );

	// rename previous save to archive name.
	CGString sSaveName;
	sSaveName.Format( "%s" GRAY_FILE "%s%s", pszBaseDir, pszBaseName, GRAY_SCRIPT );

	if ( rename( sSaveName, sArchive ))
	{
		// May not exist if this is the first time.
		g_Log.Event( LOGM_SAVE|LOGL_WARN, "Rename %s to '%s' FAILED code %d?" DEBUG_CR, (LPCTSTR) sSaveName, (const TCHAR*) sArchive, CGFile::GetLastError() );
	}

	if ( ! s.Open( sSaveName, OF_WRITE|OF_TEXT))
	{
		g_Log.Event( LOGM_SAVE|LOGL_CRIT, "Save '%s' FAILED" DEBUG_CR, (LPCTSTR) sSaveName );
		return( false );
	}

	return( true );
}

bool CWorld::SaveStage() // Save world state in stages.
{
	// Do the next stage of the save.
	// RETURN: true = continue; false = done.
	EXC_TRY(("SaveStage()"));
	bool bRc = true;
	ASSERT( IsSaving());

	if ( m_iSaveStage == -1 ) 
	{
		if ( !g_Cfg.m_fSaveGarbageCollect )
		{
			GarbageCollection_New();
			GarbageCollection_GMPages();
		}
	}
	else if ( m_iSaveStage < m_SectorsQty )
	{
		// NPC Chars in the world secors and the stuff they are carrying.
		// Sector lighting info.
		if ( m_Sectors[m_iSaveStage] )
			m_Sectors[m_iSaveStage]->r_Write();
	}
	else if ( m_iSaveStage == m_SectorsQty )
	{
		g_Serv.m_Profile.r_Write(m_FileData);

		m_FileData.WriteSection("GLOBALS");
		int	i, iQty;
		
		iQty = g_Exp.m_VarGlobals.GetCount();
		for ( i = 0; i < iQty; i++ )
		{
			CVarDefBase	*pVar = g_Exp.m_VarGlobals.GetAt(i);
			LPCTSTR pszVal = pVar->GetValStr();
			CVarDefStr *pVarStr = dynamic_cast <CVarDefStr *>( pVar );
			if ( pVarStr ) m_FileData.WriteKeyFormat(pVar->GetKey(), "\"%s\"", pszVal);
			else m_FileData.WriteKey(pVar->GetKey(), pszVal);
		}

		iQty = g_Cfg.m_RegionDefs.GetCount();
		for ( i = 0; i < iQty; i++ )
		{
			CRegionBase *pRegion = dynamic_cast <CRegionBase*> (g_Cfg.m_RegionDefs.GetAt(i));
			if ( !pRegion || !pRegion->HasResourceName() || !pRegion->m_iModified )
				continue;

			CRegionWorld *pRegionWorld = dynamic_cast <CRegionWorld*> (pRegion);

			if ( IsSetEF(EF_Size_Optimise) )
				m_FileData.WriteSection("WS %s", pRegion->GetResourceName());
			else
				m_FileData.WriteSection("WORLDSCRIPT %s", pRegion->GetResourceName());
			pRegion->r_WriteModified(m_FileData);
		}

		// GM_Pages.
		CGMPage *pPage = STATIC_CAST <CGMPage*>(m_GMPages.GetHead());
		for ( ; pPage != NULL; pPage = pPage->GetNext())
		{
			pPage->r_Write(m_FileData);
		}
	}
	else if ( m_iSaveStage == m_SectorsQty+1 )
	{
		// Save all my servers some place.
		if ( !g_Cfg.m_sMainLogServerDir.IsEmpty() )
		{
			CScript s;
			if ( OpenScriptBackup(s, g_Cfg.m_sMainLogServerDir, "serv", m_iSaveCountID))
			{
				CThreadLockRef lock( &(g_Cfg.m_Servers));
				for ( int i = 0; true; i++ )
				{
					CServerRef pServ = g_Cfg.Server_GetDef(i);
					if ( pServ )
						pServ->r_WriteCreated(s);
				}
			}
			s.WriteSection("EOF");
		}
	}
	else if ( m_iSaveStage == m_SectorsQty+2 )
	{
		// Now make a backup of the account file.
		bRc = g_Accounts.Account_SaveAll();
	}
	else if ( m_iSaveStage == m_SectorsQty+3 )
	{
		// EOF marker to show we reached the end.
		m_FileData.WriteSection("EOF");
		m_FileWorld.WriteSection("EOF");
		m_FilePlayers.WriteSection("EOF");

		m_iSaveCountID++;	// Save only counts if we get to the end winout trapping.
		m_timeSave = GetCurrentTime() + g_Cfg.m_iSavePeriod;	// next save time.

		g_Log.Event(LOGM_SAVE, "World data saved   (%s)." DEBUG_CR, (LPCTSTR) m_FileWorld.GetFilePath());
		g_Log.Event(LOGM_SAVE, "Player data saved  (%s)." DEBUG_CR, (LPCTSTR) m_FilePlayers.GetFilePath());
		g_Log.Event(LOGM_SAVE, "Context data saved (%s)." DEBUG_CR, (LPCTSTR) m_FileData.GetFilePath());

		// Now clean up all the held over UIDs
		SaveThreadClose();
		m_iSaveStage = INT_MAX;
		return false;
	}

	if ( g_Cfg.m_iSaveBackgroundTime )
	{
		int iNextTime = g_Cfg.m_iSaveBackgroundTime / m_SectorsQty;
		if ( iNextTime > TICK_PER_SEC/2 )
			iNextTime = TICK_PER_SEC/2;	// max out at 30 minutes or so.
		m_timeSave = GetCurrentTime() + iNextTime;
	}
	m_iSaveStage++;
	return bRc;

	EXC_CATCH("CWorld");

	m_iSaveStage++;	// to avoid loops, we need to skip the current operation in world save
	return false;
}

bool CWorld::SaveForce() // Save world state
{
	Broadcast( g_Cfg.GetDefaultMsg( DEFMSG_SERVER_WORLDSAVE ) );

	g_Log.FireEvent( LOGEVENT_SaveBegin );
	g_Serv.SetServerMode( SERVMODE_Saving );	// Forced save freezes the system.
	bool	bSave = true;
	bool	bSuccess = true;

	static LPCTSTR const msgs[] =
	{
		"garbage collection",
		"sectors",
		"global variables, regions, gmpages",
		"servers",
		"accounts",
		"",
	};
	const char *pCurBlock = msgs[0];

	while ( bSave )
	{
		try
		{
			if (( m_iSaveStage >= 0 ) && ( m_iSaveStage < m_SectorsQty ))
				pCurBlock = msgs[1];
			else if ( m_iSaveStage == m_SectorsQty )
				pCurBlock = msgs[2];
			else if ( m_iSaveStage == m_SectorsQty+1 )
				pCurBlock = msgs[3];
			else if ( m_iSaveStage == m_SectorsQty+2 )
				pCurBlock = msgs[4];
			else
				pCurBlock = msgs[5];

			bSave = SaveStage();
			if (! ( m_iSaveStage & 0x1FF ))
			{
				g_Log.FireEvent( LOGEVENT_SaveStatus, IMULDIV(m_iSaveStage, 100, m_SectorsQty+3 ));
				g_Serv.PrintPercent( m_iSaveStage, m_SectorsQty+3 );
			}
			if ( !bSave && ( pCurBlock != msgs[5] ))
				goto failedstage;
		}
		catch ( CGrayError &e )
		{
			g_Log.CatchEvent(&e, "Save FAILED for stage %u (%s).", m_iSaveStage, pCurBlock);
			bSuccess = false;
		}
		catch (...)
		{
			goto failedstage;
		}
		continue;
failedstage:
		g_Log.CatchEvent( NULL, "Save FAILED for stage %u (%s).", m_iSaveStage, pCurBlock);
		bSuccess = false;
	}

	g_Log.FireEvent(LOGEVENT_SaveDone);
	g_Serv.SetServerMode(SERVMODE_Run);			// Game is up and running

	DEBUG_MSG(("Save Done" DEBUG_CR));
	return bSuccess;
}

bool CWorld::SaveTry( bool fForceImmediate ) // Save world state
{
	EXC_TRY(("SaveTry(%d)", (int)fForceImmediate));
	if ( m_FileWorld.IsFileOpen())
	{
		// Save is already active !
		ASSERT( IsSaving());
		if ( fForceImmediate )	// finish it now !
		{
			return SaveForce();
		}
		else if ( g_Cfg.m_iSaveBackgroundTime )
		{
			return SaveStage();
		}
		return false;
	}

	// Do the write async from here in the future.
	if ( g_Cfg.m_fSaveGarbageCollect )
	{
		GarbageCollection();
	}

	// Determine the save name based on the time.
	// exponentially degrade the saves over time.
	if ( ! OpenScriptBackup( m_FileData, g_Cfg.m_sWorldBaseDir, "data", m_iSaveCountID ))
	{
		return false;
	}

	if ( ! OpenScriptBackup( m_FileWorld, g_Cfg.m_sWorldBaseDir, "world", m_iSaveCountID ))
	{
		return false;
	}

	if ( ! OpenScriptBackup( m_FilePlayers, g_Cfg.m_sWorldBaseDir, "chars", m_iSaveCountID ))
	{
		return false;
	}

	m_fSaveParity = ! m_fSaveParity; // Flip the parity of the save.
	m_iSaveStage = -1;
	m_timeSave.Init();

	// Write the file headers.
	r_Write(m_FileData);
	r_Write(m_FileWorld);
	r_Write(m_FilePlayers);

	if ( fForceImmediate || ! g_Cfg.m_iSaveBackgroundTime )	// Save now !
	{
		return SaveForce();
	}
	return true;
	EXC_CATCH("CWorld");
	return false;
}

bool CWorld::Save( bool fForceImmediate ) // Save world state
{
	bool	bSaved = false;
	try
	{
		CScriptTriggerArgs Args(fForceImmediate, m_iSaveStage);
		enum TRIGRET_TYPE tr;

		if ( g_Serv.r_Call("f_onserver_save", &g_Serv, &Args, NULL, &tr) )
			if ( tr == TRIGRET_RET_TRUE ) 
				return false;
			
		fForceImmediate = Args.m_iN1;
/*		if ( fForceImmediate )
		{
			CClient *pClient;
			for ( pClient = g_Serv.GetClientHead(); pClient != NULL; pClient = pClient->GetNext() )
			{
				CChar *pChar = pClient->GetChar();
				if ( pChar == NULL ) continue;
				pClient->addGumpSpecial(0);
				pClient->xFlush();
			}
		}
*/
		bSaved = SaveTry(fForceImmediate);
	}
	catch ( CGrayError &e )
	{
		g_Log.CatchEvent( &e, "Save FAILED." );
		Broadcast("Save FAILED. %s is UNSTABLE!", (LPCTSTR)g_Cfg.m_sVerName.GetPtr());
		m_FileData.Close();	// close if not already closed.
		m_FileWorld.Close();	// close if not already closed.
		m_FilePlayers.Close();	// close if not already closed.
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent( NULL, "Save FAILED" );
		Broadcast("Save FAILED. %s is UNSTABLE!", (LPCTSTR)g_Cfg.m_sVerName.GetPtr());
		m_FileData.Close();	// close if not already closed.
		m_FileWorld.Close();	// close if not already closed.
		m_FilePlayers.Close();	// close if not already closed.
	}

/*	if ( fForceImmediate )
	{
		RESOURCE_ID	rid = g_Cfg.ResourceGetIDType(RES_DIALOG, "d_server_save");
		if ( rid.IsValidUID() )
		{
			CClient *pClient;
			for ( pClient = g_Serv.GetClientHead(); pClient != NULL; pClient = pClient->GetNext() )
			{
				CChar *pChar = pClient->GetChar();
				if ( pChar == NULL ) continue;
				pClient->Dialog_Close(pChar, rid, 0);
			}
		}
	}
*/
	CScriptTriggerArgs Args(fForceImmediate, m_iSaveStage);
	g_Serv.r_Call((bSaved?"f_onserver_save_ok":"f_onserver_save_fail"), &g_Serv, &Args);
	return bSaved;
}

void CWorld::SaveStatics()
{
	try
	{
		if ( ! g_Cfg.m_fSaveGarbageCollect )
		{
			GarbageCollection_New();
			GarbageCollection_GMPages();
		}

		CScript m_FileStatics;
		if ( !OpenScriptBackup(m_FileStatics, g_Cfg.m_sWorldBaseDir, "statics", m_iSaveCountID) )
			return;
		r_Write(m_FileStatics);

		Broadcast("World statics save has been initiated.");

		//	loop through all sectors and save static items
		for ( int m = 0; m < 256; m++ )
		{
			if ( !g_MapList.m_maps[m] ) continue;

			for ( DWORD d = 0; d < g_MapList.GetSectorQty(m); d++ )
			{
				CItem	*pNext, *pItem;
				CSector	*pSector = GetSector(m, d);

				if ( !pSector ) continue;

				pItem = STATIC_CAST <CItem*>(pSector->m_Items_Inert.GetHead());
				for ( ; pItem != NULL; pItem = pNext )
				{
					pNext = pItem->GetNext();
					if ( pItem->IsAttr(ATTR_STATIC) ) pItem->r_WriteSafe(m_FileStatics);
				}

				pItem = STATIC_CAST <CItem*>(pSector->m_Items_Timer.GetHead());
				for ( ; pItem != NULL; pItem = pNext )
				{
					pNext = pItem->GetNext();
					if ( pItem->IsAttr(ATTR_STATIC) ) pItem->r_WriteSafe(m_FileStatics);
				}
			}
		}

		m_FileStatics.WriteSection( "EOF" );
		m_FileStatics.Close();
		g_Log.Event(LOGM_SAVE, "Statics data saved (%s)."	DEBUG_CR, (LPCTSTR)m_FileStatics.GetFilePath());
	}
	catch (CGrayError &e)
	{
		g_Log.CatchEvent(&e, "Statics Save FAILED.");
	}
	catch (...)
	{
		g_Log.CatchEvent(NULL, "Statics Save FAILED.");
	}
}

/////////////////////////////////////////////////////////////////////

bool CWorld::LoadFile( LPCTSTR pszLoadName, bool fError ) // Load world from script
{
	CScript s;
	if ( ! s.Open( pszLoadName ) )
	{
		if ( fError )
			g_Log.Event( LOGM_INIT|LOGL_ERROR,	"Can't Load %s" DEBUG_CR, (LPCTSTR) pszLoadName );
		else
			g_Log.Event( LOGM_INIT|LOGL_WARN,	"Can't Load %s" DEBUG_CR, (LPCTSTR) pszLoadName );
		return( false );
	}

	g_Log.Event(LOGM_INIT, "Loading %s..." DEBUG_CR, (LPCTSTR) pszLoadName );

	// Find the size of the file.
	DWORD lLoadSize = s.GetLength();
	int iLoadStage = 0;

	CScriptFileContext ScriptContext( &s );

	// Read the header stuff first.
	CScriptObj::r_Load( s );

	while ( s.FindNextSection())
	{
		if (! ( ++iLoadStage & 0x1FF ))	// don't update too often
		{
			g_Log.FireEvent( LOGEVENT_LoadStatus, IMULDIV( s.GetPosition(), 100, lLoadSize ));
			g_Serv.PrintPercent( s.GetPosition(), lLoadSize );
		}

		try
		{
			g_Cfg.LoadResourceSection(&s);
		}
		catch ( CGrayError &e )
		{
			g_Log.CatchEvent( &e, "Load Exception line %d %s is UNSTABLE!" DEBUG_CR, s.GetContext().m_iLineNum, (LPCTSTR)g_Cfg.m_sVerName.GetPtr());
		}
		catch (...)
		{
			g_Log.CatchEvent( NULL, "Load Exception line %d %s is UNSTABLE!" DEBUG_CR, s.GetContext().m_iLineNum, (LPCTSTR)g_Cfg.m_sVerName.GetPtr());
		}
	}

	if ( s.IsSectionType( "EOF" ))
	{
		// The only valid way to end.
		s.Close();
		return( true );
	}

	g_Log.Event( LOGM_INIT|LOGL_CRIT, "No [EOF] marker. '%s' is corrupt!" DEBUG_CR, (LPCTSTR) s.GetFilePath());
	return( false );
}


void	CWorld::LoadWorldConvert()
{
	if ( g_World.m_iLoadVersion >= 56 )
		return;

	g_Serv.SysMessage( "Converting spell items from old spell format..." DEBUG_CR );

	for ( int m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] ) continue;;

		for ( int i = 0; i < g_MapList.GetSectorQty(m); i++ )
		{
			CSector	*pSector = GetSector(m, i);
			if ( !pSector ) continue;

			bool	fInactive	= false;
			CChar * pChar = STATIC_CAST <CChar*>(pSector->m_Chars_Active.GetHead());
			CChar *	pCharNext;
			if ( !pChar )
			{
				fInactive = true;
				pChar = STATIC_CAST <CChar*>(pSector->m_Chars_Disconnect.GetHead());
			}

			for ( ; pChar != NULL; pChar = pCharNext )
			{
				pCharNext = pChar->GetNext();
				if ( !pCharNext && !fInactive )
				{
					fInactive = true;
					pCharNext = STATIC_CAST <CChar*>(pSector->m_Chars_Disconnect.GetHead());
				}

				// work with pChar now
				CItem* pItemNext;
				CItem* pItem	= pChar->GetContentHead();
				for ( ; pItem!=NULL; pItem=pItemNext )
				{
					pItemNext = pItem->GetNext();
					if ( pItem->IsTypeSpellable() && !pItem->IsType( IT_WAND ) && pItem->m_itSpell.m_spell  )
					{
						SPELL_TYPE spell = (SPELL_TYPE) RES_GET_INDEX( pItem->m_itSpell.m_spell );
						if ( pItem->IsAttr( ATTR_CURSED | ATTR_CURSED2 ))
							spell = SPELL_Curse;
						switch ( spell )
						{
							case SPELL_Clumsy:
							case SPELL_Hallucination:
							case SPELL_Feeblemind:
							case SPELL_Weaken:
							case SPELL_Agility:
							case SPELL_Cunning:
							case SPELL_Strength:
							case SPELL_Bless:
							case SPELL_Ale:
							case SPELL_Wine:
							case SPELL_Liquor:
							case SPELL_Curse:
							case SPELL_Mass_Curse:
							case SPELL_BeastForm:
							case SPELL_Monster_Form:
							case SPELL_Polymorph:
								break;
							default:
								continue; // ignore this spell
						}

						g_Serv.SysMessagef( "  '%s'(%x) on '%s' (%x)" DEBUG_CR,
							pItem->GetName(), (DWORD) pItem->GetUID(),
							pChar->GetName(), (DWORD) pChar->GetUID() );
						pChar->Spell_Effect_Remove( pItem, true );
						pChar->Spell_Effect_Add( pItem );
					}
				}
			}
		}
	}
}



bool CWorld::LoadWorld() // Load world from script
{
	EXC_TRY(("LoadWorld"));
	// Auto change to the most recent previous backup !
	// Try to load a backup file instead ?
	// NOTE: WE MUST Sync these files ! CHAR and WORLD !!!

	CGString sStaticsName;
	sStaticsName.Format("%s" GRAY_FILE "statics", (LPCTSTR)g_Cfg.m_sWorldBaseDir);

	CGString sWorldName;
	sWorldName.Format( "%s" GRAY_FILE "world",	(LPCTSTR) g_Cfg.m_sWorldBaseDir );

	CGString sCharsName;
	sCharsName.Format( "%s" GRAY_FILE "chars",	(LPCTSTR) g_Cfg.m_sWorldBaseDir );

	CGString sDataName;
	sDataName.Format( "%s" GRAY_FILE "data",	(LPCTSTR) g_Cfg.m_sWorldBaseDir );

	int iPrevSaveCount = m_iSaveCountID;
	while ( true )
	{
		LoadFile( sDataName, false );
		LoadFile(sStaticsName, false);
		if ( LoadFile( sWorldName ))
		{
			if ( LoadFile( sCharsName )) return true;
		}

		// If we could not open the file at all then it was a bust!
		if ( m_iSaveCountID == iPrevSaveCount ) break;

		// Erase all the stuff in the failed world/chars load.
		Close();

		// Get the name of the previous backups.
		CGString sArchive;
		GetBackupName( sArchive, g_Cfg.m_sWorldBaseDir, 'w', m_iSaveCountID );
		if ( ! sArchive.CompareNoCase( sWorldName ))	// ! same file ? break endless loop.
		{
			break;
		}
		sWorldName = sArchive;

		GetBackupName( sArchive, g_Cfg.m_sWorldBaseDir, 'c', m_iSaveCountID );
		if ( ! sArchive.CompareNoCase( sCharsName ))	// ! same file ? break endless loop.
		{
			break;
		}
		sCharsName = sArchive;

		GetBackupName( sArchive, g_Cfg.m_sWorldBaseDir, 'd', m_iSaveCountID );
		if ( ! sArchive.CompareNoCase( sDataName ))	// ! same file ? break endless loop.
		{
			break;
		}
		sDataName = sArchive;
	}

	g_Log.Event( LOGL_FATAL|LOGM_INIT, "No previous backup available ?" DEBUG_CR );
	EXC_CATCH("loading world");
	return false;
}




bool CWorld::LoadAll( LPCTSTR pszLoadName ) // Load world from script
{
	if ( LoadThreadInit())	// we already loaded?
		return( true );

	TCHAR *pszTemp = Str_GetTemp();

	g_Log.FireEvent( LOGEVENT_LoadBegin );
	DEBUG_CHECK( g_Serv.IsLoading());

	// The world has just started.
	m_Clock.Init();		// will be loaded from the world file.

	// Load all the accounts.
	if ( ! g_Accounts.Account_LoadAll( false ))
		return( false );

	// Try to load the world and chars files .
	if ( pszLoadName )
	{
		// Command line load this file. g_Cfg.m_sWorldBaseDir
		if ( ! LoadFile( pszLoadName ) )
			return( false );
	}
	else
	{
		if ( ! LoadWorld())
			return( false );
		LoadWorldConvert();
	}

	// If we are the master list. Then read the list from a sep file.
	if ( ! g_Cfg.m_sMainLogServerDir.IsEmpty())
	{
		// NOTE: Load this last because the timers are relative to the timers in the world file.
		sprintf(pszTemp, "%s" GRAY_FILE "serv", (LPCTSTR) g_Cfg.m_sMainLogServerDir);
		LoadFile(pszTemp);
	}

	m_timeStartup = GetCurrentTime();
	m_timeSave = GetCurrentTime() + g_Cfg.m_iSavePeriod;	// next save time.

	// Set all the sector light levels now that we know the time.
	// This should not look like part of the load. (CTRIG_EnvironChange triggers should run)
	for ( int s = 0; s < m_SectorsQty; s++ )
	{
		EXC_TRY(("LoadAll"));
		CSector *pSector = m_Sectors[s];

		if ( pSector )
		{
			if ( !pSector->IsLightOverriden() )
				pSector->SetLight(-1);

			// Is this area too complex ?
			int iCount = pSector->GetItemComplexity();
			if ( iCount > g_Cfg.m_iMaxSectorComplexity )
			{
				DEBUG_ERR(("Warning: %d items at %s, Sector too complex!" DEBUG_CR, iCount, (LPCTSTR)pSector->GetBasePoint().WriteUsed()));
			}
		}
		EXC_CATCH("Sector light levels");
	}

	EXC_TRY(("LoadAll"));
	GarbageCollection();
	EXC_CATCH("Garbage collect");

	// Set the current version now.
	r_SetVal( "VERSION", GRAY_VERSION );	// Set m_iLoadVersion
	g_Log.FireEvent( LOGEVENT_LoadDone );

	return true;
}

/////////////////////////////////////////////////////////////////

void CWorld::r_Write( CScript & s )
{
	// Write out the safe header.
	s.WriteKeyFormat("TITLE", "%s World Script", (LPCTSTR)g_Cfg.m_sVerName.GetPtr());
	s.WriteKey("VERSION", (LPCTSTR)g_Serv.m_sServVersion.GetPtr());
	s.WriteKeyVal( "TIME", GetCurrentTime().GetTimeRaw() );
	s.WriteKeyVal( "SAVECOUNT", m_iSaveCountID );
	s.Flush();	// Force this out to the file now.
}

bool CWorld::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	if ( ! strnicmp( pszKey, "LASTNEW", 7 ))
	{
		if ( ! strnicmp( pszKey+7, "ITEM", 4 ))
		{
			pszKey += 11;
			SKIP_SEPERATORS(pszKey);
			pRef = m_uidLastNewItem.ItemFind();
			return( true );
		}
		if ( ! strnicmp( pszKey+7, "CHAR", 4 ))
		{
			pszKey += 11;
			SKIP_SEPERATORS(pszKey);
			pRef = m_uidLastNewChar.CharFind();
			return( true );
		}
	}
	return( false );
}

enum WC_TYPE
{
	WC_REGSTATUS,
	WC_SAVECOUNT,
	WC_TIME,
	WC_TITLE,
	WC_VERSION,
	WC_QTY,
};

LPCTSTR const CWorld::sm_szLoadKeys[WC_QTY+1] =	// static
{
	"REGSTATUS",
	"SAVECOUNT",
	"TIME",
	"TITLE",
	"VERSION",
	NULL,
};

void CWorld::r_DumpLoadKeys( CTextConsole * pSrc )
{
	r_DumpKeys(pSrc,sm_szLoadKeys);
}

bool CWorld::r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc )
{
	EXC_TRY(("r_WriteVal('%s',,%x)", pszKey, pSrc));

	if ( !strnicmp(pszKey, "GMPAGE", 6) )		//	GM pages
	{
		CGMPage	*pPage;
		pszKey += 6;
		if (( *pszKey == 'S' ) || ( *pszKey == 's' ))	//	SERV.GMPAGES
			sVal.FormatVal(m_GMPages.GetCount());
		else if ( *pszKey == '.' )						//	SERV.GMPAGE.*
		{
			SKIP_SEPERATORS(pszKey);
			int iQty = Exp_GetVal(pszKey);
			if (( iQty < 0 ) || ( iQty >= m_GMPages.GetCount() )) return false;
			SKIP_SEPERATORS(pszKey);
			pPage = STATIC_CAST <CGMPage*> (m_GMPages.GetAt(iQty));
			if ( !pPage ) return false;

			if ( !strnicmp(pszKey, "HANDLED", 7) )
			{
				CClient *pClient = pPage->FindGMHandler();
				if ( pClient ) sVal.FormatVal(pClient->GetChar()->GetUID());
				else sVal.FormatVal(0);
				return true;
			}
			else return (pPage->r_WriteVal(pszKey, sVal, pSrc));
		}
		else sVal.FormatVal(0);
		return true;
	}

	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 ))
	{
	case WC_REGSTATUS:
		sVal = g_BackTask.m_sRegisterResult;
		break;
	case WC_SAVECOUNT: // "SAVECOUNT"
		sVal.FormatVal( m_iSaveCountID );
		break;
	case WC_TIME:	// "TIME"
		sVal.FormatVal( GetCurrentTime().GetTimeRaw() );
		break;
	case WC_TITLE: // 	"TITLE",
		sVal.Format("%s World Script", (LPCTSTR)g_Cfg.m_sVerName.GetPtr());
		break;
	case WC_VERSION: // "VERSION"
		sVal = (LPCTSTR)g_Serv.m_sServVersion.GetPtr();
		break;
	default:
		return( false );
	}
	return true;
	EXC_CATCH("CWorld");
	return false;
}

bool CWorld::r_LoadVal( CScript &s )
{
	LOCKDATA;
	LPCTSTR	pszKey = s.GetKey();
	LPCTSTR pszArgs = s.GetArgStr();
	EXC_TRY(("r_LoadVal('%s %s')", pszKey, pszArgs));
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, COUNTOF(sm_szLoadKeys)-1 ))
	{
	case WC_SAVECOUNT: // "SAVECOUNT"
		m_iSaveCountID = s.GetArgVal();
		break;
	case WC_TIME:	// "TIME"
		if ( ! g_Serv.IsLoading() )
		{
			DEBUG_WARN(( "Setting TIME while running is BAD!" DEBUG_CR ));
		}
		m_Clock.InitTime( s.GetArgVal());
		break;
	case WC_VERSION: // "VERSION"
		m_iLoadVersion = s.GetArgVal();
		break;
	default:
		return false;
	}
	return true;
	EXC_CATCH("CWorld");
	return false;
}

void CWorld::RespawnDeadNPCs()
{
	// Respawn dead story NPC's
	g_Serv.SetServerMode(SERVMODE_RestockAll);
	for ( int m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] ) continue;

		for ( int s = 0; s < g_MapList.GetSectorQty(m); s++ )
		{
			CSector	*pSector = GetSector(m, s);

			if ( pSector )
				pSector->RespawnDeadNPCs();
		}
	}
	g_Serv.SetServerMode(SERVMODE_Run);
}

void CWorld::Restock()
{
	// Recalc all the base items as well.
	g_Serv.SetServerMode(SERVMODE_RestockAll);

	for ( int i=0; i<COUNTOF(g_Cfg.m_ResHash.m_Array); i++ )
	for ( int j=0; j<g_Cfg.m_ResHash.m_Array[i].GetCount(); j++ )
	{
		CResourceDef* pResDef = g_Cfg.m_ResHash.m_Array[i][j];
		ASSERT(pResDef);
		if ( pResDef->GetResType() != RES_ITEMDEF )
			continue;

		CItemBase	*pBase = dynamic_cast <CItemBase *>(pResDef);
		if ( pBase )
			pBase->Restock();
	}

	for ( int m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] ) continue;

		for ( int s = 0; s < g_MapList.GetSectorQty(m); s++ )
		{
			CSector	*pSector = GetSector(m, s);

			if ( pSector )
				pSector->Restock(0);
		}
	}
	g_Serv.SetServerMode(SERVMODE_Run);
}

void CWorld::Close()
{
	if ( IsSaving())	// Must complete save now !
	{
		Save( true );
	}

	m_Stones.RemoveAll();
	m_Parties.DeleteAll();
	m_GMPages.DeleteAll();

	//	free memory allocated by sectors
	for ( int s = 0; s < m_SectorsQty; s++ )
	{
		m_Sectors[s]->Close();
		delete m_Sectors[s];
		m_Sectors[s] = NULL;
	}
	free(m_Sectors);
	m_Sectors = NULL;
	m_SectorsQty = 0;

	CloseAllUIDs();

	m_Clock.Init();	// no more sense of time.
}

void CWorld::GarbageCollection_GMPages()
{
	EXC_TRY(("GarbageCollection_GMPages()"));
	// Make sure all GM pages have accounts.
	CGMPage * pPage = STATIC_CAST <CGMPage*>( m_GMPages.GetHead());
	while ( pPage!= NULL )
	{
		CGMPage * pPageNext = pPage->GetNext();
		if ( ! pPage->FindAccount()) // Open script file
		{
			DEBUG_ERR(( "GM Page has invalid account '%s'" DEBUG_CR, (LPCTSTR) pPage->GetName()));
			delete pPage;
		}
		pPage = pPageNext;
	}
	EXC_CATCH("CWorld");
}

void CWorld::GarbageCollection()
{
	g_Log.Flush();
	GarbageCollection_GMPages();
	GarbageCollection_UIDs();
	g_Log.Flush();
}

void CWorld::Speak( const CObjBaseTemplate * pSrc, LPCTSTR pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	// ISINTRESOURCE might be SPKTAB_TYPE ?
	ASSERT(pszText);

	// if ( ISINTRESOURCE(pszText))

	bool fSpeakAsGhost = false;	// I am a ghost ?
	if ( pSrc )
	{
		if ( pSrc->IsChar())
		{
			// Are they dead ? Garble the text. unless we have SpiritSpeak
			const CChar * pCharSrc = dynamic_cast <const CChar*> (pSrc);
			ASSERT(pCharSrc);
			fSpeakAsGhost = pCharSrc->IsSpeakAsGhost();
		}
	}
	else
	{
		mode = TALKMODE_BROADCAST;
	}

	CGString sTextUID;
	CGString sTextName;	// name labelled text.
	CGString sTextGhost; // ghost speak.

	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( ! pClient->CanHear( pSrc, mode ))
			continue;

		LPCTSTR pszSpeak = pszText;
		bool fCanSee = false;
		CChar * pChar = pClient->GetChar();
		if ( pChar != NULL )
		{
			if ( fSpeakAsGhost && ! pChar->CanUnderstandGhost())
			{
				if ( sTextGhost.IsEmpty())	// Garble ghost.
				{
					sTextGhost = pszText;
					for ( int i=0; i<sTextGhost.GetLength(); i++ )
					{
						if ( sTextGhost[i] != ' ' &&  sTextGhost[i] != '\t' )
						{
							sTextGhost[i] = Calc_GetRandVal(2) ? 'O' : 'o';
						}
					}
				}
				pszSpeak = sTextGhost;
				pClient->addSound( sm_Sounds_Ghost[ Calc_GetRandVal( COUNTOF( sm_Sounds_Ghost )) ], pSrc );
			}
			fCanSee = pChar->CanSee( pSrc );	// Must label the text.
			if ( ! fCanSee && pSrc )
			{
				if ( sTextName.IsEmpty())
				{
					sTextName.Format( "<%s>%s", (LPCTSTR) pSrc->GetName(), (LPCTSTR) pszText );
				}
				pszSpeak = sTextName;
			}
		}

		if ( ! fCanSee && pSrc && pClient->IsPriv( PRIV_HEARALL|PRIV_DEBUG ))
		{
			if ( sTextUID.IsEmpty())
			{
				sTextUID.Format( "<%s [%lx]>%s", (LPCTSTR) pSrc->GetName(), (DWORD) pSrc->GetUID(), (LPCTSTR) pszText );
			}
			pszSpeak = sTextUID;
		}

		pClient->addBarkParse( pszSpeak, pSrc, wHue, mode, font );
	}
}

void CWorld::SpeakUNICODE( const CObjBaseTemplate * pSrc, const NCHAR * pwText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	ASSERT(pwText);

	bool fSpeakAsGhost = false;	// I am a ghost ?
	if ( pSrc != NULL )
	{
		if ( pSrc->IsChar())
		{
			// Are they dead ? Garble the text. unless we have SpiritSpeak
			const CChar * pCharSrc = dynamic_cast <const CChar*> (pSrc);
			ASSERT(pCharSrc);
			fSpeakAsGhost = pCharSrc->IsSpeakAsGhost();
		}
	}
	else
	{
		mode = TALKMODE_BROADCAST;
	}

	NCHAR wTextUID[MAX_TALK_BUFFER];	// uid labelled text.
	wTextUID[0] = '\0';
	NCHAR wTextName[MAX_TALK_BUFFER];	// name labelled text.
	wTextName[0] = '\0';
	NCHAR wTextGhost[MAX_TALK_BUFFER]; // ghost speak.
	wTextGhost[0] = '\0';

	for ( CClient * pClient = g_Serv.GetClientHead(); pClient!=NULL; pClient = pClient->GetNext())
	{
		if ( ! pClient->CanHear( pSrc, mode ))
			continue;

		const NCHAR * pwSpeak = pwText;
		bool fCanSee = false;
		CChar * pChar = pClient->GetChar();
		if ( pChar != NULL )
		{
			if ( fSpeakAsGhost && ! pChar->CanUnderstandGhost())
			{
				if ( wTextGhost[0] == '\0' )	// Garble ghost.
				{
					int i;
					for ( i=0; pwText[i] && i < MAX_TALK_BUFFER; i++ )
					{
						if ( pwText[i] != ' ' && pwText[i] != '\t' )
							wTextGhost[i] = Calc_GetRandVal(2) ? 'O' : 'o';
						else
							wTextGhost[i] = pwText[i];
					}
					wTextGhost[i] = '\0';
				}
				pwSpeak = wTextGhost;
				pClient->addSound( sm_Sounds_Ghost[ Calc_GetRandVal( COUNTOF( sm_Sounds_Ghost )) ], pSrc );
			}

			fCanSee = pChar->CanSee( pSrc );	// Must label the text.
			if ( ! fCanSee && pSrc )
			{
				if ( wTextName[0] == '\0' )
				{
					CGString sTextName;
					sTextName.Format("<%s>", (LPCTSTR) pSrc->GetName());
					int iLen = CvtSystemToNUNICODE( wTextName, COUNTOF(wTextName), sTextName, -1 );
					for ( int i=0; pwText[i] && iLen < MAX_TALK_BUFFER; i++, iLen++ )
					{
						wTextName[iLen] = pwText[i];
					}
					wTextName[iLen] = '\0';
				}
				pwSpeak = wTextName;
			}
		}

		if ( ! fCanSee && pSrc && pClient->IsPriv( PRIV_HEARALL|PRIV_DEBUG ))
		{
			if ( wTextUID[0] == '\0' )
			{
				TCHAR *pszMsg = Str_GetTemp();
				sprintf(pszMsg, "<%s [%lx]>", (LPCTSTR) pSrc->GetName(), (DWORD) pSrc->GetUID());
				int iLen = CvtSystemToNUNICODE( wTextUID, COUNTOF(wTextUID), pszMsg, -1 );
				for ( int i=0; pwText[i] && iLen < MAX_TALK_BUFFER; i++, iLen++ )
				{
					wTextUID[iLen] = pwText[i];
				}
				wTextUID[iLen] = '\0';
			}
			pwSpeak = wTextUID;
		}

		pClient->addBarkUNICODE( pwSpeak, pSrc, wHue, mode, font, lang );
	}
}

void __cdecl CWorld::Broadcast(LPCTSTR pMsg, ...) // System broadcast in bold text
{
	TCHAR	*sTemp = Str_GetTemp();
	va_list vargs;
	va_start(vargs, pMsg);
	vsprintf(sTemp, pMsg, vargs);
	va_end(vargs);
	Speak( NULL, sTemp, HUE_TEXT_DEF, TALKMODE_BROADCAST, FONT_BOLD );
	g_Serv.SocketsFlush();
}

CItem * CWorld::Explode( CChar * pSrc, CPointMap pt, int iDist, int iDamage, WORD wFlags )
{
	// Purple potions and dragons fire.
	// degrade damage the farther away we are. ???

	CItem * pItem = CItem::CreateBase( ITEMID_FX_EXPLODE_3 );
	ASSERT(pItem);

	pItem->SetAttr(ATTR_MOVE_NEVER | ATTR_CAN_DECAY);
	pItem->SetType(IT_EXPLOSION);
	pItem->m_uidLink = pSrc ? (DWORD) pSrc->GetUID() : UID_CLEAR;
	pItem->m_itExplode.m_iDamage = iDamage;
	pItem->m_itExplode.m_wFlags = wFlags | DAMAGE_GENERAL | DAMAGE_HIT_BLUNT;
	pItem->m_itExplode.m_iDist = iDist;
	pItem->MoveToDecay( pt, 1 );	// almost Immediate Decay

	pItem->Sound( 0x207 );	// sound is attached to the object so put the sound before the explosion.

	return( pItem );
}

//////////////////////////////////////////////////////////////////
// Game time.

DWORD CWorld::GetGameWorldTime( CServTime basetime ) const
{
	// basetime = TICK_PER_SEC time.
	// Get the time of the day in GameWorld minutes
	// 8 real world seconds = 1 game minute.
	// 1 real minute = 7.5 game minutes
	// 3.2 hours = 1 game day.
	return( basetime.GetTimeRaw() / g_Cfg.m_iGameMinuteLength );
}

CServTime CWorld::GetNextNewMoon( bool bMoonIndex ) const
{
	// "Predict" the next new moon for this moon
	// Get the period
	DWORD iSynodic = bMoonIndex ? FELUCCA_SYNODIC_PERIOD : TRAMMEL_SYNODIC_PERIOD;

	// Add a "month" to the current game time
	DWORD iNextMonth = GetGameWorldTime() + iSynodic;

	// Get the game time when this cycle will start
	DWORD iNewStart = (DWORD) (iNextMonth -
		(double) (iNextMonth % iSynodic));

	// Convert to TICK_PER_SEC ticks
	CServTime time;
	time.InitTime( iNewStart * g_Cfg.m_iGameMinuteLength );
	return(time);
}

int CWorld::GetMoonPhase (bool bMoonIndex) const
{
	// bMoonIndex is FALSE if we are looking for the phase of Trammel,
	// TRUE if we are looking for the phase of Felucca.

	// There are 8 distinct moon phases:  New, Waxing Crescent, First Quarter, Waxing Gibbous,
	// Full, Waning Gibbous, Third Quarter, and Waning Crescent

	// To calculate the phase, we use the following formula:
	//				CurrentTime % SynodicPeriod
	//	Phase = 	-----------------------------------------     * 8
	//			              SynodicPeriod
	//

	DWORD dwCurrentTime = GetGameWorldTime();	// game world time in minutes

	if (!bMoonIndex)
	{
		// Trammel
		return( IMULDIV( dwCurrentTime % TRAMMEL_SYNODIC_PERIOD, 8, TRAMMEL_SYNODIC_PERIOD ));
	}
	else
	{
		// Luna2
		return( IMULDIV( dwCurrentTime % FELUCCA_SYNODIC_PERIOD, 8, FELUCCA_SYNODIC_PERIOD ));
	}
}

LPCTSTR CWorld::GetGameTime() const
{
	return( GetTimeMinDesc( GetGameWorldTime()));
}

void CWorld::OnTick()
{
	// Do this once per tick.
	// 256 real secs = 1 GRAYhour. 19 light levels. check every 10 minutes or so.

	if ( g_Serv.IsLoading() || !m_Clock.Advance() )
		return;

	TIME_PROFILE_INIT;
	if ( IsSetSpecific )
		TIME_PROFILE_START;

	if ( m_timeSector <= GetCurrentTime())
	{
		// Only need a SECTOR_TICK_PERIOD tick to do world stuff.
		m_timeSector = GetCurrentTime() + SECTOR_TICK_PERIOD;	// Next hit time.
		m_Sector_Pulse ++;
		int	m, s;

		for ( m = 0; m < 256; m++ )
		{
			if ( !g_MapList.m_maps[m] ) continue;

			for ( s = 0; s < g_MapList.GetSectorQty(m); s++ )
			{
				try
				{
					CSector	*pSector = GetSector(m, s);
					if ( !pSector )
						g_Log.EventError("Ticking NULL sector %d on map %d." DEBUG_CR, s, m);
					else
						pSector->OnTick( m_Sector_Pulse );
				}
				catch ( CGrayError &e )
				{
					g_Log.CatchEvent(&e, "Sector OnTick");
				}
				catch (...)
				{
					g_Log.CatchEvent(NULL, "Sector OnTick");
				}
			}
		}

		m_ObjDelete.DeleteAll();	// clean up our delete list.
	}
	if ( m_timeSave <= GetCurrentTime())
	{
		// Auto save world
		m_timeSave = GetCurrentTime() + g_Cfg.m_iSavePeriod;
		g_Log.Flush();
		Save( false );
	}
	if ( m_timeRespawn <= GetCurrentTime())
	{
		// Time to regen all the dead NPC's in the world.
		m_timeRespawn = GetCurrentTime() + (20*60*TICK_PER_SEC);
		RespawnDeadNPCs();
	}
	if ( IsSetSpecific )
	{
		TIME_PROFILE_END;
		int	hi = TIME_PROFILE_GET_HI;
		if ( hi > 50 )
		{
			DEBUG_ERR(("CWorld::OnTick() [ticking sectors] took %d.%d to run\n", hi, TIME_PROFILE_GET_LO));
		}
	}
}

CSector *CWorld::GetSector(int map, int i)	// gets sector # from one map
{
									// if the map is not supported, return empty sector
	if (( map < 0 ) || ( map >= 256 ) || !g_MapList.m_maps[map] ) return NULL;

	if ( i >= g_MapList.GetSectorQty(map) )
	{
		g_Log.EventError("Unsupported sector #%d for map #%d specifyed." DEBUG_CR, i, map);
		return NULL;
	}

	int base = 0;
	for ( int m = 0; m < 256; m++ )
	{
		if ( !g_MapList.m_maps[m] ) continue;

		if ( m == map )
		{
			if ( g_MapList.GetSectorQty(map) < i ) return NULL;
			return m_Sectors[base + i];
		}
		base += g_MapList.GetSectorQty(m);
	}
	return NULL;
}
