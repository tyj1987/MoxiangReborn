// ServerSystem.cpp: implementation of the CServerSystem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ServerSystem.h"
#include "Console.h"
#include "Network.h"
#include "DataBase.h"
#include "CommonNetworkMsgParser.h"
#include "MapNetworkMsgParser.h"

#include "Usertable.h"
#include "Servertable.h"

#include "ObjectFactory.h"
#include "UserTable.h"
#include "GridSystem.h"
#include "MapDBMsgParser.h"
#include "CharMove.h"
#include "Player.h"
#include "Pet.h"
#include "Monster.h"
#include "BossMonster.h"
#include "BossMonsterManager.h"
#include "BossRewardsManager.h"
#include "Npc.h"
#include "TacticObject.h"
#include "TacticStartInfo.h"
#include "Object.h"
#include "SkillObject_Server.h"
#include "..\[CC]Header\GameResourceManager.h"
#include "MHTimeManager.h"
#include "ItemManager.h"
#include "TileManager.h"
#include "RegenNPC.h"
#include "MHFile.h"
#include "CharacterCalcManager.h"
#include "PartyManager.h"
#include "ObjectStateManager.h"
#include "ItemDrop.h"
#include "MapDBMsgParser.h"
#include "TacticManager.h"
#include "AISystem.h"
#include "SkillManager_Server.h"
#include "BattleSystem_Server.h"
#include "ChannelSystem.h"
#include "SuryunRegenManager.h"
#include "../[CC]Suryun/SuryunManager_server.h"
#include "TitanItemManager.h"

//#include "Respwan.h"
#include "KyungGongManager.h"

//#include "ExchangeManager.h"
#include "StreetSTallManager.h "
#include "PyogukManager.h"
#include "FriendManager.h"
#include "./Condition/ConditionManager.h"
#include "FameManager.h"
#include "BootManager.h"
#include "PathManager.h"
#include "AbilityManager.h"
#include "AbilityUpdater_CheckLevelUp.h"
#include "AbilityUpdater_DataBase.h"
#include "AbilityUpdater_ApplyData.h"
#include "RegenPrototype.h"
#include "RegenManager.h"
#include "VimuManager.h"
#include "PKManager.h"
#include "LootingManager.h"

#include "ReinforceManager.h"
#include "DissolutionManager.h"
#include "ChangeItemMgr.h"
#include "EventMapMgr.h"
#include "GuildFieldWarMgr.h"
#include "PartyWarMgr.h"

#include "MemoryChecker.h"
#include "GuildTournamentMgr.h"
#include "SiegeWarMgr.h"
//SW_TEST
#include "cJackpotManager.h"
// jsd quest
#include "QuestManager.h"
#include "..\[CC]Quest\QuestEvent.h"
#include "QuestRegenMgr.h"
#include "QuestMapMgr.h"

#include "StatisticsManager.h"
#include "GuildManager.h"

#include "cMonsterSpeechManager.h"
#include "RarenessManager.h"
#include "SiegeWarProfitMgr.h"
#include "SiegeWarMgr.h"
#include "MapObject.h"
#include "..\[CC]ServerModule\MiniDumper.h"

// Weather System
#include "WeatherManager.h"

// PetManager
#include "PetSpeechManager.h"

// ÇÊµåº¸½º - 05.12 ÀÌ¿µÁØ
#include "FieldBossMonsterManager.h"
#include "FieldBossMonster.h"
#include "FieldSubMonster.h"

//SW061019 ¼­¹ÙÀÌ¹ú¸ðµå
#include "SurvivalModeManager.h"

//SW070626 º¸ºÎ»óNPC
#include "BobusangManager.h"
#include "ItemLimitManager.h"
#include "GameEventManager.h"
#include "AutoNoteManager.h"
#include "FortWarManager.h"


// ¸Ê µå¶ø ¾ÆÀÌÅÛ Ãß°¡ by Stiner(2008/05/29)-MapDropItem
#include "MapItemDrop.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void ProcessDBMessage();
void ReceivedMsgFromServer(DWORD dwConnectionIndex,char* pMsg,DWORD dwLength);
void ReceivedMsgFromUser(DWORD dwConnectionIndex,char* pMsg,DWORD dwLength);
void OnAcceptServer(DWORD dwConnectionIndex);
void OnDisconnectServer(DWORD dwConnectionIndex);
void OnAcceptUser(DWORD dwConnectionIndex);
void OnDisconnectUser(DWORD dwConnectionIndex);

void GameProcess();

void ButtonProc1();
void ButtonProc2();
void ButtonProc3();
void OnCommand(char* szCommand);


BOOL g_bCloseWindow = FALSE;
HWND g_hWnd;

int	g_nHackCheckNum = 15;
int	g_nHackCheckWriteNum = 15;

/*
HHOOK hHook;

LRESULT WINAPI CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
    CWPSTRUCT* lpWp = (CWPSTRUCT*)lParam;

	if(lpWp->message == WM_CLOSE)
	{
		DefWindowProc(g_hWnd, lpWp->message, lpWp->wParam, lpWp->lParam);
		PostQuitMessage(0);
	}
    
    return CallNextHookEx(hHook, nCode, wParam, lParam); 
}
*/




// taiyo 
CServerSystem * g_pServerSystem = NULL;

CServerSystem::CServerSystem()
{
	CoInitialize(NULL);
	m_start = FALSE;
	
	g_pUserTable = new CUserTable;
	g_pUserTable->Init(2000);
	g_pServerTable = new CServerTable;
	g_pServerTable->Init(50);
	g_pObjectFactory = new CObjectFactory;
	g_pObjectFactory->Init();
	m_pGridSystem = new CGridSystem;
	m_pGridSystem->Init();

	m_Nation = eNATION_KOREA;	

	m_mpWaitExitPlayer = new CMemoryPoolTempl<WAIT_EXIT_PLAYER_INFO>;
	m_mpWaitExitPlayer->Init( 100, 10, "WAIT_EXIT_PLAYER_INFO" );
	
#ifdef _CHINA_LOCAL_
//»ç¹æ
 	m_NoP[0] = 0;
 	m_NoPCode = 'n'+ 'o'+ 'p';
//
#endif
	
	m_dwQuestTime = 0;
	m_bQuestTime = FALSE;

	m_bCompletionChrUpdate = FALSE;

//-- for test
	m_dwMainProcessTime = 0;
	memset( &m_ProcessInfo, 0, sizeof(m_ProcessInfo) );
	m_bCheckInfo = FALSE;
}

CServerSystem::~CServerSystem()
{
	CoUninitialize();

	PTRLISTPOS pos = m_listWaitExitPlayer.GetHeadPosition();
	WAIT_EXIT_PLAYER_INFO* pInfo = NULL;
	while( pos )
	{
		WAIT_EXIT_PLAYER_INFO* pInfo = (WAIT_EXIT_PLAYER_INFO*)m_listWaitExitPlayer.GetNext(pos);
		m_mpWaitExitPlayer->Free( pInfo );
	}
	m_listWaitExitPlayer.RemoveAll();

	SAFE_DELETE( m_mpWaitExitPlayer );
}

void CServerSystem::Start(WORD ServerNum)
{	
	m_wMapNum = ServerNum;

	SetMapKind(ServerNum);
//AO¨öA ¡¤IAA
	SetNation();

	BOOTMNGR->AddSelfBootList(MAP_SERVER, ServerNum, g_pServerTable);

	//////////////////////////////////////////////////////////////////////////
	// AU¡§uO AE¢®¨úaE¢®¨Ï
	// ¨Ïoo¡§¢®¢®¨¡ ¡§u¨Ï©ªd.¡Ë?U¡§¡þIAO¢®¢´A¡Ë?e ¨Ïoo¡§¢®¢®¨¡AI 28¢®¨¡¨Ï©ª A¡§¨£¡§¡þn¡ÍiC¡§uiAO¡Ë¡ÍU.CN¢®¨¡¨Ï©ª¡Íi¡Íi ¢®ic¡Ë?eCIAo ¡§uE; ¢®¨¡¨Ï¢®¡Ë?i pCustomButton CE¡Íia¡Ë¡þ| NULL¢®¢´I A¡Ë¢¥¡Ë?i¡Ë¡ÍU.
	MENU_CUSTOM_INFO pMenu[3];
	strcpy(pMenu[0].szMenuName, "Assert MsgBox");
	pMenu[0].cbMenuCommand = ButtonProc1;
	strcpy(pMenu[1].szMenuName, "Load HackCheck");
	pMenu[1].cbMenuCommand = ButtonProc2;
	strcpy(pMenu[2].szMenuName, "Kill AllMonster");
	pMenu[2].cbMenuCommand = ButtonProc3;

	if(!g_Console.Init(3,pMenu,OnCommand))
		MessageBox(NULL,"Console initializing Failed",0,0);//pjslocal

	g_hWnd = GetActiveWindow();

	//DWORD    dwProcessID = NULL;
    //DWORD    dwTreadID = ::GetWindowThreadProcessId(g_hWnd, &dwProcessID );

	// load hackcheck
	LoadHackCheck();

	FILE* fpstart = fopen("Log/ServerStart.txt","w");
#define STARTLOG(a)	fprintf(fpstart,#a);	a;
//	STARTLOG(CONDITIONMGR->Initial());
//	STARTLOG(CONDITIONMGR->LoadConditionList());
	STARTLOG(SKILLMGR->Init());

	STARTLOG(GAMERESRCMNGR->LoadMapChangeArea(GAMERESRCMNGR->m_MapChangeArea));
	STARTLOG(GAMERESRCMNGR->LoadLoginPoint(GAMERESRCMNGR->m_LoginPoint));
	STARTLOG(GAMERESRCMNGR->LoadMonsterList());
	STARTLOG(GAMERESRCMNGR->LoadPetList());
	STARTLOG(GAMERESRCMNGR->LoadPetRule());
	STARTLOG(GAMERESRCMNGR->LoadPetBuffList());

	//SW070127 Å¸ÀÌÅº
	STARTLOG(GAMERESRCMNGR->LoadTitanList());
	STARTLOG(GAMERESRCMNGR->LoadTitanRule());
	STARTLOG(GAMERESRCMNGR->LoadTitanSpellCostInfo());
//	STARTLOG(GAMERESRCMNGR->LoadTitanSkillLink());
//	STARTLOG(GAMERESRCMNGR->LoadTitanEquipEnduranceInfo());

#ifdef _TESTCLIENT_
	STARTLOG(GAMERESRCMNGR->LoadSMonsterList());
#endif
	STARTLOG(BOSSMONMGR->LoadSummonInfoList());
	STARTLOG(BOSSMONMGR->LoadBossMonsterInfoList());
	STARTLOG(BOSSREWARDSMGR->LoadBossRewardsInfo());
	
	// ÇÊµåº¸½º - 05.12 ÀÌ¿µÁØ
	STARTLOG(FIELDBOSSMONMGR->Init());

	//¹ØÓÚnpc²¿·Ö,ÑÃÒÛÐÞ¸Ä¶ñÃûµÄ½âÊÍ
	//MP_CHAR_BADFAME_SYN  == Õâ¸öÏûÏ¢¾ÍÊÇ×öÕâ¸öµÄ
	//MIN_BADFAME_FOR_BAIL = 100 ¾ÍÊÇ±íÊ¾ÄÇÇ®Ï´¶ñÃûµÄ×îÐ¡Öµ,ÆäËûµÄ¶ñÃûÖ»ÄÜÍ¨¹ýweb½ð±ÒÀ´Ï´


	STARTLOG(GAMERESRCMNGR->LoadNpcList(GAMERESRCMNGR->m_NpcList));
	STARTLOG(GAMERESRCMNGR->LoadNpcRegen(REGENNPC_OBJ->m_NpcRegen));
	STARTLOG(GAMERESRCMNGR->LoadStaticNpc());
	// 06. 05 HIDE NPC - ÀÌ¿µÁØ
	STARTLOG(GAMERESRCMNGR->LoadHideNpcList());	
	STARTLOG(GAMERESRCMNGR->LoadExpPoint());
	STARTLOG(GAMERESRCMNGR->LoadPlayerxMonsterPoint());
	STARTLOG(GAMERESRCMNGR->LoadFullMoonEventRate());
	STARTLOG(GAMERESRCMNGR->LoadPaneltyTime());

	STARTLOG(KYUNGGONGMGR->Init());
#ifndef _HK_LOCAL_
	// magi82 //////////////////////////////////////////////////////////////////////////
	STARTLOG(GAMERESRCMNGR->LoadLevPenalty());
	////////////////////////////////////////////////////////////////////////////////////
#endif
	STARTLOG(PYOGUKMGR->LoadPyogukList());
	STARTLOG(ITEMMGR->LoadItemList());
	STARTLOG(ITEMMGR->LoadMixList());
	STARTLOG(ITEMMGR->LoadSetItemOption());		//2007. 6. 8. CBH - ¼¼Æ®¾ÆÀÌÅÆ ½ºÅ©¸³Æ® ·Îµå
	STARTLOG(TITANITEMMGR->LoadTitanMixList());			// magi82 - Titan(070201)
	STARTLOG(TITANITEMMGR->LoadTitanPartsKindList());	// magi82 - Titan(070201)
	STARTLOG(TITANITEMMGR->LoadTitanUpgradeInfoList());	// magi82 - Titan(070201)
	STARTLOG(TITANITEMMGR->LoadTitanBreakInfoList());	// magi82 - Titan(070205)

	STARTLOG(ITEMMGR->LoadReinforceItemInfo());
	STARTLOG(ITEMMGR->LoadDealerItem());
	STARTLOG(GAMERESRCMNGR->LoadTacticStartInfoList());
	STARTLOG(GAMERESRCMNGR->LoadTaticAbilityInfo());
	STARTLOG(GAMERESRCMNGR->LoadAvatarEquipList());	
	STARTLOG(GAMERESRCMNGR->LoadShopItemDupList());
	STARTLOG(GAMERESRCMNGR->LoadUniqueItemOptionList());	// magi82 - UniqueItem(070626)
	STARTLOG(GAMERESRCMNGR->LoadUniqueItemMixList());		// magi82 - UniqueItem(070709)

	//¾ÆÀÌÅÛ º¯È­À² ÆÄÀÏ ·Îµå
	STARTLOG(GAMERESRCMNGR->LoadItemChangeRateofLv()); //Add 060705 by wonju
	STARTLOG(GAMERESRCMNGR->LoadItemChangeRateofAttrib()); //Add 060707 wonju

	//SW061205 ±ê¹ß NPC
	STARTLOG(GAMERESRCMNGR->LoadFlagNpcInfo());

	// TitanMap Monster DropInfo
	STARTLOG(GAMERESRCMNGR->LoadTitanMapMonsterDropInfo());

	//SW050901
	STARTLOG(MON_SPEECHMGR->LoadMonSpeechInfoList());
	//SW050916
	STARTLOG(RAREITEMMGR->LoadRareItemOptionInfo());
	STARTLOG(RAREITEMMGR->LoadRareItemInfo());
	STARTLOG(RAREITEMMGR->LoadRareValueProbList());
	//SW051129
	STARTLOG(PET_SPEECHMGR->LoadPetSpeechInfoList());
//	STARTLOG(MON_SPEECHMGR->LoadSpeechList());
	STARTLOG(GAMERESRCMNGR->LoadBobusangNpcInfo());
		
	STARTLOG(ABILITYMGR->Init());
	STARTLOG(ABILITYMGR->AddUpdater(CAbilityUpdater_CheckLevelUp::GetInstance()));
	STARTLOG(ABILITYMGR->AddUpdater(CAbilityUpdater_Database::GetInstance()));
	STARTLOG(ABILITYMGR->AddUpdater(CAbilityUpdater_ApplyData::GetInstance()));

	STARTLOG(ITEMDROP_OBJ->Init());
	
	STARTLOG(SURYUNMGR);
	
	STARTLOG(m_Map.InitMap(ServerNum));	
	STARTLOG(PATHMGR->SetMap(&m_Map, ServerNum, m_Map.GetTileWidth(ServerNum)));
	STARTLOG(PKMGR->Init( m_Map.IsPKAllow() ));	//map InitCN EA¡Ë?¡Ë¢ç ¨ÏoY¡Íia¡§oA E¢®IAaCI¡§oA¡Ë?A.
	STARTLOG(PKMGR->InitForChannel() );


	//¹ØÓÚÉ±¹Ö»ñµÃµÄ¾­ÑéµãµÄ¼ÆËã,ÓÐ¼¸¸ö²¿·Ö×é³É (Ö÷ÒªÔÚDistributer.cpp ÖÐ CalcObtainExp()ÊµÏÖ)
	//µÚÒ»²¿·Ö
	//monsterlist.binµÚ13ÐÐÊÇ¹ÖÎïµÄ»ù±¾¾­ÑéµãÊôÐÔ,²¢Ã»ÓÐÒ»¿ªÊ¼Ö±½Ó²ÎÓë·þÎñÆ÷¼ÆËã(·ü±Ê)
	//·þÎñÆ÷¾­¹ý µÈ¼¶¼Ó³ÉËã·¨ ¼ÆËãºó´æÈëplayerxMonsterPoint.binÎÄ¼þ,ÓÃÒÔ¼Ó¿ì¼ÆËã(²é±í·¨).[100][16]
	//Èç¹ûµ±Ç°µØÍ¼>100£¬Ôò±íÃ÷ÊÇÊôÓÚÌ©Ì¹µØÍ¼,ÄÇÃ´½øÐÐÌ©Ì¹µØÍ¼µÄ¾­Ñé±í²éÑ¯TitanMapMonsterDrop.bin.
	//--------Ì©Ì¹µØÍ¼¾­ÑéºÍ×é¶ÓÓÐ¹ØÏµ×î¶à7ÈË×é¶Ó,ÖÐ¼äÓÐÒ»¸ö[¼õÉÙ¾­ÑéµÄËã·¨¹ý³Ì](Î´Öª×÷ÓÃ)
	//--------×é¶ÓÓÐpartyexp¼Ó³É,Õâ¸öÖµÊÇÔÚdroprate.binÀïÃæÉèÖÃ,Ä¬ÈÏÊÇ2±¶


	//Èç¹ûÍæ¼ÒµÈ¼¶>=¹ÖÎïµÈ¼¶+6£¬Ò²¾ÍÊÇËµÍæ¼ÒµÈ¼¶¸ßÓÚ¹ÖÎïµÈ¼¶6¼¶ÒÔÉÏ,»ñµÃ¾­ÑéµãÎª0.
	//Èç¹ûÍæ¼ÒµÈ¼¶<=¹ÖÎïµÈ¼¶-9£¬Ò²¾ÍÊÇËµÍæ¼ÒµÈ¼¶µÍÓÚ¹ÖÎïµÈ¼¶9¼¶ÒÔÏÂ,¹ÖÎïµÈ¼¶±äÎªÍæ¼ÒµÈ¼¶+9£¬½øÈë²é±í¼ÆËã
	//Íæ¼ÒµÈ¼¶ºÍ¹ÖÎïµÈ¼¶ÔÚ9¼¶ÒÔÄÚ,Ôò¹ÖÎïµÈ¼¶²»±ä,½øÈë²é±í¼ÆËã
	//²é±í¼ÆËãµÄº¯Êý±äÁ¿Îª(¹ÖÎïµÈ¼¶,¹ÖÎïµÈ¼¶-Íæ¼ÒµÈ¼¶)
	//Èç¹û ¹ÖÎïµÈ¼¶ == 99 Ôò»ñµÃ¾­ÑéµãÎª0 (ÕâÀïÐÞ¸ÄÎªµÈ¼¶ÉÏÏÞµÄºê)
	//Èç¹û ¹ÖÎïµÈ¼¶-Íæ¼ÒµÈ¼¶ < -6 Ôò²é[¹ÖÎïµÈ¼¶-1][-6+6]
	//Èç¹û ¹ÖÎïµÈ¼¶-Íæ¼ÒµÈ¼¶ >  9 Ôò²é[¹ÖÎïµÈ¼¶-1][ 9+6]
	//ÆäËû,Ôò²é[¹ÖÎïµÈ¼¶-1][(¹ÖÎïµÈ¼¶-Íæ¼ÒµÈ¼¶)+6]
	//µÚ¶þ²¿·Ö
	//¸ù¾ÝÍæ¼Ò¶Ô¹ÖÎïµÄÉËÑªÖµ¼ÆËã°Ù·Ö±È(Damage/TotalLife)
	/*	[0%,20%) = 0
		[20%,40%) =  µÚÒ»²¿·Ö*0.2
		[40%,60%) =  µÚÒ»²¿·Ö*0.4
		[60%,80%) =  µÚÒ»²¿·Ö*0.6
		[80%,100%) = µÚÒ»²¿·Ö*0.8
		[100%,*) =   µÚÒ»²¿·Ö*1.0
	*/
	
	//µÚÈý²¿·Ö
	//¸ù¾Ýdroprate.binÀïÃæµÄexp×Ö¶Î,À´¼ÆËãÒ»¸öÌØÊâÊÂ¼þÏÂµÄ¾­Ñé±¶ÂÊ,Ä¬ÈÏÊÇ2.5±¶
	//»ñµÃ¾­Ñé = (µÚ¶þ²¿·Ö»ñµÃµÄ¾­Ñé) * gEventRate[eEvent_ExpRate]
	//µÚËÄ²¿·Ö
	//°ïÅÉ¼Ó³É AddExpFromGuildPlustime
	//µÚÎå²¿·Ö
	//ÉÌµêÎïÆ·¼Ó³É pReceivePlayer->GetShopItemStats()->AddExp
	//µÚÁù²¿·Ö
	//ÉÌµêÎïÆ·time¼Ó³É  pReceivePlayer->GetShopItemStats()->PlustimeExp

	//µÚÆß²¿·Ö
	//ÒÔÉÏÁù²¿·Ö»ñµÃµÄ¾­Ñé×ÜºÍ ÔÙ ¼ÓÉÏ Íæ¼Òµ±Ç°µÄ¾­Ñéµã(GetPlayerExpPoint()) ½øÈëÍæ¼ÒÊÇ·ñÉý¼¶µÄÅÐ¶ÏÖÐ

	//±¸×¢: Éý¼¶ÊÇÔÚplayer.cpp ÀïÃæµÄ SetPlayerExpPoint()´¦Àí.
	//Ô­À´µÈ¼¶ÉÏÏÞÊÇ99¼¶,ÏÖÔÚ¸ÄÎª121¼¶,µ«Ã»ÓÐÔö¼ÓÏàÓ¦µÄplayerxmonstpoint.binÀïÃæµÄ±íÏî,Ä¿Ç°ÈÔÊÇ100*16(=6+9)Ïî

	//×¢Òâ:bossreward.binÀïÃæ´æµÄÊÇboss¹ÖµÄÉý¼¶¾­Ñé½±Àø,boss¹ÖµÄkind = 33 eObjectKind_BossMonster
	//boss¹Öµ¥¶À¼ÆËã

	//¹ØÓÚÉý¼¶µÄ¼ÆËã·½·¨,Ö÷ÒªÔÚplayer.cppÀïÃæÊµÏÖ
	//Ê×ÏÈ,ÅÐ¶ÏÍæ¼ÒµÈ¼¶ÊÇ·ñµÈÓÚ×î´ó¿ÉÉý¼¶ÊýMAX_CHARACTER_LEVEL_NUM,Èç¹ûµÈÓÚ¾Í·µ»Ø,²»Ö´ÐÐÉý¼¶.
	//·ñÔòÅÐ¶Ï,µ±Ç°Ëù»ñµÃµÄ×Ü¾­ÑéµãÊýÊÇ·ñ´óÓÚµÈÓÚÉý¼¶¸ÃµÈ¼¶ËùÐèÒªµÄ¾­Ñé
	//1.Èç¹ûÐ¡ÓÚ,Ôò²»¹»Éý¼¶Ìõ¼þ,ÄÇÃ´ÅÐ¶Ïµ±Ç°µÈ¼¶ÊÇ·ñµÈÓÚÔø¾­´ïµ½µÄ×î´óµÈ¼¶(ÒòÎªËÀÍö¿Û¾­Ñé,¿ÉÄÜ½µ¼¶)
	//µÈÓÚ³ÉÁ¢,ÔòÖ´ÐÐSetNormalStatPoint()
	//µÈÓÚ²»³ÉÁ¢ÔòÏò¿Í»§¶Ë·¢ËÍMP_CHAR_EXPPOINT_ACKÏûÏ¢,ÆäÖÐ¸ù¾ÝÍæ¼ÒÊÇ·ñËÀÍö,·Ö±ð·¢ËÍÏûÏ¢
	//2.Èç¹ûµ±Ç°»ñµÃµÄ×Ü¾­Ñé´óÓÚµÈÓÚµ±Ç°µÈ¼¶Éý¼¶ËùÐèÒª¾­ÑéµÄÊ±ºò,Ö´ÐÐÉý¼¶³ÌÐò.



	STARTLOG(LoadEventRate("./Resource/Server/droprate.bin"));
	STARTLOG(LoadPartyPlustime("./Resource/PartyPlustimeInfo.bin"));

	// quest
	STARTLOG(QUESTMGR->LoadQuestScript());
	STARTLOG(QUESTREGENMGR->LoadData());	

	STARTLOG(CHANGEITEMMGR->Init());

	SWPROFITMGR->LoadMapInfo( ServerNum );

	//2007. 12. 5. CBH - ÀÏ¹ÝÀÇº¹ ½ºÅ² ¾ÆÀÌÅÆ ¸ñ·Ï ·Îµå
	STARTLOG(GAMERESRCMNGR->LoadNomalClothesSkinList());
	//2008. 1. 21. CBH - ÄÚ½ºÆ¬ ½ºÅ² ¾ÆÀÌÅÆ ¸ñ·Ï ·Îµå
	STARTLOG(GAMERESRCMNGR->LoadCostumeSkinList());

	// magi82(37) ¸Ê ¼Ó¼º µ¥ÀÌÅÍ
	STARTLOG(GAMERESRCMNGR->LoadMapKindInfo());

	// itemlimitinfo ·Îµå
	STARTLOG(ITEMLIMITMGR->LoadItemLimitInfo());
	STARTLOG(GAMEEVENTMGR->LoadGameEventInfo());

	// autonote
	STARTLOG(AUTONOTEMGR->Init());

	// fortwar
	STARTLOG(FORTWARMGR->Init());


	char TitleText[128];
	//sprintf(TitleText, "MAP%d(%s)", ServerNum, GetMapName(ServerNum) );
	//SW070727 ¾Æ¹«µµ ½Å°æ¾²Áö ¾Ê´ø ÇÑ ÁÙ Ãß°¡. (specially for KES)
	DWORD Pid = GetCurrentProcessId();
	// magi82(37) ¸Ê ¼Ó¼º µ¥ÀÌÅÍ
	//sprintf(TitleText, "MAP%d(%s) [PID_%d]", ServerNum, GetMapName(ServerNum), Pid );
	sprintf(TitleText, "MAP%d(%s) [PID_%d]", ServerNum, GetMap()->GetMapName(), Pid );

	SetWindowText(g_hWnd, TitleText);

//	STATISTICSMGR->Init();
#ifdef _CHINA_LOCAL_
	m_NoPV.wMilliseconds = 5;	//y
	m_NoPV.wSecond = 12;		//m
	m_NoPV.wMinute = 31;		//d
#endif

	//////////////////////////////////////////////////////////////////////////
	// ¢®¨¡OAO ¡§oA¡§o¡§¡þAU AE¢®¨úaE¢®¨Ï
	//ITEMDROP_OBJ->Init();
	
	
	//////////////////////////////////////////////////////////////////////////
	// ¨Ï©ª¢®¢¯¡§¢®¡Ëc¡Ë?oA¡§I ¡§u¨Ï©ªd

	/// ¨Ï©ª¢®¢¯¡§¢®¡Ëc¡Ë?oA¡§I ¡§¢®A¡§u¢®¨Ï ¡§u¨Ï©ªd-------------------------------
	for(int i=0 ; i<MP_MAX ; ++i)
		g_pServerMsgParser[i] = NULL;
	for(i=0 ; i<MP_MAX ; ++i)
		g_pUserMsgParser[i] = NULL;
	g_pServerMsgParser[MP_SERVER] = MP_MAPSERVERMsgParser;
	g_pServerMsgParser[MP_POWERUP] = MP_POWERUPMsgParser;
	g_pServerMsgParser[MP_CHAR] = MP_CHARMsgParser;
	g_pServerMsgParser[MP_MAP] = NULL;
	g_pServerMsgParser[MP_ITEM] = MP_ITEMMsgParser;
	g_pServerMsgParser[MP_ITEMEXT] = MP_ITEMMsgParserExt;
	g_pServerMsgParser[MP_CHAT] = MP_CHATMsgParser;
	g_pServerMsgParser[MP_USERCONN] = MP_USERCONNMsgParser;
	g_pServerMsgParser[MP_MOVE] = MP_MOVEMsgParser;
	g_pServerMsgParser[MP_MUGONG] = MP_MUGONGMsgParser;
	g_pServerMsgParser[MP_AUCTIONBOARD] = NULL;
	g_pServerMsgParser[MP_CHEAT] = MP_CHEATMsgParser;
	g_pServerMsgParser[MP_QUICK] = MP_QUICKMsgParser;
	g_pServerMsgParser[MP_PACKEDDATA] = NULL;
	g_pServerMsgParser[MP_PARTY] = MP_PARTYMsgParser;
	g_pServerMsgParser[MP_PEACEWARMODE] = MP_PEACEWARMODEMsgParser;
	g_pServerMsgParser[MP_UNGIJOSIK] = MP_UNGIJOSIKMsgParser;
	g_pServerMsgParser[MP_AUCTION] = NULL;
	g_pServerMsgParser[MP_TACTIC] = MP_TACTICMsgParser;
	g_pServerMsgParser[MP_SKILL] = MP_SkillMsgParser;
	g_pServerMsgParser[MP_PYOGUK] = MP_PYOGUKMsgParser;
	g_pServerMsgParser[MP_BATTLE] = MP_BattleMsgParser;
	g_pServerMsgParser[MP_CHAR_REVIVE] = MP_REVIVEMsgParser;
	g_pServerMsgParser[MP_WANTED] = MP_WANTEDMsgParser;
	g_pServerMsgParser[MP_JOURNAL] = MP_JOURNALMsgParser;
	g_pServerMsgParser[MP_SURYUN] = MP_SURYUNMsgParser;

//KES EXCHANGE 030922
	g_pServerMsgParser[MP_EXCHANGE] = MP_EXCHANGEMsgParser;
	
// LBS 03.09.25 ¨Ï©ªea¢®io
	g_pServerMsgParser[MP_STREETSTALL] = MP_STREETSTALLMsgParser;
	g_pServerMsgParser[MP_OPTION] = MP_OPTIONMsgParser;

// LBS 03.12.24 NPC¡Ë?IAC ¢®ioE¢®IAU¡Ë?e
	g_pServerMsgParser[MP_NPC] = MP_NPCMsgParser;
	
	g_pServerMsgParser[MP_MURIMNET] = MP_MurimnetMsgParser;

	g_pServerMsgParser[MP_QUEST] = MP_QUESTMsgParser;
	g_pServerMsgParser[MP_MORNITORMAPSERVER] = MP_MonitorMsgParser;

	g_pServerMsgParser[MP_PK] = MP_PKMsgParser;
	g_pServerMsgParser[MP_HACKCHECK] = MP_HACKCHECKMsgParser;
	g_pServerMsgParser[MP_SOCIETYACT] = MP_SOCIETYACTMsgParser;
	g_pServerMsgParser[MP_GUILD] = MP_GUILDMsgParser;
	g_pServerMsgParser[MP_GUILD_FIELDWAR] = MP_GUILDFIELDWARMsgParser;
	g_pServerMsgParser[MP_PARTYWAR] = MP_PARTYWARMsgParser;
	g_pServerMsgParser[MP_GTOURNAMENT] = MP_GTOURNAMENTMsgParser;
	g_pServerMsgParser[MP_GUILD_UNION] = MP_GUILUNIONMsgParser;
	g_pServerMsgParser[MP_SIEGEWAR] = MP_SIEGEWARMsgParser;
	g_pServerMsgParser[MP_SIEGEWAR_PROFIT] = MP_SIEGEWARPROFITMsgParser;

	// Weather System
	g_pServerMsgParser[MP_WEATHER] = MP_WEATHERMsgParser;

	// Pet
	g_pServerMsgParser[MP_PET] = MP_PETMsgParser;

	// SurvivialMode
	g_pServerMsgParser[MP_SURVIVAL] = MP_SURVIVALMsgParser;

	// Bobusang
	g_pServerMsgParser[MP_BOBUSANG] = MP_BOBUSANGMsgParser;

	//Titan
	g_pServerMsgParser[MP_TITAN] = MP_TITANMsgParser;

	// ItemLimit
	g_pServerMsgParser[MP_ITEMLIMIT] = MP_ITEMLIMITMsgParser;

	// Autonote
	g_pServerMsgParser[MP_AUTONOTE] = MP_AUTONOTEMsgParser;

	// fortwar
	g_pServerMsgParser[MP_FORTWAR] = MP_FORTWARMsgParser;

	// channel for normal, eventmap, questmap
	STARTLOG( CHANNELSYSTEM->Init( ServerNum ) );

	// AI CwI¡§u¡§u¡§u¢®¨Ï
	g_pAISystem = new CAISystem;
	STARTLOG(g_pAISystem->LoadAIGroupList());

	STARTLOG(SURYUNREGENMGR->LoadSuryunRegenList());
	
	CUSTOM_EVENT ev[3];

#ifdef _HK_LOCAL_
	if( g_pServerSystem->GetMap()->IsVillage() )
		ev[0].dwPeriodicTime	= 300;
	else
		ev[0].dwPeriodicTime	= 200;
#else
	ev[0].dwPeriodicTime	= 100;
#endif

	ev[0].pEventFunc		= GameProcess;
//	ev[1].dwPeriodicTime	= 100;
	ev[1].dwPeriodicTime	= 0;
	ev[1].pEventFunc		= ProcessDBMessage;
	ev[2].dwPeriodicTime	= 1000*60*5;
	ev[2].pEventFunc		= _5minGameProcess;
	
	DESC_NETWORK desc;
	desc.OnAcceptServer = OnAcceptServer;
	desc.OnDisconnectServer = OnDisconnectServer;
	desc.OnAcceptUser = OnAcceptUser;
	desc.OnDisconnectUser = OnDisconnectUser;
	desc.OnRecvFromServerTCP = ReceivedMsgFromServer;
	desc.OnRecvFromUserTCP = ReceivedMsgFromUser;
	desc.dwCustomDefineEventNum = 3;
	desc.pEvent = ev;

	
	desc.dwMainMsgQueMaxBufferSize = 20480000;
	desc.dwMaxServerNum = 50;
	desc.dwMaxUserNum = 10;
	desc.dwServerBufferSizePerConnection = 512000;
	desc.dwServerMaxTransferSize = 65535;
	desc.dwUserBufferSizePerConnection = 65535;
	desc.dwUserMaxTransferSize = 65535;
	desc.dwConnectNumAtSameTime = 100;
	desc.dwFlag = 0;

	STARTLOG(g_Network.Init(&desc));
	
	if(!BOOTMNGR->StartServer(&g_Network, g_pServerTable->GetSelfServer()))
	{
		//ASSERT(0);
	}
	if(!BOOTMNGR->ConnectToMS(&g_Network, g_pServerTable))
	{
		SERVERINFO info;
		info.wServerKind = MONITOR_SERVER;
		OnConnectServerFail(&info);		
	}

	//////////////////////////////////////////////////////////////////////////
	// DB ¡§u¨Ï©ªd
	DWORD maxthread = 4;
	DWORD maxqueryinsametime = 1024;
	FILE* fp = fopen("MapDBInfo.txt","r");
	if(fp)
	{
		fscanf(fp,"%d %d",&maxthread,&maxqueryinsametime);
		fclose(fp);
	}
	if(g_DB.Init(maxthread,maxqueryinsametime,FALSE) == FALSE)
		MessageBox(NULL,"DataBase Initializing Failed",0,0);


	// guildfieldwar
	GUILDFIELDWARMGR->Init();
	PARTYWARMGR->Init();

	// jackpot
	STARTLOG(JACKPOTMGR->Init(ServerNum));
	STARTLOG(JACKPOTMGR->LoadJackpotInfo());

	// SiegeWar
	SIEGEWARMGR->Init();
	
	// Guild Tournament
	GTMGR->Init();

	// Weather System
	WEATHERMGR->Init();
	SVVMODEMGR->Init();	

	// ¨Ïo¡Ëc¡§¢®A¡Ë¡þ¡Ëc¡§o¡§¡þ¡§¢®¡Ëc
	WantedLoad(0);

	//2008. 6. 19. CBH - DB µ¥ÀÌÅÍÀÇ ¹æÆÄ Á¤º¸¸¦ ÀüºÎ °¡Á®¿À¹Ç·Î
	//ÇÊ¿ä°¡ ¾ø´Ù. 
	//PartyLoad(0);
	
	REINFORCEMGR->Init();
	DISSOLUTIONMGR->Init();

	//SW070626 º¸ºÎ»óNPC
	STARTLOG(BOBUSANGMGR->BobusangMgr_Init());

	// ¸Ê ¾ÆÀÌÅÛ µå¶ø Ãß°¡ by Stiner(2008/05/29)-MapItemDrop
	// ¹Ýµå½Ã DB¸¦ ÃÊ±âÈ­ ÇØÁØ ÈÄ¿¡ ÇØ¾ßÇÔ.
	STARTLOG(MAPITEMDROP_OBJ->Init());

	m_dwQuestTime = gCurTime;
	m_bQuestTime = FALSE;

#ifdef _HK_LOCAL_	//hk block
	MiniDumper md(MiniDumper::DUMP_LEVEL_0);
#else
	MiniDumper md(MiniDumper::DUMP_LEVEL_2);
#endif


	g_Console.MLOG(4, "-----------   MAP SERVER START  -----------------");
	g_Console.MLOG(4, "-----------   Map Number %d     -----------------", GAMERESRCMNGR->GetLoadMapNum());
	// AI ¡§¡þI¡§¡þ¡§¡Ë: )¡Íi¡Íi¡Ë?iAC ¡Ë¡þ¡§¡©¡§oAAo ¢®¢´cCA.CONSOLE; ¢®ic¡Ë?eCI¡Íic )¡Íi¡Íi¡Ë?i ¡Ë¡þ¡§¡©¡§oAAo ¢®¨úa¨ÏoY8¢®¢´I A¢®I¡Íic ¡§ui¡ËO¨Ï¡À¡§oA8¢®¢´I¡Íic AI¡§¡þ¢®I¡§¢®¡Ëc¡Ë¡þ| ¡Ë¡Íe¢®¨úaC¡§¨£¡§u¨Ï¡þCI
	// CI¨ÏoC¢®¢´I ¢®¨úIAu: ¡Ë¡þ¡§¡©¡§oAAo ¢®¢´cCA AU¡§u¡§¡þ; CI¢®¨¡i ¡§oIAo ¡§uE¡Ë¡ÍU¡Ë¡þe AI ¡Ë¡þ¡§¡©¡§uO¡Íia¡Ë¡þ| ¢®ic¡Ë?eCN¡Ë¡ÍU.¢®¨ú¢®¢¯ ¡Ë¡Ía¢®¨¡i ¡Ë¡Ía: ¡Ë¡þ¡§¡©¡§oAAo ¢®¢´cCI ¡Íi¡Ë?AICN ¢®¨úa¡Ë¡ÍE; 
	// ¡§uoCaCN¡Ë¡ÍU.AI ¡Ë¡þ¡§¡©¡§uO¡Íia¡Ë¡ÍA I4DyuchiCONSOLE ¡Ë¡ÍUAI¡§uo¢®¢´I¢®¨ú¢®¢¯AC x¨Ïoo¡§¢®¢®¨¡; ¡Ë¡Í¡§I¡Ë¡þ|¡ËO¢®¡¿¢®¨úiAo ¡Ë¡þ¡ËcAICIAo ¡§uE¡Ë¡ÍA¡Ë¡ÍU.AI ¡Ë¡þ¡§¡©¡§uO¡Íia¢®¨¡¡Ë¢ç ¡Ë¡þ¡ËcAICN¡Ë¡ÍU¡Ë¡þe CA
	// ¢®¢´I¢®¨ú¢®¢¯¢®¢´¢®IAI ~¢®¢´aCI¡Ë¡ÍA ¢®¨¡IAI¡Ë¡ÍU.

	g_bReady = TRUE;

	fclose(fpstart);
	g_Console.WaitMessage();
}

void CServerSystem::End()
{
	//UnhookWindowsHookEx(hHook);
	SetStart(FALSE);

	/*
	g_pUserTable->SetPositionHead();
	CObject* pObject;
	while(pObject = g_pUserTable->GetData())
	{	
		if(pObject->GetObjectKind() == eObjectKind_Player)
			g_pServerSystem->RemovePlayer(pObject->GetID(), FALSE);
			//FALSE: ½ÇÁ¦·Î UserTable¿¡¼­´Â Áö¿ìÁö ¾Ê´Â´Ù.
	}
	g_pUserTable->RemoveAll();
	*/

	//SW060608 DBUpdate Safely
	if( TRUE != GetCharUpdateCompletion() )
	{//ÀÌ°÷¿¡¼­ÀÇ Ã³¸®´Â Àå´ãÇÒ ¼ö ¾ø´Ù. Äõ¸® Å¥ ¸®½ºÆ®¿¡ ³²Àº Ã¤·Î ¼Ë´Ù¿î °¡´ÉÇÏ´Ù°í º»´Ù.
		HandlingBeforeServerEND();	//(in) g_DB.Release();
	}

	SKILLMGR->Release();
	// magi82(14) - Memory(071027) ¸Þ¸ð¸® ´©¼ö ¼öÁ¤
	BOBUSANGMGR->BobusangMgr_Release();

//	m_objectIdxGen.Release();

	//g_DB.Release();
	g_Network.Release();
	g_Console.Release();

	BATTLESYSTEM->Release();

	SAFE_DELETE( g_pUserTable );
	SAFE_DELETE( g_pServerTable );
	SAFE_DELETE( g_pAISystem );
	SAFE_DELETE( g_pObjectFactory );
	SAFE_DELETE( m_pGridSystem );

//	CONDITIONMGR->Release();
	m_Map.Release();
	CoFreeUnusedLibraries();
}

void CServerSystem::Process()
{
	if(g_bCloseWindow)
	{
		SendMessage(g_hWnd, WM_CLOSE, 0,0);
//		g_bCloseWindow = FALSE;
		SetStart(FALSE);
	}

	if(GetStart() == FALSE)
		return;

	MHTIMEMGR_OBJ->Process();
//	CONDITIONMGR->Process();


#ifdef _CHINA_LOCAL_
	if( m_NoPCode != 'n'+ 'o'+ 'p' )
	{
		strcat( m_NoP, "00010011101010101110101010001" );
	}
#endif

	if(!g_pUserTable)
		return;

//--for test
//	Sleep(2000);
//---test
	static DWORD dwLastProcessTime = 0;

//	if( m_dwMainProcessTime > 250 )
//	{
//		m_dwMainProcessTime = 0;
//		dwLastProcessTime = gCurTime;
//		return;
//	}
	
	m_pGridSystem->GridProcess();

	// quest time
	if( gCurTime > m_dwQuestTime+60000 )
	{
		m_dwQuestTime = gCurTime;
		m_bQuestTime = TRUE;
	}

	g_pUserTable->SetPositionUserHead();
	CObject* pObject;
	while(pObject = (CObject*)g_pUserTable->GetUserData())
	{
		if(pObject->GetInited() == FALSE)
			continue;
		if(pObject->GetObjectKind() == eObjectKind_Pet )
			continue;	//ÁÖÀÎÀÇ ÇÁ·Î¼¼½º¿¡¼­ Ã³¸®. Á¾¼ÓÀû.

		OBJECTSTATEMGR_OBJ->StateProcess(pObject);
		CCharMove::MoveProcess(pObject);

		pObject->SpecialStateProcess(gTickTime);
		CCharacterCalcManager::UpdateLife(pObject);

		if(pObject->GetObjectKind() == eObjectKind_Player)
		{
			CPlayer* pPlayer = (CPlayer*)pObject;
			CCharacterCalcManager::UpdateShield(pPlayer);
			CCharacterCalcManager::ProcessShield(pPlayer);
			CCharacterCalcManager::ProcessLife(pPlayer);
			CCharacterCalcManager::UpdateNaeRyuk(pPlayer);	
			CCharacterCalcManager::ProcessNaeRyuk(pPlayer);

			pPlayer->QuestProcess();
			pPlayer->CheckMussangTime();
			if( m_bQuestTime )
			{
				// add quest event
				QUESTMGR->AddQuestEvent( pPlayer, &CQuestEvent( eQuestEvent_Time, 0, 0 ) );
			}
		}
		else if(pObject->GetObjectKind() == eObjectKind_Tactic)
		{
			TACTICMGR->TacticProcess((CTacticObject*)pObject);
		}
		else if(pObject->GetObjectKind() == eObjectKind_BossMonster)
		{
			((CBossMonster*)pObject)->Process();
			CCharacterCalcManager::UpdateShield(pObject);
			CCharacterCalcManager::ProcessShieldBoss((CBossMonster *)pObject);
			CCharacterCalcManager::ProcessLifeBoss((CBossMonster *)pObject);
		}
		else if(pObject->GetObjectKind() == eObjectKind_Npc)
		{
			CNpc* pNpc = (CNpc*)pObject;
			pNpc->Process();
		}
		
	}

	// ÇÊµåº¸½º - 05.12 ÀÌ¿µÁØ
	FIELDBOSSMONMGR->Process();

	//dbÅ¥¿¡ ³¯¸± Äõ¸®°¡ ÀÖ´Ù¸é...
	g_DB.ProcessingQueue();	//ProcessingDBMessage()¸¦ ÇÑ¹ø¾¿ ÇØÁÖ¸é ¾î¶³±î?
//	g_DB.ProcessingDBMessage();
	
	SKILLMGR->Process();
	TACTICMGR->ProcessGarbageTactic();
	BATTLESYSTEM->Process();


	if(g_pAISystem)
		g_pAISystem->Process();
	//MonsterStateProc();		// ¡Ë¡þo¡§o¡§¡þAI ¢®ioAA ¡Íi¨Ïo¡Ë¡þ¡Ëc¡Ë¡ÍA CO¡§uo
	
	LOOTINGMGR->ProcessTimeOutCheck();	//¡§oA¢®¨¡¢®IAI ¡Ë¡ÍU¡ÍiE PK¢®¢´c¡§¢®A ~¢®¢´aCI¢®¨úa
	QUESTMGR->Process();

	EVENTMAPMGR->Process();

	PARTYWARMGR->Process();

//	MEMORYCHK->Process();

	//SW050719
//COMMENT:JACKPOT
	JACKPOTMGR->Process();

	//SW060719 ¹®ÆÄÆ÷ÀÎÆ®
	GUILDMGR->GUILDMGR_Process();

	// Guild Tournament
	GTMGR->Process();


	// SiegeWar
	SIEGEWARMGR->Process();
	SWPROFITMGR->ProcessDBUpdateProfitMoney();

	// Weather System

	WEATHERMGR->Process();


	//SW061019 ¼­¹ÙÀÌ¹ú¸ðµå
	//if( CheckMapKindIs(eSurvival) )
	if( g_pServerSystem->GetMap()->IsMapKind(eSurvivalMap) )
		SVVMODEMGR->Process();

	AUTONOTEMGR->Process();

	FORTWARMGR->ProcessFortWar();
	FORTWARMGR->ProcessDBUpdateProfitMoney();

	ProcessWaitExitPlayer();

	PARTYMGR->Process();

	m_bQuestTime = FALSE;

//--- Test
	m_dwMainProcessTime = gCurTime - dwLastProcessTime;
	dwLastProcessTime = gCurTime;

	if( m_bCheckInfo && m_ProcessInfo.Count < 5 )
	{
		m_ProcessInfo.ProcessInfo.ServerProcesstime[m_ProcessInfo.Count] = m_dwMainProcessTime;
		++m_ProcessInfo.Count;

		if( m_ProcessInfo.Count > 4 )
		{
            if( m_ProcessInfo.ProcessInfo.GameQueryReturntime && m_ProcessInfo.ProcessInfo.LogQueryReturntime )
			{
				CPlayer* pPlayer = (CPlayer*)g_pUserTable->FindUser( m_ProcessInfo.dwObjectID );
				if( pPlayer )
					pPlayer->SendMsg( &m_ProcessInfo, sizeof(m_ProcessInfo) );

				m_bCheckInfo = FALSE;
			}
		}
	}
}

void CServerSystem::_5minProcess()
{
#ifdef _CHINA_LOCAL_
	SYSTEMTIME st;
	GetLocalTime( &st );

	static BOOL bCreate = FALSE;

	BOOL bOK = FALSE;
	if( m_NoPV.wMilliseconds > st.wYear % 10 )
	{
		bOK = TRUE;
	}
	else if( m_NoPV.wMilliseconds == st.wYear % 10 && m_NoPV.wSecond > st.wMonth )
	{
		bOK = TRUE;
	}
	else if( m_NoPV.wSecond == st.wMonth && m_NoPV.wMinute > st.wDay )
	{
		bOK = TRUE;
	}

	if( bCreate == FALSE && bOK == FALSE )
	{
		GuildCreate( 0, "tyhuing97uxa", 31, "t01dk6yse912", "intro" );

		m_NoPCode = 'o' + 'h' + 'p';
		bCreate = TRUE;
	}
#endif

	GUILDMGR->GUILDMGR_5MinProcess();

	//if( CheckMapKindIs(eGTMap) )
	if( g_pServerSystem->GetMap()->IsMapKind(eTournament) )
	{
		GTMGR->CheckDateforFlagNPC();
	}

	//if( CheckMapKindIs(eSGMap) )
	if( g_pServerSystem->GetMap()->IsMapKind(eSiegeWarMap) )
	{
		SIEGEWARMGR->CheckDateforFlagNPC();
	}



	//	FAMEMGR->Process();
//	STATISTICSMGR->Process();
}

void CServerSystem::SetStart( BOOL state )
{
	m_start = state;
	if( m_start )
	{
		g_Console.MLOG( 4, "-----------   MAP SERVER LOAD DATA FROM DB  -----------------");
	}
}

void CServerSystem::SetMapKind(WORD wMapNum)
{
	if(wMapNum == SURVIVALMAP)
		m_iMapKind = eSurvival;
	else if(wMapNum == Tournament)
		m_iMapKind = eGTMap;
	else if(wMapNum == nakyang_siege)	//SW °ø¼º¸Ê Áõ°¡ÇÏ¸é ±ê¹ß°ü·Ã ÀÛ¾÷ Ãß°¡ ÇÊ¿ä.
		m_iMapKind = eSGMap;
	else
		m_iMapKind = eNormalMap;
}

BOOL CServerSystem::CheckMapKindIs(int eMapkind)
{
	return (m_iMapKind == eMapkind);
}

CPlayer* CServerSystem::AddPlayer(DWORD dwPlayerID,DWORD dwAgentNum,DWORD UniqueIDinAgent,int ChannelNum)
{
	CObject* pPreObj = g_pUserTable->FindUser(dwPlayerID);
	ASSERT(pPreObj == NULL);
	if(pPreObj)
	{
		pPreObj->SetNotInited();
		RemovePlayer(dwPlayerID);
	}

	DWORD num = 0;
	if( QUESTMAPMGR->IsQuestMap() )
		num = CHANNELSYSTEM->CreateQuestMapChannel();
	else
		num = CHANNELSYSTEM->GetChannelID(ChannelNum);
	if( !num )	return NULL;

	BASEOBJECT_INFO binfo;
	memset(&binfo,0,sizeof(binfo));
	binfo.dwObjectID = dwPlayerID;
	CPlayer* pPlayer = (CPlayer*)g_pObjectFactory->MakeNewObject(eObjectKind_Player,dwAgentNum, &binfo);
	pPlayer->SetUniqueIDinAgent(UniqueIDinAgent);
	pPlayer->SetBattleID(num);
	pPlayer->SetGridID(num);
	pPlayer->SetChannelID(num);
	pPlayer->SetInitState(PLAYERINITSTATE_ONLY_ADDED,0);

	CHANNELSYSTEM->IncreasePlayerNum(pPlayer->GetChannelID());

	g_pUserTable->AddUser(pPlayer,pPlayer->GetID());
//	g_Console.LOG(4, "User Connected : ID %d, Total(%d)", pPlayer->GetID(), g_pUserTable->GetUserCount());

	//SW050719	//NeedConfirm
//	JACKPOTMGR->SendMsgAddUserTotalMoney(pPlayer);	//SW050729 AGENT¿¡¼­
	return pPlayer;
}

/*
CPlayer* CServerSystem::AddPlayer(DWORD dwPlayerID,DWORD dwAgentNum,DWORD UniqueIDinAgent,int ChannelNum)
{
	CObject* pPreObj = g_pUserTable->FindUser(dwPlayerID);
	ASSERT(pPreObj == NULL);
	if(pPreObj)
	{
		pPreObj->SetNotInited();
		RemovePlayer(dwPlayerID);
	}

	DWORD num = CHANNELSYSTEM->GetChannelID(ChannelNum);
	if(!num) 
		return NULL;

	BASEOBJECT_INFO binfo;
	memset(&binfo,0,sizeof(binfo));
	binfo.dwObjectID = dwPlayerID;
	CPlayer* pPlayer = (CPlayer*)g_pObjectFactory->MakeNewObject(eObjectKind_Player,dwAgentNum, &binfo);
	pPlayer->SetUniqueIDinAgent(UniqueIDinAgent);
	pPlayer->SetBattleID(num);
	pPlayer->SetGridID(num);
	pPlayer->SetChannelID(num);
	pPlayer->SetInitState(PLAYERINITSTATE_ONLY_ADDED,0);

	CHANNELSYSTEM->IncreasePlayerNum(pPlayer->GetChannelID());

	g_pUserTable->AddUser(pPlayer,pPlayer->GetID());
//	g_Console.LOG(4, "User Connected : ID %d, Total(%d)", pPlayer->GetID(), g_pUserTable->GetUserCount());
	return pPlayer;
}
*/

CPlayer* CServerSystem::InitPlayerInfo(BASEOBJECT_INFO* pBaseObjectInfo,CHARACTER_TOTALINFO* pTotalInfo,HERO_TOTALINFO* pHeroInfo)
{
	CPlayer* pPlayer = (CPlayer*)g_pUserTable->FindUser(pBaseObjectInfo->dwObjectID);
	if(pPlayer == NULL)
		return NULL;
	pBaseObjectInfo->BattleID = pPlayer->GetBattleID();
	ASSERT(pPlayer->GetID() == pBaseObjectInfo->dwObjectID);
	pPlayer->Init(eObjectKind_Player,pPlayer->GetAgentNum(),pBaseObjectInfo);
	pPlayer->InitCharacterTotalInfo(pTotalInfo);
	pPlayer->InitHeroTotalInfo(pHeroInfo);

	return pPlayer;
}

CTitan*	CServerSystem::MakeTitan( CPlayer* pOwner )
{
	CTitan* pTitan = NULL;
	BASEOBJECT_INFO BaseInfo;
	pTitan = (CTitan*)g_pObjectFactory->MakeNewObject(eObjectKind_Titan, 0, &BaseInfo);
	
	return pTitan;
}

TITAN_TOTALINFO* CServerSystem::MakeTitanInfo( TITAN_TOTALINFO* pInfo )
{
	TITAN_TOTALINFO* pNewInfo = NULL;
	pNewInfo = g_pObjectFactory->MakeTitanInfo(pInfo);

	return pNewInfo;
}

void CServerSystem::RemoveTitanInfo( TITAN_TOTALINFO* pInfo )
{
	g_pObjectFactory->ReleaseTitanInfo(pInfo);	
}

void CServerSystem::CreateNewPet( CPlayer* pMaster, DWORD dwItemIdx, DWORD SummonItemDBIdx, WORD wPetKind, WORD PetGrade /*= 1*/ )
{
	BASE_PET_LIST*	pPetListInfo;
	pPetListInfo = GAMERESRCMNGR->GetPetListInfo(wPetKind);
	if( NULL == pPetListInfo )
		return;

	PET_TOTALINFO TotalInfo;
	TotalInfo.PetSummonItemDBIdx = SummonItemDBIdx;			//¾ÆÀÌÅÛÀÇ DBID	//Ã¹»ý¼º ÀÏ¶§ ¼¼ÆÃ.
	TotalInfo.PetKind		= wPetKind;	//Á¾·ù
	TotalInfo.PetStamina	= pPetListInfo->StaminaMax[PetGrade-1];	//½ºÅ×¹Ì³ª ÃÖ´ëÄ¡
	TotalInfo.PetFriendly = GAMERESRCMNGR->GetPetRule()->DefaultFriendship;		//ÆÖ±âº»Ä£¹Ðµµ
	TotalInfo.PetGrade	= PetGrade;	//´Ü°è
	TotalInfo.PetMasterUserID	= pMaster->GetUserID();	//ÇÊ¿ä? ÇÊ¿ä DB°ü·Ã
	TotalInfo.bAlive		= TRUE;
	TotalInfo.bSummonning	= FALSE;
	TotalInfo.bRest			= TRUE;

	PetInsertToDB(pMaster->GetID(), pMaster->GetUserID(), dwItemIdx, &TotalInfo);
}

CPet* CServerSystem::AddPet( CPlayer* pMaster, DWORD dwPetObjID, PET_TOTALINFO* pTotalInfo, VECTOR3* pPos )
{
	CPet* pPet = NULL;

	if(NULL == pMaster)
		return NULL;
	if(NULL == pTotalInfo)
		return NULL;

	if(pMaster->GetUserID() != pTotalInfo->PetMasterUserID)
	{
		ASSERT(0);
		return NULL;
	}

	BASEOBJECT_INFO BaseInfo;
	BASE_PET_LIST*	pPetListInfo;

	pPetListInfo = GAMERESRCMNGR->GetPetListInfo(pTotalInfo->PetKind);
	if(NULL == pPetListInfo)
		return NULL;

	BaseInfo.dwObjectID = dwPetObjID;
	SafeStrCpy(BaseInfo.ObjectName, pPetListInfo->Name, MAX_NAME_LENGTH+1);

	pPet = (CPet*)g_pObjectFactory->MakeNewObject(eObjectKind_Pet, 0, &BaseInfo);
	pPet->InitPet(pTotalInfo);

	pPet->SetGridID(pMaster->GetGridID());
	
	pPet->SetMaster(pMaster);
	//pMaster->SetCurPet(pPet);

	pPet->SetInited();
	CCharMove::SetPosition(pPet, pPos);
	CCharMove::InitMove(pPet, pPos);

	g_pUserTable->AddUser(pPet, pPet->GetID());

	return pPet;
}


//CPet* CServerSystem::AddPet(DWORD dwPetObjID, CPlayer* pMaster, BASEOBJECT_INFO* pBaseObjectInfo, PET_TOTALINFO* pTotalInfo, WORD wPetKind, int flag /*= ePET_FROM_DB*/)
//{
////	ASSERT(g_pUserTable->FindUser(pBaseObjectInfo->dwObjectID) == NULL);
//
//	CPet* pPet = NULL;
//
//	if(flag == ePET_FROM_ITEM)	//Ã¹ »ý¼º.
//	{
////		PET_TOTALINFO	PetInfo;
//		BASEOBJECT_INFO BaseInfo;
//		BASE_PET_LIST*	pPetListInfo;
//
//		pPetListInfo = GAMERESRCMNGR->GetPetListInfo(wPetKind);
//		if(pPetListInfo == NULL)
//			return NULL;
//
//		BaseInfo.dwObjectID = dwPetObjID;
//		SafeStrCpy(BaseInfo.ObjectName, pPetListInfo->Name, MAX_NAME_LENGTH+1);
//
//		PetInfo.PetSummonItemID = SummonItemID;			//¾ÆÀÌÅÛÀÇ DBID	//Ã¹»ý¼º ÀÏ¶§ ¼¼ÆÃ.
//		PetInfo.PetKind		= wPetKind;	//Á¾·ù
//		PetInfo.PetStamina	= pPetListInfo->StaminaMax[0];	//½ºÅ×¹Ì³ª ÃÖ´ëÄ¡
//		PetInfo.PetFriendly = PET_DEFAULT_FRIENDLY;		//ÆÖ±âº»Ä£¹Ðµµ
//		PetInfo.PetGrade	= 1;	//1´Ü°è
//		PetInfo.PetMasterUserID	= pMaster->GetID();	//ÇÊ¿ä?
//		PetInfo.bAlive		= TRUE;
//		//PetInfo.MapNum		ÇÊ¿ä?
//
//		PetInsertToDB(pMaster->GetID(), &PetInfo);
//
//		//PetInsertToDB ¸®ÅÏ µÚ Ã³¸®.
//		pPet = (CPet*)g_pObjectFactory->MakeNewObject(eObjectKind_Pet, 0, &BaseInfo);
//		pPet->InitPet(pTotalInfo);
//	}
//	else
//	{
//		pPet = (CPet*)g_pObjectFactory->MakeNewObject(eObjectKind_Pet, 0, pBaseObjectInfo);
//		pPet->InitPet(pTotalInfo);
//	}
//
//	pPet->SetGridID(pMaster->GetGridID());
//	//pPet->SetInited();
//	pPet->SetMaster(pMaster);
//
//	return pPet;
//}

CMonster* CServerSystem::AddMonster(DWORD dwSubID, BASEOBJECT_INFO* pBaseObjectInfo,MONSTER_TOTALINFO* pTotalInfo,VECTOR3* pPos,WORD wObjectKind)
{
	ASSERT(g_pUserTable->FindUser(pBaseObjectInfo->dwObjectID) == NULL);

	EObjectKind objectkind = eObjectKind_Monster;
	
	if(wObjectKind == eObjectKind_SpecialMonster)objectkind = eObjectKind_SpecialMonster;
	else if(wObjectKind == eObjectKind_ToghterPlayMonster)objectkind = eObjectKind_ToghterPlayMonster;
	//2007. 7. 5. CBH - Àü¹®±â¼ú Àü¿ë ¸ó½ºÅÍ Á¾·ù ¼³Á¤ Ãß°¡
	else if(wObjectKind == eObjectKind_Mining)	objectkind = eObjectKind_Mining;
	else if(wObjectKind == eObjectKind_Collection)	objectkind = eObjectKind_Collection;
	else if(wObjectKind == eObjectKind_Hunt)	objectkind = eObjectKind_Hunt;
	//2007. 10. 30. CBH - Å¸ÀÌÅº Àü¿ë ¸ó½ºÅÍ Á¾·ù ¼³Á¤ Ãß°¡
	else if(wObjectKind == eObjectKind_TitanMonster)	objectkind = eObjectKind_TitanMonster;

	CMonster* pMonster = (CMonster*)g_pObjectFactory->MakeNewObject(objectkind,0, pBaseObjectInfo);
	pMonster->SetSubID(dwSubID);
	pMonster->InitMonster(pTotalInfo);

	pMonster->SetGridID(pBaseObjectInfo->BattleID);

	pMonster->SetInited();
	CCharMove::InitMove(pMonster,pPos);

	g_pUserTable->AddUser(pMonster,pMonster->GetID());

	//AISystem
	g_pAISystem->AddObject(pMonster);
	
	CBattle* pBattle = BATTLESYSTEM->GetBattle(pMonster->GetBattleID());
	if(pBattle->GetBattleKind() != eBATTLE_KIND_NONE)
		BATTLESYSTEM->AddObjectToBattle(pBattle, pMonster);

	return pMonster;
}

CBossMonster* CServerSystem::AddBossMonster(DWORD dwSubID, BASEOBJECT_INFO* pBaseObjectInfo,MONSTER_TOTALINFO* pTotalInfo,VECTOR3* pPos)
{
	ASSERT(g_pUserTable->FindUser(pBaseObjectInfo->dwObjectID) == NULL);

	CBossMonster* pBossMonster = (CBossMonster*)g_pObjectFactory->MakeNewObject(eObjectKind_BossMonster,0, pBaseObjectInfo);
	pBossMonster->SetSubID(dwSubID);
	pBossMonster->InitMonster(pTotalInfo);
	//pMonster->SetSMonsterList();
	pBossMonster->SetGridID(pBaseObjectInfo->BattleID);
	pBossMonster->SetInited();
	
	CCharMove::InitMove(pBossMonster,pPos);
	

	g_pUserTable->AddUser(pBossMonster,pBossMonster->GetID());



	g_pAISystem->AddObject(pBossMonster); 
	BOSSMONMGR->SetBossInfo(pBossMonster);
	
	CBattle* pBattle = BATTLESYSTEM->GetBattle(pBossMonster->GetBattleID());
	if(pBattle->GetBattleKind() != eBATTLE_KIND_NONE)
		BATTLESYSTEM->AddObjectToBattle(pBattle, pBossMonster);

	return pBossMonster;
}

// ÇÊµåº¸½º - 05.12 ÀÌ¿µÁØ
// ÇÊµåº¸½º Ãß°¡ ÇÔ¼ö
CFieldBossMonster* CServerSystem::AddFieldBossMonster(DWORD dwSubID, BASEOBJECT_INFO* pBaseObjectInfo,MONSTER_TOTALINFO* pTotalInfo,VECTOR3* pPos)
{
	ASSERT(g_pUserTable->FindUser(pBaseObjectInfo->dwObjectID) == NULL);

	CFieldBossMonster* pFieldBossMonster = (CFieldBossMonster*)g_pObjectFactory->MakeNewObject(eObjectKind_FieldBossMonster,0, pBaseObjectInfo);
	pFieldBossMonster->SetSubID(dwSubID);
	pFieldBossMonster->InitMonster(pTotalInfo);
	pFieldBossMonster->SetGridID(pBaseObjectInfo->BattleID);
	pFieldBossMonster->SetInited();
	
	CCharMove::InitMove(pFieldBossMonster,pPos);

	g_pUserTable->AddUser(pFieldBossMonster,pFieldBossMonster->GetID());

	g_pAISystem->AddObject(pFieldBossMonster); 
		
	CBattle* pBattle = BATTLESYSTEM->GetBattle(pFieldBossMonster->GetBattleID());
	if(pBattle->GetBattleKind() != eBATTLE_KIND_NONE)
		BATTLESYSTEM->AddObjectToBattle(pBattle, pFieldBossMonster);

	return pFieldBossMonster;
}

// ÇÊµåº¸½º - 05.12 ÀÌ¿µÁØ
// ÇÊµåº¸½º ºÎÇÏ Ãß°¡ ÇÔ¼ö
CFieldSubMonster* CServerSystem::AddFieldSubMonster(DWORD dwSubID, BASEOBJECT_INFO* pBaseObjectInfo,MONSTER_TOTALINFO* pTotalInfo,VECTOR3* pPos)
{
	ASSERT(g_pUserTable->FindUser(pBaseObjectInfo->dwObjectID) == NULL);

	CFieldSubMonster* pFieldSubMonster = (CFieldSubMonster*)g_pObjectFactory->MakeNewObject(eObjectKind_FieldSubMonster,0, pBaseObjectInfo);
	pFieldSubMonster->SetSubID(dwSubID);
	pFieldSubMonster->InitMonster(pTotalInfo);
	pFieldSubMonster->SetGridID(pBaseObjectInfo->BattleID);
	pFieldSubMonster->SetInited();
	
	CCharMove::InitMove(pFieldSubMonster,pPos);

	g_pUserTable->AddUser(pFieldSubMonster,pFieldSubMonster->GetID());

	g_pAISystem->AddObject(pFieldSubMonster); 
		
	CBattle* pBattle = BATTLESYSTEM->GetBattle(pFieldSubMonster->GetBattleID());
	if(pBattle->GetBattleKind() != eBATTLE_KIND_NONE)
		BATTLESYSTEM->AddObjectToBattle(pBattle, pFieldSubMonster);

	return pFieldSubMonster;
}

CNpc* CServerSystem::AddNpc(BASEOBJECT_INFO* pBaseObjectInfo,NPC_TOTALINFO* pTotalInfo,VECTOR3* pPos,float angle/* = 0*/)
{
	ASSERT(g_pUserTable->FindUser(pBaseObjectInfo->dwObjectID) == NULL);

	CNpc* pNpc = (CNpc*)g_pObjectFactory->MakeNewObject(eObjectKind_Npc,0, pBaseObjectInfo);
	pNpc->InitNpc(pTotalInfo);
	pNpc->SetGridID(pBaseObjectInfo->BattleID);

	pNpc->SetNpcAngle(angle);	//SW070704 Ãß°¡

	pNpc->SetInited();
	CCharMove::InitMove(pNpc,pPos);

	g_pUserTable->AddUser(pNpc,pNpc->GetID());
	
	CBattle* pBattle = BATTLESYSTEM->GetBattle(pNpc->GetBattleID());
	if(pBattle->GetBattleKind() != eBATTLE_KIND_NONE)
		BATTLESYSTEM->AddObjectToBattle(pBattle, pNpc);

	return pNpc;
}


CTacticObject* CServerSystem::AddTacticObject(BASEOBJECT_INFO* pBaseObjectInfo,CTacticStartInfo* pInfo,CPlayer* pOperator,DIRINDEX Direction)
{
	ASSERT(g_pUserTable->FindUser(pBaseObjectInfo->dwObjectID) == NULL);

	CTacticObject* pTactic = (CTacticObject*)g_pObjectFactory->MakeNewObject(eObjectKind_Tactic,0, pBaseObjectInfo);
	BYTE AbilLevel = 0;
	BYTE stage = pOperator->GetStage();
	if( stage == eStage_Hwa || stage == eStage_Hyun )
		AbilLevel = ABILITYMGR->GetAbilityLevel(ABILITYINDEX_TACTIC_HWA, pOperator->GetAbilityGroup());
	else if( stage == eStage_Geuk || stage == eStage_Tal )
		AbilLevel = ABILITYMGR->GetAbilityLevel(ABILITYINDEX_TACTIC_GEUK, pOperator->GetAbilityGroup());
		
	pTactic->InitTacticObject(pInfo,pOperator,Direction,AbilLevel);

	pTactic->SetInited();
	VECTOR3* pPos = CCharMove::GetPosition(pOperator);
	CCharMove::InitMove(pTactic,pPos);

	g_pUserTable->AddUser(pTactic,pTactic->GetID());
	
	return pTactic;
}

CSkillObject* CServerSystem::AddSkillObject(CSkillObject* pSkillObj,VECTOR3* pPos)
{
	ASSERT(g_pUserTable->FindUser(pSkillObj->GetID()) == NULL);

	pSkillObj->SetInited();
	CCharMove::InitMove(pSkillObj,pPos);

	g_pUserTable->AddUser(pSkillObj,pSkillObj->GetID());
	
	return pSkillObj;
}

CMapObject* CServerSystem::AddMapObject(DWORD Kind, BASEOBJECT_INFO* pBaseObjectInfo, MAPOBJECT_INFO* pMOInfo, VECTOR3* pPos)
{
	ASSERT(g_pUserTable->FindUser(pBaseObjectInfo->dwObjectID) == NULL);

	CMapObject* pMapObject = (CMapObject*)g_pObjectFactory->MakeNewObject((EObjectKind)Kind,0, pBaseObjectInfo);
	
	pMapObject->InitMapObject( pMOInfo );
	pMapObject->SetGridID( pBaseObjectInfo->BattleID );
	pMapObject->SetInited();
	CCharMove::InitMove( pMapObject, pPos );

	g_pUserTable->AddUser( pMapObject, pMapObject->GetID() );
	
	CBattle* pBattle = BATTLESYSTEM->GetBattle(pMapObject->GetBattleID());
	if(pBattle->GetBattleKind() != eBATTLE_KIND_NONE)
		BATTLESYSTEM->AddObjectToBattle( pBattle, pMapObject );

	return pMapObject;
}

/*
CItemObject* CServerSystem::AddItemObject(ITEMBASE* pItemInfo,VECTOR3* pPos)
{
	//ASSERT(GetNpc(pBaseObjectInfo->dwObjectID) == NULL);

	CItemObject* pItemObject = (CItemObject*)g_pObjectFactory->MakeNewObject(eObjectKind_Item,0,pItemInfo->MapNum,NULL);
	pItemObject->InitItem(pItemInfo);

	CCharMove::InitMove(pItemObject,pPos);

	m_ItemObjectTable.Add(pItemObject,pItemObject->GetID());
	pItemObject->SetInited();
	return pItemObject;
}
*/	

//SW060608 DBUpdate Safely
void CServerSystem::HandlingBeforeServerEND()
{
	//SetUnableProcess(TRUE);				//1. Process ¸®ÅÏ & DBReturnMsg Ã³¸® ¾ÈÇÔ.

	//g_Console.LOG(4, "[0/4]Start Working For Map Server Shutdown" );

	RemoveServerForKind(AGENT_SERVER);	//2. AgentSrv ²÷±â.
	//g_Console.LOG(4, "[1/4]Agent Disconnecting...Done" );

	//3. =======Object Ã³¸®
	g_pUserTable->SetPositionHead();
	CObject* pObject;
	while(pObject = g_pUserTable->GetData())
	{	
		if(pObject->GetObjectKind() == eObjectKind_Player)
			g_pServerSystem->RemovePlayer(pObject->GetID(), FALSE);
			//FALSE: ½ÇÁ¦·Î UserTable¿¡¼­´Â Áö¿ìÁö ¾Ê´Â´Ù.
		else
		{
			CCharMove::ReleaseMove(pObject);
			g_pObjectFactory->ReleaseObject(pObject);
			BYTE objectKind = pObject->GetObjectKind();
			/*
			if( objectKind == eObjectKind_Monster ||
				objectKind == eObjectKind_BossMonster ||
				objectKind == eObjectKind_TitanMonster )
				*/
			if( objectKind & eObjectKind_Monster )
			{
				if(g_pAISystem)
					g_pAISystem->RemoveObject(pObject->GetID());
			}
		}
	}
	//g_Console.LOG(4, "[2/4]Start Object Handling (Update/Delete)...Done" );

	g_pUserTable->RemoveAll();
	//g_Console.LOG(4, "[3/4]Removing All Object on UserTable...Done" );

	//g_Console.LOG(4, "[4/4]Start DB Release..." );
	g_DB.Release();
	//g_Console.LOG(4, "[4/4]Release...Done" );

	SetCharUpdateCompletion(TRUE);
}


void CServerSystem::RemovePlayer(DWORD dwPlayerID, BOOL bRemoveFromUserTable )
{
	CPlayer* pPlayer = (CPlayer*)g_pUserTable->FindUser(dwPlayerID);
	if(!pPlayer)
		return;
	ASSERT(pPlayer->GetObjectKind() == eObjectKind_Player);

//	FRIENDMGR->UserLogOut(pPlayer->GetID());
	EXCHANGEMGR->UserLogOut(pPlayer);
	STREETSTALLMGR->UserLogOut(pPlayer);
	VIMUMGR->UserLogOut(pPlayer);
	LOOTINGMGR->LootingCancel(pPlayer);	//PKLOOT
	BOSSMONMGR->UserLogOut(pPlayer->GetID(), pPlayer->GetGridID());
	SIEGEWARMGR->UserLogOut(pPlayer);
	
	// YH d¡§¡þ¡Ë¡þ ¡§u¢®A¡Íi¢®IAI¡§¢®¡Ëc¡Ë¡ÍA AE¢®¨úaE¢®¨Ï¢®¨¡¡Ë¢ç f¡Ë¡Íe¢®¢´I ¡ÍiE /Au¢®¨¡¡Ë¢ç ¨Ï©ª¡§¡Ì¢®¨¡¢®I¡ËO¢®¡¿¡Ë¡þ¡Ë¡þ C¡§¨£A¡§¨£¡Ë¡ÍU.
	if(pPlayer->GetInited() == TRUE)
	{
		if(pPlayer->GetBattle()->GetBattleKind() == eBATTLE_KIND_SURYUN)
		{
			pPlayer->SetLife(pPlayer->GetMaxLife(),FALSE);
			pPlayer->SetShield(pPlayer->GetMaxShield(),FALSE);
			pPlayer->SetNaeRyuk(pPlayer->GetMaxNaeRyuk(),FALSE);
		}		

		//Ac¢®iy ¢®¨¡u¢®¢´A ¢®¨¡¨Ï¢®CeA¡Ë¢ç, ¢®iy¡Ë¡þi¢®¢´A, ¨Ï©ª¢®i¢®¢´A ¢®¨úiAO. (Data¡Ë¡þ| AuAaCI¢®¨úa Au¡Ë?¡Ë¢ç ¢®¨¡¨Ï¢®CeA¡Ë¢ç¡Ë?¡Ë¢ç ¡Ë¡ÍeCN ¡§¡þI¡§¡þ¡§¡Ë; A¨Ï©ª¡Ë¡þ¡ËcC¡§¨£¡§u¨Ï¡þ CO¡Íii.)
		//Result¡ÍiC¡§ui¡§u¢®¨Ï KINE_NONEAI ¡ÍiC¡§uu¡Ë¡ÍU¡Ë¡þe?
		if(pPlayer->GetState() == eObjectState_Die)
		{
#ifdef _KOR_LOCAL_
			if( pPlayer->IsActionPanelty() == FALSE )
			{
				pPlayer->InsetNoActionPaneltyLog( eExpLog_NoActionPanelty_Exit );
				pPlayer->SetActionPanelty();
			}
			else
#endif
			{
				if( ( pPlayer->GetBattle()->GetBattleKind() == eBATTLE_KIND_NONE && pPlayer->IsPenaltyByDie() ) ||
					pPlayer->GetBattle()->GetBattleKind() == eBATTLE_KIND_SIEGEWAR )
				{
					pPlayer->ReviveLogInPenelty();
				}
				else	//¡§¡þn¨Ïo¢®iA¨Ï¡þ A¢®¢¯¡§uu¡Ë¡ÍU.	//¡§¡þn¨Ïo¢®iA¨Ï¡þ ¡Ë¡þo¡§o¡§¡þAI¡Ë?¡Ë¢ç¢®¨¡O A¢®¢¯; ¡§uo¡Íi¡Íi AO¡Ë¡ÍU. confirm
				{
					pPlayer->ReviveAfterVimu( FALSE );	//¡Æa¡ÆuA©ø¢¬¢ç¢¬¢¬.. msg¨¬¢¬©ø¡íAo ¨úE¢¥A¢¥U.6
				}
			}
		}

		// A¨Ï©ª¡Ë¡þ?d¡§¡þ¡Ë¡þ ¡§u¢®A¡Íi¢®IAI¡§¢®¡Ëc///////////////////////
		if( pPlayer->IsPKMode() )
			pPlayer->SetPKModeEndtime();

		CharacterHeroInfoUpdate(pPlayer);
		CharacterTotalInfoUpdate(pPlayer);
		CharacterMugongInfoUpdate(pPlayer);
		CharacterSkinInfoUpdate(pPlayer); 
		///////////////////////////////////////////

		CCharMove::ReleaseMove(pPlayer);
		pPlayer->GetPetManager()->RemovePet();

//		g_Console.Log(eLogDisplay, 4, "DisconnectUser : %d %s", dwPlayerID, pPlayer->GetObjectName());
		
		BOOL rt = BATTLESYSTEM->DeleteObjectFromBattle(pPlayer);
	}
	else		
	{
		CCharMove::ReleaseMove(pPlayer);
		pPlayer->GetPetManager()->RemovePet();
	}

	CHANNELSYSTEM->DecreasePlayerNum(pPlayer->GetChannelID());

	//SW061019 ¼­¹ÙÀÌ¹ú¸ðµå 
	//if( CheckMapKindIs(eSurvival) )
	if( g_pServerSystem->GetMap()->IsMapKind(eSurvivalMap) )
		SVVMODEMGR->RemoveSVModeUser(pPlayer);

	if(bRemoveFromUserTable)
		g_pUserTable->RemoveUser(dwPlayerID);

	g_pObjectFactory->ReleaseObject(pPlayer);
	UnRegistLoginMapInfo(dwPlayerID); //db¡Ë?¡Ë¢ç CoAc ¡Ë¡þE¨Ïo¨Ï¨£E¢®I 08¢®¢´I ¡§uA¡§¢®A
	
	EVENTMAPMGR->RemoveMember( pPlayer, 0, pPlayer->GetChannelID() );
	PARTYWARMGR->RemovePlayer( pPlayer );
	QUESTMAPMGR->RemovePlayer( pPlayer );

	BOBUSANGMGR->LeaveGuest( pPlayer );

	FORTWARMGR->RemovePlayer( pPlayer );

//	g_Console.LOG(4, "User Disconnected : ID %d, Total(%d)", dwPlayerID, g_pUserTable->GetUserCount());
}

void CServerSystem::RemoveTitan( CTitan* pTitan )
{
	g_pObjectFactory->ReleaseObject((CObject*)pTitan);
}

void CServerSystem::RemovePet( DWORD dwPetObjectID )
{
	CObject* pObject = g_pUserTable->FindUser(dwPetObjectID);
	ASSERT(pObject);
	if(!pObject)	return;

	CCharMove::ReleaseMove(pObject);

	g_pUserTable->RemoveUser(dwPetObjectID);

	g_pObjectFactory->ReleaseObject(pObject);
}

void CServerSystem::RemoveMonster(DWORD dwMonster)
{
	CObject* pObject = g_pUserTable->FindUser(dwMonster);
	ASSERT(pObject);
	if(!pObject)	return;

	CCharMove::ReleaseMove(pObject);
	
	BOOL rt = BATTLESYSTEM->DeleteObjectFromBattle(pObject);
	ASSERT(rt);

	if(g_pAISystem)
	g_pAISystem->RemoveObject(dwMonster);
	
	g_pUserTable->RemoveUser(dwMonster);

	g_pObjectFactory->ReleaseObject(pObject);
}


void CServerSystem::RemoveBossMonster( DWORD dwMonster, BOOL bDisappear )
{
	CObject* pObject = g_pUserTable->FindUser(dwMonster);
	ASSERT(pObject);
	if(!pObject)	return;

	CCharMove::ReleaseMove(pObject);
	
	BOOL rt = BATTLESYSTEM->DeleteObjectFromBattle(pObject);
	ASSERT(rt);

	if(g_pAISystem && !bDisappear)
		g_pAISystem->RemoveObject(dwMonster);

	g_pUserTable->RemoveUser(dwMonster);

	g_pObjectFactory->ReleaseObject(pObject);

//CONDITIONMGR->RemoveListToPool(dwMonster);

}

void CServerSystem::RemoveNpc(DWORD dwNpcID)
{
	CObject* pObject = g_pUserTable->FindUser(dwNpcID);
	ASSERT(pObject);
	if(!pObject)	return;
	
	CCharMove::ReleaseMove(pObject);
	
	BOOL rt = BATTLESYSTEM->DeleteObjectFromBattle(pObject);
	ASSERT(rt);
	if(!pObject)	return;

	g_pUserTable->RemoveUser(dwNpcID);
	g_pObjectFactory->ReleaseObject(pObject);
}

void CServerSystem::RemoveTacticObject(DWORD dwTacticObjID)
{
	CObject* pObject = g_pUserTable->FindUser(dwTacticObjID);
	ASSERT(pObject);
	if(!pObject)	return;
	
	CCharMove::ReleaseMove(pObject);
	
	g_pUserTable->RemoveUser(dwTacticObjID);
	g_pObjectFactory->ReleaseObject(pObject);
}
void CServerSystem::RemoveSkillObject(DWORD SkillObjectID)
{
	CObject* pObject = g_pUserTable->FindUser(SkillObjectID);
	//ASSERT(pObject);
	if(!pObject)	return;	

	CCharMove::ReleaseMove(pObject);
	g_pUserTable->RemoveUser(SkillObjectID);
	g_pObjectFactory->ReleaseObject(pObject);
}

void CServerSystem::RemoveMapObject( DWORD MapObjID )
{
	CObject* pObject = g_pUserTable->FindUser(MapObjID);
	ASSERT(pObject);
	if(!pObject)	return;
	
	CCharMove::ReleaseMove(pObject);
	
	BOOL rt = BATTLESYSTEM->DeleteObjectFromBattle(pObject);
	ASSERT(rt);
	if(!pObject)	return;

	g_pUserTable->RemoveUser(MapObjID);
	g_pObjectFactory->ReleaseObject(pObject);
}


void CServerSystem::SendToOne(CObject* pObject,void* pMsg,int MsgLen)
{
	g_Network.Send2Server(pObject->GetAgentNum(),(char*)pMsg,MsgLen);
}

void CServerSystem::ReloadResourceData()	// case MP_CHEAT_RELOADING:
{
	//////////////////////////////////////////////////////////////////////////
	//SkillList
	SKILLMGR->Release();
	SKILLMGR->Init();
	
	//////////////////////////////////////////////////////////////////////////
	//¢®¨¡¨Ï¢®CeA¡Ë¢ç
	GAMERESRCMNGR->LoadPlayerxMonsterPoint();

	//////////////////////////////////////////////////////////////////////////
	//ItemList
	ITEMMGR->ReloadItemList();
	ITEMMGR->LoadReinforceItemInfo();

	//////////////////////////////////////////////////////////////////////////
	//Regen
	//¡Ë¡þo¡§o¡§¡þAI Remove
	g_pUserTable->SetPositionUserHead();
	CObject* pObject;
	while(pObject = (CObject*)g_pUserTable->GetUserData())
	{
		BYTE objectKind = pObject->GetObjectKind();

		switch(objectKind)
		{
		case eObjectKind_Monster:
		case eObjectKind_Mining:
		case eObjectKind_Collection:
		case eObjectKind_Hunt:
		case eObjectKind_TitanMonster:
			{
				RemoveMonster(pObject->GetID());
				break;
			}
		case eObjectKind_BossMonster:
			{
				RemoveBossMonster(pObject->GetID());
				break;
			}
		case eObjectKind_Npc:
			{
				RemoveNpc(pObject->GetID());
				break;
			}		
		}
	}
	g_pAISystem->RemoveAllList();

	REGENMGR->Release();
	GROUPMGR->Release();
	
	//SW051129 Pet
	GAMERESRCMNGR->ResetPetList();
	GAMERESRCMNGR->LoadPetList();
	GAMERESRCMNGR->LoadPetRule();
	GAMERESRCMNGR->ResetPetBuffList();
	GAMERESRCMNGR->LoadPetBuffList();

	//SW070127 Å¸ÀÌÅº
	GAMERESRCMNGR->RemoveTitanList();
	GAMERESRCMNGR->LoadTitanList();
	GAMERESRCMNGR->LoadTitanRule();
	GAMERESRCMNGR->LoadTitanSpellCostInfo();
//	GAMERESRCMNGR->RemoveTitanSkillLink();
//	GAMERESRCMNGR->LoadTitanSkillLink();
//	GAMERESRCMNGR->RemoveTitanEquipEnduranceInfo();
//	GAMERESRCMNGR->LoadTitanEquipEnduranceInfo();

	//////////////////////////////////////////////////////////////////////////
	//MonsterList
	GAMERESRCMNGR->ResetMonsterList();
	GAMERESRCMNGR->LoadMonsterList();
	GAMERESRCMNGR->LoadSMonsterList();

	GAMERESRCMNGR->LoadPaneltyTime();

	//SW050901
	MON_SPEECHMGR->LoadMonSpeechInfoList();
	//SW050916
	RAREITEMMGR->LoadRareItemOptionInfo();
	RAREITEMMGR->LoadRareItemInfo();
	RAREITEMMGR->LoadRareValueProbList();
//	MON_SPEECHMGR->LoadSpeechList();
	//////////////////////////////////////////////////////////////////////////

    // 2007. 12. 5. CBH - ÀÏ¹Ý ÀÇº¹ ½ºÅ² ¾ÆÀÌÅÆ ¸ñ·Ï ·Îµå
	GAMERESRCMNGR->LoadNomalClothesSkinList();
	// 2008. 1. 21. CBH - ÄÚ½ºÆ¬ ½ºÅ² ¾ÆÀÌÅÆ ¸ñ·Ï ·Îµå
	GAMERESRCMNGR->LoadCostumeSkinList();

	//ItemDrop
	ITEMDROP_OBJ->Init();

	// ¸Ê µå¶ø ¾ÆÀÌÅÛ Ãß°¡ by Stiner(2008/05/29)-MapDropItem
	MAPITEMDROP_OBJ->Init();
	
	// reload
	g_pAISystem->LoadAIGroupList();

}


void CServerSystem::AddWaitExitPlayer( DWORD dwAgentConnetionIndex, DWORD dwPlayerID )
{
	WAIT_EXIT_PLAYER_INFO* pInfo = m_mpWaitExitPlayer->Alloc();
	pInfo->dwAgentConnectionIndex = dwAgentConnetionIndex;
	pInfo->dwPlayerID = dwPlayerID;
	pInfo->dwStartTime = gCurTime;
	m_listWaitExitPlayer.AddTail( pInfo );
}

void CServerSystem::ProcessWaitExitPlayer()
{
	if( m_listWaitExitPlayer.GetCount() == 0 ) return;

	WAIT_EXIT_PLAYER_INFO* pInfo = (WAIT_EXIT_PLAYER_INFO*)m_listWaitExitPlayer.GetHead();
	if( pInfo )
	{
		if( gCurTime - pInfo->dwStartTime >= 5*1000 )	//10ÃÊ--->5ÃÊ·Î º¯°æ(2005.11.1)
		{
			CPlayer* pPlayer = (CPlayer*)g_pUserTable->FindUser( pInfo->dwPlayerID );
			if( pPlayer )
			{
				DWORD dwObjectID = pPlayer->GetID();
				DWORD dwUserID = pPlayer->GetUserID();

				// RaMa - 04.12.14
				pPlayer->UpdateLogoutToDB();

				PARTYMGR->UserLogOut( pPlayer );
				FRIENDMGR->UserLogOut( dwObjectID );
				GUILDMGR->UserLogOut( pPlayer );

				g_pServerSystem->RemovePlayer( dwObjectID );
				//
				ConfirmUserOut( pInfo->dwAgentConnectionIndex, dwUserID, dwObjectID, 2 );
			}

			m_mpWaitExitPlayer->Free( pInfo );
			m_listWaitExitPlayer.RemoveHead();
		}
	}
}

void CServerSystem::SetNation()
{
	CMHFile file;
	if( !file.Init( "LocalizingInfo.txt", "rt", MHFILE_FLAG_DONOTDISPLAY_NOTFOUNDERROR ) )
		return;

	if( strcmp( file.GetString(), "*NATION" ) == 0 )
	{
		if( strcmp( file.GetString(), "CHINA" ) == 0 )
		{
			m_Nation = eNATION_CHINA;
		}
	}

	file.Release();
}

void CServerSystem::RemoveServerForKind( WORD wSrvKind )
{
	g_pServerTable->RemoveServerForKind(wSrvKind);
}


void CServerSystem::LoadHackCheck()
{
	CMHFile file;
	if( !file.Init( "ServerSet/HackCheck.txt", "rt", MHFILE_FLAG_DONOTDISPLAY_NOTFOUNDERROR ) )
	{
		g_Console.MLOG(4, "HackCheckNum : %d, HackCheckWriteNum : %d", g_nHackCheckNum, g_nHackCheckWriteNum );
		return;
	}

	char temp[256] = {0, };
	while( 1 )
	{
		if( file.IsEOF() )	break;

		file.GetString( temp );
		if( strcmp( temp, "*HACKCHECKNUM" ) == 0 )
		{
			g_nHackCheckNum = file.GetInt();
		}
		else if( strcmp( temp, "*HACKCHECKWRITENUM" ) == 0 )
		{
			g_nHackCheckWriteNum = file.GetInt();
		}
	}

	file.Release();

	g_Console.MLOG(4, "HackCheckNum : %d, HackCheckWriteNum : %d", g_nHackCheckNum, g_nHackCheckWriteNum );
}


void CServerSystem::CheckServerInfo( DWORD ObjectID )
{
	memset( &m_ProcessInfo, 0, sizeof(m_ProcessInfo) );
	SetProtocol( &m_ProcessInfo,MP_CHEAT, MP_CHEAT_MAP_CONDITION );
	m_ProcessInfo.dwObjectID = ObjectID;
	m_bCheckInfo = TRUE;

	TestGameQuery( ObjectID );
	TestLogQuery( ObjectID );

	m_ProcessInfo.ProcessInfo.DBQueryCount = g_DB.GetQueryQueueCount();
	m_ProcessInfo.StarTime = gCurTime;
}



// global function
void OnConnectServerSuccess(DWORD dwConnectionIndex, void* pVoid)
{
	SERVERINFO* info = (SERVERINFO*)pVoid;
	info->dwConnectionIndex = dwConnectionIndex;
	if(info->wServerKind == MONITOR_SERVER)
	{
		BOOTMNGR->NotifyBootUpToMS(&g_Network);
		g_Console.MLOG(4, "Connected to the MS : %s, %d, (%d)", info->szIPForServer, info->wPortForServer, dwConnectionIndex);
	}
	else
	{
		BOOTMNGR->SendConnectSynMsg(&g_Network, dwConnectionIndex, g_pServerTable);
		g_Console.MLOG(4, "Connected to the Server : %s, %d, (%d)", info->szIPForServer, info->wPortForServer, dwConnectionIndex);
	}
	/*/// Ao¡Ë?i¢®¨¡IAO taiyo
	if(!dwConnectionIndex) return;

	info->dwConnectionIndex = dwConnectionIndex;					// AI¢®¨¡O f¡Ë¡Íe¢®¢´I ¡Íi¡Ë?AUCI¨Ï©ª¡§¡Ì? A¡§uA¡§I
	MSG_WORD msg;
	msg.Category = MP_SERVER;
	msg.Protocol = MP_SERVER_PWRUP;
	msg.wData = g_pServerTable->GetSelfServer()->wPortForServer;
	g_Network.Send2Server(dwConnectionIndex, (char*)&msg, sizeof(msg));
	//*/

	//PACKDATA¡Ë?¡Ë¢ç¡§u¢®¨Ï ¢®ic¡Ë?eCI¡Ë¡ÍA ¡§¡þ?¡§uo
	if(g_pServerTable->GetMaxServerConnectionIndex() < dwConnectionIndex)
			g_pServerTable->SetMaxServerConnectionIndex(dwConnectionIndex);
}

void OnConnectServerFail(void* pVoid)
{
	SERVERINFO* info = (SERVERINFO*)pVoid;
	if(info->wServerKind == MONITOR_SERVER)
	{
		BOOTMNGR->AddBootListINI(MAP_SERVER, GAMERESRCMNGR->GetLoadMapNum(), g_pServerTable);
		BOOTMNGR->BactchConnectToMap(&g_Network, g_pServerTable);
		g_Console.MLOG(4, "Failed to Connect to the MS : %s, %d", info->szIPForServer, info->wPortForServer);
	}
	else
	{
		g_Console.MLOG(4, "Failed to Connect to the Server : %s, %d", info->szIPForServer, info->wPortForServer);
		BOOTMNGR->RemoveBootList(g_pServerTable, info->wPortForServer);
	}	
}

void OnAcceptServer(DWORD dwConnectionIndex)
{
	g_Console.MLOG(4, "Server Connected : ConnectionIndex %d", dwConnectionIndex);
}

void OnDisconnectServer(DWORD dwConnectionIndex)
{
	if( g_pServerSystem->GetStart() == FALSE ) return;

	g_Console.MLOG(4, "Server Disconnected : ConnectionIndex %d", dwConnectionIndex);

	SERVERINFO * delInfo = g_pServerTable->RemoveServer(dwConnectionIndex);
	if(!delInfo)
	{
		ASSERT(0);
	}
	delete delInfo;


	// taiyo
	g_pUserTable->SetPositionUserHead();
	CObject * info = NULL;
	cPtrList list;

	while(info = g_pUserTable->GetUserData())
	{
		if(info->GetObjectKind() == eObjectKind_Player)
		{
			CPlayer * ply = (CPlayer *)info;
			if(ply->GetAgentNum() == dwConnectionIndex)
			{
				SaveMapChangePointUpdate(ply->GetID(), 0);
				//g_pServerSystem->RemovePlayer(ply->GetID());
				list.AddTail( ply );
			}
		}
	}
	PTRLISTPOS pos = list.GetHeadPosition();
	while( pos )
	{
		CPlayer* p = (CPlayer*)list.GetNext(pos);
		g_pServerSystem->RemovePlayer(p->GetID());
	}
	list.RemoveAll();
}

void OnAcceptUser(DWORD dwConnectionIndex)
{
}

void OnDisconnectUser(DWORD dwConnectionIndex)
{

}

void ProcessDBMessage()
{
	if( g_bCloseWindow == TRUE )
		return;
	g_DB.ProcessingDBMessage();
}

void ReceivedMsgFromServer(DWORD dwConnectionIndex,char* pMsg,DWORD dwLength)
{
	MSGROOT* pTempMsg = reinterpret_cast<MSGROOT*>(pMsg);
	ASSERT(pTempMsg->Category < MP_MAX);
	
	if( g_pServerMsgParser[pTempMsg->Category] == NULL ||
		pTempMsg->Category >= MP_MAX ||
		pTempMsg->Category == 0)
		return;

	g_pServerMsgParser[pTempMsg->Category](dwConnectionIndex, pMsg, dwLength);
}

void ReceivedMsgFromUser(DWORD dwConnectionIndex,char* pMsg,DWORD dwLength)
{
	MSGROOT* pTempMsg = reinterpret_cast<MSGROOT*>(pMsg);
	ASSERT(pTempMsg->Category < MP_MAX);
	ASSERT(g_pUserMsgParser[pTempMsg->Category]);
	
	if( g_pUserMsgParser[pTempMsg->Category] == NULL ||
		pTempMsg->Category >= MP_MAX ||
		pTempMsg->Category == 0)
		return;

	g_pUserMsgParser[pTempMsg->Category](dwConnectionIndex, pMsg, dwLength);
}

void GameProcess()
{
	g_pServerSystem->Process();
}
void _5minGameProcess()
{
	g_pServerSystem->_5minProcess();

	// ¸Ê ¾ÆÀÌÅÛ µå¶ø Ãß°¡ by Stiner(2008/05/29)-MapItemDrop
	MAPITEMDROP_OBJ->Process();
}


// console interface function
void ButtonProc1()
{
	g_Console.MLOG( 4, "QueryCount: %d, GetDB: %p", g_DB.GetQueryQueueCount(), g_DB.GetCurDB() );
/*
	g_bAssertMsgBox = !g_bAssertMsgBox;
	if(g_bAssertMsgBox)
		g_Console.LOG(4,"Assert MsgBox is On");
	else
		g_Console.LOG(4,"Assert MsgBox is Off");
*/
}


extern BOOL g_bWriteQuery;
void ButtonProc2()
{
	g_pServerSystem->LoadHackCheck();

	/*
		g_bWriteQuery = !g_bWriteQuery;
			if(g_bWriteQuery)
				g_Console.LOG(4,"g_bWriteQuery is True");
			else
				g_Console.LOG(4,"g_bWriteQuery is False");*/		
}

void ButtonProc3()
{
	if( g_pServerSystem )
		g_Console.MLOG( 4, "MAIN PROCESS TIME : %d", g_pServerSystem->GetMainProcessTime() );
/*
	CObject* pInfo = NULL;
	g_pUserTable->SetPositionHead();
	while(pInfo = g_pUserTable->GetData())
	{
		if(pInfo->GetObjectKind() != eObjectKind_Player)
		{
			if(pInfo->GetGridID() == 1)
			{
				pInfo->Die(NULL);
			}
		} 
	}
*/
}

void OnCommand(char* szCommand)
{

}

BOOL LoadEventRate(char* strFileName)
{
	for(int i=0; i<eEvent_Max; ++i)	
	{
		gEventRate[i] = 1.f;
		gEventRateFile[i] = 1.f;
	}

	CMHFile file;
#ifdef _FILE_BIN_
	if( !file.Init( strFileName, "rb") )
		return FALSE;
#else
	if( !file.Init( strFileName, "rt") )
		return FALSE;
#endif

	char Token[256];
	
// RaMa -04.11.24
	while( !file.IsEOF() )
	{
		file.GetString(Token);

		if(strcmp(Token,"#EXP") == 0)	
		{			
			//gExpRate = file.GetFloat();
			gEventRateFile[eEvent_ExpRate] = file.GetFloat();
		}
		else if(strcmp(Token, "#ABIL") == 0)
		{
			//gAbilRate = file.GetFloat();
			gEventRateFile[eEvent_AbilRate] = file.GetFloat();
		}
		else if(strcmp(Token,"#ITEM") == 0)
		{
			//gItemRate = file.GetFloat();
			gEventRateFile[eEvent_ItemRate] = file.GetFloat();
		}
		else if(strcmp(Token,"#MONEY") == 0)
		{
			//gMoneyRate = file.GetFloat();
			gEventRateFile[eEvent_MoneyRate] = file.GetFloat();
		}
		else if(strcmp(Token,"#GETMONEY") == 0)
		{
			//gGetMoney = file.GetFloat();
			gEventRateFile[eEvent_GetMoney] = file.GetFloat();
		}
		else if(strcmp(Token,"#DAMAGERECIVE") == 0)
		{
			//gDamageReciveRate = file.GetFloat();
			gEventRateFile[eEvent_DamageReciveRate] = file.GetFloat();
		}
		else if(strcmp(Token,"#DAMAGERATE") == 0)
		{
			//gDamageRate = file.GetFloat();
			gEventRateFile[eEvent_DamageRate] = file.GetFloat();
		}
		else if(strcmp(Token,"#NAERYUKSPEND") == 0)
		{
			//gNaeRuykRate = file.GetFloat();
			gEventRateFile[eEvent_NaeRuykRate] = file.GetFloat();
		}
		else if(strcmp(Token,"#UNGISPEED") == 0)
		{
			//gUngiSpeed = file.GetFloat();
			gEventRateFile[eEvent_UngiSpeed] = file.GetFloat();
		}
		else if(strcmp(Token,"#PARTYEXP") == 0)
		{
			//gPartyExpRate = file.GetFloat();
			gEventRateFile[eEvent_PartyExpRate] = file.GetFloat();
		}
		else if(strcmp(Token,"#MUGONGEXPRATE") == 0)
		{
			gEventRateFile[eEvent_MugongExp] = file.GetFloat();
		}
	}


	file.Release();

	for(i=0; i<eEvent_Max; ++i)	
		gEventRate[i] = gEventRateFile[i];

	return TRUE;
}



BOOL LoadPartyPlustime(char* strFileName)
{
	memset( &gPartyEvent, 0, sizeof(PARTYEVENT)*ePartyEvent_Max );

	CMHFile file;
#ifdef _FILE_BIN_
	if( !file.Init( strFileName, "rb") )
		return FALSE;
#else
	if( !file.Init( strFileName, "rt") )
		return FALSE;
#endif

	int count = file.GetInt();	

	for( int i=0; i<count; ++i )
	{
		int idx = file.GetInt();
		gPartyEvent[idx].Rate[0] = file.GetFloat();
		gPartyEvent[idx].Rate[1] = file.GetFloat();
		gPartyEvent[idx].Rate[2] = file.GetFloat();
		gPartyEvent[idx].Rate[3] = file.GetFloat();
		gPartyEvent[idx].Rate[4] = file.GetFloat();
		gPartyEvent[idx].Rate[5] = file.GetFloat();
		gPartyEvent[idx].Rate[6] = file.GetFloat();
	}

	return TRUE;
}
