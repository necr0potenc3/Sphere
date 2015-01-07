//
// CCharNPCAct.CPP
// Copyright Menace Software (www.menasoft.com).
//
// Actions specific to an NPC.
//

#include "graysvr.h"	// predef header.
#include "../graysvr/CPathFinder.h"

//////////////////////////
// CChar

enum NV_TYPE
{
	NV_BUY,
	NV_BYE,
	NV_FLEE,
	NV_GOTO,
	NV_HIRE,
	NV_LEAVE,
	NV_MIDILIST,
	NV_PETRETRIEVE,
	NV_PETSTABLE,
	NV_RESTOCK,
	NV_RUN,
	NV_SCRIPT,
	NV_SELL,
	NV_SHRINK,
	NV_TRAIN,
	NV_WALK,
	NV_QTY,
};

LPCTSTR const CCharNPC::sm_szVerbKeys[NV_QTY+1] =
{
	"BUY",
	"BYE",
	"FLEE",
	"GOTO",
	"HIRE",
	"LEAVE",
	"MIDILIST",
	"PETRETRIEVE",
	"PETSTABLE",
	"RESTOCK",
	"RUN",
	"SCRIPT",
	"SELL",
	"SHRINK",
	"TRAIN",
	"WALK",
	NULL,
};

bool CChar::NPC_OnVerb( CScript &s, CTextConsole * pSrc ) // Execute command from script
{
	// Stuff that only NPC's do.
	ASSERT( m_pNPC );

	CChar * pCharSrc = pSrc->GetChar();

	switch ( FindTableSorted( s.GetKey(), CCharNPC::sm_szVerbKeys, COUNTOF(CCharNPC::sm_szVerbKeys)-1 ))
	{
	case NV_BUY:
		// Open up the buy dialog.
		if ( pCharSrc == NULL || ! pCharSrc->IsClient())
			return( false );
		if ( m_pNPC->m_Brain == NPCBRAIN_VENDOR_OFFDUTY )
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_VENDOR_OFFDUTY ) );
			return( true );
		}
		if ( ! pCharSrc->GetClient()->addShopMenuBuy( this ))
		{
			if ( GetKeyItemBase( "RESEARCH.ITEM" ) )
				Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_VENDOR_NOTHING_BUY ) );
			else
				Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_VENDOR_NO_GOODS ) );
		}
		break;
	case NV_BYE:
		Skill_Start( SKILL_NONE );
		m_Act_Targ.InitUID();
		break;
	case NV_FLEE:
do_leave:
		// Short amount of fleeing.
		m_atFlee.m_iStepsMax = s.GetArgVal();	// how long should it take to get there.
		if ( ! m_atFlee.m_iStepsMax )
			m_atFlee.m_iStepsMax = 20;
		m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
		Skill_Start( NPCACT_FLEE );
		break;
	case NV_GOTO:
		m_Act_p.Read(s.GetArgRaw());
		NPC_WalkToPoint(true);
		break;
	case NV_HIRE:
		return NPC_OnHireHear( pCharSrc);
	case NV_LEAVE:
		goto do_leave;
	case NV_MIDILIST:	// just ignore this.
		return( true );
	case NV_PETRETRIEVE:
		return( NPC_StablePetRetrieve( pCharSrc ));
	case NV_PETSTABLE:
		return( NPC_StablePetSelect( pCharSrc ));
	case NV_RESTOCK:	// individual restock command.
		return NPC_Vendor_Restock( s.GetArgVal());
	case NV_RUN:
		m_Act_p = GetTopPoint();
		m_Act_p.Move( GetDirStr( s.GetArgRaw()));
		NPC_WalkToPoint( true );
		break;
	case NV_SCRIPT:
		// Give the NPC a script book !
		return NPC_Script_Command( s.GetArgStr(), true );
	case NV_SELL:
		// Open up the sell dialog.
		if ( pCharSrc == NULL || ! pCharSrc->IsClient())
			return( false );
		if ( m_pNPC->m_Brain == NPCBRAIN_VENDOR_OFFDUTY )
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_VENDOR_OFFDUTY ) );
			return( true );
		}
		if ( ! pCharSrc->GetClient()->addShopMenuSell( this ))
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_VENDOR_NOTHING_BUY ) );
		}
		break;
	case NV_SHRINK:
		// we must own it.
		if ( ! NPC_IsOwnedBy( pCharSrc ))
			return( false );
		return( NPC_Shrink() != NULL );
	case NV_TRAIN:
		return( NPC_OnTrainHear( pCharSrc, s.GetArgStr()));
	case NV_WALK:
		m_Act_p = GetTopPoint();
		m_Act_p.Move( GetDirStr( s.GetArgRaw()));
		NPC_WalkToPoint( false );
		break;
	default:
		// Eat all the CClient::sm_szVerbKeys and CCharPlayer::sm_szVerbKeys verbs ?
		if ( CClient::r_GetVerbIndex( s.GetKey()) >= 0 )
			return( true );
		return( false );
	}
	return( true );
}

const LAYER_TYPE CChar::sm_VendorLayers[] = // static
{
	LAYER_VENDOR_STOCK, LAYER_VENDOR_EXTRA, LAYER_VENDOR_BUYS, LAYER_BANKBOX,
};

bool CChar::NPC_Vendor_Dump( CItemContainer * pBank )
{
	// Dump the contents of my vendor boxes into this container.

	if ( ! NPC_IsVendor())
		return false;

	for ( int i=0; i<3; i++ )
	{
		CItemContainer * pCont = GetBank( sm_VendorLayers[i] );
		if ( pCont == NULL )
			return( false );
		CItem * pItemNext;
		CItem * pItem = pCont->GetContentHead();
		for ( ; pItem != NULL; pItem = pItemNext )
		{
			pItemNext = pItem->GetNext();
			pBank->ContentAdd( pItem );
		}
	}

	return( true );
}

bool CChar::NPC_Vendor_Restock( int iTimeSec )
{
	// Restock this NPC char.
	// Then Set the next restock time for this .

	if ( m_pNPC == NULL )
		return false;

	if ( IsStatFlag( STATF_Spawned ))
	{
		// kill all spawned creatures.
		Delete();
		return( true );
	}

	// Not a player vendor.
	if ( ! IsStatFlag( STATF_Pet ))
	{
		// Delete all non-newbie stuff we have first ?
		// ReadScriptTrig( Char_GetDef(), CTRIG_NPCRestock );

		if ( NPC_IsVendor())
		{
			for ( int i=0; i<COUNTOF(sm_VendorLayers); i++ )
			{
				CItemContainer * pCont = GetBank( sm_VendorLayers[i] );
				if ( pCont == NULL )
					return( false );
				if ( iTimeSec )
				{
					pCont->SetRestockTimeSeconds( iTimeSec );
				}
				pCont->Restock();
			}
		}

		if ( m_ptHome.IsValidPoint() && ! IsStatFlag( STATF_Freeze | STATF_Stone ))
		{
			// send them back to their "home"
			MoveNear( m_ptHome, 5 );
		}
	}

	return( true );
}

bool CChar::NPC_StablePetSelect( CChar * pCharPlayer )
{
	// I am a stable master.
	// I will stable a pet for the player.

	if ( pCharPlayer == NULL )
		return( false );
	if ( ! pCharPlayer->IsClient())
		return( false );

	// Might have too many pets already ?

	int iCount = 0;
	CItemContainer * pBank = GetBank();
	if ( pBank->GetCount() >= MAX_ITEMS_CONT )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_FULL ) );
		return( false );
	}

	CItem* pItem = pBank->GetContentHead();
	for ( ; pItem!=NULL ; pItem = pItem->GetNext())
	{
		if ( pItem->IsType( IT_FIGURINE ) && pItem->m_uidLink == pCharPlayer->GetUID())
			iCount++;
	}

	if ( iCount > 10 )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_TOOMANY ) );
		return( false );
	}

	pCharPlayer->m_pClient->m_Targ_PrvUID = GetUID();
	pCharPlayer->m_pClient->addTarget( CLIMODE_TARG_PET_STABLE, g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_TARG_STABLE ) );

	return( true );
}

bool CChar::NPC_StablePetRetrieve( CChar * pCharPlayer )
{
	// Get pets for this person from my inventory.
	// May want to put up a menu ???

	if ( m_pNPC == NULL )
		return( false );
	if ( m_pNPC->m_Brain != NPCBRAIN_STABLE )
		return( false );

	int iCount = 0;
	CItem* pItem = GetBank()->GetContentHead();
	while ( pItem!=NULL )
	{
		CItem * pItemNext = pItem->GetNext();
		if ( pItem->IsType( IT_FIGURINE ) && pItem->m_uidLink == pCharPlayer->GetUID())
		{
			if ( pCharPlayer->Use_Figurine( pItem, 2 ))
			{
				pItem->Delete();
			}
			iCount++;
		}
		pItem = pItemNext;
	}

	if ( ! iCount )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_NOPETS ) );
		return( false );
	}
	else
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_TREATWELL ) );
	}

	return( true );
}

void CChar::NPC_ActStart_SpeakTo( CChar * pSrc )
{
	// My new action is that i am speaking to this person.
	// Or just update the amount of time i will wait for this person.

	// if ( IsStatFlag( STATF_Pet )) return;
	ASSERT(pSrc);
	m_Act_Targ = pSrc->GetUID();
	m_atTalk.m_WaitCount = 20;
	m_atTalk.m_HearUnknown = 0;

	Skill_Start( ( pSrc->Stat_GetAdjusted(STAT_FAME) > 7000 ) ? NPCACT_TALK_FOLLOW : NPCACT_TALK );
	SetTimeout( 3*TICK_PER_SEC );
	UpdateDir( pSrc );
}



void CChar::NPC_OnHear( LPCTSTR pszCmd, CChar * pSrc )
{
	// This CChar has heard you say something.
	ASSERT( pSrc );
	if ( m_pNPC == NULL )
		return;

	// Pets always have a basic set of actions.
	if ( NPC_OnHearPetCmd( pszCmd, pSrc, false ))
		return;
	if ( ! NPC_CanSpeak())	// can they speak ?
		return;

	// What where we doing ?
	// too busy to talk ?

	switch ( Skill_GetActive())
	{
	case SKILL_BEGGING: // busy begging. (hack)
		if ( !g_Cfg.IsSkillFlag( SKILL_BEGGING, SKF_SCRIPTED ) )
			return;
		break;
	case NPCACT_TALK:
	case NPCACT_TALK_FOLLOW:
		// Was NPC talking to someone else ?
		if ( m_Act_Targ != pSrc->GetUID())
		{
			if ( NPC_Act_Talk())
			{
				CChar * pCharOld = m_Act_Targ.CharFind();
				char	*z = Str_GetTemp();
				sprintf(z, g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_INTERRUPT),
					(LPCTSTR) pCharOld->GetName(), pSrc->GetName());
				Speak(z);
			}
		}
		break;
	}

	// I've heard them for the first time.
	CItemMemory * pMemory = Memory_FindObjTypes( pSrc, MEMORY_SPEAK );
	if ( pMemory == NULL )
	{
		// This or CTRIG_SeeNewPlayer will be our first contact with people.
		if ( OnTrigger( CTRIG_NPCHearGreeting, pSrc ) == TRIGRET_RET_TRUE )
			return;

		// record that we attempted to speak to them.
		pMemory = Memory_AddObjTypes( pSrc, MEMORY_SPEAK );
		ASSERT(pMemory);
		pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_FIRSTSPEAK;
		// m_Act_Hear_Unknown = 0;
	}

	// Do the scripts want me to take some action based on this speech.
	SKILL_TYPE skill = m_Act_SkillCurrent;

	TALKMODE_TYPE	mode	= TALKMODE_SAY;
	int i;
	for ( i=0; i<m_pNPC->m_Speech.GetCount(); i++ )
	{
		CResourceLink * pLink = m_pNPC->m_Speech[i];
		ASSERT(pLink);
		CResourceLock s;
		if ( ! pLink->ResourceLock( s ))
			continue;
		if ( ! pLink->HasTrigger(XTRIG_UNKNOWN))
			continue;
		TRIGRET_TYPE iRet = OnHearTrigger( s, pszCmd, pSrc, mode );
		if ( iRet == TRIGRET_ENDIF || iRet == TRIGRET_RET_FALSE )
			continue;
		if ( iRet == TRIGRET_RET_DEFAULT && skill == m_Act_SkillCurrent )
		{
			// You are the new speaking target.
			NPC_ActStart_SpeakTo( pSrc );
		}
		return;
	}

	CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	for ( i=0; i<pCharDef->m_Speech.GetCount(); i++ )
	{
		CResourceLink * pLink = pCharDef->m_Speech[i];
		ASSERT(pLink);
		CResourceLock s;
		if ( ! pLink->ResourceLock( s ))
			continue;
		DEBUG_CHECK( pLink->HasTrigger(XTRIG_UNKNOWN));
		TRIGRET_TYPE iRet = OnHearTrigger( s, pszCmd, pSrc, mode );
		if ( iRet == TRIGRET_ENDIF || iRet == TRIGRET_RET_FALSE )
			continue;
		if ( iRet == TRIGRET_RET_DEFAULT && skill == m_Act_SkillCurrent )
		{
			// You are the new speaking target.
			NPC_ActStart_SpeakTo( pSrc );
		}
		return;
	}

	// hard code some default reactions.
	if ( m_pNPC->m_Brain == NPCBRAIN_HEALER	|| Skill_GetBase( SKILL_SPIRITSPEAK ) >= 1000 )
	{
		if ( NPC_LookAtChar( pSrc, 1 ))
			return;
	}

	// can't figure you out.
	if ( OnTrigger( CTRIG_NPCHearUnknown, pSrc ) == TRIGRET_RET_TRUE )
		return;

	if ( Skill_GetActive() == NPCACT_TALK ||
		Skill_GetActive() == NPCACT_TALK_FOLLOW )
	{
		++ m_atTalk.m_HearUnknown;
		int iMaxUnk = 4;
		if ( GetDist( pSrc ) > 4 )
			iMaxUnk = 1;
		if ( m_atTalk.m_HearUnknown > iMaxUnk )
		{
			Skill_Start( SKILL_NONE ); // say good by
		}
	}
}

int CChar::NPC_OnTrainCheck( CChar * pCharSrc, SKILL_TYPE Skill )
{
	// Can we train in this skill ?
	// RETURN: Amount of skill we can train.
	//

	if ( ! IsSkillBase( Skill ))
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_DUNNO_1 ) );
		return( 0 );
	}

	int iSkillSrcVal = pCharSrc->Skill_GetBase(Skill);
	int iSkillVal = Skill_GetBase(Skill);
	int iTrainCost = NPC_GetTrainMax( pCharSrc, Skill ) - iSkillSrcVal;

	LPCTSTR pszMsg;
	if ( iSkillVal <= 0 )
	{
		pszMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_DUNNO_2 );
	}
	else if ( iSkillSrcVal > iSkillVal )
	{
		pszMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_DUNNO_3 );
	}
	else if ( iTrainCost <= 0 )
	{
		pszMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_DUNNO_4 );
	}
	else
	{
		return( iTrainCost );
	}

	char	*z = Str_GetTemp();
	sprintf(z, pszMsg, g_Cfg.GetSkillKey(Skill));
	Speak(z);
	return 0;
}

bool CChar::NPC_OnTrainPay( CChar * pCharSrc, CItemMemory * pMemory, CItem * pGold )
{
	ASSERT( pMemory );

	SKILL_TYPE skill = (SKILL_TYPE)( pMemory->m_itEqMemory.m_Skill );
	if ( ! IsSkillBase( skill ))
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_FORGOT ) );
		return( false );
	}

	int iTrainCost = NPC_OnTrainCheck( pCharSrc, skill );
	if ( iTrainCost <= 0 )
		return false;

	Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_SUCCESS ) );

	// Consume as much money as we can train for.
	ASSERT( pGold );
	if ( pGold->GetAmount() < iTrainCost )
	{
		iTrainCost = pGold->GetAmount();
	}
	else if ( pGold->GetAmount() == iTrainCost )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_THATSALL_1 ) );
		pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_NONE;
	}
	else
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_THATSALL_2 ) );
		pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_NONE;

		// Give change back.
		pGold->UnStackSplit( iTrainCost, pCharSrc );
	}

	GetPackSafe()->ContentAdd( pGold );	// take my cash.

	// Give credit for training.
	pCharSrc->Skill_SetBase( skill, pCharSrc->Skill_GetBase(skill) + iTrainCost );
	return( true );
}

bool CChar::NPC_OnTrainHear( CChar * pCharSrc, LPCTSTR pszCmd )
{
	// We are asking for training ?

	if ( ! m_pNPC )
		return( false );
	if ( ! NPC_IsVendor() &&
		m_pNPC->m_Brain != NPCBRAIN_HUMAN )
	{
		return false;
	}

	if ( Memory_FindObjTypes( pCharSrc, MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY|MEMORY_AGGREIVED ))
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_ENEMY ) );
		return false;
	}

	// Did they mention a skill name i recognize ?
	TCHAR *pszMsg = Str_GetTemp();

	int i=SKILL_NONE+1;
	for ( ; i<MAX_SKILL; i++ )
	{
		LPCTSTR pSkillKey = g_Cfg.GetSkillKey( (SKILL_TYPE) i );
		if ( ! FindStrWord( pszCmd, pSkillKey ))
			continue;

		// Can we train in this ?
		int iTrainCost = NPC_OnTrainCheck( pCharSrc, (SKILL_TYPE) i );
		if ( iTrainCost <= 0 )
			return true;

		sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_PRICE), iTrainCost, (LPCTSTR)pSkillKey);
		Speak(pszMsg);
		CItemMemory * pMemory = Memory_AddObjTypes( pCharSrc, MEMORY_SPEAK );
		ASSERT(pMemory);
		pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_SPEAK_TRAIN;
		pMemory->m_itEqMemory.m_Skill = i;
		return true;
	}

	// What can he teach me about ?
	// Just tell them what we can teach them or set up a memory to train.
	strcpy( pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_PRICE_1 ) );

	LPCTSTR pPrvSkill = NULL;

	int iCount = 0;
	for ( i=SKILL_NONE+1; i<MAX_SKILL; i++ )
	{
		int iDiff = NPC_GetTrainMax( pCharSrc, (SKILL_TYPE)i ) - pCharSrc->Skill_GetBase( (SKILL_TYPE) i);
		if ( iDiff <= 0 )
			continue;

		if ( iCount > 6 )
		{
			pPrvSkill = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_PRICE_2 );
			break;
		}
		if ( iCount > 1 )
		{
			strcat( pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_PRICE_3 ) );
		}
		if ( pPrvSkill )
		{
			strcat( pszMsg, pPrvSkill );
		}
		pPrvSkill = g_Cfg.GetSkillKey( (SKILL_TYPE) i );
		iCount ++;
	}

	if ( iCount == 0 )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_THATSALL_3 ) );
		return true;
	}
	if ( iCount > 1 )
	{
		strcat( pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_THATSALL_4 ) );
	}

	strcat( pszMsg, pPrvSkill );
	strcat( pszMsg, "." );
	Speak( pszMsg );
	return( true );
}

bool CChar::NPC_Act_Begging( CChar * pChar )
{
	// SKILL_BEGGING
	bool fSpeak;

	if ( pChar )
	{
		// Is this a proper target for begging ?
		if ( pChar == this ||
			pChar->m_pNPC ||	// Don't beg from NPC's or the PC you just begged from
			pChar->GetUID() == m_Act_TargPrv )	// Already targetting this person.
			return( false );

		if ( pChar->GetUID() == m_Act_Targ && Skill_GetActive() == SKILL_BEGGING )
			return( true );

		Skill_Start( SKILL_BEGGING );
		m_Act_Targ = pChar->GetUID();
		fSpeak = true;
	}
	else
	{
		// We are already begging.
		pChar = m_Act_Targ.CharFind();
		if ( pChar == NULL )
		{
	bailout:
			m_Act_Targ.InitUID();
			Skill_Start( SKILL_NONE );
			return( false );
		}

		if ( ! CanSee( pChar ))	// can we still see them ?
		{
			if ( Calc_GetRandVal(75) > Stat_GetAdjusted(STAT_INT)) // Dumb beggars think I'm gone
			{
				goto bailout;
			}
		}

		// Time to beg ?
		fSpeak = ! Calc_GetRandVal(6);
	}

	SetTimeout( 2*TICK_PER_SEC );

	static UINT const sm_szSpeakBeggar[] =
	{
		DEFMSG_NPC_BEGGAR_BEG_1,
		DEFMSG_NPC_BEGGAR_BEG_2,
		DEFMSG_NPC_BEGGAR_BEG_3,
		DEFMSG_NPC_BEGGAR_BEG_4,
		DEFMSG_NPC_BEGGAR_BEG_5,
		DEFMSG_NPC_BEGGAR_BEG_6,
	};

	UpdateDir( pChar );	// face PC
	if ( fSpeak )
	{
		Speak( g_Cfg.GetDefaultMsg(sm_szSpeakBeggar[ Calc_GetRandVal( COUNTOF( sm_szSpeakBeggar )) ]) );
	}

	if ( ! Calc_GetRandVal( ( 10100 - pChar->Stat_GetAdjusted(STAT_FAME)) / 50 ))
	{
		UpdateAnimate( ANIM_BOW );
		return( true );
	}

	return( NPC_Act_Follow( false, 2 ));
}

void CChar::NPC_OnNoticeSnoop( CChar * pCharThief, CChar * pCharMark )
{
	ASSERT(m_pNPC);

	// start making them angry at you.
	static UINT const sm_szTextSnoop[] =
	{
		DEFMSG_NPC_GENERIC_SNOOPED_1,
		DEFMSG_NPC_GENERIC_SNOOPED_2,
		DEFMSG_NPC_GENERIC_SNOOPED_3,
		DEFMSG_NPC_GENERIC_SNOOPED_4,
	};

	if ( pCharMark != this )	// not me so who cares.
		return;

	if ( NPC_CanSpeak())
	{
		Speak( g_Cfg.GetDefaultMsg(sm_szTextSnoop[ Calc_GetRandVal( COUNTOF( sm_szTextSnoop )) ]));
	}
	if ( ! Calc_GetRandVal(4))
	{
		m_Act_Targ = pCharThief->GetUID();
		m_atFlee.m_iStepsMax = 20;	// how long should it take to get there.
		m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
		Skill_Start( NPCACT_FLEE );
	}
}

int CChar::NPC_WalkToPoint( bool fRun )
{
	// Move toward my target .
	//
	// RETURN:
	//  0 = we are here.
	//  1 = took the step.
	//  2 = can't take this step right now. (obstacle)
	int			iDex = Stat_GetAdjusted(STAT_DEX);
	int			iInt = Stat_GetAdjusted(STAT_INT);
	CPointMap	pMe = GetTopPoint();
	CPointMap	pTarg = m_Act_p;
	DIR_TYPE	Dir = pMe.GetDir(pTarg);
	bool		bUsePathfinding = false;

	if ( Dir >= DIR_QTY ) return(0);	// we are already in the spot
	if ( iDex <= 0 ) return(2);			// we cannot move now

	//	Use pathfinding
	if ( g_Cfg.m_iNpcAi&NPC_AI_PATH )
	{
		if ( !IsSetOF(OF_Multithreaded) )
			NPC_Pathfinding();

		//	walk the saved path
		CPointMap local;
		local.m_x = m_pNPC->m_nextX[0];
		local.m_y = m_pNPC->m_nextY[0];
			// no steps available yet, or pathfinding not usable in this situation
			// so, use default movements
		if (( local.m_x < 1 ) || ( local.m_y < 1 )) goto nopathfinding;
		Dir = pMe.GetDir(local);
		local = pMe;
		pMe.Move(Dir);

		//	also shift the steps array
		int j;
		for ( j = 0; j < MAX_NPC_PATH_STORAGE_SIZE-1; j++ )
		{
			m_pNPC->m_nextX[j] = m_pNPC->m_nextX[j+1];
			m_pNPC->m_nextY[j] = m_pNPC->m_nextY[j+1];
		}
		m_pNPC->m_nextX[MAX_NPC_PATH_STORAGE_SIZE-1] = 0;
		m_pNPC->m_nextY[MAX_NPC_PATH_STORAGE_SIZE-1] = 0;

		//	do the recorded move
		if ( !CanMoveWalkTo(pMe, true, false, Dir) )
		{
			//	make standart default move if failed the recorded one
			pMe = local;
			goto nopathfinding;
		}
		goto finishmoveaction;
	}
	else
	{
nopathfinding:
//		DEBUG_ERR(("ADVAI:%d: NPC '%s' uses default walk" DEBUG_CR, irrr, GetName()));
		pMe.Move( Dir );
		if ( ! CanMoveWalkTo(pMe, true, false, Dir ) )
		{
			CPointMap	ptFirstTry = pMe;

			// try to step around it ?
			int iDiff = 0;
			int iRand = Calc_GetRandVal( 100 );
			if ( iRand < 30 )	// do nothing.
				return( 2 );
			if ( iRand < 35 ) iDiff = 4;	// 5
			else if ( iRand < 40 ) iDiff = 3;	// 10
			else if ( iRand < 65 ) iDiff = 2;
			else iDiff = 1;
			if ( iRand & 1 ) iDiff = -iDiff;
			pMe = GetTopPoint();
			Dir = GetDirTurn( Dir, iDiff );
			pMe.Move( Dir );
			if ( ! CanMoveWalkTo(pMe, true, false, Dir ))
			{
				bool	bClearedWay = false;
				// Some object in my way that i could move ? Try to move it.
				if (( g_Cfg.m_iNpcAi&NPC_AI_EXTRA ) && ( iInt > iRand ))
				{
					int			i;
					CPointMap	point;
					for ( i = 0; i < 2; i++ )
					{
						if ( !i ) point = pMe;
						else point = ptFirstTry;

						//	Scan point for items that could be moved by me and move them to my position
						CWorldSearch	AreaItems(point);
						while ( true )
						{
							CItem	*pItem = AreaItems.GetItem();
							if ( !pItem ) break;
							else if ( abs(pItem->GetTopZ() - pMe.m_z) > 3 ) continue;		// item is too high
							else if ( pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_OWNED|ATTR_INVIS|ATTR_STATIC) ) bClearedWay = false;
							else if ( !pItem->Item_GetDef()->Can(CAN_I_BLOCK) ) continue;	// this item not blocking me
							else if ( !CanCarry(pItem) ) bClearedWay = false;
							else
							{
								//	If the item is a valuable one, investigate the way to loot it
								//	or not. Do it only with first direction we try to move items and
								//	if we are not too busy with something else
								SKILL_TYPE	st = Skill_GetActive();
								if ( !Calc_GetRandVal(4) )	// 25% chance for item review
								{
									if (( st == NPCACT_WANDER ) || ( st == NPCACT_LOOKING ) || ( st == NPCACT_GO_HOME ) || ( st == NPCACT_GO_OBJ ))
									{
										CItemVendable *pVend = dynamic_cast <CItemVendable*>(pItem);
										if ( pVend )	// the item is valuable
										{
											LONG price = pVend->GetBasePrice();
														//	the more intelligent i am, the more valuable
														//	staff I will look to
											if ( Calc_GetRandVal(price) < iInt )
											{
												m_Act_Targ = pItem->GetUID();
												NPC_Act_Looting();
												bClearedWay = false;
												break;
											}
										}
									}
								}
								bClearedWay = true;

								//	move this item to the position I am currently in
								if ( g_Cfg.m_wDebugFlags & DEBUGF_NPCAI )
									DEBUG_ERR(("%s moving %s blocking his path." DEBUG_CR, GetName(), pItem->GetName()));

								pItem->SetTopPoint(GetTopPoint());
								pItem->Update();
							}

							if ( !bClearedWay ) break;
						}

						if ( bClearedWay ) break;
						//	If not cleared the way still, but I am still clever enough
						//	I should try to move in the first step I was trying to move to
						else if ( iInt < iRand*3 ) break;
					}

					//	we have just cleared our way
					if ( bClearedWay )
						if ( point == ptFirstTry )
							Dir = pMe.GetDir(m_Act_p);
				}
				if ( !bClearedWay ) return 2;
			}
		}
	}
finishmoveaction:
	CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	// ??? Make sure we are not facing a wall.
	m_dirFace = Dir;	// Face this direction.
	if ( fRun && ( ! pCharDef->Can(CAN_C_RUN|CAN_C_FLY) || Stat_GetVal(STAT_DEX) <= 1 ))
		fRun = false;

	StatFlag_Mod(STATF_Fly, fRun);

	CPointMap pold = GetTopPoint();
	CheckRevealOnMove();
	MoveToChar(pMe);
	UpdateMove(pold);
	CheckLocation(false);	// Look for teleports etc.

	// How fast can they move.
	int iTickNext;
	if ( fRun )
	{
		if ( IsStatFlag( STATF_Pet ))	// pets run a little faster.
		{
			if ( iDex < 75 )
				iDex = 75;
		}
		iTickNext = TICK_PER_SEC/4 + Calc_GetRandVal( (100-iDex)/5 ) * TICK_PER_SEC / 10;
	}
	else
		iTickNext = TICK_PER_SEC + Calc_GetRandVal( (100-iDex)/3 ) * TICK_PER_SEC / 10;

	iTickNext = (iTickNext * pCharDef->m_iMoveRate)/100;
	if ( iTickNext < 1 )
		iTickNext = 1;

	SetTimeout(iTickNext);
	return 1;
}

bool CChar::NPC_MotivationCheck( int iMotivation )
{
	// Am i currently doing something more important ?
	ASSERT( m_pNPC );
	if ( iMotivation < m_pNPC->m_Act_Motivation )
		return( false );
	m_pNPC->m_Act_Motivation = iMotivation;
	return( true );
}

bool CChar::NPC_Script_Command( LPCTSTR pszCmd, bool fSystemCheck )
{
	// Does the NPC have a script command by this name ?
	// Execute this command.
	// fSystemCheck = load new command sfrom the system.
	//   else just check command swe have in memory.

	CItem* pItem=GetContentHead();
	for ( ; pItem!=NULL; pItem=pItem->GetNext())
	{
		if ( ! pItem->IsType( IT_EQ_NPC_SCRIPT ))
			continue;
		if ( strcmpi( pszCmd, pItem->GetName()))
			continue;
		NPC_Script_OnTick( dynamic_cast <CItemMessage *>( pItem ), true );
		return true;
	}

	if ( ! fSystemCheck )
		return( false );

	RESOURCE_ID rid = g_Cfg.ResourceGetIDType( RES_BOOK, pszCmd );
	if ( ! rid.IsValidUID())
		return( false );

	// Assume there is a book id in RES_BOOK
	// Create the book for it.
	CItemMessage *pBook = dynamic_cast <CItemMessage *>( CItem::CreateScript(ITEMID_BOOK1), this);
	ASSERT(pBook);
	pBook->SetType(IT_EQ_NPC_SCRIPT);
	pBook->SetAttr(ATTR_CAN_DECAY);
	// Does the book id exist in scripts.
	pBook->m_itNPCScript.m_ResID = rid;
	if ( ! pBook->LoadSystemPages())
	{
		pBook->Delete();
		return( false );
	}

	LayerAdd( pBook, LAYER_SPECIAL );
	m_Act_Targ = pBook->GetUID();	// for last target stuff. (trigger stuff)
	NPC_Script_OnTick( pBook, true );
	return( true );
}

void CChar::NPC_Script_OnTick( CItemMessage * pScriptItem, bool fForceStart )
{
	// IT_EQ_NPC_SCRIPT
	// Take a tick for the current running script book or start it.

	if ( m_pNPC == NULL || pScriptItem == NULL )
		return;

	// Default re-eval time.
	pScriptItem->SetTimeout( 5 * 60 * TICK_PER_SEC );

	// Did the script get superceded by something more important ?
	int iPriLevel = pScriptItem->m_itNPCScript.m_iPriorityLevel;
	if ( m_pNPC->m_Act_Motivation > iPriLevel )
		return;

	// Is the script running ?
	int iPage = pScriptItem->m_itNPCScript.m_ExecPage;
	int iOffset = pScriptItem->m_itNPCScript.m_ExecOffset;

	if ( fForceStart )
	{
		if ( iPage )	// we should just wait for our tick !
			return;

		Skill_Cleanup();

		// Time to play. (continue from last)
		if ( pScriptItem->IsBookSystem())
		{
			// Need to load the book.
			if ( ! pScriptItem->LoadSystemPages())
				return;
		}
	}
	else
	{
		// just a tick.
		// ??? Auto starting scripts !!!
		if ( ! iPage )	// but it's not running
			return;
	}

	Skill_Start( NPCACT_ScriptBook );
	m_pNPC->m_Act_Motivation = iPriLevel;

	// Sample actions.
	// "GO=123,23,3"			// teleport directly here.
	// "TARG=123,23,3;ACT=goto"	// try to walk to this place.
	// "S=Hello, Welcome to my show!;T=30;M=W;M=W;M=E;M=E;S=Done;ANIM=Bow"

	LPCTSTR pszPage = pScriptItem->GetPageText( iPage );
	if ( pszPage == NULL )
	{
		// Done playing.
play_stop:
		pScriptItem->m_itNPCScript.m_ExecPage = 0;
		pScriptItem->m_itNPCScript.m_ExecOffset = 0;

		if ( pScriptItem->IsBookSystem())
		{
			// unload the book.
			pScriptItem->UnLoadSystemPages();
		}
		if ( pScriptItem->IsAttr(ATTR_CAN_DECAY))
		{
			pScriptItem->Delete();
		}
		else if ( pScriptItem->GetScriptTimeout() )
		{
			pScriptItem->SetTimeout( pScriptItem->GetScriptTimeout() );
		}
		return;
	}

	// STATF_Script_Play
	// Exec the script command
	TCHAR *pszTemp = Str_GetTemp();
	TCHAR * pszVerb = pszTemp;
	int len = 0;

restart_read:
	while (true)
	{
		TCHAR ch = pszPage[ iOffset ];
		if ( ch )
		{
			iOffset++;
			if ( ch == '\n' || ch == '\r' || ch == '\t' )
				continue;	// ignore these.
			if ( ch == ';' )
			{
				break;	// end of command marker.
			}
			pszVerb[len++] = ch;
		}
		else
		{
			pszPage = pScriptItem->GetPageText( ++iPage );
			if ( pszPage == NULL || pszPage[0] == '\0' )
			{
				if ( ! len ) goto play_stop;
				break;
			}
			iOffset = 0;
		}
	}

	pszVerb[len] = '\0';
	pScriptItem->m_itNPCScript.m_ExecPage = iPage;
	pScriptItem->m_itNPCScript.m_ExecOffset = iOffset;

	// Execute the action.
	if ( len )
	{
		// Set the default time interval.
		if ( pszVerb[0] == 'T' && pszVerb[1] == '=' )
		{
			pszVerb += 2;
			pScriptItem->SetScriptTimeout( Exp_GetVal(pszVerb)); // store the last time interval here.
			len = 0;
			goto restart_read;
		}
		g_Serv.m_iModeCode = SERVMODE_ScriptBook;	// book mode. (lower my priv level) never do account stuff here.

		// NOTE: We should do a priv check on all verbs here.
		if ( g_Cfg.CanUsePrivVerb( this, pszVerb, &g_Serv ))
		{
			CScript sLine( pszVerb );
			if ( ! r_Verb( sLine, this ))
			{
				DEBUG_MSG(( "Bad Book Script verb '%s'" DEBUG_CR, pszVerb ));
				// should we stop ?
			}
		}
		g_Serv.m_iModeCode = SERVMODE_Run;
	}

	// When to check here again.
	pScriptItem->SetTimeout( pScriptItem->GetScriptTimeout());
}

bool CChar::NPC_LookAtCharGuard( CChar * pChar )
{
	// Does the guard hate the target ?

	if ( pChar->IsStatFlag( STATF_INVUL|STATF_DEAD ))
		return( false );
	if ( ! pChar->Noto_IsCriminal())
		return false;

	static UINT const sm_szSpeakGuardJeer[] =
	{
		DEFMSG_NPC_GUARD_THREAT_1,
		DEFMSG_NPC_GUARD_THREAT_2,
		DEFMSG_NPC_GUARD_THREAT_3,
		DEFMSG_NPC_GUARD_THREAT_4,
		DEFMSG_NPC_GUARD_THREAT_5,
	};

	if ( ! pChar->m_pArea->IsGuarded())
	{
		// At least jeer at the criminal.
		if ( Calc_GetRandVal(10))
			return( false );

		TCHAR *pszMsg = Str_GetTemp();
		sprintf(pszMsg, g_Cfg.GetDefaultMsg(sm_szSpeakGuardJeer[ Calc_GetRandVal( COUNTOF( sm_szSpeakGuardJeer )) ]), (LPCTSTR) pChar->GetName());
		Speak(pszMsg);
		UpdateDir( pChar );
		return false;
	}

	static UINT const sm_szSpeakGuardStrike[] =
	{
		DEFMSG_NPC_GUARD_STRIKE_1,
		DEFMSG_NPC_GUARD_STRIKE_2,
		DEFMSG_NPC_GUARD_STRIKE_3,
		DEFMSG_NPC_GUARD_STRIKE_4,
		DEFMSG_NPC_GUARD_STRIKE_5,
	};

	if ( GetTopDist3D( pChar ) > 1 )
	{
		if ( pChar->Skill_GetBase(SKILL_MAGERY))
		{
			Spell_Teleport( pChar->GetTopPoint(), false, false );
		}
		// If we got intant kill guards enabled, allow the guards
		// to take a swing directly after the teleport.
		if ( g_Cfg.m_fGuardsInstantKill )
		{
			Fight_Hit( pChar );
		}
	}
	if ( ! IsStatFlag( STATF_War ) || m_Act_Targ != pChar->GetUID())
	{
		Speak( g_Cfg.GetDefaultMsg(sm_szSpeakGuardStrike[ Calc_GetRandVal( COUNTOF( sm_szSpeakGuardStrike )) ]) );
		Fight_Attack( pChar );
	}
	return( true );
}

bool CChar::NPC_LookAtCharMonster( CChar * pChar )
{
	// return:
	//   true = take new action.
	//   false = continue with any previous action.
	//  motivation level =
	//  0 = not at all.
	//  100 = definitly.
	//

	int iFoodLevel = Food_GetLevelPercent();

	// Attacks those not of my kind.
	if ( ! Noto_IsCriminal() && iFoodLevel > 40 )		// I am not evil ?
	{
		return NPC_LookAtCharHuman( pChar );
	}

	// Attack if i am stronger.
	// or i'm just stupid.
	int iActMotivation = NPC_GetAttackMotivation( pChar );
	if ( iActMotivation <= 0 )
		return( false );
	if ( Fight_IsActive() && m_Act_Targ == pChar->GetUID())	// same targ.
		return( false );
	ASSERT( m_pNPC );
	if ( iActMotivation < m_pNPC->m_Act_Motivation )
		return( false );

	int iDist = GetTopDist3D( pChar );
	if ( IsStatFlag( STATF_Hidden ) &&
		! NPC_FightMayCast() &&
		iDist > 1 )
		return false;	// element of suprise.

	Fight_Attack( pChar );
	m_pNPC->m_Act_Motivation = iActMotivation;
	return true;
}

bool CChar::NPC_LookAtCharHuman( CChar * pChar )
{
	if ( pChar->IsStatFlag( STATF_DEAD ))
		return false;

	ASSERT( m_pNPC );

	if ( Noto_IsEvil())		// I am evil.
	{
		// Attack others if we are evil.
		return( NPC_LookAtCharMonster( pChar ));
	}

	if ( ! pChar->Noto_IsCriminal())	// not interesting.
		return( false );

	// Yell for guard if we see someone evil.
	if ( NPC_CanSpeak() &&
		m_pArea->IsGuarded() &&
		! Calc_GetRandVal( 3 ))
	{
		Speak( pChar->IsStatFlag( STATF_Criminal) ?
			 g_Cfg.GetDefaultMsg( DEFMSG_NPC_GENERIC_SEECRIM ) :
			 g_Cfg.GetDefaultMsg( DEFMSG_NPC_GENERIC_SEEMONS ) );
		// Find a guard.
		CallGuards( pChar );
		if ( IsStatFlag( STATF_War ))
			return( false );

		// run away like a coward.
		m_Act_Targ = pChar->GetUID();
		m_atFlee.m_iStepsMax = 20;	// how long should it take to get there.
		m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
		Skill_Start( NPCACT_FLEE );
		m_pNPC->m_Act_Motivation = 80;
		return true;
	}

	// Attack an evil creature ?

	return( false );
}

bool CChar::NPC_LookAtCharHealer( CChar * pChar )
{
	static LPCTSTR const sm_szHealerRefuseEvils[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_EVIL_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_EVIL_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_EVIL_3 ),
	};
	static LPCTSTR const sm_szHealerRefuseCriminals[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_CRIM_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_CRIM_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_CRIM_3 ),
	};
	static LPCTSTR const sm_szHealerRefuseGoods[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_GOOD_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_GOOD_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_GOOD_3 ),
	};
	static LPCTSTR const sm_szHealer[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_5 ),
	};

	if ( ! pChar->IsStatFlag( STATF_DEAD ))
		return false;

	UpdateDir( pChar );

	LPCTSTR pszRefuseMsg;

	int iDist = GetDist( pChar );
	if ( pChar->IsStatFlag( STATF_Insubstantial ))
	{
		pszRefuseMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_MANIFEST );
refuse_to_rez:
		if ( Calc_GetRandVal(5) || iDist > 3 )
			return false;
		Speak( pszRefuseMsg );
		return true;
	}

	if ( iDist > 3 )
	{
		if ( Calc_GetRandVal(5))
			return false;
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RANGE ) );
		return( true );
	}

	// What noto is this char to me ?
	bool ImEvil = Noto_IsEvil();
	bool ImNeutral = Noto_IsNeutral();
	NOTO_TYPE NotoThem = pChar->Noto_GetFlag( this, true );

	if ( !IsStatFlag( STATF_Criminal ) && NotoThem == NOTO_CRIMINAL )
	{
		pszRefuseMsg = sm_szHealerRefuseCriminals[ Calc_GetRandVal( COUNTOF( sm_szHealerRefuseCriminals )) ];
		goto refuse_to_rez;
	}

	if (( !ImNeutral && !ImEvil) && NotoThem >= NOTO_NEUTRAL )
	{
		pszRefuseMsg = sm_szHealerRefuseEvils[ Calc_GetRandVal( COUNTOF( sm_szHealerRefuseEvils )) ];
		goto refuse_to_rez;
	}

	if (( ImNeutral || ImEvil ) && NotoThem == NOTO_GOOD )
	{
		pszRefuseMsg = sm_szHealerRefuseGoods[ Calc_GetRandVal( COUNTOF( sm_szHealerRefuseGoods )) ];
		goto refuse_to_rez;
	}

	// Attempt to res.
	Speak( sm_szHealer[ Calc_GetRandVal( COUNTOF( sm_szHealer )) ] );
	UpdateAnimate( ANIM_CAST_AREA );
	if ( ! pChar->OnSpellEffect( SPELL_Resurrection, this, 1000, NULL ))
	{
		if ( Calc_GetRandVal(2))
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_FAIL_1 ) );
		else
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_FAIL_2 ) );
	}

	return true;
}

bool CChar::NPC_LookAtItem( CItem * pItem, int iDist )
{

	// I might want to go pickup this item ?
	if ( ! CanSee( pItem ))
		return( false );

	int iWantThisItem = NPC_WantThisItem(pItem);

	if ( IsSetEF( EF_New_Triggers ) )
	{
		if ( !pItem->IsAttr( ATTR_MOVE_NEVER ) )
		{

			CScriptTriggerArgs	Args( iDist, iWantThisItem, pItem );
			switch( OnTrigger( CTRIG_NPCLookAtItem, this, &Args ) )
			{
			case  TRIGRET_RET_TRUE:		return true;
			case  TRIGRET_RET_FALSE:	return false;
			}
			iWantThisItem = Args.m_iN2;
		}
	}

	//	loot the item if wish to loot it, and not already looted
	if ( iWantThisItem && ( Memory_FindObj( pItem ) == NULL ))
	{
		if ( Calc_GetRandVal(100) < iWantThisItem )
		{
			m_Act_Targ = pItem->GetUID();
			CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
			ASSERT(pObjTop);
			m_Act_TargPrv = pObjTop->GetUID();
			m_Act_p = pObjTop->GetTopPoint();
			m_atLooting.m_iDistEstimate = GetTopDist(pObjTop) * 2;
			m_atLooting.m_iDistCurrent = 0;
			Skill_Start( NPCACT_LOOTING );
			return true;
		}
	}

	CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	// check for doors we can open.
	if ( Stat_GetAdjusted(STAT_INT) > 20 &&
		pCharDef->Can(CAN_C_USEHANDS) &&
		pItem->IsType( IT_DOOR ) &&	// not locked.
		GetDist( pItem ) <= 1 &&
		CanTouch( pItem ) &&
		!Calc_GetRandVal(2))
	{
		// Is it opened or closed?
		if ( pItem->IsDoorOpen())
			return( false );

		// The door is closed.
		UpdateDir( pItem );
		if ( ! Use_Item( pItem ))	// try to open it.
			return( false );

		// Walk through it
		CPointMap pt = GetTopPoint();
		pt.MoveN( GetDir( pItem ), 2 );
		if ( CanMoveWalkTo( pt ))
		{
			m_Act_p = pt;
			NPC_WalkToPoint();
			return true;
		}
	}

	return( false );
}



bool CChar::NPC_LookAtChar( CChar * pChar, int iDist )
{
	// I see a char.
	// Do I want to do something to this char (more that what i'm already doing ?)
	// RETURN:
	//   true = yes i do want to take a new action.
	//

	if ( ! m_pNPC || !pChar )
		return( false );
	if ( pChar == this )
		return( false );

	if ( ! CanSeeLOS( pChar ))
		return( false );

	if ( IsSetEF( EF_New_Triggers ) )
	{
		switch( OnTrigger( CTRIG_NPCLookAtChar, pChar ) )
		{
		case  TRIGRET_RET_TRUE:		return true;
		case  TRIGRET_RET_FALSE:	return false;
		}
	}

	if ( NPC_IsOwnedBy( pChar, false ))
	{
		// pets should protect there owners unless told otherwise.
		if ( pChar->Fight_IsActive())
		{
			CChar * pCharTarg = pChar->m_Act_Targ.CharFind();
			if ( Fight_Attack(pCharTarg))
				return true;
		}

		// follow my owner again. (Default action)
		m_Act_Targ = pChar->GetUID();
		m_atFollowTarg.m_DistMin = 1;
		m_atFollowTarg.m_DistMax = 6;
		m_pNPC->m_Act_Motivation = 50;
		Skill_Start( NPCACT_FOLLOW_TARG );
		return true;
	}

	else
	{
		// initiate a conversation ?
		if ( ! IsStatFlag( STATF_War ) &&
			( Skill_GetActive() == SKILL_NONE || Skill_GetActive() == NPCACT_WANDER ) && // I'm idle
			pChar->m_pPlayer &&
			! Memory_FindObjTypes( pChar, MEMORY_SPEAK ))
		{
			if ( OnTrigger( CTRIG_NPCSeeNewPlayer, pChar ) != TRIGRET_RET_TRUE )
			{
				// record that we attempted to speak to them.
				CItemMemory * pMemory = Memory_AddObjTypes( pChar, MEMORY_SPEAK );
				pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_FIRSTSPEAK;
				// m_Act_Hear_Unknown = 0;
			}
		}
	}

	switch ( m_pNPC->m_Brain )	// my type of brain
	{
	case NPCBRAIN_GUARD:
		// Guards should look around for criminals or nasty creatures.
		if ( NPC_LookAtCharGuard( pChar ))
			return true;
		break;

	case NPCBRAIN_BEGGAR:
		if ( NPC_Act_Begging( pChar ))
			return true;
		if ( NPC_LookAtCharHuman( pChar ))
			return( true );
		break;

	case NPCBRAIN_MONSTER:
	case NPCBRAIN_UNDEAD:
	case NPCBRAIN_DRAGON:
		if ( NPC_LookAtCharMonster( pChar ))
			return( true );
		break;

	case NPCBRAIN_BESERK:
		// Blades or EV.
		// ??? Attack everyone you touch !
		if ( iDist <= CalcFightRange( m_uidWeapon.ItemFind() ) )
		{
			Fight_Hit( pChar );
		}
		if ( Fight_IsActive()) // Is this a better target than my last ?
		{
			CChar * pCharTarg = m_Act_Targ.CharFind();
			if ( pCharTarg != NULL )
			{
				if ( iDist >= GetTopDist3D( pCharTarg ))
					break;
			}
		}
		// if ( ! NPC_MotivationCheck( 50 )) continue;
		Fight_Attack( pChar );
		break;

	case NPCBRAIN_HEALER:
		// Healers should look around for ghosts.
		if ( NPC_LookAtCharHealer( pChar ))
			return( true );
		if ( NPC_LookAtCharHuman( pChar ))
			return( true );
		break;

	case NPCBRAIN_CRIER:
	case NPCBRAIN_BANKER:
	case NPCBRAIN_VENDOR:
	case NPCBRAIN_STABLE:
	case NPCBRAIN_ANIMAL:
	case NPCBRAIN_HUMAN:
	case NPCBRAIN_THIEF:
		if ( NPC_LookAtCharHuman( pChar ))
			return( true );
		break;
	}

	return( false );
}

bool CChar::NPC_LookAround( bool fForceCheckItems )
{
	// Take a look around for other people/chars.
	// We may be doing something already. So check current action motivation level.
	// RETURN:
	//   true = found something better to do.

	ASSERT( m_pNPC );
	if ( m_pNPC->m_Brain == NPCBRAIN_BESERK || ! Calc_GetRandVal(6))
	{
		// Make some random noise
		//
		SoundChar( Calc_GetRandVal(2) ? CRESND_RAND1 : CRESND_RAND2 );
	}

	int iRange = UO_MAP_VIEW_SIGHT;
	int iRangeBlur = iRange;

	CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef);

	// If I can't move don't look to far.
	if ( ! pCharDef->Can( CAN_C_WALK | CAN_C_FLY | CAN_C_SWIM ) ||
		IsStatFlag( STATF_Stone | STATF_Freeze ))
	{
		// I'm NOT mobile.
		if ( ! NPC_FightMayCast())	// And i have no distance attack.
		{
			iRange = 2;
			iRangeBlur = 2;
		}
	}
	else
	{
		// I'm mobile. do basic check if i would move here first.
		if ( ! NPC_CheckWalkHere( GetTopPoint(), m_pArea, 0 ))
		{
			// I should move. Someone lit a fire under me.
			m_Act_p = GetTopPoint();
			m_Act_p.Move( (DIR_TYPE) Calc_GetRandVal( DIR_QTY ));
			NPC_WalkToPoint( true );
			return( true );
		}

		if ( Stat_GetAdjusted(STAT_INT) < 50 )
			iRangeBlur = iRange/2;
	}

	// Lower the number of chars we look at if complex.
	if ( GetTopPoint().GetSector()->GetCharComplexity() > g_Cfg.m_iMaxCharComplexity / 4 )
		iRange /= 4;

	// Any interesting chars here ?
	CWorldSearch Area( GetTopPoint(), iRange );
	while (true)
	{
		CChar * pChar = Area.GetChar();
		if ( pChar == NULL )
			break;
		if ( pChar == this )	// just myself.
			continue;

		int iDist = GetTopDist3D( pChar );
		if ( iDist > iRangeBlur && ! pChar->IsStatFlag(STATF_Fly))
		{
			if ( Calc_GetRandVal(iDist))
				continue;	// can't see them.
		}
		if ( NPC_LookAtChar( pChar, iDist ))
			return( true );
	}

	// Check the ground for good stuff.
	if ( ! fForceCheckItems &&
		Stat_GetAdjusted(STAT_INT) > 10 &&
		! IsSkillBase( Skill_GetActive()) &&
		! Calc_GetRandVal( 3 ))
	{
		fForceCheckItems = true;
	}

	if ( fForceCheckItems )
	{
		CWorldSearch Area( GetTopPoint(), iRange );
		while (true)
		{
			CItem * pItem = Area.GetItem();
			if ( pItem == NULL )
				break;
			int iDist = GetTopDist3D( pItem );
			if ( iDist > iRangeBlur )
			{
				if ( Calc_GetRandVal(iDist))
					continue;	// can't see them.
			}
			if ( NPC_LookAtItem( pItem, iDist ))
				return( true );
		}
	}

	// Move stuff that is in our way ? (chests etc.)

	return m_pNPC->m_Act_Motivation; // Is this a better target than my last ?
}

void CChar::NPC_Act_Wander()
{
	// NPCACT_WANDER
	// just wander aimlessly. (but within bounds)
	// Stop wandering and re-eval frequently

	if ( ! Calc_GetRandVal( 7 + ( Stat_GetVal(STAT_DEX) / 30 )))
	{
		// Stop wandering ?
		Skill_Start( SKILL_NONE );
		return;
	}

	if ( Calc_GetRandVal( 2 ) )
	{
		if ( NPC_LookAround() )
			return;
	}

	// Staggering Walk around.
	m_Act_p = GetTopPoint();
	m_Act_p.Move( GetDirTurn( m_dirFace, 1 - Calc_GetRandVal(3)));

	ASSERT( m_pNPC );
	if ( m_pNPC->m_Home_Dist_Wander && m_ptHome.IsValidPoint())
	{
		if ( m_Act_p.GetDist( m_ptHome ) > m_pNPC->m_Home_Dist_Wander )
		{
			Skill_Start( NPCACT_GO_HOME );
			return;
		}
	}

	NPC_WalkToPoint();
}

bool CChar::NPC_Act_Follow( bool fFlee, int maxDistance, bool forceDistance )
{
	// Follow our target or owner. (m_Act_Targ) we may be fighting.
	// false = can't follow any more. give up.

	CChar * pChar = m_Act_Targ.CharFind();
	if ( pChar == NULL )
	{
		// free to do as i wish !
		Skill_Start( SKILL_NONE );
		return( false );
	}

	if ( IsSetEF( EF_New_Triggers ) )
	{
		CScriptTriggerArgs Args( fFlee, maxDistance, forceDistance );
		switch ( OnTrigger( CTRIG_NPCActFollow, pChar, &Args ) )
		{
			case TRIGRET_RET_TRUE:	return FALSE;
			case TRIGRET_RET_FALSE:	return TRUE;
		}
		fFlee			= (Args.m_iN1 != 0);
		maxDistance		= Args.m_iN2;
		forceDistance	= Args.m_iN3;
	}

	// Have to be able to see target to follow.
	if ( CanSee( pChar ))
	{
		m_Act_p = pChar->GetTopPoint();
	}
	else
	{
		// Monster may get confused because he can't see you.
		// There is a chance they could forget about you if hidden for a while.
		if ( ! Calc_GetRandVal( 1 + (( 100 - Stat_GetAdjusted(STAT_INT)) / 20 )))
			return( false );
		if ( fFlee )
			return( false );
	}

	int dist = GetTopPoint().GetDist( m_Act_p );
	if ( dist > UO_MAP_VIEW_RADAR*2 )			// too far away ?
		return( false );

	if ( forceDistance )
	{
		if ( dist < maxDistance )
		{
			// start moving away
			fFlee = true;
		}
	}
	else
	{
		if ( fFlee )
		{
			if ( dist >= maxDistance/*1*/ )
				return( false );
		}
		else
		{
			if ( dist <= maxDistance/*1*/ )
			{
				// I'm happy here ?
				// Random side step ? How jumpy an i ?
				//if ( IsStatFlag(STATF_War) && ! Calc_GetRandVal(3))
				//{
				//	NPC_WalkToPoint( Calc_GetRandVal(2));
				//}
				return( true );
			}
		}
	}

	if ( fFlee )
	{
		CPointMap ptOld = m_Act_p;
		m_Act_p = GetTopPoint();
		m_Act_p.Move( GetDirTurn( m_Act_p.GetDir( ptOld ), 4 + 1 - Calc_GetRandVal(3)));
		NPC_WalkToPoint( dist < Calc_GetRandVal(10));
		m_Act_p = ptOld;	// last known point of the enemy.
		return( true );
	}

	NPC_WalkToPoint( IsStatFlag( STATF_War ) ? true : ( dist > 3 ));
	return( true );
}

bool CChar::NPC_FightArchery( CChar * pChar )
{
	if ( Skill_GetActive() != SKILL_ARCHERY )
		return( false );

	int iDist = GetTopDist3D( pChar );
	if ( iDist > g_Cfg.m_iArcheryMaxDist )	// way too far away . close in.
		return( false );

	if ( iDist > g_Cfg.m_iArcheryMinDist )
		return( true );		// always use archery if distant enough

	if ( !Calc_GetRandVal( 2 ) )	// move away
	{
		// Move away
		NPC_Act_Follow( false, 5, true );
		return( true );
	}

	// Fine here.
	return( true );
}

bool CChar::NPC_FightMagery( CChar * pChar )
{
	// cast a spell if i can ?
	// or i can throw or use archery ?
	// RETURN:
	//  false = revert to melee type fighting.

	ASSERT( pChar );
	if ( ! NPC_FightMayCast())
		return( false );
	CChar	*pMageryTarget = pChar;

	int iDist = GetTopDist3D( pChar );
	if ( iDist > ((UO_MAP_VIEW_SIGHT*3)/4))	// way too far away . close in.
		return( false );

	if ( iDist <= 1 &&
		Skill_GetBase(SKILL_TACTICS) > 200 &&
		! Calc_GetRandVal(2))
	{
		// Within striking distance.
		// Stand and fight for a bit.
		return( false );
	}

	// A creature with a greater amount of mana will have a greater
	// chance of casting
	int iStatInt = Stat_GetAdjusted(STAT_INT);
	int iSkillVal = Skill_GetBase(SKILL_MAGERY);
	int mana	= Stat_GetVal(STAT_INT);
	int iChance = iSkillVal +
		(( mana >= ( iStatInt / 2 )) ? mana : ( iStatInt - mana ));
	if ( Calc_GetRandVal( iChance ) < 400 )
	{
		// we failed this test, but we could be casting next time
		// back off from the target a bit
		if ( mana > ( iStatInt / 3 ) && Calc_GetRandVal( iStatInt ))
		{
			if ( iDist < 4 || iDist > 8  )	// Here is fine?
			{
				NPC_Act_Follow( false, Calc_GetRandVal( 3 ) + 2, true );
			}
			return( true );
		}
		return( false );
	}

	// select proper spell.
	// defensive spells ???
	int imaxspell = min(( iSkillVal / 12 ) * 8, SPELL_BASE_QTY ) +1;

	// does the creature have a spellbook.
	CItem * pSpellbook = GetSpellbook();
	int i;

	CVarDefBase	*	pVar	= GetKey( "CASTSPELLS", true );

	if ( pVar )
		imaxspell	= i = GETINTRESOURCE( pVar->GetValNum() );
	else
		i = Calc_GetRandVal( imaxspell );

	for ( ; 1; i++ )
	{
		if ( i > imaxspell )	// didn't find a spell.
			return( false );

		SPELL_TYPE spell = (SPELL_TYPE) i;
		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef( spell );
		if ( pSpellDef == NULL )
			continue;

		if ( pSpellDef->IsSpellType(SPELLFLAG_DISABLED|SPELLFLAG_PLAYERONLY) ) continue;

		if ( pSpellbook )
		{
			if ( ! pSpellbook->IsSpellInBook(spell))
				continue;

			if ( ! pSpellDef->IsSpellType( SPELLFLAG_HARM ))
			{
				if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_CHAR) && pSpellDef->IsSpellType(SPELLFLAG_GOOD) )
				{
					//	help self or friends if needed. support 3 friends + self for castings
					bool	bSpellSuits = false;
					CChar	*pFriend[4];
					int		iFriendIndex = 0;
					CChar	*pTarget = NULL;

					//	since i scan the surface near me for this code, i need to be sure that it is neccessary
					if (( spell != SPELL_Heal ) && ( spell != SPELL_Great_Heal ) && ( spell != SPELL_Reactive_Armor ) &&
						( spell != SPELL_Cure ) && ( spell != SPELL_Protection ) && ( spell != SPELL_Bless ) &&
						( spell != SPELL_Magic_Reflect )) continue;

					pFriend[0] = this;
					pFriend[1] = pFriend[2] = pFriend[3] = NULL;
					iFriendIndex = 1;

					if ( g_Cfg.m_iNpcAi&NPC_AI_EXTRA )
					{
						//	search for the neariest friend in combat
						CWorldSearch AreaChars(GetTopPoint(), UO_MAP_VIEW_SIGHT);
						while ( true )
						{
							pTarget = AreaChars.GetChar();
							if ( !pTarget )
								break;

							CItem *pMemory = pTarget->Memory_FindObj(pChar);
							if ( pMemory && pMemory->IsMemoryTypes(MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY) )
							{
								pFriend[iFriendIndex++] = pTarget;
								if ( iFriendIndex >= 4 ) break;
							}
						}
					}

					//	i cannot cast this on self. ok, then friends only
					if ( pSpellDef->IsSpellType(SPELLFLAG_TARG_NOSELF) )
					{
						pFriend[0] = pFriend[1];
						pFriend[1] = pFriend[2];
						pFriend[2] = pFriend[3];
						pFriend[3] = NULL;
					}
					for ( iFriendIndex = 0; iFriendIndex < 4; iFriendIndex++)
					{
						pTarget = pFriend[iFriendIndex];
						if ( !pTarget ) break;
						//	check if the target need that
						switch ( spell )
						{
						case SPELL_Heal:
						case SPELL_Great_Heal:
							if ( pTarget->Stat_GetVal(STAT_STR) < pTarget->Stat_GetAdjusted(STAT_STR)/3 ) bSpellSuits = true;
							break;
						case SPELL_Reactive_Armor:
							if ( pTarget->LayerFind(LAYER_SPELL_Reactive) == NULL ) bSpellSuits = true;
							break;
						case SPELL_Cure:
							if ( pTarget->LayerFind(LAYER_FLAG_Poison) != NULL ) bSpellSuits = true;
							break;
						case SPELL_Protection:
							if ( pTarget->LayerFind(LAYER_SPELL_Protection) == NULL ) bSpellSuits = true;
							break;
						case SPELL_Bless:
							if ( pTarget->LayerFind(LAYER_SPELL_STATS) == NULL ) bSpellSuits = true;
							break;
						case SPELL_Magic_Reflect:
							if ( pTarget->LayerFind(LAYER_SPELL_Magic_Reflect) == NULL ) bSpellSuits = true;
							break;
						}

						if ( bSpellSuits ) break;
					}
					if ( bSpellSuits && Spell_CanCast(spell, true, this, false) )
					{
						if (( pTarget != this ) && ( g_Cfg.m_wDebugFlags & DEBUGF_NPCAI ))
						{
							DEBUG_ERR(("%s helping friend %s in combat casting spell %d" DEBUG_CR,
								GetName(), pTarget->GetName(), spell));
						}

						pMageryTarget = pTarget;
						m_atMagery.m_Spell = spell;
						break;
					}
					continue;
				}
				else if ( pSpellDef->IsSpellType(SPELLFLAG_SUMMON) )
				{
					//	spell is good, but does not harm. the target should obey me. hoping sphere can do this ;)
					switch ( spell )
					{
					case SPELL_Air_Elem:
					case SPELL_Daemon:
					case SPELL_Earth_Elem:
					case SPELL_Fire_Elem:
					case SPELL_Water_Elem:
					case SPELL_Summon_Undead:
						break;
					default:
						continue;
					}
				}

			}
		}
		else
		{
			if ( !pVar && !pSpellDef->IsSpellType( SPELLFLAG_HARM ))
				continue;

			// less chance for berserker spells
			if ( pSpellDef->IsSpellType( SPELLFLAG_SUMMON ) && Calc_GetRandVal( 2 ))
				continue;

			// less chance for field spells as well
			if ( pSpellDef->IsSpellType( SPELLFLAG_FIELD ) && Calc_GetRandVal( 4 ))
				continue;
		}

		if ( ! Spell_CanCast( spell, true, this, false ))
			continue;

		m_atMagery.m_Spell = spell;
		break;	// I like this spell.
	}

	// KRJ - give us some distance
	// if the opponent is using melee
	// the bigger the disadvantage we have in hitpoints, the further we will go
	if ( mana > iStatInt / 3 && Calc_GetRandVal( iStatInt << 1 ))
	{
		if ( iDist < 4 || iDist > 8  )	// Here is fine?
		{
			NPC_Act_Follow( false, 5, true );
		}
	}
	else NPC_Act_Follow();

	Reveal();

	m_Act_Targ = pMageryTarget->GetUID();
	m_Act_TargPrv = GetUID();	// I'm casting this directly.
	m_Act_p = pMageryTarget->GetTopPoint();

	// Calculate the difficulty
	return( Skill_Start( SKILL_MAGERY ));
}

void CChar::NPC_Act_Fight()
{
	// I am in an attack mode.
	DEBUG_CHECK( IsTopLevel());
	ASSERT( m_pNPC );

	if ( ! Fight_IsActive())	// we did something else i guess.
		return;

	// Review our targets periodically.
	if ( ! IsStatFlag(STATF_Pet) ||
		m_pNPC->m_Brain == NPCBRAIN_BESERK )
	{
		int iObservant = ( 130 - Stat_GetAdjusted(STAT_INT)) / 20;
		if ( ! Calc_GetRandVal( 2 + max( 0, iObservant )))
		{
			if ( NPC_LookAround())
				return;
		}
	}

	CChar * pChar = m_Act_Targ.CharFind();
	if ( pChar == NULL || ! pChar->IsTopLevel()) // target is not valid anymore ?
		return;
	int iDist = GetDist( pChar );

	if ( m_pNPC->m_Brain == NPCBRAIN_GUARD &&
		m_atFight.m_War_Swing_State == WAR_SWING_READY &&
		! Calc_GetRandVal(3))
	{
		// If a guard is ever too far away (missed a chance to swing)
		// Teleport me closer.
		NPC_LookAtChar( pChar, iDist );
	}


	// If i'm horribly damaged and smart enough to know it.
	int iMotivation = NPC_GetAttackMotivation( pChar );

	bool		fSkipHardcoded	= false;
	if ( IsSetEF( EF_New_Triggers ) )
	{
		CScriptTriggerArgs Args( iDist, iMotivation );
		switch ( OnTrigger( CTRIG_NPCActFight, pChar, &Args ) )
		{
			case TRIGRET_RET_TRUE:	return;
			case TRIGRET_RET_FALSE:	fSkipHardcoded	= true;	break;
		}
		iDist		= Args.m_iN1;
		iMotivation	= Args.m_iN2;
	}

	if ( ! IsStatFlag(STATF_Pet))
	{
		if ( iMotivation < 0 )
		{
			m_atFlee.m_iStepsMax = 20;	// how long should it take to get there.
			m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
			Skill_Start( NPCACT_FLEE );	// Run away!
			return;
		}
	}



	// Can only do that with full stamina !
	if ( !fSkipHardcoded && Stat_GetVal(STAT_DEX) >= Stat_GetAdjusted(STAT_DEX))
	{
		// If I am a dragon maybe I will breath fire.
		// NPCACT_BREATH
		if ( m_pNPC->m_Brain == NPCBRAIN_DRAGON &&
			iDist >= 1 &&
			iDist <= 8 &&
			CanSeeLOS( pChar ))
		{
			UpdateDir( pChar );
			Skill_Start( NPCACT_BREATH );
			return;
		}

		// If I am a giant. i can throw stones.
		// NPCACT_THROWING

		if (( GetDispID() == CREID_OGRE ||
			GetDispID() == CREID_ETTIN ||
			GetDispID() == CREID_Cyclops ) &&
			iDist >= 2 &&
			iDist <= 9 &&
			CanSeeLOS( pChar ) &&
			ContentFind( RESOURCE_ID(RES_TYPEDEF,IT_AROCK), 0, 2 ))
		{
			UpdateDir( pChar );
			Skill_Start( NPCACT_THROWING );
			return;
		}
	}

	// Maybe i'll cast a spell if I can. if so maintain a distance.
	if ( NPC_FightMagery( pChar ))
		return;

	if ( NPC_FightArchery( pChar ))
		return;

	// Move in for melee type combat.
	NPC_Act_Follow( false, CalcFightRange( m_uidWeapon.ItemFind() ), false );
}

bool CChar::NPC_Act_Talk()
{
	// NPCACT_TALK:
	// NPCACT_TALK_FOLLOW
	// RETURN:
	//  false = do something else. go Idle
	//  true = just keep waiting.

	CChar * pChar = m_Act_Targ.CharFind();
	if ( pChar == NULL )	// they are gone ?
		return( false );

	// too far away.
	int iDist = GetTopDist3D( pChar );
	if ( iDist >= UO_MAP_VIEW_SIGHT )	// give up.
		return( false );

	if ( Skill_GetActive() == NPCACT_TALK_FOLLOW && iDist > 3 )
	{
		// try to move closer.
		if ( ! NPC_Act_Follow( false, 4, false ))
			return( false );
	}

	if ( m_atTalk.m_WaitCount <= 1 )
	{
		if ( NPC_CanSpeak())
		{
			static LPCTSTR const sm_szText[] =
			{
				g_Cfg.GetDefaultMsg( DEFMSG_NPC_GENERIC_GONE_1 ),
				g_Cfg.GetDefaultMsg( DEFMSG_NPC_GENERIC_GONE_2 ),
			};
			TCHAR *pszMsg = Str_GetTemp();
			sprintf(pszMsg, sm_szText[ Calc_GetRandVal( COUNTOF( sm_szText )) ], (LPCTSTR) pChar->GetName() );
			Speak(pszMsg);
		}
		return( false );
	}

	m_atTalk.m_WaitCount--;
	return( true );	// just keep waiting.
}

void CChar::NPC_Act_GoHome()
{
	// NPCACT_GO_HOME
	// If our home is not valid then

	if ( ! Calc_GetRandVal( 2 ))
	{
		if ( NPC_LookAround() )
			return;
	}

	ASSERT( m_pNPC );
	if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )
	{
		// Had to change this guards were still roaming the forests
		// this goes hand in hand with the change that guards arent
		// called if the criminal makes it outside guarded territory.

		const CRegionBase * pArea = m_ptHome.GetRegion( REGION_TYPE_AREA );
		if ( pArea && pArea->IsGuarded())
		{
			if ( ! m_pArea->IsGuarded())
			{
				if ( Spell_Teleport( m_ptHome, false, false ))
				{
					Skill_Start( SKILL_NONE );
					return;
				}
			}
		}
		else
		{
			g_Log.Event( LOGL_WARN, "Guard 0%lx '%s' has no guard post (%s)!" DEBUG_CR, (DWORD) GetUID(), (LPCTSTR) GetName(), (LPCTSTR) GetTopPoint().WriteUsed());


			// If we arent conjured and still got no valid home
			// then set our status to conjured and take our life.
			if ( ! IsStatFlag(STATF_Conjured))
			{
				StatFlag_Set( STATF_Conjured );
				Stat_SetVal(STAT_STR, -1000);
				return;
			}
		}

		// else we are conjured and probably a timer started already.
	}

	if ( ! m_ptHome.IsValidPoint() || !GetTopPoint().IsValidPoint() )
	{
   		Skill_Start( SKILL_NONE );
		return;
	}

	if ( g_Cfg.m_iLostNPCTeleport )
	{
		int	iDistance	= m_ptHome.GetDist( GetTopPoint() );
		if (   iDistance > g_Cfg.m_iLostNPCTeleport
			&& iDistance > m_pNPC->m_Home_Dist_Wander )
		{
			if ( !IsSetEF(EF_Minimize_Triggers) )
			{
				CScriptTriggerArgs Args(iDistance);	// ARGN1 - distance
				if ( OnTrigger(CTRIG_NPCLostTeleport, this, &Args) != TRIGRET_RET_TRUE )
					goto doteleporthome;
			}
			else
			{
doteleporthome:
				Spell_Teleport(m_ptHome, true, false);
			}
		}
	}

   	m_Act_p = m_ptHome;
   	if ( ! NPC_WalkToPoint()) // get there
   	{
   		Skill_Start( SKILL_NONE );
		return;
	}

	if ( ! Calc_GetRandVal(40)) // give up...
	{
		// Some NPCs should be able to just teleport back home...
		switch( m_pNPC->m_Brain )
		{
		case NPCBRAIN_VENDOR:
		case NPCBRAIN_STABLE:
		case NPCBRAIN_BANKER:
		case NPCBRAIN_HEALER:
		case NPCBRAIN_CRIER:
			// if ( ! Calc_GetRandVal(100)) // give up...
			{
				Spell_Teleport( m_ptHome, true, false );
			}
		default:
			Skill_Start( SKILL_NONE );
			break;
		}
	}
}

void CChar::NPC_LootMemory( CItem * pItem )
{
	// Create a memory of this item.
	// I have already looked at it.
	CItem * pMemory = Memory_AddObjTypes( pItem, MEMORY_SPEAK );
	ASSERT(pMemory);
	pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_IGNORE;

	// If the item is set to decay.
	if ( pItem->IsTimerSet() && ! pItem->IsTimerExpired())
	{
		pMemory->SetTimeout( pItem->GetTimerDiff());	// We'll forget about this once the item is gone
	}
}

bool CChar::NPC_LootContainer( CItemContainer * pContainer )
{
	// Go through the pack and see if there is anything I want there...
	CItem * pNext;
	CItem * pLoot = pContainer->GetContentHead();
	for ( ; pLoot != NULL; pLoot = pNext )
	{
		pNext = pLoot->GetNext();
		// Have I checked this one already?
		if ( Memory_FindObj( pLoot ))
			continue;

		if ( pLoot->IsContainer())
		{
			// Loot it as well
			if ( ! NPC_LootContainer( dynamic_cast <CItemContainer *> (pLoot)))
			{
				// Not finished with it
				return false;
			}
		}

		if ( ! NPC_WantThisItem( pLoot ))
			continue;

		// How much can I carry
		if ( CanCarry( pLoot ))
		{
			UpdateAnimate( ANIM_BOW );
			ItemEquip( pLoot );
		}
		else
		{
			// Can't carry the whole stack...how much can I carry?
			NPC_LootMemory( pLoot );
		}

		// I can only pick up one thing at a time, so come back here on my next tick
		SetTimeout( 1 );	// Don't go through things so fast.
		return false;
	}

	// I've gone through everything here...remember that we've looted this container
	NPC_LootMemory( pContainer );
	return true;
}

void CChar::NPC_Act_Looting()
{
	// NPCACT_LOOTING
	// We have seen something good that we want. checking it out now.
	// We just killed something, so we should see if it has anything interesting on it.
	// Find the corpse first

	CItem * pItem = m_Act_Targ.ItemFind();
	if ( pItem == NULL )
	{
		Skill_Start( SKILL_NONE );
		return;
	}

	ASSERT( m_pNPC );

	CObjBaseTemplate * pObjTop = pItem->GetTopLevelObj();
	ASSERT(pObjTop);

	if ( m_Act_TargPrv != pObjTop->GetUID() ||
		m_Act_p != pObjTop->GetTopPoint())
	{
		// It moved ?
cantgetit:
		// give up on this.
		NPC_LootMemory( pItem );
		Skill_Start( SKILL_NONE );
		return;
	}

	if ( GetDist( pItem ) > 1 )	// move toward it.
	{
		if ( ++(m_atLooting.m_iDistCurrent) > m_atLooting.m_iDistEstimate )
		{
			goto cantgetit;
		}
		NPC_WalkToPoint();
		return;
	}

	// Have I already searched this one?
	DEBUG_CHECK( ! Memory_FindObj( pItem ));

	if ( pItem->IsTypeLocked())
	{
		goto cantgetit;
	}

	if ( pItem->IsType(IT_CORPSE))
	{
		// Did I kill this one?
		if ( pItem->m_itCorpse.m_uidKiller != GetUID())
		{
			// Wasn't me...less chance of actually looting it.
			if ( Calc_GetRandVal( 4 ))
			{
				goto cantgetit;
			}
		}
	}

	// Can i reach the object that i want ?
	if ( ! CanTouch( pItem ))
	{
		goto cantgetit;
	}

	// I can reach it
	CItemCorpse * pCorpse = dynamic_cast <CItemCorpse *> ( pItem );
	if ( pCorpse )
	{
		if ( ! NPC_LootContainer( pCorpse ))
			return;

		// Eat raw meat ? or just evil ?
		if ( m_pNPC->m_Brain == NPCBRAIN_MONSTER ||
			m_pNPC->m_Brain == NPCBRAIN_ANIMAL ||
			m_pNPC->m_Brain == NPCBRAIN_DRAGON )
		{
			// Only do this if it has a resource type we want...
			if ( pCorpse->m_itCorpse.m_timeDeath.IsTimeValid() )
			{
				Use_CarveCorpse( pCorpse );
				Skill_Start( NPCACT_LOOKING );
				return;
			}
		}
	}
	else
	{
		if ( ! CanCarry( pItem ))
		{
			goto cantgetit;
		}

		// can i eat it on the ground ?

		UpdateAnimate( ANIM_BOW );
		ItemBounce( pItem );
	}

	// Done looting
	// We might be looting this becuase we are hungry...
	// What was I doing before this?

	Skill_Start( SKILL_NONE );
}

void CChar::NPC_Act_Flee()
{
	// NPCACT_FLEE
	// I should move faster this way.
	// ??? turn to strike if they are close.
	if ( ++ m_atFlee.m_iStepsCurrent >= m_atFlee.m_iStepsMax )
	{
		Skill_Start( SKILL_NONE );
		return;
	}
	if ( ! NPC_Act_Follow( true, m_atFlee.m_iStepsMax ))
	{
		Skill_Start( SKILL_NONE );
		return;
	}
}

void CChar::NPC_Act_Goto()
{
	// NPCACT_GOTO:
	// Still trying to get to this point.

	switch ( NPC_WalkToPoint())
	{
	case 0:
		// We reached our destination
		NPC_Act_Idle();	// look for something new to do.
		break;
	case 1:
		// Took a step....keep trying to get there.
		break;
	case 2:
		// Give it up...
		// Go directly there...
		if ( m_Act_p.IsValidPoint() &&
			IsHuman() &&
			!IsStatFlag( STATF_Freeze|STATF_Stone ))
			Spell_Teleport( m_Act_p, true, false);
		else
			NPC_Act_Idle();	// look for something new to do.
		break;
	}
}

void CChar::NPC_Act_Idle()
{
	// Free to do as we please. decide what we want to do next.
	// Idle NPC's should try to take some action.

	ASSERT( m_pNPC );
	m_pNPC->m_Act_Motivation = 0;	// we have no motivation to do anything.

	// Look around for things to do.
	if ( NPC_LookAround() )
		return;

	// ---------- If we found nothing else to do. do this. -----------

	// If guards are found outside guarded territories, do the following.
	if ( m_pNPC->m_Brain == NPCBRAIN_GUARD &&
		! m_pArea->IsGuarded() &&
		m_ptHome.IsValidPoint())
	{
		Skill_Start( NPCACT_GO_HOME );
		return;
	}

	// Specific creature random actions.
	if ( Stat_GetVal(STAT_DEX) >= Stat_GetAdjusted(STAT_DEX) && ! Calc_GetRandVal( 3 ))
	{
		switch ( GetDispID())
		{
		case CREID_FIRE_ELEM:
			if ( ! g_World.IsItemTypeNear( GetTopPoint(), IT_FIRE ))
			{
				Action_StartSpecial( CREID_FIRE_ELEM );
			}
			return;
		case CREID_GIANT_SPIDER:
			if ( ! g_World.IsItemTypeNear( GetTopPoint(), IT_WEB ))
			{
				Action_StartSpecial( CREID_GIANT_SPIDER );
			}
			return;
		}
	}

	if ( m_ptHome.IsValidPoint() && ! Calc_GetRandVal( 15 ))
	{
		// Periodically head home.
		Skill_Start( NPCACT_GO_HOME );
		return;
	}

	if ( Skill_GetBase(SKILL_HIDING) > 30 &&
		! Calc_GetRandVal( 15 - Skill_GetBase(SKILL_HIDING)/100) &&
		! m_pArea->IsGuarded())
	{
		// Just hide here.
		if ( IsStatFlag( STATF_Hidden ))
			return;
		Skill_Start( SKILL_HIDING );
		return;
	}
	if ( Calc_GetRandVal( 100 - Stat_GetAdjusted(STAT_DEX)) < 25 )
	{
		// dex determines how jumpy they are.
		// Decide to wander about ?
		Skill_Start( NPCACT_WANDER );
		return;
	}

	// just stand here for a bit.
	Skill_Start( SKILL_NONE );
	SetTimeout( TICK_PER_SEC * 1 + Calc_GetRandVal(TICK_PER_SEC*2) );
}

bool CChar::NPC_OnItemGive( CChar * pCharSrc, CItem * pItem )
{
	// Someone (Player) is giving me an item.
	// RETURN: true = accepted.

	ASSERT( pCharSrc );

	if ( m_pNPC == NULL )
		return( false );	// must just be an offline player.

	CScriptTriggerArgs Args( pItem );
	if ( !IsSetEF(EF_Minimize_Triggers) )
	{
		if ( OnTrigger( CTRIG_ReceiveItem, pCharSrc, &Args ) == TRIGRET_RET_TRUE )
			return( true );
	}

	if ( NPC_IsOwnedBy( pCharSrc ))
	{
		// Giving something to my own pet.
		if ( pCharSrc->IsPriv( PRIV_GM ))
		{
			return( ItemEquip( pItem ) );
		}

		// Take stuff from my master.
		if ( NPC_IsVendor())
		{
			if ( pItem->IsType(IT_GOLD))
			{
				Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_MONEY ) );
				NPC_OnHirePayMore(pItem);
			}
			else
			{
				Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_SELL ) );
				GetBank(LAYER_VENDOR_STOCK)->ContentAdd( pItem );
			}
			return true;
		}

		// Human hireling
		if ( NPC_CanSpeak())
		{
			if ( Food_CanEat(pItem))
			{
				if ( NPC_WantThisItem( pItem ) &&
					Use_Eat( pItem ))
				{
					Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_FOOD_TY ) );
					return( true );
				}
				else
				{
					Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_FOOD_NO ) );
					return( false );
				}
			}
			if ( ! CanCarry( pItem ))
			{
				Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_WEAK ) );
				return( false );
			}
			if ( pItem->IsType(IT_GOLD))
			{
				// Pay me for my work ? (hirelings)
				Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_THANKS ) );
				// NPC_OnHirePayMore(pItem);
				// return;
			}

			if ( Use_Item( pItem ))
				return true;

			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_DROP ) );
			GetPackSafe()->ContentAdd( pItem );
			return( true );
		}

		switch ( pItem->GetType())
		{
		case IT_POTION:
		case IT_DRINK:
		case IT_PITCHER:
		case IT_WATER_WASH:
		case IT_BOOZE:
			if ( Use_Item( pItem ))
				return true;
		}
	}

	// Does the NPC know you ?
	CItemMemory * pMemory = Memory_FindObj( pCharSrc );
	if ( pMemory )
	{
		// Am i being paid for something ?
		if ( pItem->IsType(IT_GOLD))
		{
			if ( pMemory->m_itEqMemory.m_Action == NPC_MEM_ACT_SPEAK_TRAIN )
			{
				return( NPC_OnTrainPay( pCharSrc, pMemory, pItem ));
			}
			if ( pMemory->m_itEqMemory.m_Action == NPC_MEM_ACT_SPEAK_HIRE )
			{
				return( NPC_OnHirePay( pCharSrc, pMemory, pItem ));
			}
		}
	}

	if ( ! NPC_WantThisItem( pItem ))
	{
		if ( OnTrigger( CTRIG_NPCRefuseItem, pCharSrc, &Args ) == TRIGRET_RET_TRUE )
			return false;

		pCharSrc->GetClient()->addObjMessage( g_Cfg.GetDefaultMsg( DEFMSG_NPC_GENERIC_DONTWANT ), this );
		return( false );
	}

	if ( pItem->IsType(IT_GOLD))
	{
		if ( m_pNPC->m_Brain == NPCBRAIN_BANKER )
		{
			// I shall put this item in your bank account.
			TCHAR *pszMsg = Str_GetTemp();
			sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_NPC_BANKER_DEPOSIT), pItem->GetAmount());
			Speak(pszMsg);
			pCharSrc->GetBank()->ContentAdd( pItem );
			return( true );
		}

		if ( NPC_CanSpeak())
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_GENERIC_THANKS ) );
		}

		ItemEquip( pItem );
		pItem->Update();
		return( true );
	}

	if ( NPC_IsVendor())
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_VENDOR_SELL ) );
		return( false );
	}

	// The NPC might want it ?
	switch ( m_pNPC->m_Brain )
	{
	case NPCBRAIN_DRAGON:
	case NPCBRAIN_ANIMAL:
	case NPCBRAIN_MONSTER:
		// Might want food ?
		if ( Food_CanEat(pItem))
		{
			// ??? May know it is poisoned ?
			// ??? May not be hungry
			if ( Use_Eat( pItem, pItem->GetAmount() ))
				return( true );
		}
		break;

	case NPCBRAIN_BEGGAR:
	case NPCBRAIN_THIEF:
		if ( Food_CanEat(pItem) &&
			Use_Eat( pItem ))
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_BEGGAR_FOOD_TY ) );
			return( true );
		}
		if ( ! CanCarry( pItem ))
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_WEAK ) );
			return( false );
		}
		if ( pItem->IsType(IT_GOLD) || Food_CanEat(pItem))
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_BEGGAR_FOOD_TAL ) );
		else
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_BEGGAR_SELL ) );
		ItemEquip( pItem );
		pItem->Update();
		if (m_Act_Targ == pCharSrc->GetUID())
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_BEGGAR_IFONLY ) );
			m_Act_TargPrv = m_Act_Targ;
			m_Act_Targ.InitUID();
			Skill_Start( SKILL_NONE );
		}
		return( true );
	}

	if ( OnTrigger( CTRIG_NPCAcceptItem, pCharSrc, &Args ) == TRIGRET_RET_TRUE )
		return true;

	ItemBounce( pItem );
	return( true );
}

bool CChar::NPC_OnTickFood( int nFoodLevel )
{
	// Check Food usage.
	// Are we hungry enough to take some new action ?
	// RETURN: true = we have taken an action.

   	if ( IsStatFlag( STATF_Pet ))
   	{
		TCHAR *pszMsg = Str_GetTemp();
   		sprintf(pszMsg, "looks %s", (LPCTSTR) Food_GetLevelMessage(true, false));
   		Emote(pszMsg, GetClient());
   		SoundChar( CRESND_RAND2 );

		if ( nFoodLevel <= 0 )
		{
			// How happy are we with being a pet ?
			NPC_PetDesert();
			return true;
		}
   	}

	if ( IsStatFlag(STATF_Stone|STATF_Freeze|STATF_DEAD|STATF_Sleeping) ) return false;
	SoundChar(CRESND_RAND2);
	return( true );
}

void CChar::NPC_OnTickAction()
{
	static LPCTSTR const excInfo[] =
	{
		"",					// 0
		"fighting skill",
		"idle",
		"begging",
		"hiding",
		"stealth",
		"fighting",
		"follow target",
		"going to",
		"wandering",
		"flying away",		// 10
		"talking",
		"talking idle",
		"going home",
		"looting",
		"looking around",
		"idle looking",
		"set new timer",
	};
	const char *zTemp = excInfo[0];

	// Our action timer has expired.
	// last skill or task might be complete ?
	// What action should we take now ?
#if !defined( _DEBUG ) && !defined( NO_INTERNAL_EXCEPTIONS )
	try
	{
#endif
		SKILL_TYPE iSkillActive	= Skill_GetActive();
		if ( g_Cfg.IsSkillFlag( iSkillActive, SKF_SCRIPTED ) )
		{
			// SCRIPTED SKILL OnTickAction
		}
		else if ( g_Cfg.IsSkillFlag( iSkillActive, SKF_FIGHT ) )
		{
			zTemp = excInfo[1];
			NPC_Act_Fight();
		}
		else switch ( iSkillActive )
		{
		case SKILL_NONE:
			// We should try to do something new.
			zTemp = excInfo[2];
			NPC_Act_Idle();
			break;

		case SKILL_BEGGING:
			zTemp = excInfo[3];
			NPC_Act_Begging( NULL );
			break;

		case SKILL_STEALTH:
		case SKILL_HIDING:
			// We are currently hidden.
			zTemp = excInfo[4];
			if ( NPC_LookAround())
				break;
			// just remain hidden unless we find something new to do.
			if ( Calc_GetRandVal( Skill_GetBase(SKILL_HIDING)))
				break;
			zTemp = excInfo[5];
			NPC_Act_Idle();
			break;

		case SKILL_ARCHERY:
		case SKILL_FENCING:
		case SKILL_MACEFIGHTING:
		case SKILL_SWORDSMANSHIP:
		case SKILL_WRESTLING:
			// If we are fighting . Periodically review our targets.
			zTemp = excInfo[6];
			NPC_Act_Fight();
			break;

		case NPCACT_FOLLOW_TARG:
		case NPCACT_GUARD_TARG:
			// continue to follow our target.
			zTemp = excInfo[7];
			NPC_LookAtChar( m_Act_Targ.CharFind(), 1 );
			NPC_Act_Follow();
			break;
		case NPCACT_STAY:
			// Just stay here til told to do otherwise.
			break;
		case NPCACT_GOTO:
			zTemp = excInfo[8];
			NPC_Act_Goto();
			break;
		case NPCACT_WANDER:
			zTemp = excInfo[9];
			NPC_Act_Wander();
			break;
		case NPCACT_FLEE:
			zTemp = excInfo[10];
			NPC_Act_Flee();
			break;
		case NPCACT_TALK:
		case NPCACT_TALK_FOLLOW:
			// Got bored just talking to you.
			zTemp = excInfo[11];
			if ( ! NPC_Act_Talk())
			{
				zTemp = excInfo[12];
				NPC_Act_Idle();	// look for something new to do.
			}
			break;
		case NPCACT_GO_HOME:
			zTemp = excInfo[13];
			NPC_Act_GoHome();
			break;
		case NPCACT_LOOTING:
			zTemp = excInfo[14];
			NPC_Act_Looting();
			break;
		case NPCACT_LOOKING:
			zTemp = excInfo[15];
			if ( NPC_LookAround( true ) )
				break;
			zTemp = excInfo[16];
			NPC_Act_Idle();
			break;

		default:
			if ( ! IsSkillBase( iSkillActive ) )	// unassigned skill ? that's weird
			{
				Skill_Start( SKILL_NONE );
			}
			break;
		}

		zTemp = excInfo[17];
		if ( IsTimerExpired() )	// Was not reset ?
		{
			int		timeout	= (150-Stat_GetAdjusted(STAT_DEX))/2;
			if ( timeout < 0 )
				timeout	= 0;
			timeout		= timeout / 2 + Calc_GetRandVal( timeout /2 );
			// default next brain/move tick
			SetTimeout( TICK_PER_SEC + timeout * TICK_PER_SEC / 10 );
		}
#if !defined( _DEBUG ) && !defined( NO_INTERNAL_EXCEPTIONS )
	}
	catch ( CGrayError &e )
	{
		g_Log.CatchEvent( &e, "NPC Char OnTick (%s) (UID=0%lx)", zTemp, (DWORD) GetUID());
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent( NULL, "NPC Char OnTick (%s) (UID=0%lx)", zTemp, (DWORD) GetUID());
	}
#endif
}

void CChar::NPC_Pathfinding()
{
	EXC_TRY(("NPC_Pathfinding()"));

	CPointMap	local = GetTopPoint();
	int			i;
	int			iInt = Stat_GetAdjusted(STAT_INT);
	CPointMap	pTarg = m_Act_p;
	int			dist = local.GetDist(pTarg);

	//	do we really need to find the path?
	if ( iInt < 75 ) return;					// too dumb
	if ( !pTarg.IsValidPoint() ) return;		// invalid point
	if (( pTarg.m_x == local.m_x ) && ( pTarg.m_y == local.m_y )) return; // same spot
	if ( pTarg.m_map != local.m_map ) return;	// cannot just move to another map
	if ( dist >= PATH_SIZE/2 ) return;			// skip too far locations which should be too slow
	if ( dist < 2 ) return;						// skip too low distance (1 step) - good in default
												// pathfinding is buggy near the edges of the map,
												// so do not use it there
	if (( local.m_x <= PATH_SIZE/2 ) || ( local.m_y <= PATH_SIZE/2 ) ||
		( local.m_x >= ( g_MapList.GetX(local.m_map) - PATH_SIZE/2) ) ||
		( local.m_y >= ( g_MapList.GetY(local.m_map) - PATH_SIZE/2) ))
		return;
												// need 300 int at least to pathfind each step, but always
												// search if this is a first step
	if (( Calc_GetRandVal(300) > iInt ) && ( m_pNPC->m_nextX[0] )) return;

	if ( g_Cfg.m_wDebugFlags & DEBUGF_NPCAI )
	{
		DEBUG_ERR(("NPC_Pathfinding %s (int=%d): [%d,%d,%d,%d] -> [%d,%d,%d,%d]" DEBUG_CR,
			GetName(), iInt,
			local.m_x, local.m_y, local.m_z, local.m_map,
			pTarg.m_x, pTarg.m_y, pTarg.m_z, pTarg.m_map));
	}

	//	clear saved steps list
	memset(m_pNPC->m_nextX, 0, sizeof(m_pNPC->m_nextX));
	memset(m_pNPC->m_nextY, 0, sizeof(m_pNPC->m_nextY));

	//	proceed with the pathfinding
	CPathFinder	path(this, pTarg);

	if ( path.FindPath() == PATH_NONEXISTENT )
	{
		if ( g_Cfg.m_wDebugFlags & DEBUGF_NPCAI )
		{
			DEBUG_ERR((" -- no path" DEBUG_CR));
		}
		return;
	}

	if ( g_Cfg.m_wDebugFlags & DEBUGF_NPCAI )
	{
		DEBUG_ERR((" -- path found" DEBUG_CR));
	}
	//	save the found path
	for ( i = 1; ( path.m_xPath[1] != pTarg.m_x ) || ( path.m_yPath[1] != pTarg.m_y ); i++ )
	{
		path.ReadStep(i);
		if ( g_Cfg.m_wDebugFlags & DEBUGF_NPCAI )
		{
			DEBUG_ERR((" ---- new step (%d,%d)" DEBUG_CR, path.m_xPath[1], path.m_yPath[1]));
		}
		m_pNPC->m_nextX[i] = path.m_xPath[1];
		m_pNPC->m_nextY[i] = path.m_yPath[1];
	}

	EXC_CATCH("CChar");
}

void CChar::NPC_Food()
{
	EXC_TRY(("NPC_Food()"));

	int		iFood = Stat_GetVal(STAT_FOOD);
	int		iFoodLevel = Food_GetLevelPercent();
	int		iEatAmount = 1;
	int		iSearchDistance = 2;
	CItem	*pClosestFood = NULL;

	if ( iFood >= 10 ) return;							//	search for food is starving or very hungry
	if ( iFoodLevel > 40 ) return;						// and it is at least 60% hungry

	if ( g_Cfg.m_wDebugFlags & DEBUGF_NPCAI )
	{
		DEBUG_ERR(("%s hungry at %d%% (food=%d)" DEBUG_CR, GetName(), iFoodLevel, iFood));
	}

	CItemContainer	*pPack = GetPack();
	if ( pPack )
	{
		for ( CItem *pFood = pPack->GetContentHead(); pFood != NULL; pFood = pFood->GetNext() )
		{
									// i have some food personaly, so no need to search for something
			if ( pFood->IsType(IT_FOOD) )
			{
				if ( iEatAmount = Food_CanEat(pFood) )
				{
					if ( g_Cfg.m_wDebugFlags & DEBUGF_NPCAI )
					{
						DEBUG_ERR((" -- eating %d of %s" DEBUG_CR, iEatAmount, pFood->GetName()));
					}
					Use_EatQty(pFood, iEatAmount);
					return;
				}
			}
		}
	}

	if ( iFoodLevel <= 1 ) iSearchDistance = 20;
	else if ( iFoodLevel <= 5 ) iSearchDistance = 12;
	else if ( iFoodLevel <= 10 ) iSearchDistance = 8;
	else if ( iFoodLevel <= 15 ) iSearchDistance = 4;

	//	Search for food nearby
	CWorldSearch	AreaItems(GetTopPoint(), iSearchDistance);
	while (true)
	{
		CItem	*pItem = AreaItems.GetItem();
		if ( !pItem ) break;
		if ( !CanSee(pItem) ) continue;
		if ( pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_STATIC) ) continue;

		if ( iEatAmount = Food_CanEat(pItem) )
		{
			if ( pClosestFood )
			{
				if ( GetDist(pItem) < GetDist(pClosestFood) )
					pClosestFood = pItem;
			}
			else pClosestFood = pItem;
		}
	}

	if ( pClosestFood )
	{
		if ( GetDist(pClosestFood) <= 1 )
		{
			//	can take and eat just in place
			int iEaten = pClosestFood->ConsumeAmount(iEatAmount);
			if ( g_Cfg.m_wDebugFlags & DEBUGF_NPCAI )
			{
				DEBUG_ERR((" -- eating %d of %s (closest food)" DEBUG_CR, iEaten, pClosestFood->GetName()));
			}
			EatAnim(pClosestFood->GetName(), iEaten);
			if ( !pClosestFood->GetAmount() )
			{
				pClosestFood->Plant_CropReset();	// set growth if this is a plant
			}
		}
		else
		{
			//	move towards this item
			switch ( m_Act_SkillCurrent )
			{
			case NPCACT_STAY:
			case NPCACT_GOTO:
			case NPCACT_WANDER:
			case NPCACT_LOOKING:
			case NPCACT_GO_HOME:
			case NPCACT_Napping:
			case NPCACT_FLEE:
				{
					CPointMap pt = pClosestFood->GetTopPoint();
					if ( CanMoveWalkTo(pt) )
					{
						if ( g_Cfg.m_wDebugFlags & DEBUGF_NPCAI )
						{
							DEBUG_ERR((" -- going to %s (%d,%d) (closest food)" DEBUG_CR, pClosestFood->GetName(), pt.m_x, pt.m_y));
						}
						m_Act_p = pt;
						Skill_Start(NPCACT_GOTO);
						NPC_WalkToPoint((iFoodLevel < 5) ? true : false);
					}
					break;
				}
			}
		}
	}
	else			// no food around, but maybe i am ok with grass?
	{
		CCharBase			*pCharDef = Char_GetDef();
		RESOURCE_ID_BASE	rid = RESOURCE_ID(RES_TYPEDEF, IT_GRASS);

		if ( pCharDef->m_FoodType.FindResourceID(rid) )	//	do I accept grass as a food?
		{
			CItem	*pResBit = g_World.CheckNaturalResource(GetTopPoint(), IT_GRASS, true, this);
			if ( pResBit && pResBit->GetAmount() )
			{
				int iEaten = pResBit->ConsumeAmount(15);
				if ( g_Cfg.m_wDebugFlags & DEBUGF_NPCAI )
				{
					DEBUG_ERR((" -- eating %d of nearest grass" DEBUG_CR, iEaten));
				}
				EatAnim("grass", iEaten/10);
				return;
			}
			else									//	search for grass nearby
			{
				switch ( m_Act_SkillCurrent )
				{
				case NPCACT_STAY:
				case NPCACT_GOTO:
				case NPCACT_WANDER:
				case NPCACT_LOOKING:
				case NPCACT_GO_HOME:
				case NPCACT_Napping:
				case NPCACT_FLEE:
					{
						CPointMap pt = g_World.FindItemTypeNearby(GetTopPoint(), IT_GRASS, iSearchDistance);
						if (( pt.m_x >= 1 ) && ( pt.m_y >= 1 ))
						{
							if ( CanMoveWalkTo(pt) )
							{
								if ( g_Cfg.m_wDebugFlags & DEBUGF_NPCAI )
								{
									DEBUG_ERR((" -- going to (%d,%d) (closest grass)" DEBUG_CR, pt.m_x, pt.m_y));
								}
								m_Act_p = pt;
								Skill_Start(NPCACT_GOTO);
								NPC_WalkToPoint((iFoodLevel < 5) ? true : false);
								return;
							}
						}
						break;
					}
				}
			}
		}
	}
	if ( g_Cfg.m_wDebugFlags & DEBUGF_NPCAI )
	{
		DEBUG_ERR((" -- no food found" DEBUG_CR));
	}

	EXC_CATCH("CChar");
}

void CChar::NPC_AI()
{
	EXC_TRY(("NPC_AI()"));

	int			iInt = Stat_GetAdjusted(STAT_INT);

	//	TODO: Some special/general actions should go here.

	EXC_CATCH("CChar");
}
