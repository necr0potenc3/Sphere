//
// CResource.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "graysvr.h"	// predef header.
#include "../common/grayver.h"
#include "../common/CFileList.h"

extern CRaceClassDef g_BaseRaceClass;

CResource::CResource()
{
	m_timePeriodic.Init();

	m_fUseANSI = false;
	m_fWriteNumIDs = false;
	m_fUseNTService = false;
	m_iUseGodPort	= 0;
	m_fUseHTTP		= true;
	m_iPollServers = 0;
	m_iMapCacheTime = 2 * 60 * TICK_PER_SEC;
	m_iSectorSleepMask = (1<<10)-1;

	m_wDebugFlags = 0; //DEBUGF_NPC_EMOTE
	m_fSecure = true;
	m_iFreezeRestartTime = 10;

	//Magic
	m_fReagentsRequired = true;
	m_fReagentLossFail = true;
	m_fWordsOfPowerPlayer = true;
	m_fWordsOfPowerStaff = false;
	m_fEquippedCast = true;
	m_iMagicUnlockDoor = 1000;

	m_iSpell_Teleport_Effect_Staff = ITEMID_FX_FLAMESTRIKE;	// drama
	m_iSpell_Teleport_Sound_Staff = 0x1f3;
	m_iSpell_Teleport_Effect_Players = ITEMID_FX_TELE_VANISH;
	m_iSpell_Teleport_Sound_Players = 0x01fe;

	// Decay
	m_iDecay_Item = 30*60*TICK_PER_SEC;
	m_iDecay_CorpsePlayer = 45*60*TICK_PER_SEC;
	m_iDecay_CorpseNPC = 15*60*TICK_PER_SEC;

	// Accounts
	m_iClientsMax		= FD_SETSIZE-1;
	m_iConnectingMax	= 24;
	m_iConnectingMaxIP	= 8;
	m_iClientsMaxIP		= 0;
	
	m_fRequireEmail = false;
	m_iGuestsMax = 0;
	m_fArriveDepartMsg = true;
	m_iClientLingerTime = 60 * TICK_PER_SEC;
	m_iDeadSocketTime = 5*60*TICK_PER_SEC;
	m_iMinCharDeleteTime = 3*24*60*60*TICK_PER_SEC;
	m_iMaxCharsPerAccount = MAX_CHARS_PER_ACCT;
	m_fLocalIPAdmin = true;
	m_pServAccountMaster = NULL;

	// Save
	m_iSaveBackupLevels = 3;
	m_iSaveBackgroundTime = 5* 60 * TICK_PER_SEC;	// Use the new background save.
	m_fSaveGarbageCollect = false;	// Always force a full garbage collection.
	m_iSavePeriod = 15*60*TICK_PER_SEC;

	// In game effects.
	m_fMonsterFight		= true;
	m_fMonsterFear		= true;
	m_iLightDungeon		= 17;
	m_iLightNight		= 17;	// dark before t2a.
	m_iLightDay		= LIGHT_BRIGHT;
	m_iBankIMax		= 1000;
	m_iBankWMax		= 400 * WEIGHT_UNITS;
	m_fGuardsInstantKill	= true;
	m_iSnoopCriminal	= 500;
	m_iTrainSkillPercent	= 50;
	m_iTrainSkillMax	= 500;
	m_iSkillPracticeMax	= 300;
	m_fCharTags		= true;
	m_iVendorMaxSell	= 30;
	m_iGameMinuteLength	= 8 * TICK_PER_SEC;
	m_fNoWeather		= false;
	m_fFlipDroppedItems	= true;
	m_iMurderMinCount	= 5;
	m_iMurderDecayTime	= 8*60*60* TICK_PER_SEC;
	m_iMaxCharComplexity	= 16;
	m_iMaxItemComplexity	= 25;
	m_iMaxSectorComplexity	= 1024;
	m_iPlayerKarmaNeutral	= -2000; // How much bad karma makes a player neutral?
	m_iPlayerKarmaEvil	= -8000;
	m_iGuardLingerTime	= 1*60*TICK_PER_SEC; // "GUARDLINGER",
	m_iCriminalTimer	= 3*60*TICK_PER_SEC;
	m_iHitpointPercentOnRez	= 10;
	m_fLootingIsACrime	= true;
	m_fHelpingCriminalsIsACrime = true;
	m_fPlayerGhostSounds 	= true;
	m_fAutoNewbieKeys 	= true;
	m_iMaxBaseSkill		= 250;
	m_iStamRunningPenalty 	= 50;
	m_iStaminaLossAtWeight 	= 100;
	m_iMountHeight		= PLAYER_HEIGHT + 5;
	m_iArcheryMinDist	= 2;
	m_iArcheryMaxDist	= 12;
	m_iHitsUpdateRate	= TICK_PER_SEC;
	m_iSpeedScaleFactor 	= 15000;

	m_fNoResRobe		= 0;
	m_iLostNPCTeleport	= 0;
	m_iExperimental		= 0;
	m_iDistanceYell		= UO_MAP_VIEW_RADAR;
	m_iDistanceWhisper	= 3;
	m_iDistanceTalk		= UO_MAP_VIEW_SIZE;
	m_iOptionFlags		= 0;

	m_iMaxSkill		= SKILL_SCRIPTED;
	m_iWalkBuffer		= 50;
	m_iWalkRegen		= 25;
	m_iWoolGrowthTime	= 30*60*TICK_PER_SEC;

	m_iCommandLog		= 0;
	m_pEventsPetLink 	= NULL;
	
	m_fUsecrypt 		= true; // Server want crypt client ?
	m_iUsenocrypt		= 0; // Assume this as Unencrypted client var (0 i don't want Unenc. clients)
	m_fPayFromPackOnly	= 0; // pay vendors from packs only
	
	m_sVerName		= GRAY_TITLE;
	m_fReplyPeerConnects 	= true;
	m_iOverSkillMultiply 	= 0;
	m_fMstatFollow 		= false;
	m_fSuppressCapitals = false;
	
	// These are going to be removed
	m_iFeatures		= CLI_FEAT_T2A; // was CLI_FEAT_T2A_CHAT
	m_iFeaturesLogin	= 0;
	// New ones
	m_iFeatureT2A		= 0;
	m_iFeatureLBR		= 0;
	m_iFeatureAOS		= 0;
	m_iFeatureSE		= 0;

	m_iNpcAi = 0;
	m_iMaxLoopTimes = 10000;

	//	Experience
	m_bExperienceSystem = false;
	m_iExperienceMode = 0;
	m_iExperienceKoefPVP = 100;
	m_iExperienceKoefPVM = 100;
	m_bLevelSystem = false;
	m_iLevelMode = 1;
	m_iLevelNextAt = 0;

	//	mySQL support
	m_bMySql = false;
}

CResource::~CResource()
{
	for ( int i=0; i<COUNTOF(m_ResHash.m_Array); i++ )
	for ( int j=0; j<m_ResHash.m_Array[i].GetCount(); j++ )
	{
		CResourceDef* pResDef = m_ResHash.m_Array[i][j];
		if ( pResDef )
		{
			pResDef->UnLink();
		}
	}
	Unload(false);
}


// SKILL ITEMDEF, etc
bool CResource::r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef )
{
	TCHAR * pszSep = strchr( pszKey, '(' );	// acts like const_cast
	if ( pszSep == NULL )
	{
		pszSep = strchr( pszKey, '.' );	
		if ( pszSep == NULL )
			return( false );
	}

	*pszSep = '\0';

	int iResType = FindTableSorted( pszKey, sm_szResourceBlocks, RES_QTY );
	if ( iResType < 0 )
	{
		*pszSep = '.';
		return( false );
	}

	*pszSep = '.';

	// Now get the index.
	pszKey = pszSep+1;
	if ( pszKey[0] == ' \0' )
		return( false );

	pszSep = strchr( pszKey, '.' );
	if ( pszSep != NULL )
	{
		*pszSep = '\0';
	}

	if ( iResType == RES_SERVERS )
	{
		pRef = NULL;
	}
	else if ( iResType == RES_SPELL && *pszKey == '-' )
	{
		pszKey++;
		int		iOrder	= Exp_GetVal( pszKey );
		if ( !m_SpellDefs_Sorted.IsValidIndex( iOrder ) )
			pRef	= NULL;
		else
			pRef	= m_SpellDefs_Sorted[iOrder];
	}
	else
	{
		RESOURCE_ID	rid	= ResourceGetID( (RES_TYPE) iResType, pszKey );
		pRef = ResourceGetDef( rid );
	}

	if ( pszSep != NULL )
	{
		*pszSep = '.';
		pszKey = pszSep+1;
	}
	else
	{
		pszKey += strlen(pszKey);
	}
	return( true );
}


enum RC_TYPE
{
	RC_ACCTFILES,			// m_sAcctBaseDir
	RC_ANSI,
	RC_ARCHERYMAXDIST,		// m_iArcheryMaxDist
	RC_ARCHERYMINDIST,		// m_iArcheryMinDist
	RC_ARRIVEDEPARTMSG,
	RC_AUTONEWBIEKEYS,		// m_fAutoNewbieKeys
	RC_BACKUPLEVELS,		// m_iSaveBackupLevels
	RC_BANKMAXITEMS,
	RC_BANKMAXWEIGHT,
	RC_CHARTAGS,			// m_fCharTags
	RC_CLIENTLINGER,
	RC_CLIENTMAX,			// m_iClientsMax
	RC_CLIENTMAXIP,			// m_iClientsMaxIP
	RC_CLIENTS,
	RC_COMMANDLOG,
	RC_CONNECTINGMAX,		// m_iConnectingMax
	RC_CONNECTINGMAXIP,		// m_iConnectingMaxIP
	RC_CORPSENPCDECAY,
	RC_CORPSEPLAYERDECAY,
	RC_CRIMINALTIMER,		// m_iCriminalTimer
	RC_DEADSOCKETTIME,
	RC_DEBUGFLAGS,
	RC_DECAYTIMER,
	RC_DISTANCETALK,
	RC_DISTANCEWHISPER,
	RC_DISTANCEYELL,
	RC_DUNGEONLIGHT,
	RC_EQUIPPEDCAST,		// m_fEquippedCast
	RC_EVENTSPET,			// m_sEventsPet
	RC_EXPERIENCEKOEFPVM,	// m_iExperienceKoefPVM
	RC_EXPERIENCEKOEFPVP,	// m_iExperienceKoefPVP
	RC_EXPERIENCEMODE,		// m_iExperienceMode
	RC_EXPERIENCESYSTEM,	// m_bExperienceSystem
	RC_EXPERIMENTAL,		// m_iExperimental
	RC_FEATURES,			// m_iFeatures
	RC_FEATURESLOGIN,		// m_iFeaturesLogin
	RC_FLIPDROPPEDITEMS,	// m_fFlipDroppedItems
	RC_FORCEGARBAGECOLLECT,	// m_fSaveGarbageCollect
	RC_FREEZERESTARTTIME,	// m_iFreezeRestartTime
	RC_GAMEMINUTELENGTH,	// m_iGameMinuteLength
	RC_GUARDLINGER,			// m_iGuardLingerTime
	RC_GUARDSINSTANTKILL,
	RC_GUESTSMAX,
	RC_GUILDS,
	RC_HEARALL,
	RC_HELPINGCRIMINALSISACRIME,	// m_fHelpingCriminalsIsACrime
	RC_HITPOINTPERCENTONREZ, // m_iHitpointPercentOnRez
	RC_HITSUPDATERATE,
	RC_LEVELMODE,			// m_iLevelMode
	RC_LEVELNEXTAT,			// m_iLevelNextAt
	RC_LEVELSYSTEM,			// m_bLevelSystem
	RC_LIGHTDAY,			// m_iLightDay
	RC_LIGHTNIGHT,			// m_iLightNight
	RC_LOCALIPADMIN,		// m_fLocalIPAdmin
	RC_LOG,
	RC_LOGMASK,				// GetLogMask
	RC_LOOTINGISACRIME,		// m_fLootingIsACrime
	RC_LOSTNPCTELEPORT,		// m_fLostNPCTeleport
	RC_MAGICUNLOCKDOOR,		// m_iMagicUnlockDoor
	RC_MAINLOGSERVER,		// m_sMainLogServerDir
	RC_MAPCACHETIME,
	RC_MAXBASESKILL,			// m_iMaxBaseSkill
	RC_MAXCHARSPERACCOUNT,	// m_iMaxCharsPerAccount
	RC_MAXCOMPLEXITY,			// m_iMaxCharComplexity
	RC_MAXITEMCOMPLEXITY,		// m_iMaxItemComplexity
	RC_MAXLOOPTIMES,			// m_iMaxLoopTimes
	RC_MAXSECTORCOMPLEXITY,		// m_iMaxSectorComplexity
	RC_MAXSKILL,
	RC_MD5PASSWORDS,			// m_fMd5Passwords
	RC_MINCHARDELETETIME,
	RC_MONSTERFEAR,			// m_fMonsterFear
	RC_MONSTERFIGHT,
	RC_MOUNTHEIGHT,			// m_iMountHeight
	RC_MSTATFOLLOW,			// m_fMstatFollow
	RC_MULFILES,
	RC_MURDERDECAYTIME,		// m_iMurderDecayTime;
	RC_MYSQL,					//	m_bMySql
	RC_MYSQLDB,					//	m_sMySqlDatabase
	RC_MYSQLHOST,				//	m_sMySqlHost
	RC_MYSQLPASS,				//	m_sMySqlPassword
	RC_MYSQLUSER,				//	m_sMySqlUser
	RC_MURDERMINCOUNT,		// m_iMurderMinCount;		// amount of murders before we get title.
	RC_NORESROBE,
	RC_NOWEATHER,				// m_fNoWeather
	RC_NPCAI,					// m_iNpcAi
	RC_NPCTRAINMAX,			// m_iTrainSkillMax
	RC_NPCTRAINPERCENT,			// m_iTrainSkillPercent
	RC_NTSERVICE,				// m_fUseNTService
	RC_OPTIONFLAGS,			// m_iOptionFlags
	RC_OVERSKILLMULTIPLY,		//	m_iOverSkillMultiply
	RC_PAYFROMPACKONLY,			//	m_fPayFromPackOnly
	RC_PLAYERGHOSTSOUNDS,	// m_fPlayerGhostSounds
	RC_PLAYERNEUTRAL,		// m_iPlayerKarmaNeutral
	RC_POLLSERVERS,				// m_iPollServers
	RC_PROFILE,
	RC_RCLOCK,
	RC_REAGENTLOSSFAIL,			// m_fReagentLossFail
	RC_REAGENTSREQUIRED,
	RC_REGEN,
	RC_REGISTERFLAG,
	RC_REGISTERSERVER,			// m_sRegisterServer
	RC_REPLYPEERCONNECTS,	//	m_fReplyPeerConnects
	RC_REQUIREEMAIL,		// m_fRequireEmail
	RC_RTIME,
	RC_RTIMETEXT,
	RC_RUNNINGPENALTY,		// m_iStamRunningPenalty
	RC_SAVEBACKGROUND,			// m_iSaveBackgroundTime
	RC_SAVEPERIOD,
	RC_SCPFILES,
	RC_SECTORSLEEP,				// m_iSectorSleepMask
	RC_SECURE,
	RC_SKILLPRACTICEMAX,	// m_iSkillPracticeMax
	RC_SNOOPCRIMINAL,
	RC_SPEECHOTHER,
	RC_SPEECHPET,
	RC_SPEECHSELF,
	RC_SPEEDSCALEFACTOR,
	RC_STAMINALOSSATWEIGHT,	// m_iStaminaLossAtWeight
	RC_STATGUILDS,
	RC_SUPPRESSCAPITALS,
	RC_TELEPORTEFFECTPLAYERS,	//	m_iSpell_Teleport_Effect_Players
	RC_TELEPORTEFFECTSTAFF,		//	m_iSpell_Teleport_Effect_Staff
	RC_TIMEUP,
	RC_TOTALPOLLEDACCOUNTS,
	RC_TOTALPOLLEDCLIENTS,
	RC_TOTALPOLLEDSERVERS,
	RC_USECRYPT,				// m_Usecrypt
	RC_USEGODPORT,				// m_iUseGodPort
	RC_USEHTTP,					// m_fUseHTTP
	RC_USENOCRYPT,				// m_Usenocrypt
	RC_VENDORMAXSELL,			// m_iVendorMaxSell
	RC_VERBOSE,
	RC_VERSION,
	RC_WALKBUFFER,
	RC_WALKREGEN,
	RC_WOOLGROWTHTIME,			// m_iWoolGrowthTime
	RC_WOPPLAYER,
	RC_WOPSTAFF,
	RC_WORLDSAVE,
	RC_WRITENUMIDS,				// m_fWriteNumIDs
	RC_QTY,
};

const CAssocReg CResource::sm_szLoadKeys[RC_QTY+1] =
{
	{ "ACCTFILES",				{ ELEM_CSTRING,	offsetof(CResource,m_sAcctBaseDir)	}},
	{ "ANSI",					{ ELEM_BOOL,	offsetof(CResource,m_fUseANSI)		}},
	{ "ARCHERYMAXDIST",			{ ELEM_INT,		offsetof(CResource,m_iArcheryMaxDist) }},
	{ "ARCHERYMINDIST",			{ ELEM_INT,		offsetof(CResource,m_iArcheryMinDist) }},
	{ "ARRIVEDEPARTMSG",		{ ELEM_BOOL,	offsetof(CResource,m_fArriveDepartMsg)	}},
	{ "AUTONEWBIEKEYS",			{ ELEM_BOOL,	offsetof(CResource,m_fAutoNewbieKeys)	}},
	{ "BACKUPLEVELS",			{ ELEM_INT,		offsetof(CResource,m_iSaveBackupLevels)	}},
	{ "BANKMAXITEMS",			{ ELEM_INT,		offsetof(CResource,m_iBankIMax)	}},
	{ "BANKMAXWEIGHT",			{ ELEM_INT,		offsetof(CResource,m_iBankWMax)	}},
	{ "CHARTAGS",				{ ELEM_BOOL,	offsetof(CResource,m_fCharTags)	}},
	{ "CLIENTLINGER",			{ ELEM_INT,		offsetof(CResource,m_iClientLingerTime)	}},
	{ "CLIENTMAX",				{ ELEM_INT,		offsetof(CResource,m_iClientsMax)	}},
	{ "CLIENTMAXIP",			{ ELEM_INT,		offsetof(CResource,m_iClientsMaxIP)	}},
	{ "CLIENTS" },	// duplicate
	{ "COMMANDLOG",				{ ELEM_INT,		offsetof(CResource,m_iCommandLog)		}},
	{ "CONNECTINGMAX",			{ ELEM_INT,		offsetof(CResource,m_iConnectingMax)	}},
	{ "CONNECTINGMAXIP",		{ ELEM_INT,		offsetof(CResource,m_iConnectingMaxIP)	}},
	{ "CORPSENPCDECAY",			{ ELEM_INT,		offsetof(CResource,m_iDecay_CorpseNPC)	}},
	{ "CORPSEPLAYERDECAY",		{ ELEM_INT,		offsetof(CResource,m_iDecay_CorpsePlayer) }},
	{ "CRIMINALTIMER",			{ ELEM_INT,		offsetof(CResource,m_iCriminalTimer)	}},
	{ "DEADSOCKETTIME",			{ ELEM_INT,		offsetof(CResource,m_iDeadSocketTime)	}},
	{ "DEBUGFLAGS",				{ ELEM_WORD,	offsetof(CResource,m_wDebugFlags)	}},
	{ "DECAYTIMER",				{ ELEM_INT,		offsetof(CResource,m_iDecay_Item)	}},
	{ "DISTANCETALK",			{ ELEM_INT,		offsetof(CResource,m_iDistanceTalk )	}},
	{ "DISTANCEWHISPER",		{ ELEM_INT,		offsetof(CResource,m_iDistanceWhisper )	}},
	{ "DISTANCEYELL",			{ ELEM_INT,		offsetof(CResource,m_iDistanceYell )	}},
	{ "DUNGEONLIGHT",			{ ELEM_INT,		offsetof(CResource,m_iLightDungeon)	}},
	{ "EQUIPPEDCAST",			{ ELEM_BOOL,	offsetof(CResource,m_fEquippedCast)	}},
	{ "EVENTSPET",				{ ELEM_CSTRING,	offsetof(CResource,m_sEventsPet)	}},
	{ "EXPERIENCEKOEFPVM",		{ ELEM_INT,		offsetof(CResource,m_iExperienceKoefPVM)}},
	{ "EXPERIENCEKOEFPVP",		{ ELEM_INT,		offsetof(CResource,m_iExperienceKoefPVP)}},
	{ "EXPERIENCEMODE",			{ ELEM_INT,		offsetof(CResource,m_iExperienceMode)	}},
	{ "EXPERIENCESYSTEM",		{ ELEM_BOOL,	offsetof(CResource,m_bExperienceSystem)	}},
	{ "EXPERIMENTAL",			{ ELEM_INT,		offsetof(CResource,m_iExperimental)	}},
	{ "FEATURES",				{ ELEM_INT,		offsetof(CResource,m_iFeatures)	}},
	{ "FEATURESLOGIN",			{ ELEM_INT,		offsetof(CResource,m_iFeaturesLogin)	}},
	{ "FLIPDROPPEDITEMS",		{ ELEM_BOOL,	offsetof(CResource,m_fFlipDroppedItems)	}},
	{ "FORCEGARBAGECOLLECT",	{ ELEM_BOOL,	offsetof(CResource,m_fSaveGarbageCollect) }},
	{ "FREEZERESTARTTIME",		{ ELEM_INT,		offsetof(CResource,m_iFreezeRestartTime)	}},
	{ "GAMEMINUTELENGTH",		{ ELEM_INT,		offsetof(CResource,m_iGameMinuteLength)	}},
	{ "GUARDLINGER",			{ ELEM_INT,		offsetof(CResource,m_iGuardLingerTime)	}},
	{ "GUARDSINSTANTKILL",		{ ELEM_BOOL,	offsetof(CResource,m_fGuardsInstantKill)	}},
	{ "GUESTSMAX",				{ ELEM_INT,		offsetof(CResource,m_iGuestsMax)	}},
	{ "GUILDS" },
	{ "HEARALL"	},
	{ "HELPINGCRIMINALSISACRIME",{ ELEM_BOOL,	offsetof(CResource,m_fHelpingCriminalsIsACrime)	}},
	{ "HITPOINTPERCENTONREZ",	{ ELEM_INT,		offsetof(CResource,m_iHitpointPercentOnRez)	} },
	{ "HITSUPDATERATE" },
	{ "LEVELMODE",				{ ELEM_INT,		offsetof(CResource,m_iLevelMode)	}},
	{ "LEVELNEXTAT",			{ ELEM_INT,		offsetof(CResource,m_iLevelNextAt)	}},
	{ "LEVELSYSTEM",			{ ELEM_BOOL,	offsetof(CResource,m_bLevelSystem)	}},
	{ "LIGHTDAY",				{ ELEM_INT,		offsetof(CResource,m_iLightDay)	} },
	{ "LIGHTNIGHT",				{ ELEM_INT,		offsetof(CResource,m_iLightNight)	} },
	{ "LOCALIPADMIN",			{ ELEM_BOOL,	offsetof(CResource,m_fLocalIPAdmin)	} }, // The local ip is assumed to be the admin.
	{ "LOG" },
	{ "LOGMASK" },		// GetLogMask
	{ "LOOTINGISACRIME",		{ ELEM_BOOL,	offsetof(CResource,m_fLootingIsACrime)	}},
	{ "LOSTNPCTELEPORT",		{ ELEM_INT,		offsetof(CResource,m_iLostNPCTeleport)	}},
	{ "MAGICUNLOCKDOOR",		{ ELEM_INT,		offsetof(CResource,m_iMagicUnlockDoor)	}},
	{ "MAINLOGSERVER",			{ ELEM_CSTRING,	offsetof(CResource,m_sMainLogServerDir)	}},
	{ "MAPCACHETIME",			{ ELEM_INT,		offsetof(CResource,m_iMapCacheTime)	}},
	{ "MAXBASESKILL",			{ ELEM_INT,		offsetof(CResource,m_iMaxBaseSkill)	}},
	{ "MAXCHARSPERACCOUNT",		{ ELEM_INT,		offsetof(CResource,m_iMaxCharsPerAccount)	}},
	{ "MAXCOMPLEXITY",			{ ELEM_INT,		offsetof(CResource,m_iMaxCharComplexity)	}},
	{ "MAXITEMCOMPLEXITY",		{ ELEM_INT,		offsetof(CResource,m_iMaxItemComplexity)	}},
	{ "MAXLOOPTIMES",			{ ELEM_INT,		offsetof(CResource,m_iMaxLoopTimes)			}},
	{ "MAXSECTORCOMPLEXITY",	{ ELEM_INT,		offsetof(CResource,m_iMaxSectorComplexity)	}},
	{ "MAXSKILL",				{ ELEM_INT,		offsetof(CResource,m_iMaxSkill)	}},
	{ "MD5PASSWORDS",			{ ELEM_BOOL,	offsetof(CResource,m_fMd5Passwords) }},
	{ "MINCHARDELETETIME",		{ ELEM_INT,		offsetof(CResource,m_iMinCharDeleteTime)	}},
	{ "MONSTERFEAR",			{ ELEM_BOOL,	offsetof(CResource,m_fMonsterFear)	}},
	{ "MONSTERFIGHT",			{ ELEM_BOOL,	offsetof(CResource,m_fMonsterFight)	}},
	{ "MOUNTHEIGHT",			{ ELEM_INT,		offsetof(CResource,m_iMountHeight)	}},
	{ "MSTATFOLLOW",			{ ELEM_BOOL,	offsetof(CResource,m_fMstatFollow) }},
	{ "MULFILES" },
	{ "MURDERDECAYTIME",		{ ELEM_INT,		offsetof(CResource,m_iMurderDecayTime)	}},
	{ "MURDERMINCOUNT",			{ ELEM_INT,		offsetof(CResource,m_iMurderMinCount)	}},		// amount of murders before we get title.
	{ "MYSQL",					{ ELEM_BOOL,	offsetof(CResource,m_bMySql)		}},
	{ "MYSQLDATABASE",			{ ELEM_CSTRING,	offsetof(CResource,m_sMySqlDB)		}},
	{ "MYSQLHOST",				{ ELEM_CSTRING, offsetof(CResource,m_sMySqlHost)	}},
	{ "MYSQLPASSWORD",			{ ELEM_CSTRING,	offsetof(CResource,m_sMySqlPass)	}},
	{ "MYSQLUSER",				{ ELEM_CSTRING,	offsetof(CResource,m_sMySqlUser)	}},
	{ "NORESROBE",				{ ELEM_BOOL,	offsetof(CResource,m_fNoResRobe)	}},
	{ "NOWEATHER",				{ ELEM_BOOL,	offsetof(CResource,m_fNoWeather)	}},
	{ "NPCAI",					{ ELEM_INT,		offsetof(CResource,m_iNpcAi)		}},
	{ "NPCTRAINMAX",			{ ELEM_INT,		offsetof(CResource,m_iTrainSkillMax)	}},
	{ "NPCTRAINPERCENT",		{ ELEM_INT,		offsetof(CResource,m_iTrainSkillPercent) }},
	{ "NTSERVICE",				{ ELEM_BOOL,	offsetof(CResource,m_fUseNTService)	}},
	{ "OPTIONFLAGS",			{ ELEM_INT,		offsetof(CResource,m_iOptionFlags)	}},
	{ "OVERSKILLMULTIPLY",		{ ELEM_INT,		offsetof(CResource,m_iOverSkillMultiply)	}},
	{ "PAYFROMPACKONLY",		{ ELEM_BOOL,	offsetof(CResource,m_fPayFromPackOnly)	}},
	{ "PLAYERGHOSTSOUNDS",		{ ELEM_BOOL,	offsetof(CResource,m_fPlayerGhostSounds)	}},
	{ "PLAYERNEUTRAL",			{ ELEM_INT,		offsetof(CResource,m_iPlayerKarmaNeutral)	}},
	{ "POLLSERVERS",			{ ELEM_INT,		offsetof(CResource,m_iPollServers)	}},
	{ "PROFILE" },
	{ "RCLOCK" },
	{ "REAGENTLOSSFAIL",		{ ELEM_BOOL,	offsetof(CResource,m_fReagentLossFail)	}},
	{ "REAGENTSREQUIRED",		{ ELEM_BOOL,	offsetof(CResource,m_fReagentsRequired)	}},
	{ "REGEN" },
	{ "REGISTERFLAG" },
	{ "REGISTERSERVER",			{ ELEM_CSTRING,	offsetof(CResource,m_sRegisterServer)	}},
	{ "REPLYPEERCONNECTS",		{ ELEM_BOOL,	offsetof(CResource,m_fReplyPeerConnects)	}},
	{ "REQUIREEMAIL",			{ ELEM_BOOL,	offsetof(CResource,m_fRequireEmail)	}},
	{ "RTIME" },
	{ "RTIMETEXT" },
	{ "RUNNINGPENALTY",			{ ELEM_INT,		offsetof(CResource,m_iStamRunningPenalty)	}},
	{ "SAVEBACKGROUND",			{ ELEM_INT,		offsetof(CResource,m_iSaveBackgroundTime)	}},
	{ "SAVEPERIOD",				{ ELEM_INT,		offsetof(CResource,m_iSavePeriod)	}},
	{ "SCPFILES",				{ ELEM_CSTRING,	offsetof(CResource,m_sSCPBaseDir)	}},
	{ "SECTORSLEEP",			{ ELEM_INT,		offsetof(CResource,m_iSectorSleepMask)	}},
	{ "SECURE",					{ ELEM_BOOL,	offsetof(CResource,m_fSecure)	}},
	{ "SKILLPRACTICEMAX",		{ ELEM_INT,		offsetof(CResource,m_iSkillPracticeMax) }},
	{ "SNOOPCRIMINAL",			{ ELEM_INT,		offsetof(CResource,m_iSnoopCriminal)	}},
	{ "SPEECHOTHER",			{ ELEM_CSTRING,	offsetof(CResource,m_sSpeechOther )	}},
	{ "SPEECHPET",				{ ELEM_CSTRING,	offsetof(CResource,m_sSpeechPet )	}},
	{ "SPEECHSELF",				{ ELEM_CSTRING,	offsetof(CResource,m_sSpeechSelf)	}},
	{ "SPEEDSCALEFACTOR",		{ ELEM_INT,		offsetof(CResource,m_iSpeedScaleFactor)	}},
	{ "STAMINALOSSATWEIGHT",	{ ELEM_INT,		offsetof(CResource,m_iStaminaLossAtWeight)	}},
	{ "STATGUILDS" },
	{ "SUPPRESSCAPITALS",		{ ELEM_BOOL,	offsetof(CResource,m_fSuppressCapitals) }},
	{ "TELEPORTEFFECTPLAYERS",	{ ELEM_INT,		offsetof(CResource,m_iSpell_Teleport_Effect_Players) }},
	{ "TELEPORTEFFECTSTAFF",	{ ELEM_INT,		offsetof(CResource,m_iSpell_Teleport_Effect_Staff) }},
	{ "TIMEUP" },
	{ "TOTALPOLLEDACCOUNTS" },
	{ "TOTALPOLLEDCLIENTS" },
	{ "TOTALPOLLEDSERVERS" },
	{ "USECRYPT",				{ ELEM_BOOL,	offsetof(CResource,m_fUsecrypt)	}},	// we don't want crypt clients
	{ "USEGODPORT",				{ ELEM_INT,		offsetof(CResource,m_iUseGodPort)	}},
	{ "USEHTTP",				{ ELEM_BOOL,	offsetof(CResource,m_fUseHTTP)	}},
	{ "USENOCRYPT",				{ ELEM_INT,	offsetof(CResource,m_iUsenocrypt)	}},	// we don't want no-crypt clients
	{ "VENDORMAXSELL",			{ ELEM_INT,		offsetof(CResource,m_iVendorMaxSell) }},
	{ "VERBOSE" },
	{ "VERSION" },
	{ "WALKBUFFER",				{ ELEM_INT,		offsetof(CResource,m_iWalkBuffer) }},
	{ "WALKREGEN",				{ ELEM_INT,		offsetof(CResource,m_iWalkRegen) }},
	{ "WOOLGROWTHTIME",			{ ELEM_INT,		offsetof(CResource,m_iWoolGrowthTime) }},
	{ "WOPPLAYER",				{ ELEM_BOOL,	offsetof(CResource,m_fWordsOfPowerPlayer)	}},
	{ "WOPSTAFF",				{ ELEM_BOOL,	offsetof(CResource,m_fWordsOfPowerStaff)	}},
	{ "WORLDSAVE",				{ ELEM_CSTRING,	offsetof(CResource,m_sWorldBaseDir)	}},
	{ "WRITENUMIDS",			{ ELEM_BOOL,	offsetof(CResource,m_fWriteNumIDs)	}},
	{ NULL },
};

void CResource::r_DumpLoadKeys( CTextConsole * pSrc )
{
	r_DumpKeys( pSrc, (LPCTSTR const *) sm_szLoadKeys, sizeof(sm_szLoadKeys[0]));
}

bool CResource::r_LoadVal( CScript &s )
{
	LOCKDATA;
	EXC_TRY(("r_LoadVal('%s %s')", s.GetKey(), s.GetArgStr()));

	int i = FindTableHeadSorted( s.GetKey(), (LPCTSTR const *) sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1, sizeof(sm_szLoadKeys[0]));
	if ( i < 0 )
	{
		if ( s.IsKeyHead( "REGEN", 5 ))			//	REGENx=<stat regeneration rate>
		{
			int index = atoi(s.GetKey()+5);
			return g_BaseRaceClass.SetRegenRate( (STAT_TYPE) index, s.GetArgVal() * TICK_PER_SEC );
		}
		else if ( s.IsKeyHead("MAP", 3) )		//	MAPx=settings
		{
			return g_MapList.Load(atoi(s.GetKey() + 3), s.GetArgRaw());
		}
		else if ( s.IsKeyHead("PACKET", 6) )	//	PACKETx=<function name to execute upon packet>
		{
			int index = atoi(s.GetKey() + 6);
			if (( index >= 0 ) && ( index < XCMD_QTY ))
			{
				char *args = s.GetArgRaw();
				if ( !args || ( strlen(args) >= 31 ))
					g_Log.EventError("Invalid function name for packet filtering (limit is 30 chars)." DEBUG_CR);
				else
				{
					strcpy(g_Serv.m_PacketFilter[index], args);
					return true;
				}
			}
			else
				g_Log.EventError("Packet filtering index %d out of range [0..%d]" DEBUG_CR, index, XCMD_QTY-1);
		}

		return(false);
	}

	if ( i == RC_MAXSKILL && !g_Serv.IsLoading() )
		return false;

	switch (i)
	{
	case RC_ACCTFILES:	// Put acct files here.
		m_sAcctBaseDir = CGFile::GetMergedFileName( s.GetArgStr(), "" );
		break;
	case RC_BANKMAXWEIGHT:
		m_iBankWMax = s.GetArgVal() * WEIGHT_UNITS;
		break;
	case RC_CLIENTLINGER:
		m_iClientLingerTime = s.GetArgVal() * TICK_PER_SEC;
		break;
	case RC_CLIENTMAX:
	case RC_CLIENTS:
		m_iClientsMax = s.GetArgVal();
		if ( m_iClientsMax > FD_SETSIZE-1 )	// Max number we can deal with. compile time thing.
		{
			m_iClientsMax = FD_SETSIZE-1;
		}
		break;
	case RC_CORPSENPCDECAY:
		m_iDecay_CorpseNPC = s.GetArgVal()*60*TICK_PER_SEC;
		break;
	case RC_CORPSEPLAYERDECAY:
		m_iDecay_CorpsePlayer = s.GetArgVal()*60*TICK_PER_SEC ;
		break;
	case RC_CRIMINALTIMER:
		m_iCriminalTimer = s.GetArgVal() * 60 * TICK_PER_SEC;
		break;
	case RC_DEADSOCKETTIME:
		m_iDeadSocketTime = s.GetArgVal()*60*TICK_PER_SEC;
		break;
	case RC_DECAYTIMER:
		m_iDecay_Item = s.GetArgVal() *60*TICK_PER_SEC;
		break;
	case RC_GUARDLINGER:
		m_iGuardLingerTime = s.GetArgVal() * 60 * TICK_PER_SEC;
		break;
	case RC_HEARALL:
		g_Log.SetLogMask( s.GetArgFlag( g_Log.GetLogMask(), LOGM_PLAYER_SPEAK ));
		break;
	case RC_HITSUPDATERATE:
		m_iHitsUpdateRate = s.GetArgVal() * TICK_PER_SEC;
		break;
	case RC_LOG:
		g_Log.OpenLog( s.GetArgStr());
		break;
	case RC_LOGMASK:
		g_Log.SetLogMask( s.GetArgVal());
		break;
	case RC_MULFILES:
		g_Install.SetPreferPath( CGFile::GetMergedFileName( s.GetArgStr(), "" ));
		break;
	case RC_MAPCACHETIME:
		m_iMapCacheTime = s.GetArgVal() * TICK_PER_SEC;
		break;
	case RC_MAXCHARSPERACCOUNT:
		m_iMaxCharsPerAccount = s.GetArgVal();
		if ( m_iMaxCharsPerAccount > MAX_CHARS_PER_ACCT )
			m_iMaxCharsPerAccount = MAX_CHARS_PER_ACCT;
		break;
	case RC_MAINLOGSERVER:
		m_sMainLogServerDir = s.GetArgStr();
		if ( m_sMainLogServerDir[0] == '0' )
			m_sMainLogServerDir.Empty();
		break;
	case RC_MINCHARDELETETIME:
		m_iMinCharDeleteTime = s.GetArgVal()*60*TICK_PER_SEC;
		break;
	case RC_MURDERDECAYTIME:
		m_iMurderDecayTime = s.GetArgVal() * TICK_PER_SEC;
		break;
	case RC_WOOLGROWTHTIME:
		m_iWoolGrowthTime = s.GetArgVal() * 60 * TICK_PER_SEC;
		break;
	case RC_PROFILE:
		g_Serv.m_Profile.SetActive( s.GetArgVal());
		break;
	case RC_POLLSERVERS:
		m_iPollServers = s.GetArgVal() *60*TICK_PER_SEC;
		g_BackTask.CreateThread();
		break;
	case RC_PLAYERNEUTRAL:	// How much bad karma makes a player neutral?
		m_iPlayerKarmaNeutral = s.GetArgVal();
		if ( m_iPlayerKarmaEvil > m_iPlayerKarmaNeutral )
			m_iPlayerKarmaEvil = m_iPlayerKarmaNeutral;
		break;
	case RC_REGEN:
		break;
	case RC_REGISTERFLAG:
	case RC_REGISTERSERVER:
		if ( ! strcmp( s.GetArgStr(), "0" ))
		{
			m_sRegisterServer.Empty();
		}
		else if ( ! strcmp( s.GetArgStr(), "1" ))
		{
			m_sRegisterServer = GRAY_MAIN_SERVER;
		}
		else
		{
			m_sRegisterServer = s.GetArgStr();
		}
		g_BackTask.CreateThread();
		break;
	case RC_SCPFILES: // Get SCP files from here.
		m_sSCPBaseDir = CGFile::GetMergedFileName( s.GetArgStr(), "" );
		break;
	case RC_SECURE:
		m_fSecure = s.GetArgVal();
		if ( !g_Serv.IsLoading() )
			g_Serv.SetSignals();
		break;
	case RC_SKILLPRACTICEMAX:
		m_iSkillPracticeMax = s.GetArgVal();
		break;
	case RC_SAVEPERIOD:
		m_iSavePeriod = s.GetArgVal()*60*TICK_PER_SEC;
		break;
	case RC_SECTORSLEEP:
		m_iSectorSleepMask = ( 1 << s.GetArgVal()) - 1;
		break;
	case RC_SAVEBACKGROUND:
		m_iSaveBackgroundTime = s.GetArgVal() * 60 * TICK_PER_SEC;
		break;
	case RC_VERBOSE:
		g_Log.SetLogLevel( s.GetArgVal() ? LOGL_TRACE : LOGL_EVENT );
		break;

	case RC_WORLDSAVE: // Put save files here.
		m_sWorldBaseDir = CGFile::GetMergedFileName( s.GetArgStr(), "" );
		break;

	case RC_COMMANDLOG:
		m_iCommandLog = s.GetArgVal();
		break;
	
	case RC_USENOCRYPT:
		m_iUsenocrypt = s.GetArgVal();
		break;

	case RC_EXPERIMENTAL:
		g_Cfg.m_iExperimental = s.GetArgVal();
		PrintEFOFFlags(true, false);
		break;

	case RC_OPTIONFLAGS:
		g_Cfg.m_iOptionFlags = s.GetArgVal();
		PrintEFOFFlags(true, false);
		break;

	default:
		return( sm_szLoadKeys[i].m_elem.SetValStr( this, s.GetArgRaw()));
	}
	return true;
	EXC_CATCH("CResource");
	return false;
}


const CSkillDef * CResource::SkillLookup( LPCTSTR pszKey )
{

	int		iLen	= strlen( pszKey );
    const CSkillDef *		pDef;
	for ( int i = 0; i < m_SkillIndexDefs.GetCount(); ++i )
	{
		pDef	= STATIC_CAST <const CSkillDef*>(m_SkillIndexDefs[i]);
		if ( pDef->m_sName.IsEmpty() ?
				!strnicmp( pszKey, pDef->GetKey(), iLen )
			:	!strnicmp( pszKey, pDef->m_sName, iLen ) )
			return pDef;
	}
	return NULL;
}



bool CResource::r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc )
{
	EXC_TRY(("r_WriteVal('%s',,%x)", pszKey, pSrc));
	// Just do stats values for now.
	int i = FindTableHeadSorted( pszKey, (LPCTSTR const *) sm_szLoadKeys, COUNTOF( sm_szLoadKeys )-1, sizeof(sm_szLoadKeys[0]) );
	if ( i<0 )
	{
		if ( ! strnicmp( pszKey, "REGEN", 5 ))
		{
			int index = atoi(pszKey+5);
			if ( index >= STAT_QTY )
				return( false );
			sVal.FormatVal( g_BaseRaceClass.GetRegenRate( (STAT_TYPE) index ));
			return( true );
		}

		if ( !strnicmp( pszKey, "LOOKUPSKILL", 11 ) )
		{
			pszKey	+= 12;
			SKIP_SEPERATORS( pszKey );
			GETNONWHITESPACE( pszKey );

			const CSkillDef *	pSkillDef	= SkillLookup( pszKey );
			if ( !pSkillDef )
				sVal.FormatVal( -1 );
			else
				sVal.FormatVal( pSkillDef->GetResourceID().GetResIndex() );
			return true;
		}

		if ( !strnicmp( pszKey, "MAP(", 4 ) )
		{
			pszKey += 4;
			CPointMap pt;	// invalid point
			LPCTSTR __pszTemp = pszKey;
			TCHAR *pszTemp = Str_GetTemp();
			int x(0);

			while ( *pszKey != ')' ) { pszKey++; x++; }
			strcpylen(pszTemp, __pszTemp, ++x);

			x = 0;
			if ( isdigit( pszTemp[0] ) || pszTemp[0] == '-' )
			{
				pt.m_map = 0; pt.m_z = 0;
				TCHAR * ppVal[3];
				x = Str_ParseCmds( pszTemp, ppVal, COUNTOF( ppVal ), "," );
				switch ( x )
				{
					default:
					case 3:
						if ( isdigit(ppVal[2][0]))
						{
							pt.m_map = atoi(ppVal[2]);
						}
					case 2:
						{
							pt.m_y = atoi(ppVal[1]);
							pt.m_x = atoi(ppVal[0]);
						}
					case 1:
					case 0:
						break;
				}
			}

			if ( !pt.IsValidPoint() )
			{
				sVal.FormatVal( 0 );
				return false;
			}

			while ( *pszKey == ')' ) { pszKey++; }
			SKIP_SEPERATORS( pszKey );

			return pt.r_WriteVal( pszKey, sVal );
		}

		if ( !strnicmp( pszKey, "CLIENT.",7))
		{
			pszKey += 7;
			int cli_num = Exp_GetVal(pszKey);
			int i(0);
			SKIP_SEPERATORS(pszKey);

			sVal.FormatVal(0);
			for ( CClient * pClient = g_Serv.GetClientHead(); pClient != NULL; pClient = pClient->GetNext())
			{
				if ( cli_num == i )
				{
					CChar * pChar = pClient->GetChar();

					if ( pChar == NULL ) continue;
					else return pChar->r_WriteVal(pszKey, sVal, pSrc);
				}
				i++;
			}
			sVal.FormatVal(0);
			return true;
		}
		if ( !strnicmp(pszKey, "ITEMDEF.", 8) )
		{
			pszKey += 8;
			char *dot = strchr(pszKey, '.');
			if ( dot )
			{
				*dot = 0;
				dot++;
				ITEMID_TYPE id = (ITEMID_TYPE)g_Cfg.ResourceGetIndexType(RES_ITEMDEF, pszKey);
				CItemBase *pItemDef = CItemBase::FindItemBase(id);
				if ( pItemDef )
				{
					SKIP_SEPERATORS(dot);
					if ( *dot )
					{
						return pItemDef->r_WriteVal(dot, sVal, pSrc);
					}
				}
			}
			return false;
		}
		return( false );
	}

	switch (i)
	{
	case RC_BANKMAXWEIGHT:
		sVal.FormatVal( m_iBankWMax / WEIGHT_UNITS );
		break;
	case RC_CLIENTLINGER:
		sVal.FormatVal( m_iClientLingerTime / TICK_PER_SEC );
		break;
	case RC_CORPSENPCDECAY:
		sVal.FormatVal( m_iDecay_CorpseNPC / (60*TICK_PER_SEC));
		break;
	case RC_CORPSEPLAYERDECAY:
		sVal.FormatVal( m_iDecay_CorpsePlayer / (60*TICK_PER_SEC));
		break;
	case RC_CRIMINALTIMER:
		sVal.FormatVal( m_iCriminalTimer / (60*TICK_PER_SEC));
		break;
	case RC_DEADSOCKETTIME:
		sVal.FormatVal( m_iDeadSocketTime / (60*TICK_PER_SEC));
		break;
	case RC_DECAYTIMER:
		sVal.FormatVal( m_iDecay_Item / (60*TICK_PER_SEC));
		break;
	case RC_GUARDLINGER:
		sVal.FormatVal( m_iGuardLingerTime / (60*TICK_PER_SEC));
		break;
	case RC_HEARALL:
		sVal.FormatVal( g_Log.GetLogMask() & LOGM_PLAYER_SPEAK );
		break;
	case RC_HITSUPDATERATE:
		sVal.FormatVal( m_iHitsUpdateRate / TICK_PER_SEC );
		break;
	case RC_LOG:
		sVal = g_Log.GetLogDir();
		break;
	case RC_LOGMASK:
		sVal.FormatHex( g_Log.GetLogMask());
		break;
	case RC_MULFILES:
		sVal = g_Install.GetPreferPath(NULL);
		break;
	case RC_MAPCACHETIME:
		sVal.FormatVal( m_iMapCacheTime / TICK_PER_SEC );
		break;
	case RC_MINCHARDELETETIME:
		sVal.FormatVal( m_iMinCharDeleteTime /( 60*TICK_PER_SEC ));
		break;
	case RC_MURDERDECAYTIME:
		sVal.FormatVal( m_iMurderDecayTime / (TICK_PER_SEC ));
		break;
	case RC_WOOLGROWTHTIME:
		sVal.FormatVal( m_iWoolGrowthTime /( 60*TICK_PER_SEC ));
		break;
	case RC_PROFILE:
		sVal.FormatVal( g_Serv.m_Profile.GetActiveWindow());
		break;
	case RC_POLLSERVERS:
		sVal.FormatVal( m_iPollServers / (60*TICK_PER_SEC));
		break;
	case RC_RCLOCK:
		sVal.Format( "0%lx / 0%lx",	CWorldClock::GetSystemClock(), (long) CLOCKS_PER_SEC );
		return( true );
	case RC_REGISTERFLAG:
		sVal = m_sRegisterServer.IsEmpty() ? "0" : "1";
		break;
	case RC_RTIME:	// the current real world time.
	case RC_RTIMETEXT: // the current real world time.
		{
			CGTime datetime = CGTime::GetCurrentTime();
			sVal = datetime.Format(NULL);
		}
		return( true );
	case RC_SAVEPERIOD:
		sVal.FormatVal( m_iSavePeriod / (60*TICK_PER_SEC));
		break;
	case RC_SECTORSLEEP:
		sVal.FormatVal( Calc_GetLog2( m_iSectorSleepMask+1 )-1 );
		break;
	case RC_SAVEBACKGROUND:
		sVal.FormatVal( m_iSaveBackgroundTime / (60 * TICK_PER_SEC));
		break;
	case RC_GUILDS:
	case RC_STATGUILDS:
		sVal.FormatVal( g_World.m_Stones.GetCount());
		return( true );
	case RC_TIMEUP:
		sVal.FormatVal( ( - g_World.GetTimeDiff( g_World.m_timeStartup )) / TICK_PER_SEC );
		return( true );
	case RC_TOTALPOLLEDACCOUNTS:
		sVal.FormatVal( g_BackTask.m_iTotalPolledAccounts );
		return( true );
	case RC_TOTALPOLLEDCLIENTS:
		sVal.FormatVal( g_BackTask.m_iTotalPolledClients );
		return( true );
	case RC_TOTALPOLLEDSERVERS:
		sVal.FormatVal( m_Servers.GetCount() + 1 );
		return( true );
	case RC_VERBOSE:
		sVal.FormatVal(( g_Log.GetLogLevel() >= LOGL_TRACE ) ? 1 : 0 );
		break;
	case RC_VERSION:
		{
			TCHAR * tVersionOut = Str_GetTemp();
			sprintf(tVersionOut, g_szServerDescription, (LPCTSTR)g_Cfg.m_sVerName.GetPtr(), (LPCTSTR)g_Serv.m_sServVersion.GetPtr());
			sVal = (LPCTSTR) tVersionOut;
		}
		break;
	case RC_EXPERIMENTAL:
		sVal.FormatHex( g_Cfg.m_iExperimental );
		PrintEFOFFlags(true, false, pSrc);
		break;
	case RC_OPTIONFLAGS:
		sVal.FormatHex( g_Cfg.m_iOptionFlags );
		PrintEFOFFlags(false, true, pSrc);
		break;
	case RC_COMMANDLOG:
		sVal.FormatHex( g_Cfg.m_iCommandLog );
		break;
	case RC_CLIENTS:		// this is handled by CServerDef as SV_CLIENTS
		return false;
	default:
		return( sm_szLoadKeys[i].m_elem.GetValStr( this, sVal ));
	}
	return true;
	EXC_CATCH("CResource");
	return false;
}

void CResource::r_Write( CScript &s )
{
	CResource defaults;	// compare against the defaults.

	// Now write all the stuff in it.
	for ( int i=0; i<RC_QTY; i++ )
	{
		if ( sm_szLoadKeys[i].m_elem.m_type != ELEM_VOID )
		{
			if ( ! sm_szLoadKeys[i].m_elem.CompareVal( this, &defaults ))
				continue;
		}
		else switch ( i )	// handle the special cases here.
		{
		case RC_MULFILES:
			if ( ! g_Install.GetPreferPath().IsEmpty() )
				break;
			continue;
		case RC_LOGMASK:
			if ( g_Log.GetLogMask() != (LOGM_INIT | LOGM_CLIENTS_LOG | LOGM_GM_PAGE ))
				break;
			continue;
		case RC_LOG:
			if ( g_Log.GetLogDir()[0] != '\0' )
				break;
			continue;
		case RC_PROFILE:
			if ( g_Serv.m_Profile.GetActiveWindow() != 10 )
				break;
			continue;
		case RC_VERBOSE:
			if ( g_Log.GetLogLevel() != LOGL_EVENT )
				break;
			continue;
		default:
			continue;
		}

		CGString sVal;
		if ( ! r_WriteVal( sm_szLoadKeys[i].m_pszKey, sVal, &g_Serv ))
			continue;

		s.WriteKey( sm_szLoadKeys[i].m_pszKey, sVal );
	}
}

//*************************************************************

bool CResource::IsConsoleCmd( TCHAR ch ) const
{
	return( ch == '.' || ch == '=' || ch == '/' );
}

bool CResource::CanRunBackTask() const
{
	// Run the background task or not ?

	if ( g_Serv.IsLoading() || g_Serv.m_iExitFlag )
		return( false );
	if ( m_iPollServers && m_Servers.GetCount())
		return( true );
	if ( m_sRegisterServer.GetLength())
		return( true );

	return( false );
}

SKILL_TYPE CResource::FindSkillKey( LPCTSTR pszKey ) const
{
	// Find the skill name in the alpha sorted list.
	// RETURN: SKILL_NONE = error.

	if ( isdigit( pszKey[0] ))
	{
		SKILL_TYPE skill = (SKILL_TYPE) Exp_GetVal(pszKey);
		if ( ! CChar::IsSkillBase(skill) &&
			! CChar::IsSkillNPC(skill))
		{
			return( SKILL_NONE );
		}
		return( skill );
	}

	const CSkillDef* pSkillDef = FindSkillDef( pszKey );
	if ( pSkillDef == NULL )
		return( SKILL_NONE );
	return( (SKILL_TYPE)( pSkillDef->GetResourceID().GetResIndex()));
}

STAT_TYPE CResource::FindStatKey( LPCTSTR pszKey ) // static
{
	return (STAT_TYPE) FindTable( pszKey, g_Stat_Name, COUNTOF( g_Stat_Name ));
}

int CResource::GetSpellEffect( SPELL_TYPE spell, int iSkillVal ) const
{
	// NOTE: Any randomizing of the effect must be done by varying the skill level .
	// iSkillVal = 0-1000
	DEBUG_CHECK(spell);
	if ( ! spell )
		return( 0 );
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef( spell );
	if ( pSpellDef == NULL )
		return( 0 );
	return( pSpellDef->m_Effect.GetLinear( iSkillVal ));
}

bool CResource::IsValidEmailAddressFormat( LPCTSTR pszEmail ) // static
{
	// what are the invalid email name chars ?
	// Valid characters are, numbers, letters, underscore "_", dash "-" and the dot ".").

	int len1 = strlen( pszEmail );
	if ( len1 <= 0 || len1 > 128 )
		return( false );

	TCHAR szEmailStrip[256];
	int len2 = Str_GetBare( szEmailStrip, pszEmail,
		sizeof(szEmailStrip),
		" !\"#%&()*,/:;<=>?[\\]^{|}'`+" );
	if ( len2 != len1 )
		return( false );

	TCHAR * pszAt = strchr( pszEmail, '@' );
	if ( ! pszAt )
		return( false );
	if ( pszAt == pszEmail )
		return( false );
	if ( ! strchr( pszAt, '.' ))
		return( false );

	// ??? list of banned email addresses ?

	return( true );
}

CServerRef CResource::Server_GetDef( int index )
{
	CThreadLockRef lock( &m_Servers );
	if ( ! m_Servers.IsValidIndex(index))
		return( NULL );
	return( CServerRef( STATIC_CAST <CServerDef*>( m_Servers[index] )));
}

CWebPageDef * CResource::FindWebPage( LPCTSTR pszPath ) const
{
	// "--WEBBOT-SELF--" == the Referer.

	if ( pszPath == NULL )
	{
takedefault:
		if ( ! m_WebPages.GetCount())
			return( NULL );
		// Take this as the default page.
		return( STATIC_CAST <CWebPageDef*>( m_WebPages[0] ));
	}

	LPCTSTR pszTitle = CGFile::GetFilesTitle(pszPath);

	if ( pszTitle == NULL || pszTitle[0] == '\0' )
	{
		// This is just the root index.
		goto takedefault;
	}

	for ( int i=0; i<m_WebPages.GetCount(); i++ )
	{
		if ( m_WebPages[i] == NULL )	// not sure why this would happen
			continue;
		CWebPageDef * pWeb = STATIC_CAST <CWebPageDef*>(m_WebPages[i] );
		ASSERT(pWeb);
		if ( pWeb->IsMatch(pszTitle))
			return( pWeb );
	}

	return( NULL );
}

bool CResource::IsObscene( LPCTSTR pszText ) const
{
	// does this text contain obscene content?
	// NOTE: allow partial match syntax *fuck* or ass (alone)

	for ( int i=0; i<m_Obscene.GetCount(); i++ )
	{
		MATCH_TYPE ematch = Str_Match( m_Obscene[i], pszText );
		if ( ematch == MATCH_VALID )
			return( true );
	}
	return( false );
}

CLogIP * CResource::FindLogIP( CSocketAddressIP IP, bool fCreate )
{
	if ( ! IP.IsValidAddr() )
	{
		// this address is always blocked.
		return( NULL );
	}

	// Will create if not found.
	for ( int i=0; i<m_LogIP.GetCount(); i++ )
	{
		CLogIP * pLogIP = m_LogIP[i];

		if ( pLogIP->IsSameIP( IP ))
		{
			if ( pLogIP->IsTimeDecay())	// time to decay the whole record ?
			{
				pLogIP->InitTimes();	// it has decayed but we didn't get rid of it yet.
			}
			return( pLogIP );
		}
		if ( pLogIP->IsTimeDecay())
		{
			// remove it.
			m_LogIP.DeleteAt( i );
			i--;
		}
	}

	// Create a new 1.
	if ( ! fCreate )
		return( NULL );

	CLogIP * pLogIP = new CLogIP( IP );
	m_LogIP.Add( pLogIP );
	return( pLogIP );
}

bool CResource::SetLogIPBlock( LPCTSTR szIP, bool fBlock )
{
	// Block or unblock an IP.
	// RETURN: true = set, false = already that state.
	CSocketAddressIP dwIP;
	dwIP.SetAddrStr( szIP );

	CLogIP * pLogIP = FindLogIP( dwIP, fBlock );
	if ( pLogIP == NULL )
		return( false );

	bool fPrevBlock = pLogIP->IsBlocked();
	if ( ! fBlock )
	{
		if ( ! fPrevBlock )
			return( false );	// not here.
		m_LogIP.DeleteOb( pLogIP );
	}
	else
	{
		if ( fPrevBlock )
			return( false );	// not here.
		pLogIP->SetBlocked( true, -1 );
	}

	// Write change to *.INI
	if ( ! g_Serv.IsLoading())
	{
		m_scpIni.WriteProfileStringSec( "BLOCKIP", szIP, fBlock ? "" : NULL );
	}

	return( true );
}

const CGrayMulti * CResource::GetMultiItemDefs( ITEMID_TYPE itemid )
{
	if ( ! CItemBase::IsID_Multi(itemid))
		return( NULL );

	MULTI_TYPE id = itemid - ITEMID_MULTI;
	int index = m_MultiDefs.FindKey( id );
	if ( index < 0 )
	{
		index = m_MultiDefs.AddSortKey( new CGrayMulti( id ), id );
	}
	else
	{
		m_MultiDefs[index]->HitCacheTime();
	}
	const CGrayMulti * pMulti = m_MultiDefs[index];
	ASSERT(pMulti);
	return( pMulti );
}

PLEVEL_TYPE CResource::GetPrivCommandLevel( LPCTSTR pszCmd ) const
{
	// What is this commands plevel ?
	// NOTE: This doe snot attempt to parse anything.

	int ilevel = PLEVEL_QTY-1;

	// Is this command avail for your priv level (or lower) ?
	for ( ; ilevel >= 0; ilevel-- )
	{
		LPCTSTR const * pszTable = m_PrivCommands[ilevel].GetBasePtr();
		int iCount = m_PrivCommands[ilevel].GetCount();
		if ( FindTableHeadSorted( pszCmd, pszTable, iCount ) >= 0 )
			return( (PLEVEL_TYPE)ilevel );
	}

	// A GM will default to use all commands.
	// xcept those that are specifically named that i can't use.
	return( PLEVEL_GM );	// default level.
}

bool CResource::CanUsePrivVerb( const CScriptObj * pObjTarg, LPCTSTR pszCmd, CTextConsole * pSrc ) const
{
	// can i use this verb on this object ?
	// Check just at entry points where commands are entered or targetted.
	// NOTE:
	//  Call this each time we change pObjTarg such as r_GetRef()
	// RETURN: 
	//  true = i am ok to use this command.

	if ( pSrc == NULL )
	{
		DEBUG_CHECK(pSrc);
		return( false );
	}
	ASSERT(pObjTarg);

	// Are they more privleged than me ?

	const CChar * pChar = dynamic_cast <const CChar*> (pObjTarg);
	if ( pChar )
	{
		if ( pSrc->GetChar() == pChar )
			return( true );
		if ( pSrc->GetPrivLevel() < pChar->GetPrivLevel())
		{
			pSrc->SysMessage( "Target is more privileged than you" DEBUG_CR );
			return false;
		}
	}

	// can i use this verb on this object ?

	if ( pSrc->GetChar() == NULL )
	{
		// I'm not a cchar. what am i ?
		CClient * pClient = dynamic_cast <CClient *>(pSrc);
		if ( pClient )
		{
			// We are not logged in as a player char ? so we cannot do much !
			if ( pClient->GetAccount() == NULL )
			{
				// must be a console or web page ?
				// I guess we can just login !
				if ( ! strnicmp( pszCmd, "LOGIN", 5 ))
					return( true );
				return( false );
			}
		}
		else
		{
			// we might be the g_Serv or web page ?
		}
	}

	// Is this command avail for your priv level (or lower) ?

	PLEVEL_TYPE ilevel = GetPrivCommandLevel( pszCmd );
	if ( ilevel > pSrc->GetPrivLevel())
		return( false );

	return( true );
}

//*************************************************************

CPointMap CResource::GetRegionPoint( LPCTSTR pCmd ) const // Decode a teleport location number into X/Y/Z
{
	// get a point from a name. (probably the name of a region)
	// Might just be a point coord number ?

	GETNONWHITESPACE( pCmd );
	if ( pCmd[0] == '-' && !strchr( pCmd, ',' ) )	// Get location from start list.
	{
		int i = ( - atoi(pCmd)) - 1;
		if ( ! m_StartDefs.IsValidIndex( i ))
			i = 0;
		return( m_StartDefs[i]->m_pt );
	}

	CPointMap pt;	// invalid point
	if ( isdigit( pCmd[0] ) || pCmd[0] == '-' )
	{
		TCHAR *pszTemp = Str_GetTemp();
		strcpy( pszTemp, pCmd );
		int iCount = pt.Read( pszTemp );
		if ( iCount >= 2 )
		{
			return( pt );
		}
	}
	else
	{
		// Match the region name with global regions.

		for ( int i=0; i<COUNTOF(m_ResHash.m_Array); i++ )
		for ( int j=0; j<m_ResHash.m_Array[i].GetCount(); j++ )
		{
			CResourceDef* pResDef = m_ResHash.m_Array[i][j];
			ASSERT(pResDef);
			CRegionBase * pRegion = dynamic_cast <CRegionBase*> (pResDef);
			if ( pRegion == NULL )
				continue;

			if ( ! pRegion->GetNameStr().CompareNoCase( pCmd ) ||
				! strcmpi( pRegion->GetResourceName(), pCmd ))
			{
				return( pRegion->m_pt );
			}
		}
	}
	// no match.
	return( pt );
}




void	CResource::LoadSortSpells()
{
	int		iQtySpells	= m_SpellDefs.GetCount();
	m_SpellDefs_Sorted.RemoveAll();
	m_SpellDefs_Sorted.Add( m_SpellDefs[0] );		// the null spell

	for ( int i = 1; i < iQtySpells; i++ )
	{
		if ( !m_SpellDefs.IsValidIndex( i ) )
			continue;

		int	iVal	= 0;
		m_SpellDefs[i]->GetPrimarySkill( NULL, &iVal );

		int		iQty	= m_SpellDefs_Sorted.GetCount();
		int		k;
		for ( k = 1; k < iQty; k++ )
		{
			int	iVal2	= 0;
			m_SpellDefs_Sorted[k]->GetPrimarySkill( NULL, &iVal2 );
			if ( iVal2 > iVal )
				break;
		}
		m_SpellDefs_Sorted.InsertAt( k, m_SpellDefs[i] );
	}
}

//*************************************************************

bool CResource::LoadResourceSection( CScript * pScript )
{
	bool	fNewStyleDef	= false;

	// Index or read any resource blocks we know how to handle.
	ASSERT(pScript);
	CScriptFileContext FileContext( pScript );	// set this as the context.
	CVarDefNum * pVarNum = NULL;
	RESOURCE_ID rid;
	LPCTSTR		pszSection	= pScript->GetSection();
	
	RES_TYPE restype;
	if ( !strnicmp( pszSection, "DEFMESSAGE", 10 ) )
	{
		restype			= RES_DEFNAME;
		fNewStyleDef	= true;
	}
	else if ( !strnicmp( pszSection, "AREADEF", 7 ) )
	{
		restype			= RES_AREA;
		fNewStyleDef	= true;
	}
	else if ( !strnicmp( pszSection, "ROOMDEF", 7 ) )
	{
		restype			= RES_ROOM;
		fNewStyleDef	= true;
	}
	else if ( !strnicmp( pszSection, "GLOBALS", 7 ) )
	{
		restype			= RES_WORLDVARS;
	}
	else if ( !strncmp(pszSection, "PROFILES", 8) )
		restype			= RES_WORLDPROFILE;
	else 
		restype	= (RES_TYPE) FindTableSorted( pszSection, sm_szResourceBlocks, COUNTOF( sm_szResourceBlocks ));


	if (( restype == RES_WORLDSCRIPT ) || ( restype == RES_WS ))
	{
		LPCTSTR	pszDef			= pScript->GetArgStr();
		CVarDefBase * pVarBase	= g_Exp.m_VarDefs.GetKey( pszDef );
		CVarDefBase * pVarNum	= NULL;
		if ( pVarBase )
			pVarNum				= dynamic_cast <CVarDefNum*>( pVarBase );
		if ( !pVarNum )
		{
			g_Log.Event( LOGL_WARN|LOGM_INIT, "Resource '%s' not found" DEBUG_CR, pszDef );
			return false;
		}

		rid.SetPrivateUID( pVarNum->GetValNum() );
		restype	= rid.GetResType();

		int		index	= m_ResHash.FindKey( rid );


		CResourceDef *	pRes	= NULL;
		if ( index >= 0 )
			pRes	= dynamic_cast <CResourceDef*> (m_ResHash.GetAt( rid, index ) );
		if ( !pRes )
		{
			g_Log.Event( LOGL_WARN|LOGM_INIT, "Resource '%s' not found" DEBUG_CR, pszDef );
			return false;
		}
		
		pRes->r_Load( *pScript );
		return true;
	}

	if ( restype < 0 )
	{
		g_Log.Event( LOGL_WARN|LOGM_INIT, "Unknown section '%s' in '%s'" DEBUG_CR, (LPCTSTR) pScript->GetKey(), (LPCTSTR) pScript->GetFileTitle());
		return( false );
	}
	else
	{
		// Create a new index for the block.
		// NOTE: rid is not created for all types.
		// NOTE: GetArgStr() is not always the DEFNAME
		rid = ResourceGetNewID( restype, pScript->GetArgStr(), &pVarNum, fNewStyleDef );
	}

	if ( ! rid.IsValidUID())
	{
		DEBUG_ERR(( "Invalid %s block index '%s'" DEBUG_CR, pszSection, (LPCTSTR) pScript->GetArgStr()));
		return( false );
	}

	EXC_TRY(("LoadResourceSection('%s %s')", pszSection, pScript->GetArgStr()));

	// NOTE: It is possible to be replacing an existing entry !!! Check for this.

	CResourceLink * pNewLink = NULL;
	CResourceDef * pNewDef = NULL;
	CResourceDef * pPrvDef = NULL;

	switch ( restype )
	{
	case RES_SPHERE:
		// Define main global config info.
		g_Serv.r_Load(*pScript);
		return( true );

	case RES_ACCOUNT:	// NOTE: ArgStr is not the DEFNAME
		// Load the account now. Not normal method !
		return g_Accounts.Account_Load( pScript->GetArgStr(), *pScript, false );

	case RES_ADVANCE:
		// Stat advance rates.
		while ( pScript->ReadKeyParse())
		{
			int i = FindStatKey( pScript->GetKey());
			if ( i >= STAT_BASE_QTY )
				continue;
			m_StatAdv[i].Load( pScript->GetArgStr());
		}
		return( true );

	case RES_BLOCKIP:
		while ( pScript->ReadKeyParse())
		{
			SetLogIPBlock( pScript->GetKey(), true );
		}
		return( true );

	case RES_COMMENT:
		// Just toss this.
		return( true );

	case RES_DEFNAME:
		// just get a block of defs.
		while ( pScript->ReadKeyParse())
		{
			LPCTSTR	pszKey = pScript->GetKey();
			if ( fNewStyleDef )
			{
				//	search for this.
				long	l;
				for ( l = 0; l < DEFMSG_QTY; l++ )
				{
					if ( !strcmpi(pszKey, (const char *)g_Exp.sm_szMsgNames[0][l]) )
					{
						strcpy(g_Exp.sm_szMessages[l], pScript->GetArgStr());
						break;
					}
				}
				if ( l == DEFMSG_QTY )
					g_Log.Event(LOGM_INIT|LOGL_ERROR, "Setting not used message override named '%s'" DEBUG_CR, pszKey);
				continue;
			}
			else g_Exp.m_VarDefs.SetStr(pszKey, false, pScript->GetArgStr());
		}
		return( true );
	case RES_LOCATION:
		// ignore this. (Axis stuff)
		return( true );
	case RES_NOTOTITLES:
		{
			int i=0;
			while ( pScript->ReadKey())
			{
				TCHAR * pName = pScript->GetKeyBuffer();
				if ( * pName == '<' )
					pName = "";
				TCHAR * pNew = new TCHAR [ strlen( pName ) + 1 ];
				strcpy( pNew, pName );
				m_NotoTitles.SetAtGrow( i, pNew );
				i++;
			}
		}
		return( true );
	case RES_OBSCENE:
		while ( pScript->ReadKey())
		{
			m_Obscene.AddSortString( pScript->GetKey());
		}
		return( true );
	case RES_PLEVEL:
		{
			int index = rid.GetResIndex();
			if ( index >= COUNTOF(m_PrivCommands) )
				return false;
			while ( pScript->ReadKey() )
			{
				LPCTSTR	key = pScript->GetKey();
				m_PrivCommands[index].AddSortString(key);

#ifdef _DEBUG
				for ( int i = 0; i < m_PrivCommands[index].GetCount()-1; i++ )
				{
					if ( strcmpi(m_PrivCommands[index].GetAt(i), m_PrivCommands[index].GetAt(i+1)) >= 0 )
						goto sortfail;
				}
				goto sortok;
sortfail:
				DEBUG_ERR(("Adding %s : sort FAIL FAIL FAIL (%d)" DEBUG_CR, key, index));
sortok:			;
#endif
			}
		}
		return( true );
	case RES_RESOURCES:
		// Add these all to the list of files we need to include.
		while ( pScript->ReadKey())
		{
			AddResourceFile( pScript->GetKey());
		}
		return( true );
	case RES_RUNES:
		// The full names of the magic runes.
		m_Runes.RemoveAll();
		while ( pScript->ReadKey())
		{
			TCHAR * pNew = new TCHAR [ strlen( pScript->GetKey()) + 1 ];
			strcpy( pNew, pScript->GetKey());
			m_Runes.Add( pNew );
		}
		return( true );
	case RES_SECTOR: // saved in world file.
		{
			CPointMap pt = GetRegionPoint( pScript->GetArgStr() ); // Decode a teleport location number into X/Y/Z
			return( pt.GetSector()->r_Load(*pScript));
		}
		return( true );
	case RES_SPELL:
		{
			CSpellDef * pSpell;
			pPrvDef = ResourceGetDef( rid );
			if ( pPrvDef )
			{
				pSpell = dynamic_cast <CSpellDef*>(pPrvDef);
			}
			else
			{
				pSpell = new CSpellDef( (SPELL_TYPE) rid.GetResIndex() );
			}
			ASSERT(pSpell);
			pNewLink = pSpell;

			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext( LineContext );

			if ( !pPrvDef )
			{		
				m_SpellDefs.SetAtGrow( rid.GetResIndex(), pSpell );
			}
		}
		break;

	case RES_SKILL:
		{
			CSkillDef * pSkill;
			pPrvDef = ResourceGetDef( rid );
			if ( pPrvDef )
			{
				pSkill = dynamic_cast <CSkillDef*>(pPrvDef);
			}
			else
			{
				if ( rid.GetResIndex() >= g_Cfg.m_iMaxSkill )
					g_Cfg.m_iMaxSkill	= rid.GetResIndex() +1 ;

				// Just replace any previous CSkillDef
				pSkill = new CSkillDef( (SKILL_TYPE) rid.GetResIndex());
			}

			ASSERT(pSkill);
			pNewLink = pSkill;

			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext( LineContext );

			if ( !pPrvDef )
			{
				// build a name sorted list.
				m_SkillNameDefs.AddSortKey( pSkill, pSkill->GetKey());
				// Hard coded value for skill index.
				m_SkillIndexDefs.SetAtGrow( rid.GetResIndex(), pSkill );
			}
		}
		break;

	case RES_TYPEDEF:
	{
		// Just index this for access later.
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			CItemTypeDef	* pTypeDef	= dynamic_cast <CItemTypeDef*>(pPrvDef);
			ASSERT( pTypeDef );
			pNewLink = pTypeDef;
			ASSERT(pNewLink);

			// clear old tile links to this type
			int iQty	= g_World.m_TileTypes.GetCount();
			for ( int i = 0; i < iQty; i++ )
			{
				if ( g_World.m_TileTypes.GetAt(i) == pTypeDef )
					g_World.m_TileTypes.SetAt( i, NULL );
			}

		}
		else
		{
			pNewLink = new CItemTypeDef( rid );
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}

		CScriptLineContext LineContext = pScript->GetContext();
		pNewLink->r_Load(*pScript);
		pScript->SeekContext( LineContext );
		break;
	}

	//*******************************************************************
	// Might need to check if the link already exists ?

	case RES_BOOK:
	case RES_CRYSTALBALL:
	case RES_EMAILMSG:
	case RES_EVENTS:
	case RES_MENU:
	case RES_NAMES:
	case RES_NEWBIE:
	case RES_TIP:
	case RES_SPEECH:
	case RES_SCROLL:
	case RES_TEMPLATE:
	case RES_SKILLMENU:
		// Just index this for access later.
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CResourceLink*>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CResourceLink( rid );
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		break;
	case RES_DIALOG:
		// Just index this for access later.
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CDialogDef*>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CDialogDef( rid );
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		break;

	case RES_REGIONRESOURCE:
		// No need to Link to this really .
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CRegionResourceDef*>( pPrvDef );
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CRegionResourceDef( rid );
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		{
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext( LineContext ); // set the pos back so ScanSection will work.
		}

		break;
	case RES_AREA:	
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef && fNewStyleDef )
		{
			CRegionWorld *	pRegion = dynamic_cast <CRegionWorld*>( pPrvDef );
			pNewDef	= pRegion;
			ASSERT(pNewDef);
			pRegion->UnRealizeRegion();
			pRegion->r_Load(*pScript);
			pRegion->RealizeRegion();
		}
		else
		{
			CRegionWorld * pRegion = new CRegionWorld( rid, pScript->GetArgStr());
			pNewDef = pRegion;
			ASSERT(pNewDef);
			pRegion->r_Load( *pScript );
			if ( ! pRegion->RealizeRegion() )
				delete pRegion; // might be a dupe ?
			else
			{
				m_ResHash.AddSortKey( rid, pRegion );
				// if it's old style but has a defname, it's already set via r_Load,
				// so this will do nothing, which is good
				// if ( !fNewStyleDef )
				//	pRegion->MakeRegionName();
				m_RegionDefs.Add( pRegion );
			}
		}
		break;
	case RES_ROOM:	
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef && fNewStyleDef )
		{
			CRegionBase *	pRegion = dynamic_cast <CRegionBase*>( pPrvDef );
			pNewDef	= pRegion;
			ASSERT(pNewDef);
			pRegion->UnRealizeRegion();
			pRegion->r_Load(*pScript);
			pRegion->RealizeRegion();
		}
		else
		{
			CRegionBase * pRegion = new CRegionBase( rid, pScript->GetArgStr());
			pNewDef = pRegion;
			ASSERT(pNewDef);
			pRegion->r_Load(*pScript);
			if ( !pRegion->RealizeRegion() )
				delete pRegion; // might be a dupe ?
			else
			{
				m_ResHash.AddSortKey( rid, pRegion );
				// if it's old style but has a defname, it's already set via r_Load,
				// so this will do nothing, which is good
				// if ( !fNewStyleDef )
				//	pRegion->MakeRegionName();
				m_RegionDefs.Add( pRegion );
			}
		}
		break;
	case RES_REGIONTYPE:
	case RES_SPAWN:
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CRandGroupDef*>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CRandGroupDef( rid );
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		{
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load( *pScript );
			pScript->SeekContext( LineContext );
		}
		break;

	case RES_SKILLCLASS:
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CSkillClassDef*>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CSkillClassDef( rid );
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		{
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load( *pScript );
			pScript->SeekContext( LineContext );
		}
		break;

	case RES_CHARDEF:
	case RES_ITEMDEF:
		// ??? existing hard pointers to RES_CHARDEF ?
		// ??? existing hard pointers to RES_ITEMDEF ?
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast<CResourceLink*>(pPrvDef);
			if ( pNewLink == NULL )
			{
				// CItemBaseDupe ?
				return( true );
			}
			CBaseBaseDef * pBaseDef = dynamic_cast <CBaseBaseDef*> (pNewLink);
			if ( pBaseDef )
			{
				pBaseDef->UnLink();
				CScriptLineContext LineContext = pScript->GetContext();
				pBaseDef->r_Load(*pScript);
				pScript->SeekContext( LineContext );
			}
		}
		else
		{
			pNewLink = new CResourceLink( rid );
			ASSERT(pNewLink);
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		break;

	// Map stuff that could be duplicated !!!
	// NOTE: ArgStr is NOT the DEFNAME in this case
	// ??? duplicate areas ?
	// ??? existing hard pointers to areas ?
	// KELL HERE
	case RES_WEBPAGE:
		// Read a web page entry.
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CWebPageDef *>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CWebPageDef( rid );
			ASSERT(pNewLink);
			m_WebPages.AddSortKey( pNewLink, rid );
		}
		{
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext( LineContext ); // set the pos back so ScanSection will work.
		}
		break;

	case RES_HELP:	// (Name is NOT DEFNAME)
		pNewLink = new CResourceNamed( rid, pScript->GetArgStr());
		m_HelpDefs.AddSortKey( pNewLink, pNewLink->GetName());
		break;

	case RES_FUNCTION:
		// Define a char macro. (Name is NOT DEFNAME)
		pNewLink = new CResourceNamed( rid, pScript->GetArgStr());
		m_Functions.AddSortKey( pNewLink, pNewLink->GetName());
		break;

	case RES_SERVER:	// saved in world file.
		{
			CThreadLockRef lock( &m_Servers );
			CServerRef pServ;
			int i = m_Servers.FindKey( pScript->GetKey());
			if ( i < 0 )
			{
				pServ = new CServerDef( pScript->GetArgStr(), CSocketAddressIP( SOCKET_LOCAL_ADDRESS ));
			}
			else
			{
				pServ = Server_GetDef(i);
			}
			pServ->r_Load( *pScript );
			if ( pServ->GetName()[0] == '\0' )
			{
				delete pServ;
				return( false );
			}
			if ( i < 0 )
			{
				m_Servers.AddSortKey( pServ, pServ->GetName() );
			}
		}
		return( true );
	case RES_SERVERS:	// Old way to define a block of servers.
		{
			bool fReadSelf = false;

			CThreadLockRef lock( &m_Servers );
			while ( pScript->ReadKey())
			{
				// Does the name already exist ?
				bool fAddNew = false;
				CServerRef pServ;
				int i = m_Servers.FindKey( pScript->GetKey());
				if ( i < 0 )
				{
					pServ = new CServerDef( pScript->GetKey(), CSocketAddressIP( SOCKET_LOCAL_ADDRESS ));
					fAddNew = true;
				}
				else
				{
					pServ = Server_GetDef(i);
				}
				if ( pScript->ReadKey())
				{
					pServ->m_ip.SetHostPortStr( pScript->GetKey());
					if ( pScript->ReadKey())
					{
						pServ->m_ip.SetPort( pScript->GetArgVal());
					}
				}
				if ( ! strcmpi( pServ->GetName(), g_Serv.GetName()))
				{
					fReadSelf = true;
				}
				if ( g_Serv.m_ip == pServ->m_ip )
				{
					fReadSelf = true;
				}
				if ( fReadSelf )
				{
					// I can be listed first here. (old way)
					g_Serv.SetName( pServ->GetName());
					g_Serv.m_ip = pServ->m_ip;
					delete pServ;
					fReadSelf = false;
					continue;
				}
				if ( fAddNew )
				{
					m_Servers.AddSortKey( pServ, pServ->GetName());
				}
			}
		}
		return( true );

	case RES_TYPEDEFS:
		// just get a block of defs.
		while ( pScript->ReadKeyParse())
		{
			RESOURCE_ID ridnew( RES_TYPEDEF, pScript->GetArgVal() );
			pPrvDef = ResourceGetDef( ridnew );
			if ( pPrvDef )
			{
				pPrvDef->SetResourceName( pScript->GetKey() );
			}
			else
			{
				CResourceDef * pResDef = new CItemTypeDef( ridnew );
				pResDef->SetResourceName( pScript->GetKey() );
				ASSERT(pResDef);
				m_ResHash.AddSortKey( ridnew, pResDef );
			}
		}
		return( true );

	case RES_STARTS:
		m_StartDefs.RemoveAll();
		while ( pScript->ReadKey())
		{
			CStartLoc * pStart = new CStartLoc( pScript->GetKey());
			if ( pScript->ReadKey())
			{
				pStart->m_sName = pScript->GetKey();
				if ( pScript->ReadKey())
				{
					pStart->m_pt.Read( pScript->GetKeyBuffer());
				}
			}
			m_StartDefs.Add( pStart );
		}
		return( true );

	case RES_MOONGATES:
		m_MoonGates.RemoveAll();
		while ( pScript->ReadKey())
		{
			CPointMap pt = GetRegionPoint( pScript->GetKey());
			m_MoonGates.Add( pt );
		}
		return( true );
	case RES_WORLDVARS:
		while ( pScript->ReadKeyParse() )
		{
			bool fQuoted = false;
			g_Exp.m_VarGlobals.SetStr( pScript->GetKey(), fQuoted, pScript->GetArgStr( &fQuoted ) );
		}
		return true;
	case RES_TELEPORTERS:
		while ( pScript->ReadKey())
		{
			// Add the teleporter to the CSector.
			CTeleport * pTeleport = new CTeleport( pScript->GetKeyBuffer());
			ASSERT(pTeleport);
			// make sure this is not a dupe.
			if ( ! pTeleport->RealizeTeleport())
			{
				delete pTeleport;
			}
		}
		return( true );

	// Saved in the world file.

	case RES_GMPAGE:	// saved in world file. (Name is NOT DEFNAME)
		{
			CGMPage * pGMPage = new CGMPage( pScript->GetArgStr());
			return( pGMPage->r_Load( *pScript ));
		}
		return( true );
	case RES_WC:
	case RES_WORLDCHAR:	// saved in world file.
		if ( ! rid.IsValidUID())
		{
			g_Log.Event( LOGL_ERROR|LOGM_INIT, "Undefined char type '%s'" DEBUG_CR, (LPCTSTR) pScript->GetArgStr() );
			return( false );
		}
		return( CChar::CreateBasic((CREID_TYPE)rid.GetResIndex())->r_Load(*pScript));
	case RES_WI:
	case RES_WORLDITEM:	// saved in world file.
		if ( ! rid.IsValidUID())
		{
			g_Log.Event( LOGL_ERROR|LOGM_INIT, "Undefined item type '%s'" DEBUG_CR, (LPCTSTR) pScript->GetArgStr() );
			return( false );
		}
		return( CItem::CreateBase((ITEMID_TYPE)rid.GetResIndex())->r_Load(*pScript));

	case RES_WORLDPROFILE:
		return g_Serv.m_Profile.r_Load(*pScript);

	default:
		ASSERT(0);
		return( false );
	}

	if ( pNewLink )
	{
		pNewLink->SetResourceVar( pVarNum );

		// NOTE: we should not be linking to stuff in the *WORLD.SCP file.
		CResourceScript* pResScript = dynamic_cast <CResourceScript*>(pScript);
		if ( pResScript == NULL )	// can only link to it if it's a CResourceScript !
		{
			DEBUG_ERR(( "Can't link resources in the world save file" DEBUG_CR ));
			return( false );
		}
		// Now scan it for DEFNAME= or DEFNAME2= stuff ?
		pNewLink->SetLink(pResScript);
		pNewLink->ScanSection( restype );
	}
	else if ( pNewDef && pVarNum )
	{
		// Not linked but still may have a var name
		pNewDef->SetResourceVar( pVarNum );
	}
	return( true );

	EXC_CATCH(("CResource"));
	return false;
}

//*************************************************************

RESOURCE_ID CResource::ResourceGetNewID( RES_TYPE restype, LPCTSTR pszName, CVarDefNum ** ppVarNum, bool fNewStyleDef )
{
	// We are reading in a script block.
	// We may be creating a new id or replacing an old one.
	// ARGS:
	//	restype = the data type we are reading in.
	//  pszName = MAy or may not be the DEFNAME depending on type.

	ASSERT(pszName);
	ASSERT(ppVarNum);

	const RESOURCE_ID ridinvalid;	// LINUX wants this for some reason.
	RESOURCE_ID rid;
	int iPage = 0;	// sub page

	// Some types don't use named indexes at all. (Single instance)
	switch ( restype )
	{
	case RES_UNKNOWN:
		return( ridinvalid);
	case RES_CRYSTALBALL:
		// (single instance) m_ResourceLinks linked stuff.
		return( RESOURCE_ID( restype ));
	case RES_ADVANCE:
	case RES_BLOCKIP:
	case RES_COMMENT:
	case RES_DEFNAME:
	case RES_MOONGATES:
	case RES_NOTOTITLES:
	case RES_OBSCENE:
	case RES_RESOURCES:
	case RES_RUNES:
	case RES_SERVERS:
	case RES_SPHERE:
	case RES_STARTS:
	case RES_TELEPORTERS:
	case RES_TYPEDEFS:
	case RES_WORLDVARS:
		// Single instance stuff. (fully read in)
		// Ignore any resource name.
		return( RESOURCE_ID( restype ));
	case RES_FUNCTION:		// Define a new command verb script that applies to a char.
	case RES_HELP:
		// Private name range.
	case RES_ACCOUNT:
	case RES_GMPAGE:
	case RES_SERVER:
	case RES_SECTOR:
		// These must have a resource name but do not use true RESOURCE_ID format.
		// These are multiple instance but name is not a RESOURCE_ID
		if ( pszName[0] == '\0' )
			return( ridinvalid );	// invalid
		return( RESOURCE_ID( restype ));
	// Extra args are allowed.
	case RES_BOOK:	// BOOK BookName page
	case RES_DIALOG:	// DIALOG GumpName ./TEXT/BUTTON
	case RES_REGIONTYPE:
		{
			if ( pszName[0] == '\0' )
				return( ridinvalid );
			TCHAR * pArg1 = Str_GetTemp();
			strcpy( pArg1, pszName );
			pszName = pArg1;
			TCHAR * pArg2;
			Str_Parse( pArg1, &pArg2 );
			if ( ! strcmpi( pArg2, "TEXT" ))
				iPage = RES_DIALOG_TEXT;
			else if ( ! strcmpi( pArg2, "BUTTON" ))
				iPage = RES_DIALOG_BUTTON;
			else
				iPage = RES_GET_INDEX( Exp_GetVal( pArg2 ));
			if ( iPage > 255 )
			{
				DEBUG_ERR(( "Bad resource index page %d" DEBUG_CR, iPage ));
			}
		}
		break;
	case RES_NEWBIE:	// MALE_DEFAULT, FEMALE_DEFAULT, Skill
		if ( ! strcmpi( pszName, "MALE_DEFAULT" ))
			return( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_MALE_DEFAULT ));
		if ( ! strcmpi( pszName, "FEMALE_DEFAULT" ))
			return( RESOURCE_ID( RES_NEWBIE, RES_NEWBIE_FEMALE_DEFAULT ));
		break;
	case RES_AREA:
	case RES_ROOM:
		if ( !fNewStyleDef )
		{
			// Name is not the defname or id, just find a free id.
			pszName = NULL;	// fake it out for now.
			break;
		}
		// otherwise, passthrough to default
	default:
		// The name is a DEFNAME or id number
		ASSERT( restype < RES_QTY );
		break;
	}


	int index;
	if ( pszName )
	{
		if ( pszName[0] == '\0' )	// absense of resourceid = index 0
		{
			// This might be ok.
			return( RESOURCE_ID( restype, 0, iPage ) );
		}
		if ( isdigit(pszName[0]))	// Its just an index.
		{
			index = Exp_GetVal(pszName);
			rid = RESOURCE_ID( restype, index );
			switch ( restype )
			{
			case RES_BOOK:			// A book or a page from a book.
			case RES_DIALOG:			// A scriptable gump dialog: text or handler block.
			case RES_REGIONTYPE:	// Triggers etc. that can be assinged to a RES_AREA
				rid = RESOURCE_ID( restype, index, iPage );
				break;
			case RES_SKILLMENU:	
			case RES_MENU:			// General scriptable menus.
			case RES_EMAILMSG:		// define an email msg that could be sent to an account.
			case RES_EVENTS:		// An Event handler block with the trigger type in it. ON=@Death etc.
			case RES_SPEECH:		// A speech block with ON=*blah* in it.
			case RES_NAMES:			// A block of possible names for a NPC type. (read as needed)
			case RES_SCROLL:		// SCROLL_GUEST=message scroll sent to player at guest login. SCROLL_MOTD: SCROLL_NEWBIE
			case RES_TIP:			// Tips (similar to RES_SCROLL) that can come up at startup.
			case RES_TYPEDEF:			// Define a trigger block for a RES_WORLDITEM m_type.
			case RES_CHARDEF:		// Define a char type.
			case RES_ITEMDEF:		// Define an item type
			case RES_TEMPLATE:		// Define lists of items. (for filling loot etc)
				break;
			default:
				return( rid );
			}
#ifdef _DEBUG
			if ( g_Serv.m_iModeCode != SERVMODE_ResyncLoad )	// this really is ok.
			{
				// Warn of  duplicates.
				index = m_ResHash.FindKey( rid );
				if ( index >= 0 )	// i found it. So i have to find something else.
				{
					CResourceDef * pDef = m_ResHash.GetAt( rid, index );
					ASSERT(pDef);
					// KELL
					// DEBUG_WARN(( "redefinition of '%s' block %d ('%s')" DEBUG_CR, (LPCTSTR) GetResourceBlockName(restype), index, (LPCTSTR) pDef->GetName() ));
				}
			}
#endif
			return( rid );
		}


		CVarDefBase * pVarBase = g_Exp.m_VarDefs.GetKey( pszName );
		if ( pVarBase )
		{
			// An existing VarDef with the same name ?
			// We are creating a new Block but using an old name ? weird.
			// just check to see if this is a strange type conflict ?
			CVarDefNum * pVarNum = dynamic_cast <CVarDefNum*>( pVarBase );
			if ( pVarNum == NULL )
			{
				DEBUG_ERR(( "Re-Using name '%s' to define block" DEBUG_CR, (LPCTSTR) pszName ));
				return( ridinvalid );
			}
			rid.SetPrivateUID( pVarNum->GetValNum());
			if ( restype != rid.GetResType())
			{
				switch ( restype )
				{
				case RES_WC:
				case RES_WI:
				case RES_WORLDCHAR:
				case RES_WORLDITEM:
				case RES_NEWBIE:
				case RES_PLEVEL:
					// These are not truly defining a new DEFNAME
					break;
				default:
					DEBUG_ERR(( "Redefined name '%s' from %s to %s" DEBUG_CR, (LPCTSTR) pszName, (LPCTSTR) GetResourceBlockName(rid.GetResType()), (LPCTSTR) GetResourceBlockName(restype) ));
					return( ridinvalid );
				}
			}
			else if ( fNewStyleDef && pVarNum->GetValNum() != rid.GetPrivateUID() )
			{
				DEBUG_ERR(( "WARNING: region redefines DEFNAME '%s' for another region!" DEBUG_CR, pszName ));
			}
			else if ( iPage == rid.GetResPage())	// Books and dialogs have pages.
			{
				// We are redefining an item we have already read in ?
				// Why do this unless it's a Resync ?
				if ( g_Serv.m_iModeCode != SERVMODE_ResyncLoad )
				{
					DEBUG_WARN(( "Redef resource '%s'" DEBUG_CR, (LPCTSTR) pszName ));
				}
			}
			rid = RESOURCE_ID( restype, rid.GetResIndex(), iPage );
			*ppVarNum = pVarNum;
			return( rid );
		}
	}

	// we must define this as a new unique entry.
	// Find a new free entry.

	int iHashRange = 0;
	switch ( restype )
	{

	// Some cannot create new id's
	// Do not allow NEW named indexs for some types for now.

	case RES_SKILL:			// Define attributes for a skill (how fast it raises etc)
		// rid = m_SkillDefs.GetCount();
	case RES_SPELL:			// Define a magic spell. (0-64 are reserved)
		// rid = m_SpellDefs.GetCount();
		return( ridinvalid );

	// These MUST exist !

	case RES_NEWBIE:	// MALE_DEFAULT, FEMALE_DEFAULT, Skill
		return( ridinvalid );
	case RES_PLEVEL:	// 0-7
		return( ridinvalid );
	case RES_WC:
	case RES_WI:
	case RES_WORLDCHAR:
	case RES_WORLDITEM:
		return( ridinvalid );

	// Just find a free entry in proper range.

	case RES_CHARDEF:		// Define a char type.
		iHashRange = 2000;
		index = NPCID_SCRIPT2 + 0x2000;	// add another offset to avoid Sphere ranges.
		break;
	case RES_ITEMDEF:		// Define an item type
		iHashRange = 2000;
		index = ITEMID_SCRIPT2 + 0x4000;	// add another offset to avoid Sphere ranges.
		break;
	case RES_TEMPLATE:		// Define lists of items. (for filling loot etc)
		iHashRange = 2000;
		index = ITEMID_TEMPLATE + 100000;
		break;

	case RES_BOOK:			// A book or a page from a book.
	case RES_DIALOG:			// A scriptable gump dialog: text or handler block.
		if ( iPage )	// We MUST define the main section FIRST !
			return( ridinvalid );

	case RES_REGIONTYPE:	// Triggers etc. that can be assinged to a RES_AREA
		iHashRange = 100;
		index = 1000;
		break;

	case RES_AREA:
		iHashRange = 1000;
		index = 10000;
		break;
	case RES_ROOM:
	case RES_SKILLMENU:
	case RES_MENU:			// General scriptable menus.
	case RES_EMAILMSG:		// define an email msg that could be sent to an account.
	case RES_EVENTS:			// An Event handler block with the trigger type in it. ON=@Death etc.
	case RES_SPEECH:			// A speech block with ON=*blah* in it.
	case RES_NAMES:			// A block of possible names for a NPC type. (read as needed)
	case RES_SCROLL:		// SCROLL_GUEST=message scroll sent to player at guest login. SCROLL_MOTD: SCROLL_NEWBIE
	case RES_TIP:			// Tips (similar to RES_SCROLL) that can come up at startup.
	case RES_TYPEDEF:			// Define a trigger block for a RES_WORLDITEM m_type.
	case RES_SKILLCLASS:		// Define specifics for a char with this skill class. (ex. skill caps)
	case RES_REGIONRESOURCE:
		iHashRange = 1000;
		index = 10000;
		break;
	case RES_SPAWN:			// Define a list of NPC's and how often they may spawn.
		iHashRange = 1000;
		index = SPAWNTYPE_START + 100000;
		break;
	case RES_WEBPAGE:		// Define a web page template.
		index = m_WebPages.GetCount() + 1;
		break;

	default:
		ASSERT(0);
		return( ridinvalid );
	}

	if ( iPage )
	{
		rid = RESOURCE_ID( restype, index, iPage );
	}
	else
	{
		rid = RESOURCE_ID( restype, index );
	}

	if ( iHashRange )
	{
		// find a new FREE entry starting here
		rid.SetPrivateUID( rid.GetPrivateUID() + Calc_GetRandVal( iHashRange ));
		while(true)
		{
			if ( m_ResHash.FindKey( rid ) < 0 )
				break;
			rid.SetPrivateUID( rid.GetPrivateUID()+1 );
		}
	}
	else
	{
		// find a new FREE entry starting here
		if ( ! index )
		{
			rid.SetPrivateUID( rid.GetPrivateUID()+1 );
		}
	}

	if ( pszName )
	{
		int iVarNum = g_Exp.m_VarDefs.SetNum( pszName, rid.GetPrivateUID() );
		if ( iVarNum >= 0 )
		{
			*ppVarNum = dynamic_cast <CVarDefNum*>( g_Exp.m_VarDefs.GetAt(iVarNum));
		}
	}

	return( rid );
}

CResourceDef * CResource::ResourceGetDef( RESOURCE_ID_BASE rid ) const
{
	// Get a CResourceDef from the RESOURCE_ID.
	// ARGS:
	//	restype = id must be this type.

	if ( ! rid.IsValidUID())
		return( NULL );

	int index = rid.GetResIndex();
	switch ( rid.GetResType() )
	{
	case RES_WEBPAGE:
		index = m_WebPages.FindKey( rid );
		if ( index < 0 )
			return( NULL );
		return( m_WebPages.GetAt( index ));

	case RES_SKILL:
		if ( ! m_SkillIndexDefs.IsValidIndex(index))
			return( NULL );
		return( const_cast <CSkillDef *>( m_SkillIndexDefs[ index ] ));

	case RES_SPELL:
		if ( ! m_SpellDefs.IsValidIndex(index))
			return( NULL );
		return( const_cast <CSpellDef *>( m_SpellDefs[ index ] ));

	case RES_SERVER:
	case RES_UNKNOWN:	// legal to use this as a ref but it is unknown
		return( NULL );

	case RES_BOOK:			// A book or a page from a book.
	case RES_CRYSTALBALL:
	case RES_EMAILMSG:		// define an email msg that could be sent to an account.
	case RES_EVENTS:
	case RES_DIALOG:			// A scriptable gump dialog: text or handler block.
	case RES_MENU:
	case RES_NAMES:			// A block of possible names for a NPC type. (read as needed)
	case RES_NEWBIE:	// MALE_DEFAULT, FEMALE_DEFAULT, Skill
	case RES_REGIONRESOURCE:
	case RES_REGIONTYPE:		// Triggers etc. that can be assinged to a RES_AREA
	case RES_SCROLL:		// SCROLL_GUEST=message scroll sent to player at guest login. SCROLL_MOTD: SCROLL_NEWBIE
	case RES_SPEECH:
	case RES_TIP:			// Tips (similar to RES_SCROLL) that can come up at startup.
	case RES_TYPEDEF:			// Define a trigger block for a RES_WORLDITEM m_type.
	case RES_TEMPLATE:
	case RES_SKILLMENU:
	case RES_ITEMDEF:
	case RES_CHARDEF:
	case RES_SPAWN:	// the char spawn tables
	case RES_SKILLCLASS:
	case RES_AREA:
	case RES_ROOM:
		break;

	default:
		DEBUG_CHECK(0);
		return( NULL );
	}

	return CResourceBase::ResourceGetDef( rid );
}

////////////////////////////////////////////////////////////////////////////////

void CResource::OnTick( bool fNow )
{
	// Give a tick to the less critical stuff.

	if ( ! fNow )
	{
		if ( g_Serv.IsLoading())
			return;
		if ( m_timePeriodic > CServTime::GetCurrentTime())
			return;
	}

	// Do periodic resource stuff.

	for ( int i=0; i<m_WebPages.GetCount(); i++ )
	{
		if ( m_WebPages[i] == NULL )
			continue;
		CWebPageDef * pWeb = STATIC_CAST <CWebPageDef *>(m_WebPages[i]);
		ASSERT(pWeb);
		pWeb->WebPageUpdate( fNow, NULL, &g_Serv );
		pWeb->WebPageLog();
	}

	// Check to see if the resource files have not been acccessed in a while.
	for ( int k=0; true; k++ )
	{
		CResourceScript * pResFile = GetResourceFile(k);
		if ( pResFile == NULL )
			break;
		pResFile->CheckCloseUnused(fNow);
	}
	m_scpIni.CheckCloseUnused(fNow);
	m_scpTables.CheckCloseUnused(fNow);

	m_timePeriodic = CServTime::GetCurrentTime() + ( 60 * TICK_PER_SEC );
}

#define catresname(a,b)	\
{	\
	if ( *(a) ) strcat(a, " + "); \
	strcat(a, b); \
}

void CResource::PrintEFOFFlags(bool bEF, bool bOF, CTextConsole *pSrc)
{
	if ( g_Serv.IsLoading() ) return;
	if ( bOF )
	{
		char	zOptionFlags[512];
		zOptionFlags[0] = 0;

		if ( IsSetOF(OF_Magic_IgnoreAR) ) catresname(zOptionFlags, "OptionFlags");
		if ( IsSetOF(OF_Magic_CanHarmSelf) ) catresname(zOptionFlags, "CanHarmSelf");
		if ( IsSetOF(OF_Magic_StackStats) ) catresname(zOptionFlags, "StackStats");
		if ( IsSetOF(OF_Archery_CanMove) ) catresname(zOptionFlags, "ArcheryCanMove");
		if ( IsSetOF(OF_Magic_PreCast) ) catresname(zOptionFlags, "MageryPreCast");
		if ( IsSetOF(OF_Items_AutoName) ) catresname(zOptionFlags, "ItemsAutoName");
		if ( IsSetOF(OF_FileCommands) ) catresname(zOptionFlags, "FileCommands");
		if ( IsSetOF(OF_NoItemNaming) ) catresname(zOptionFlags, "NoItemNaming");
		if ( IsSetOF(OF_NoHouseMuteSpeech) ) catresname(zOptionFlags, "NoHouseMuteSpeech");
		if ( IsSetOF(OF_Multithreaded) ) catresname(zOptionFlags, "Multithreaded");
		if ( IsSetOF(OF_Advanced_LOS) ) catresname(zOptionFlags, "AdvancedLOS");

		if ( pSrc ) pSrc->SysMessagef("Option flags: %s" DEBUG_CR, zOptionFlags);
		else g_pLog->Event(LOGM_INIT, "Option flags: %s" DEBUG_CR, zOptionFlags);
	}
	if ( bEF )
	{
		char	zExperimentalFlags[512];
		zExperimentalFlags[0] = 0;

		if ( IsSetEF(EF_DiagonalWalkCheck) ) catresname(zExperimentalFlags, "DiagonalWalkCheck");
		if ( IsSetEF(EF_UNICODE) ) catresname(zExperimentalFlags, "Unicode");
		if ( IsSetEF(EF_Scripts_Ret_Strings) ) catresname(zExperimentalFlags, "ScriptsReturnStrings");
		if ( IsSetEF(EF_New_Triggers) ) catresname(zExperimentalFlags, "NewTriggersEnable");
		if ( IsSetEF(EF_Scripts_Parse_Verbs) ) catresname(zExperimentalFlags, "ScriptsParseVerbs");
		if ( IsSetEF(EF_Intrinsic_Locals) ) catresname(zExperimentalFlags, "IntrinsicLocals");
		if ( IsSetEF(EF_Item_Strict_Comparison) ) catresname(zExperimentalFlags, "ItemStrictComparison");
		if ( IsSetEF(EF_No_Pause_Packet) ) catresname(zExperimentalFlags, "NoPausePacket");
		if ( IsSetEF(EF_WalkCheck) ) catresname(zExperimentalFlags, "WalkCheck");
		// if ( IsSetEF(EF_AgeOfShadows) ) catresname(zExperimentalFlags, "AgeOfShadows");
		if ( IsSetEF(EF_Script_Profiler) ) catresname(zExperimentalFlags, "ScriptProfiler");
		if ( IsSetEF(EF_Size_Optimise) ) catresname(zExperimentalFlags, "SizeOptimize");
		if ( IsSetEF(EF_Minimize_Triggers) ) catresname(zExperimentalFlags, "MinimizeTriggers");

		if ( pSrc ) pSrc->SysMessagef("Experimental flags: %s" DEBUG_CR, zExperimentalFlags);
		else g_pLog->Event(LOGM_INIT, "Experimental flags: %s" DEBUG_CR, zExperimentalFlags);
	}
}

bool CResource::LoadIni( bool fTest )
{
	// Load my INI file first.
	if ( ! OpenResourceFind( m_scpIni, GRAY_FILE ".ini", !fTest )) // Open script file
	{
		if ( ! fTest )
		{
			g_Log.Event( LOGL_FATAL|LOGM_INIT, "Can't open " GRAY_FILE ".ini" DEBUG_CR );
		}
		return( false );
	}

	LoadResourcesOpen(&m_scpIni);
	m_scpIni.Close();
	m_scpIni.CloseForce();

	return true;
}

bool CResource::SaveIni()
{
 	// Save out stuff in the INI file.
	// Delete the previous [SPHERE] section.
	m_scpIni.WriteProfileStringSec( GetResourceBlockName(RES_SPHERE), NULL, NULL );

	// Now write a new one at the end of the file.
	if ( ! m_scpIni.Open( NULL, OF_WRITE|OF_READWRITE|OF_TEXT )) // Open script file for append write
	{
		return( false );
	}

	g_Serv.r_Write( m_scpIni );

	m_scpIni.Close();
	m_scpIni.CloseForce();

	// Resync this file (in case there are linked things in it.
	m_scpIni.ReSync();

	return true;
}

void CResource::Unload( bool fResync )
{
	if ( fResync )
	{
		// Unlock all the SCP and MUL files.
		g_Install.CloseFiles();
		for ( int j=0; true; j++ )
		{
			CResourceScript * pResFile = GetResourceFile(j);
			if ( pResFile == NULL )
				break;
			pResFile->CloseForce();
		}
		m_scpIni.CloseForce();
		m_scpTables.CloseForce();
		return;
	}

	m_ResourceFiles.RemoveAll();

	// m_ResHash.RemoveAll();

	// m_LogIP
	m_Obscene.RemoveAll();
	m_NotoTitles.RemoveAll();
	m_Runes.RemoveAll();	// Words of power. (A-Z)
	// m_MultiDefs
	m_SkillNameDefs.RemoveAll();	// Defined Skills
	m_SkillIndexDefs.RemoveAll();
	// m_Servers
	m_Functions.RemoveAll();
	m_HelpDefs.RemoveAll();
	m_StartDefs.RemoveAll(); // Start points list
	// m_StatAdv
	for ( int j=0; j<PLEVEL_QTY; j++ )
	{
		m_PrivCommands[j].RemoveAll();
	}
	m_MoonGates.Empty();
	// m_WebPages
	m_SpellDefs.RemoveAll();	// Defined Spells
}

bool CResource::Load( bool fResync )
{
	// ARGS: 
	//  fResync = just look for changes.

	if ( ! fResync )
	{
		g_Install.FindInstall();
	}
	else
	{
		m_scpIni.ReSync();
		m_scpIni.CloseForce();
	}

	// Open the MUL files I need.
	VERFILE_TYPE i = g_Install.OpenFiles(
		(1<<VERFILE_MAP)|
		(1<<VERFILE_STAIDX)|
		(1<<VERFILE_STATICS)|
		(1<<VERFILE_TILEDATA)|
		(1<<VERFILE_MULTIIDX)|
		(1<<VERFILE_MULTI)|
		(1<<VERFILE_VERDATA)
		);
	if ( i != VERFILE_QTY )
	{
      g_Log.Event( LOGL_FATAL|LOGM_INIT, "The " GRAY_FILE ".INI file is corrupt or missing" DEBUG_CR );
		g_Log.Event( LOGL_FATAL|LOGM_INIT, "MUL File '%s' not found..." DEBUG_CR, (LPCTSTR) g_Install.GetBaseFileName(i));
		return( false );
	}

	// Load the optional verdata cache. (modifies MUL stuff)
	try
	{
		g_VerData.Load( g_Install.m_File[VERFILE_VERDATA] );
	}
	catch ( CGrayError &e )
	{
      g_Log.Event( LOGL_FATAL|LOGM_INIT, "The " GRAY_FILE ".INI file is corrupt or missing" DEBUG_CR );
		g_Log.CatchEvent( &e, "g_VerData.Load" );
		return( false );
	}
	catch(...)
	{
      g_Log.Event( LOGL_FATAL|LOGM_INIT, "The " GRAY_FILE ".INI file is corrupt or missing" DEBUG_CR );
		g_Log.CatchEvent( NULL, "g_VerData.Load" );
		return( false );
	}

	// Now load the *TABLES.SCP file.
	if ( ! fResync )
	{
		if ( ! OpenResourceFind( m_scpTables, GRAY_FILE "tables" ))
		{
         		g_Log.Event( LOGL_FATAL|LOGM_INIT, "The " GRAY_FILE ".INI file is corrupt or missing" DEBUG_CR );
			g_Log.Event( LOGL_FATAL|LOGM_INIT, "Error opening table definitions file..." DEBUG_CR );
			return false;
		}

		LoadResourcesOpen(&m_scpTables);
		m_scpTables.Close();
	}
	else
	{
		m_scpTables.ReSync();
	}
	m_scpTables.CloseForce();

	//	Initialize the world sectors
	g_World.Init();

	// open and index all my script files i'm going to use.
	AddResourceDir( m_sSCPBaseDir );		// if we want to get *.SCP files from elsewhere.

	g_Log.Event( LOGM_INIT, "Indexing %d scripts..." DEBUG_CR, m_ResourceFiles.GetCount());

	for ( int j=0; true; j++ )
	{
		CResourceScript * pResFile = GetResourceFile(j);
		if ( pResFile == NULL )
			break;
		if ( ! fResync )
		{
			LoadResources( pResFile );
		}
		else
		{
			pResFile->ReSync();
		}
		g_Serv.PrintPercent( j+1, m_ResourceFiles.GetCount());
	}

	// Make sure we have the basics.

	//ASSERT( m_ResourceLinks.TestSort());

	if ( g_Serv.GetName()[0] == '\0' )	// make sure we have a set name
	{
		TCHAR szName[ MAX_SERVER_NAME_SIZE ];
		int iRet = gethostname( szName, sizeof( szName ));
		g_Serv.SetName(( ! iRet && szName[0] ) ? szName : (LPCTSTR)g_Cfg.m_sVerName.GetPtr() );
	}

	if ( ! g_Cfg.ResourceGetDef( RESOURCE_ID( RES_SKILLCLASS, 0 )))
	{
		// must have at least 1 skill class.
		CSkillClassDef * pSkillClass = new CSkillClassDef( RESOURCE_ID( RES_SKILLCLASS ));
		ASSERT(pSkillClass);
		m_ResHash.AddSortKey( RESOURCE_ID( RES_SKILLCLASS, 0 ), pSkillClass );
	}
	if ( ! m_StartDefs.GetCount())	// must have 1 start location
	{
		CStartLoc * pStart = new CStartLoc((LPCTSTR)g_Cfg.m_sVerName.GetPtr());
		ASSERT(pStart);
		pStart->m_sName = "The Throne Room";
		pStart->m_pt = g_pntLBThrone;
		m_StartDefs.Add( pStart );
	}

	g_Serv.SysMessage( "Done loading scripts." DEBUG_CR );

	
	// Make region DEFNAMEs
	{
		int			iMax	= g_Cfg.m_RegionDefs.GetCount();
		for ( int i = 0; i < iMax; i++ )
		{
			CRegionBase * pRegion = dynamic_cast <CRegionBase*> (g_Cfg.m_RegionDefs.GetAt(i));
			if ( !pRegion )
				continue;
			pRegion->MakeRegionName();
		}
	}
	if ( ! m_sEventsPet.IsEmpty() )
	{
		m_pEventsPetLink = dynamic_cast<CResourceLink *>( g_Cfg.ResourceGetDefByName( RES_EVENTS, m_sEventsPet ) );

		if ( m_pEventsPetLink == NULL )
			g_Log.Event( LOGM_INIT|LOGL_ERROR, "Can't find definition for '%s' (EVENTSPET)" DEBUG_CR, (LPCTSTR) m_sEventsPet );
	}

	LoadSortSpells();
	g_Serv.SysMessage( DEBUG_CR );
	return true;
}


LPCTSTR CResource::GetDefaultMsg( LPCTSTR pszKey )
{
	DEBUG_ERR(("Using not a fast method to lookup a string for '%s'" DEBUG_CR, pszKey));

	long l;
	for ( l = 0; l < DEFMSG_QTY; l++ )	//	search the key using the mask
	{
		if ( !strcmpi((const char *)g_Exp.sm_szMsgNames[0][l], pszKey) ) return GetDefaultMsg(l);
	}
	g_Log.Event(LOGM_INIT|LOGL_ERROR, "Using of not defined default message '%s'" DEBUG_CR, (LPCTSTR)pszKey);
	g_Exp.m_sTmp.Format("Message string '%s' unknown.", pszKey);
	return g_Exp.m_sTmp.GetPtr();
}

LPCTSTR CResource::GetDefaultMsg(long lKeyNum)
{
	if (( lKeyNum < 0 ) || ( lKeyNum > DEFMSG_QTY ))
	{
		g_Exp.m_sTmp.Format("Invalid message string %l specifyed.", lKeyNum);
		return g_Exp.m_sTmp.GetPtr();
	}
	return(g_Exp.sm_szMessages[lKeyNum]);
}

int		CItemTypeDef::GetItemType()
{
	return GETINTRESOURCE( GetResourceID() );
}


bool	CItemTypeDef::r_LoadVal( CScript & s )
{
	LPCTSTR		pszKey	= s.GetKey();
	LPCTSTR		pszArgs	= s.GetArgStr();
	LOCKDATA;
	EXC_TRY(("r_LoadVal('%s %s')", pszKey, pszArgs));

	if ( !strnicmp( pszKey, "TERRAIN", 7 ) )
	{
		int		iLo;
		int		iHi;
		iLo	= Exp_GetVal( pszArgs );
		GETNONWHITESPACE( pszArgs );
		if ( *pszArgs == ',' )
		{
			pszArgs++;
			GETNONWHITESPACE( pszArgs );
		}
		if ( !*pszArgs )
			iHi	= iLo;
		else
			iHi	= Exp_GetVal( pszArgs );

		if ( iLo > iHi )		// swap
		{
			int	iTmp	= iHi;
			iHi	= iLo;
			iLo	= iTmp;
		}

		for ( int i = iLo; i <= iHi; i++ )
		{
			g_World.m_TileTypes.SetAtGrow( i, this );
		}
		return true;
	}
	EXC_CATCH("CItemTypeDef");
	return false;
}

bool CResource::ResourceTestItemMuls()
{
	// Xref the RES_ITEMDEF blocks with the items database to make sure we have defined all.
	int i = 0;
	for ( ; i < ITEMID_MULTI; i++ )
	{
		if ( !( i%0x1ff ))
		{
			g_Serv.PrintPercent( i, ITEMID_MULTI );
		}

		CUOItemTypeRec item;
		if ( ! CItemBase::GetItemData((ITEMID_TYPE) i, &item ))
			continue;

		CItemBase * pItemDef = CItemBase::FindItemBase((ITEMID_TYPE) i );
		if ( pItemDef == NULL )
		{
			// Seems we have a new item on our hands.
			// Write it to the file !
			g_Log.Event( LOGL_EVENT, "[%04x] // %s //New" DEBUG_CR, i, (LPCTSTR) item.m_name );
			continue;
		}

	}
	return( true );
}

bool CResource::ResourceTestCharAnims()
{
	// xref the ANIM= lines in RES_CHARDEF with ANIM.IDX

	if ( ! g_Install.OpenFile( VERFILE_ANIMIDX ))
	{
		return( false );
	}

	TCHAR *pszTemp = Str_GetTemp();
	for ( int id = 0; id < CREID_QTY; id ++ )
	{
		if ( id >= CREID_MAN )
		{
			// must already have an entry in the RES_CHARDEF. (don't get the clothing etc)

			continue;
		}

		// Does it have an entry in "ANIM.IDX" ?

		int index;
		int animqty;
		if ( id < CREID_HORSE1 )
		{
			// Monsters and high detail critters.
			// 22 actions per char.
			index = ( id * ANIM_QTY_MON * 5 );
			animqty = ANIM_QTY_MON;
		}
		else if ( id < CREID_MAN )
		{
			// Animals and low detail critters.
			// 13 actions per char.
			index = 22000 + (( id - CREID_HORSE1 ) * ANIM_QTY_ANI * 5 );
			animqty = ANIM_QTY_ANI;
		}
		else
		{
			// Human and equip
			// 35 actions per char.
			index = 35000 + (( id - CREID_MAN ) * ANIM_QTY_MAN * 5 );
			animqty = ANIM_QTY_MAN;
		}

		// It MUST have a walk entry !

		DWORD dwAnim = 0;	// mask of valid anims.
		for ( int anim = 0; anim < animqty; anim ++, index += 5 )
		{
			CUOIndexRec Index;
			if ( ! g_Install.ReadMulIndex( VERFILE_ANIMIDX, VERFILE_ANIM, index, Index ))
			{
				if ( anim == 0 ) break;	// skip this.
				continue;
			}

			dwAnim |= ( 1L<<anim );
		}

		if ( dwAnim )
		{
			// report the valid animations.
			sprintf(pszTemp, "0%x", dwAnim);

			CCharBase * pCharDef = CCharBase::FindCharBase((CREID_TYPE) id );
			if ( pCharDef == NULL )
			{
				continue;
			}

			CResourceScript * pResFile = pCharDef->GetLinkFile();
			ASSERT(pResFile);

			pResFile->CloseForce();
			pResFile->WriteProfileStringOffset(pCharDef->GetLinkOffset(), "ANIM", pszTemp);
			pResFile->CloseForce();
		}
	}
	return( true );
}
