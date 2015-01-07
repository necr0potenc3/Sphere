//
// CResource.h
//

#ifndef _INC_CRESOURCE_H
#define _INC_CRESOURCE_H
#pragma once

#include "../common/CAssoc.h"

class CAccount;
class CClient;
class CLogIP;
class CServerDef;

typedef CServerDef * CServerRef; // CThreadLockRef

#define MAX_SKILL	(g_Cfg.m_iMaxSkill)

// option flags
enum OF_TYPE
{
	OF_Magic_IgnoreAR			= 0x0000001,
	OF_Magic_CanHarmSelf		= 0x0000002,
	OF_Magic_StackStats			= 0x0000004,
//	OF_Advanced_AI				= 0x0000008,
	OF_Archery_CanMove			= 0x0000010,
	OF_Magic_PreCast			= 0x0000020,
	OF_Items_AutoName			= 0x0000040,
	OF_FileCommands				= 0x0000080,
	OF_NoItemNaming				= 0x0000100,
	OF_NoHouseMuteSpeech		= 0x0000200,
	OF_Multithreaded			= 0x0000400,	// do not set while server is running!
	OF_Advanced_LOS				= 0x0000800,
	OF_Specific					= 0x1000000
};

enum EF_TYPE
{
	EF_DiagonalWalkCheck		= 0x0000001,
	EF_UNICODE					= 0x0000002,
	EF_Scripts_Ret_Strings		= 0x0000004,
	EF_New_Triggers				= 0x0000008,
	EF_Scripts_Parse_Verbs		= 0x0000010,
	EF_Intrinsic_Locals			= 0x0000020,
	EF_Item_Strict_Comparison	= 0x0000040,
	EF_No_Pause_Packet			= 0x0000080,
	EF_WalkCheck				= 0x0000100,
	EF_Unused0200				= 0x0000200, // EF_AgeOfShadows
	EF_Script_Profiler			= 0x0000400,
	EF_Size_Optimise			= 0x0000800,
	EF_Minimize_Triggers		= 0x0001000,
	EF_Specific					= 0x1000000,	// Specific behaviour, not completly tested
};


enum BODYPART_TYPE
{
	ARMOR_HEAD = 0,
	ARMOR_NECK,
	ARMOR_BACK,
	ARMOR_CHEST,	// or thorax
	ARMOR_ARMS,
	ARMOR_HANDS,
	ARMOR_LEGS,
	ARMOR_FEET,
	ARMOR_QTY,		// All the parts that armor will cover.

	BODYPART_LEGS2,	// Alternate set of legs (spider)
	BODYPART_TAIL,	// Dragon, Snake, Alligator, etc. (tail attack?)
	BODYPART_WINGS,	// Dragon, Mongbat, Gargoyle
	BODYPART_CLAWS,	// can't wear any gloves here!
	BODYPART_HOOVES,	// No shoes
	BODYPART_HORNS,	// Bull, Daemon

	BODYPART_STALKS,		// Gazer or Corpser
	BODYPART_BRANCHES,	// Reaper.
	BODYPART_TRUNK,		// Reaper.
	BODYPART_PSEUDOPOD,	// Slime
	BODYPART_ABDOMEN,		// Spider or insect. asusme throax and chest are the same.

	BODYPART_QTY,
};

#define DAMAGE_GOD			0x0001	// Nothing can block this.
#define DAMAGE_HIT_BLUNT	0x0002	// Physical hit of some sort.
#define DAMAGE_MAGIC		0x0004	// Magic blast of some sort. (we can be immune to magic to some extent)
#define DAMAGE_POISON		0x0008	// Or biological of some sort ? (HARM spell)
#define DAMAGE_FIRE			0x0010	// Fire damage of course.  (Some creatures are immune to fire)
#define DAMAGE_ELECTRIC		0x0020	// lightning.
#define DAMAGE_DRAIN		0x0040	// level drain = negative energy.
#define DAMAGE_GENERAL		0x0080	// All over damage. As apposed to hitting just one point.
#define DAMAGE_ACID			0x0100	// Corrosive will destroy armor.
#define DAMAGE_COLD			0x0200	// Cold freezing damage
#define DAMAGE_HIT_SLASH	0x0400	// sword
#define DAMAGE_HIT_PIERCE	0x0800	// spear.

typedef WORD DAMAGE_TYPE;		// describe a type of damage.

///////////////////////////////////////

struct CValueRangeDef
{
	// Simple linearity
public:
	int m_iLo;
	int m_iHi;
public:
	void Init()
	{
		m_iLo = INT_MIN;
		m_iHi = INT_MIN;
	}
	int GetRange() const
	{
		return( m_iHi - m_iLo );
	}
	int GetLinear( int iPercent ) const
	{	
		// ARGS: iPercent = 0-1000
		return( m_iLo + IMULDIV( GetRange(), iPercent, 1000 ));
	}
	int GetRandom() const
	{	
		return( m_iLo + Calc_GetRandVal( GetRange()));
	}
	int GetRandomLinear( int iPercent ) const;
	bool Load( TCHAR * pszDef );
	const TCHAR * Write() const;
	CValueRangeDef()
	{
		Init();
	}
};

struct CValueCurveDef
{
	// Describe an arbitrary curve.
	// for a range from 0.0 to 100.0 (1000)
	// May be a list of probabilties from 0 skill to 100.0% skill.
public:
	CGTypedArray<int,int> m_aiValues;		// 0 to 100.0 skill levels
public:
	void Init()
	{
		m_aiValues.Empty();
	}
	bool Load( TCHAR * pszDef );
	const TCHAR * Write() const;
	int GetLinear( int iSkillPercent ) const;
	int GetChancePercent( int iSkillPercent ) const;
	int GetRandom() const;
	int GetRandomLinear( int iPercent ) const;
};

class CCharRefArray
{
private:
	// List of Players and NPC's involved in the quest/party/account etc..
	CGTypedArray< CGrayUID, CGrayUID> m_uidCharArray;
public:
	int FindChar( const CChar * pChar ) const;
	bool IsCharIn( const CChar * pChar ) const
	{
		return( FindChar( pChar ) >= 0 );
	}
	int AttachChar( const CChar * pChar );
	void DetachChar( int i );
	int DetachChar( const CChar * pChar );
	void DeleteChars();
	int GetCharCount() const
	{
		return( m_uidCharArray.GetCount());
	}
	CGrayUID GetChar( int i ) const
	{
		return( m_uidCharArray[i] );
	}
	void WritePartyChars( CScript & s );
};

class CRegionResourceDef : public CResourceLink
{
	// RES_REGIONRESOURCE
	// When mining/lumberjacking etc. What can we get?
protected:
	DECLARE_MEM_DYNAMIC;
public:
	static LPCTSTR const sm_szLoadKeys[];

	// What item do we get when we try to mine this.
	ITEMID_TYPE m_ReapItem;	// ITEMID_ORE_1 most likely
	CValueCurveDef m_ReapAmount;	// How much can we reap at one time (based on skill)

	CValueCurveDef m_Amount;		// How is here total
	CValueCurveDef m_Skill;			// Skill levels required to mine this.
	int m_iRegenerateTime;			// TICK_PER_SEC once found how long to regen this type.

public:
	CRegionResourceDef( RESOURCE_ID rid );
	virtual ~CRegionResourceDef();
	bool r_LoadVal( CScript & s );
	void r_DumpLoadKeys( CTextConsole * pSrc );
	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc = NULL );
	TRIGRET_TYPE OnTrigger( LPCTSTR pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs );
};

enum WEBPAGE_TYPE
{
	WEBPAGE_TEMPLATE,
	WEBPAGE_TEXT,
	WEBPAGE_BMP,
	WEBPAGE_GIF,
	WEBPAGE_JPG,
	WEBPAGE_QTY,
};

enum WTRIG_TYPE
{
	// XTRIG_UNKNOWN	= some named trigger not on this list.
	WTRIG_Load=1,
	WTRIG_QTY,
};

class CWebPageDef : public CResourceLink
{
	// RES_WEBPAGE

	// This is a single web page we are generating or serving.
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szPageType[];
	static LPCTSTR const sm_szPageExt[];
protected:
	DECLARE_MEM_DYNAMIC;
private:
	WEBPAGE_TYPE m_type;				// What basic format of file is this ? 0=text
	CGString m_sSrcFilePath;	// source template for the generated web page.
private:
	PLEVEL_TYPE m_privlevel;	// What priv level to see this page ?

	// For files that are being translated and updated.
	CGString m_sDstFilePath;	// where is the page served from ?
	int  m_iUpdatePeriod;	// How often to update the web page. 0 = never.
	int  m_iUpdateLog;		// create a daily log of the page.
	CServTime  m_timeNextUpdate;

public:
	static int sm_iListIndex;
	static LPCTSTR const sm_szTrigName[WTRIG_QTY+1];
private:
	int ServPageRequest( CClient * pClient, LPCTSTR pszURLArgs, CGTime * pdateLastMod );
public:
	LPCTSTR GetName() const
	{
		return( m_sSrcFilePath );
	}
	LPCTSTR GetDstName() const
	{
		return( m_sDstFilePath );
	}
	void SetPageType( WEBPAGE_TYPE iType )
	{
		m_type = iType;
	}
	bool IsMatch( LPCTSTR IsMatchPage ) const;

	bool SetSourceFile( LPCTSTR pszName, CClient * pClient );
	bool ServPagePost( CClient * pClient, LPCTSTR pszURLArgs, TCHAR * pPostData, int iContentLength );

	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc = NULL );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );	// some command on this object as a target

	void WebPageLog();
	bool WebPageUpdate( bool fNow, LPCTSTR pszDstName, CTextConsole * pSrc );

	static bool ServPage( CClient * pClient, TCHAR * pszPage, CGTime * pdateLastMod );

	CWebPageDef( RESOURCE_ID id );
	virtual ~CWebPageDef()
	{
	}
};

enum SPTRIG_TYPE
{
	SPTRIG_EFFECT	= 1,
	SPTRIG_FAIL,
	SPTRIG_SELECT,
	SPTRIG_START,
	SPTRIG_SUCCESS,
	SPTRIG_QTY,
};

class CSpellDef : public CResourceLink	// 1 based spells. See SPELL_*
{
	// RES_SPELL
protected:
	DECLARE_MEM_DYNAMIC;
private:
	DWORD	m_dwFlags;
	DWORD	m_dwGroup;

#define SPELLFLAG_DIR_ANIM  0x00001	// Evoke type cast or directed. (animation)
#define SPELLFLAG_TARG_ITEM 0x00002	// Need to target an object
#define SPELLFLAG_TARG_CHAR 0x00004	// Needs to target a living thing
#define SPELLFLAG_TARG_OBJ	(SPELLFLAG_TARG_ITEM|SPELLFLAG_TARG_CHAR)

#define SPELLFLAG_TARG_XYZ  0x00008	// Can just target a location.
#define SPELLFLAG_HARM		0x00010	// The spell is in some way harmfull.
#define SPELLFLAG_FX_BOLT	0x00020	// Effect is a bolt to the target.
#define SPELLFLAG_FX_TARG	0x00040	// Effect is at the target.
#define SPELLFLAG_FIELD		0x00080	// create a field of stuff. (fire,poison,wall)
#define SPELLFLAG_SUMMON	0x00100	// summon a creature.
#define SPELLFLAG_GOOD		0x00200	// The spell is a good spell. u intend to help to receiver.
#define SPELLFLAG_RESIST	0x00400	// Allowed to resist this.	
#define SPELLFLAG_TARG_NOSELF	0x00800
#define SPELLFLAG_DISABLED	0x08000
#define SPELLFLAG_SCRIPTED	0x10000
#define	SPELLFLAG_PLAYERONLY	0x20000	// casted by players only
	CGString m_sName;	// spell name

public:
	static LPCTSTR const sm_szTrigName[SPTRIG_QTY+1];
	static LPCTSTR const sm_szLoadKeys[];
	CGString m_sTargetPrompt;	// targetting prompt. (if needed)
	SOUND_TYPE m_sound;			// What noise does it make when done.
	CGString m_sRunes;			// Letter Runes for Words of power.
	CResourceQtyArray m_Reags;	// What reagents does it take ?
	CResourceQtyArray m_SkillReq;	// What skills/unused reagents does it need to cast.
	ITEMID_TYPE m_idSpell;		// The rune graphic for this.
	ITEMID_TYPE m_idScroll;		// The scroll graphic item for this.
	ITEMID_TYPE m_idEffect;		// Animation effect ID
	WORD m_wManaUse;			// How much mana does it need.

	CValueCurveDef	m_CastTime;		// In TICK_PER_SEC.
	CValueCurveDef	m_Effect;		// Damage or effect level based on skill of caster.100% magery
	CValueCurveDef	m_Duration;		// length of effect. in TICK_PER_SEC
	CValueCurveDef	m_Interrupt;	// chance to interrupt a spell
	
public:
	bool IsSpellType( DWORD wFlags ) const
	{
		return(( m_dwFlags & wFlags ) ? true : false );
	}
	CSpellDef( SPELL_TYPE id );
	virtual ~CSpellDef()
	{
	}
	LPCTSTR GetName() const { return( m_sName ); }
	bool r_LoadVal( CScript & s );
	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );

	
	bool GetPrimarySkill( int * iSkill = NULL, int * iQty = NULL ) const;
};

class CRandGroupDef	: public CResourceLink // A spawn group.
{
	// RES_SPAWN or RES_REGIONTYPE
protected:
	DECLARE_MEM_DYNAMIC;
private:
	static LPCTSTR const sm_szLoadKeys[];
	int m_iTotalWeight;
	CResourceQtyArray m_Members;
private:
	int CalcTotalWeight();
public:
	CRandGroupDef( RESOURCE_ID rid ) :
		CResourceLink( rid )
	{
		m_iTotalWeight = 0;
	}
	virtual ~CRandGroupDef()
	{
	}
	virtual bool r_LoadVal( CScript & s );
	int GetTotalWeight() const
	{
		return m_iTotalWeight;
	}
	int GetRandMemberIndex( CChar * pCharSrc = NULL ) const;
	CResourceQty GetMember( int i ) const
	{
		return( m_Members[i] );
	}
	RESOURCE_ID GetMemberID( int i ) const
	{
		return( m_Members[i].GetResourceID() );
	}
	int GetMemberWeight( int i ) const
	{
		return( m_Members[i].GetResQty() );
	}
};

enum STAT_TYPE	// Character stats
{
	STAT_NONE = -1,
	STAT_STR = 0,
	STAT_INT,
	STAT_DEX,
	STAT_BASE_QTY,
	STAT_FOOD = 3,	// just used as a regen rate. (as karma does not decay)

	// Notoriety.
	STAT_KARMA = 4,		// -10000 to 10000 - also used as the food consumption main timer.
	STAT_FAME,
	STAT_QTY,
};

class CSkillClassDef : public CResourceLink // For skill def table
{
	// Similar to character class.
	// RES_SKILLCLASS
	static LPCTSTR const sm_szLoadKeys[];
protected:
	DECLARE_MEM_DYNAMIC;
public:
	CGString m_sName;	// The name of this skill class.

	WORD m_StatSumMax;
	DWORD m_SkillSumMax;

	WORD m_StatMax[STAT_BASE_QTY];	// STAT_BASE_QTY
	WORD m_SkillLevelMax[ SKILL_QTY ];

private:
	void Init();
public:
	CSkillClassDef( RESOURCE_ID rid ) :
		CResourceLink( rid )
	{
		// If there was none defined in scripts.
		Init();
	}
	virtual ~CSkillClassDef()
	{
	}

	LPCTSTR GetName() const { return( m_sName ); }

	void r_DumpLoadKeys( CTextConsole * pSrc );
	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	bool r_LoadVal( CScript & s );
};

enum SKTRIG_TYPE
{
	// All skills may be scripted.
	// XTRIG_UNKNOWN	= some named trigger not on this list.
	SKTRIG_ABORT=1,	// Some odd thing went wrong.
	SKTRIG_FAIL,	// we failed the skill check.
	SKTRIG_GAIN,	
	SKTRIG_SELECT,	// just selecting params for the skill
	SKTRIG_START,	// params for skill are done. (stroke)
	SKTRIG_STROKE,	
	SKTRIG_SUCCESS,
	SKTRIG_QTY,
};


enum	SKF_TYPE
{
	SKF_SCRIPTED		= 0x0001,		// fully scripted, no hardcoded behaviour
	SKF_FIGHT			= 0x0002,		// considered a fight skill, mantains fight active
	SKF_MAGIC			= 0x0004,
	SKF_IMMOBILE		= 0x0010,
	SKF_SELECTABLE		= 0x0020
};

struct CSkillDef : public CResourceLink // For skill def table
{
	// RES_SKILL
	static LPCTSTR const sm_szTrigName[SKTRIG_QTY+1];
	static LPCTSTR const sm_szLoadKeys[];
protected:
	DECLARE_MEM_DYNAMIC;
private:
	CGString m_sKey;	// script key word for skill.
public:
	CGString m_sTitle;	// title one gets if it's your specialty.
	CGString m_sName;	// fancy skill name
	CGString m_sTargetPrompt;	// targetting prompt. (if needed)

	CValueCurveDef m_Delay;	//	The base delay for the skill. (tenth of seconds)
	CValueCurveDef m_Effect;	// depends on skill

	// Stat effects.
	// You will tend toward these stat vals if you use this skill a lot.
	BYTE m_Stat[STAT_BASE_QTY];	// STAT_STR, STAT_INT, STAT_DEX
	BYTE m_StatPercent; // BONUS_STATS = % of success depending on stats
	BYTE m_StatBonus[STAT_BASE_QTY]; // % of each stat toward success at skill, total 100

	CValueCurveDef	m_AdvRate;		// ADV_RATE defined "skill curve" 0 to 100 skill levels.
	CValueCurveDef	m_Values;	// VALUES= influence for items made with 0 to 100 skill levels.

	DWORD			m_dwFlags;
	DWORD			m_dwGroup;
	


	// Delay before skill complete. modified by skill level of course !
public:
	CSkillDef( SKILL_TYPE iSkill );
	virtual ~CSkillDef()
	{
	}
	LPCTSTR GetKey() const
	{
		return( m_sKey );
	}

	LPCTSTR GetName() const { return( GetKey()); }
	bool r_LoadVal( CScript & s );
	void r_DumpLoadKeys( CTextConsole * pSrc );
	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
};

class CSkillKeySortArray : public CGObSortArray< CValStr*, LPCTSTR >
{
	int CompareKey( LPCTSTR pszKey, CValStr * pVal, bool fNoSpaces ) const
	{
		ASSERT( pszKey );
		ASSERT( pVal->m_pszName );
		return( strcmpi( pszKey, pVal->m_pszName ));
	}
};

struct CMultiDefArray : public CGObSortArray< CGrayMulti*, MULTI_TYPE >
{
	// store the static components of a IT_MULTI
	// Sorted array
	int CompareKey( MULTI_TYPE id, CGrayMulti* pBase, bool fNoSpaces ) const
	{
		ASSERT( pBase );
		return( id - pBase->GetMultiID());
	}
};

struct CThreadLockableDefArray : public CObNameSortArray, public CThreadLockableObj
{
	bool DeleteOb( CScriptObj* pObj )
	{
		CThreadLockRef lock( this );
		return CObNameSortArray::DeleteOb( pObj );
	}
	int AddSortKey( CScriptObj* pNew, LPCTSTR key )
	{
		CThreadLockRef lock( this );
		return( CObNameSortArray::AddSortKey( pNew, key ));
	}

	// FindKey, RemoveAt and DeleteAt must use a lock outside as well ! (for index to be meaningful)
};

extern class CResource : public CResourceBase
{
	// Script defined resources (not saved in world file)
	static const CAssocReg sm_szLoadKeys[];

public:
	CServTime m_timePeriodic;	// When to perform the next periodic update

	// Begin INI file options.
	bool m_fUseANSI;				// console can use ANSI output.
	bool m_fUseNTService;
	int  m_iUseGodPort;
	bool m_fUseHTTP;
	int  m_iPollServers;		// background polling of peer servers. (minutes)
	CGString m_sRegisterServer;	// GRAY_MAIN_SERVER
	CGString m_sMainLogServerDir;	// This is the main log server Directory. Will list any server that polls.
	int  m_iMapCacheTime;		// Time in sec to keep unused map data.
	int	 m_iSectorSleepMask;	// The mask for how long sectors will sleep.

	CGString m_sWorldBaseDir;	// "e:\graysvr\worldsave\" = world files go here.
	CGString m_sAcctBaseDir;	// Where do the account files go/come from ?

	bool m_fSecure;				// Secure mode. (will trap exceptions)
	int  m_iFreezeRestartTime;	// # seconds before restarting.
#define DEBUGF_NPC_EMOTE		0x0001
#define DEBUGF_ADVANCE_STATS	0x0002
#define DEBUGF_MOTIVATION		0x0004	// display motication level debug messages.
#define DEBUGF_SKILL_TIME		0x0010	// Show the amount of time til skill exec.
#define DEBUGF_WALKCODES		0x0080	// try the new walk code checking stuff.
#define DEBUGF_NPCAI			0x0100	// NPC AI debugging
#define DEBUGF_EXP				0x0200	// experience gain/loss
#define DEBUGF_LEVEL			0x0400	// experience level changes
	WORD m_wDebugFlags;			// DEBUG In game effects to turn on and off.

	// Decay
	int  m_iDecay_Item;			// Base decay time in minutes.
	int  m_iDecay_CorpsePlayer;	// Time for a playercorpse to decay (mins).
	int  m_iDecay_CorpseNPC;	// Time for a NPC corpse to decay.

	// Save
	int  m_iSavePeriod;			// Minutes between saves.
	int  m_iSaveBackupLevels;	// How many backup levels.
	int  m_iSaveBackgroundTime;	// Speed of the background save in minutes.
	bool m_fSaveGarbageCollect;	// Always force a full garbage collection.
	bool m_fSaveInBackground;	// Do background save stuff.

	// Account
	bool m_fRequireEmail;		// Valid Email required to leave GUEST mode.
	int  m_iDeadSocketTime;
	bool m_fArriveDepartMsg;    // General switch to turn on/off arrival/depart messages.
	int  m_iClientsMax;		// Maximum (FD_SETSIZE) open connections to server
	int  m_iClientsMaxIP;		// Maximum (FD_SETSIZE) open connections to server per IP
	int  m_iConnectingMax;		// max clients connecting
	int  m_iConnectingMaxIP;		// max clients connecting

	int  m_iGuestsMax;		// Allow guests who have no accounts ?
	int  m_iClientLingerTime;	// How long logged out clients linger in seconds.
	int  m_iMinCharDeleteTime;	// How old must a char be ? (minutes)
	int  m_iMaxCharsPerAccount;	// MAX_CHARS_PER_ACCT
	bool m_fLocalIPAdmin;		// The local ip is the admin ?
	bool m_fMd5Passwords;		// Should MD5 hashed passwords be used?

	CServerRef m_pServAccountMaster;	// Verify my accounts through here.

	// Magic
	bool m_fReagentsRequired;
	bool m_fWordsOfPowerPlayer; // Words of Power for players
	bool m_fWordsOfPowerStaff;	// Words of Power for staff
	bool m_fEquippedCast;		// Allow casting while equipped.
	bool m_fReagentLossFail;	// ??? Lose reags when failed.
	int m_iMagicUnlockDoor;  // 1 in N chance of magic unlock working on doors -- 0 means never
	ITEMID_TYPE m_iSpell_Teleport_Effect_Players;
	SOUND_TYPE m_iSpell_Teleport_Sound_Players;
	ITEMID_TYPE m_iSpell_Teleport_Effect_Staff;
	SOUND_TYPE m_iSpell_Teleport_Sound_Staff;

	// In Game Effects
	int	 m_iLightDungeon;
	int  m_iLightDay;		// Outdoor light level.
	int  m_iLightNight;		// Outdoor light level.
	int  m_iGameMinuteLength;	// Length of the game world minute in real world (TICK_PER_SEC) seconds.
	bool m_fNoWeather;			// Turn off all weather.
	bool m_fCharTags;			// Put [NPC] tags over chars.
	bool m_fFlipDroppedItems;	// Flip dropped items.
	bool m_fMonsterFight;	// Will creatures fight amoung themselves.
	bool m_fMonsterFear;	// will they run away if hurt ?
	int	 m_iBankIMax;	// Maximum number of items allowed in bank.
	int  m_iBankWMax;	// Maximum weight in WEIGHT_UNITS stones allowed in bank.
	int  m_iVendorMaxSell;		// Max things a vendor will sell in one shot.
	int  m_iMaxCharComplexity;		// How many chars per sector.
	int  m_iMaxItemComplexity;		// How many items per meter.
	int  m_iMaxSectorComplexity;	// How many items per sector.
	bool m_fPlayerGhostSounds;	// Do player ghosts make a ghostly sound?
	bool m_fAutoNewbieKeys;		// Are house and boat keys newbied automatically?
	int  m_iStamRunningPenalty;		// Weight penalty for running (+N% of max carry weight)
	int  m_iStaminaLossAtWeight;	// %Weight at which characters begin to lose stamina
	int  m_iHitpointPercentOnRez;// How many hitpoints do they get when they are rez'd?
	int  m_iMaxBaseSkill;		// Maximum value for base skills at char creation
	int  m_iTrainSkillPercent;	// How much can NPC's train up to ?
	int	 m_iTrainSkillMax;
	int	 m_iMountHeight;		// The height at which a mounted person clears ceilings.
	int  m_iArcheryMaxDist;
	int  m_iArcheryMinDist;
	int  m_iHitsUpdateRate;		// how often send my hits updates to visible clients
	int  m_iSpeedScaleFactor;	// fight skill delay = m_iSpeedScaleFactor / ( (dex + 100) * Weapon Speed )
	int  m_iSkillPracticeMax;	// max skill level a player can practice on dummies/targets upto

	// Criminal/Karma
	bool m_fGuardsInstantKill;	// Will guards kill instantly or follow normal combat rules?
	int	 m_iGuardLingerTime;	// How long do guards linger about.
	int  m_iSnoopCriminal;		// 1 in # chance of getting criminalflagged when succesfully snooping.
	int  m_iMurderMinCount;		// amount of murders before we get title.
	int	 m_iMurderDecayTime;	// (minutes) Roll murder counts off this often.
	bool m_fHelpingCriminalsIsACrime;// If I help (rez, heal, etc) a criminal, do I become one too?
	bool m_fLootingIsACrime;	// Looting a blue corpse is bad.
	int  m_iCriminalTimer;		// How many minutes are criminals flagged for?
	int	 m_iPlayerKarmaNeutral;	// How much bad karma makes a player neutral?
	int	 m_iPlayerKarmaEvil;


	// other
	bool	m_fNoResRobe;
	int		m_iLostNPCTeleport;
	int		m_iExperimental;
	int		m_iOptionFlags;
	int		m_iWoolGrowthTime;	// how long till wool grows back on sheared sheep, in minutes

	int		m_iDistanceYell;
	int		m_iDistanceWhisper;
	int		m_iDistanceTalk;

	int		m_iMaxSkill;
	CGString	m_sSpeechSelf;
	CGString	m_sSpeechPet;
	CGString	m_sSpeechOther;

	CGString	m_sEventsPet;
	CResourceLink *	m_pEventsPetLink;

	int		m_iWalkBuffer;
	int		m_iWalkRegen;

	int		m_iCommandLog;
	
	bool 		m_fUsecrypt;
	int 		m_iUsenocrypt;
	bool		m_fPayFromPackOnly;	// Pay only from main pack?
	bool		m_fReplyPeerConnects;	// allow master&peer connect routines?
	int		m_iOverSkillMultiply;	// multiplyer to get over skillclass
	bool		m_fMstatFollow;
	bool	m_fSuppressCapitals;	// Enable/Disable capital letters suppression
	
	// These are going to be removed
	int		m_iFeatures;
	int		m_iFeaturesLogin;
	// New ones
	int		m_iFeatureT2A;
	int		m_iFeatureLBR;
	int		m_iFeatureAOS;
	int		m_iFeatureSE;
	
	CGString 	m_sVerName;

	int		m_iMaxLoopTimes;

#define NPC_AI_PATH		0x0001		//	NPC pathfinding
#define	NPC_AI_FOOD		0x0002		//	NPC food search (objects + grass)
#define	NPC_AI_EXTRA	0x0004		//	NPC magics, combat, etc
	int		m_iNpcAi;

	//	Experience system
	bool	m_bExperienceSystem;
#define EXP_MODE_RAISE_COMBAT	0x0001
#define	EXP_MODE_RAISE_CRAFT	0x0002
#define	EXP_MODE_ALLOW_DOWN		0x0004
#define	EXP_MODE_DOWN_NOLEVEL	0x0008
#define	EXP_MODE_AUTOSET_EXP	0x0010
#define EXP_MODE_TRIGGER_EXP	0x0020
#define EXP_MODE_TRIGGER_LEVEL	0x0040
	int		m_iExperienceMode;
	int		m_iExperienceKoefPVM;
	int		m_iExperienceKoefPVP;
	bool	m_bLevelSystem;
#define LEVEL_MODE_LINEAR		0
#define	LEVEL_MODE_DOUBLE		1
	int		m_iLevelMode;
	int		m_iLevelNextAt;

	//	mySQL features
	bool		m_bMySql;
	CGString	m_sMySqlHost;
	CGString	m_sMySqlUser;
	CGString	m_sMySqlPass;
	CGString	m_sMySqlDB;

	// End INI file options.
	
	CResourceScript m_scpIni;	// Keep this around so we can link to it.
	CGObArray< CLogIP *> m_LogIP;	// Block these IP numbers

private:
	CResourceScript m_scpTables;

	CStringSortArray m_Obscene;	// Bad Names/Words etc.
	CGObArray< TCHAR* > m_NotoTitles;	// Noto titles.
	CGObArray< TCHAR* > m_Runes;	// Words of power. (A-Z)

	CMultiDefArray m_MultiDefs;	// read from the MUL files. Cached here on demand.

	CObNameSortArray m_SkillNameDefs;	// const CSkillDef* Name sorted
	CGPtrTypeArray< CSkillDef* > m_SkillIndexDefs;	// Defined Skills indexed by number
	CGObArray< CSpellDef* > m_SpellDefs;	// Defined Spells
	CGObArray< CSpellDef* > m_SpellDefs_Sorted;	// Defined Spells, in skill order

	CStringSortArray m_PrivCommands[PLEVEL_QTY];	// what command are allowed for a priv level?

public:
	CThreadLockableDefArray m_Servers; // Servers list. we act like the login server with this.
	CObNameSortArray m_Functions;	// subroutines that can be used in scripts.
	CObNameSortArray m_HelpDefs;	// Help on these commands.
	//CGPtrTypeArray< CRegionBase * > m_RegionDefs;
	CRegionLinks m_RegionDefs;

	// static definition stuff from *TABLE.SCP mostly.
	CGObArray< const CStartLoc* > m_StartDefs; // Start points list
	CValueCurveDef m_StatAdv[STAT_BASE_QTY]; // "skill curve"
	CGTypedArray<CPointBase,CPointBase&> m_MoonGates;	// The array of moongates.

	CResourceHashArray m_WebPages;	// These can be linked back to the script.

private:
	RESOURCE_ID ResourceGetNewID( RES_TYPE restype, LPCTSTR pszName, CVarDefNum ** ppVarNum, bool fNewStyleDef );

public:
	CResource();
	~CResource();

	virtual void r_DumpLoadKeys( CTextConsole * pSrc );
	bool r_LoadVal( CScript &s );
	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal, CTextConsole * pSrc );
	bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	void r_Write( CScript &s );

	bool SaveIni();
	bool LoadIni( bool fTest );
	bool Load( bool fResync );
	void Unload( bool fResync );
	void OnTick( bool fNow );

	bool ResourceTestItemMuls();
	bool ResourceTestCharAnims();

	bool LoadResourceSection( CScript * pScript );
	void LoadSortSpells();
	CResourceDef * ResourceGetDef( RESOURCE_ID_BASE rid ) const;
	
	// Print EF/OF Flags
	
	void PrintEFOFFlags(bool bEF = true, bool bOF = true, CTextConsole *pSrc = NULL);

	// Specialized resource accessors.

	bool CanUsePrivVerb( const CScriptObj * pObjTarg, LPCTSTR pszCmd, CTextConsole * pSrc ) const;
	PLEVEL_TYPE GetPrivCommandLevel( LPCTSTR pszCmd ) const;

	CLogIP * FindLogIP( CSocketAddressIP dwIP, bool fCreate );
	bool SetLogIPBlock( LPCTSTR pszIP, bool fBlock );

	static STAT_TYPE FindStatKey( LPCTSTR pszKey );
	static bool IsValidEmailAddressFormat( LPCTSTR pszText );
	bool IsObscene( LPCTSTR pszText ) const;

	CWebPageDef * FindWebPage( LPCTSTR pszPath ) const;

	CServerRef Server_GetDef( int index );

	const CSpellDef* GetSpellDef( SPELL_TYPE index ) const
	{
		if ( ! index || ! m_SpellDefs.IsValidIndex(index))
			return( NULL );
		return( m_SpellDefs[index] );
	}

	CSpellDef* GetSpellDef( SPELL_TYPE index ) 
	{
		if ( ! index || ! m_SpellDefs.IsValidIndex(index))
			return( NULL );
		return( m_SpellDefs[index] );
	}

	LPCTSTR GetSkillKey( SKILL_TYPE index ) const
	{
		return( m_SkillIndexDefs[index]->GetKey());
	}

	const bool IsSkillFlag( SKILL_TYPE index, SKF_TYPE skf ) const
	{
		if ( !m_SkillIndexDefs.IsValidIndex(index) )
			return false;
		const CSkillDef *	pSkillDef	= GetSkillDef( index );
		return ( pSkillDef && (pSkillDef->m_dwFlags & skf) );
	}

	const CSkillDef* GetSkillDef( SKILL_TYPE index ) const
	{
		return( m_SkillIndexDefs[index] );
	}
	
	CSkillDef* GetSkillDef( SKILL_TYPE index )
	{
		return( m_SkillIndexDefs[index] );
	}

	const CSkillDef* FindSkillDef( LPCTSTR pszKey ) const
	{
		// Find the skill name in the alpha sorted list.
		// RETURN: SKILL_NONE = error.
		int i = m_SkillNameDefs.FindKey( pszKey );
		if ( i < 0 )
			return( NULL );
		return( STATIC_CAST <const CSkillDef*>(m_SkillNameDefs[i]));
	}
	const CSkillDef* SkillLookup( LPCTSTR pszKey );
	SKILL_TYPE FindSkillKey( LPCTSTR pszKey ) const;

	int GetSpellEffect( SPELL_TYPE spell, int iSkillval ) const;

	LPCTSTR GetRune( TCHAR ch ) const
	{
		ch = toupper(ch) - 'A';
		if ( ! m_Runes.IsValidIndex(ch))
			return "?";
		return( m_Runes[ ch ] );
	}
	LPCTSTR GetNotoTitle( int iLevel ) const
	{
		if ( ! m_NotoTitles.IsValidIndex(iLevel))
		{
			return "";
		}
		else
		{
			return m_NotoTitles[ iLevel ];
		}
	}

	const CGrayMulti * GetMultiItemDefs( ITEMID_TYPE itemid );

	bool CanRunBackTask() const;
	bool IsConsoleCmd( TCHAR ch ) const;

	CPointMap GetRegionPoint( LPCTSTR pCmd ) const; // Decode a teleport location number into X/Y/Z

	int Calc_MaxCarryWeight( const CChar * pChar ) const;
	int Calc_DropStamWhileMoving( CChar * pChar, int iWeightLoadPercent );
	int Calc_WalkThroughChar( CChar * pCharMove, CChar * pCharObstacle );
	int Calc_CombatAttackSpeed( CChar * pChar, CItem * pWeapon );
	int Calc_CombatChanceToHit( CChar * pChar, SKILL_TYPE skill, CChar * pCharTarg, CItem * pWeapon );
	int Calc_CombatDamage( CChar * pChar, CItem * pWeapon, CChar * pCharTarg );
	int Calc_CharDamageTake( CChar * pChar, CItem * pWeapon, CChar * pCharAttacker, int iDamage, DAMAGE_TYPE DamageType, BODYPART_TYPE LocationHit );
	int Calc_ItemDamageTake( CItem * pItem, CItem * pWeapon, CChar * pCharAttacker, int iDamage, DAMAGE_TYPE DamageType, BODYPART_TYPE LocationHit );
	bool Calc_SkillCheck( int iSkillLevel, int iDifficulty );
	int  Calc_StealingItem( CChar * pCharThief, CItem * pItem, CChar * pCharMark );
	bool Calc_CrimeSeen( CChar * pCharThief, CChar * pCharViewer, SKILL_TYPE SkillToSee, bool fBonus );
	int Calc_FameKill( CChar * pKill );
	int Calc_FameScale( int iFame, int iFameChange );
	int Calc_KarmaKill( CChar * pKill, NOTO_TYPE NotoThem );
	int Calc_KarmaScale( int iKarma, int iKarmaChange );
	
	int Calc_FeatureLogin( int iReslevel );
	int Calc_FeatureGame( int iReslevel );
	bool IsFeatureForRes( int iResLevel, int iFeauture );

#define SysMessageDefault( msg )	SysMessage( g_Cfg.GetDefaultMsg( msg ) )
	LPCTSTR CResource::GetDefaultMsg(LPCTSTR pszKey);
	LPCTSTR	GetDefaultMsg(long lKeyNum);
} g_Cfg;



class CDialogDef : public CResourceLink
{
	static LPCTSTR const sm_szLoadKeys[];

public:
	bool		GumpSetup( int iPage, CClient * pClientSrc, CObjBase * pObj, LPCTSTR Arguments = "" );
	int			GumpAddText( LPCTSTR pszText );		// add text to the text section, return insertion index
	bool		r_Verb( CScript &s, CTextConsole * pSrc );
	bool		r_LoadVal( CScript & s );
	bool		r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );

	CDialogDef( RESOURCE_ID rid );
	virtual ~CDialogDef()
	{
	}

public:	
	// temporary placeholders - valid only during dialog setup
	CObjBase *	m_pObj;
	CGString	m_sControls[1024];
	CGString	m_sText[512];
	int			m_iTexts;
	int			m_iControls;
	int			m_x;
	int			m_y;

	int			m_iOriginX;	// keep track of position when parsing
	int			m_iOriginY;
	WORD		m_iPage;		// page to open the dialog in
};




class CItemTypeDef : public CResourceLink
{
public:
	CItemTypeDef( RESOURCE_ID rid ) : CResourceLink( rid )
	{	
	}

	bool		r_LoadVal( CScript & s );

	int			GetItemType();
};

#define IsSetEF(ef)		(g_Cfg.m_iExperimental & ef)
#define IsSetOF(of)		(g_Cfg.m_iOptionFlags & of)
#define IsSetSpecific	((g_Cfg.m_iExperimental & EF_Specific) && (g_Cfg.m_iOptionFlags & OF_Specific))

#endif	// _INC_CRESOURCE_H
