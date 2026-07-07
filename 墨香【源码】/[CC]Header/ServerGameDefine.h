#ifndef __SERVERGAMEDEFINE_H__
#define __SERVERGAMEDEFINE_H__



#define MAX_TOTAL_PLAYER_NUM		800
#define MAX_TOTAL_TITAN_NUM			800
#define MAX_TITANINFO_NUM			800
#define MAX_TOTAL_PET_NUM			800
#define MAX_TOTAL_MONSTER_NUM		1000
#define MAX_TOTAL_BOSSMONSTER_NUM	10
#define MAX_TOTAL_NPC_NUM			50
#define MAX_TOTAL_TACTIC_NUM		200
#define MAX_MAPOBJECT_NUM			20



#define GRID_BIT 5


extern DWORD gCurTime;
extern DWORD gTickTime;

// 째챈횉챔횆징 & 쩐횈?횑횇횤 & 쨉쨌 쨉책쨋첩?짼 횁쨋?첵 - RaMa 04.10.16 
extern float gExpRate;
extern float gAbilRate;
extern float gItemRate;
extern float gMoneyRate;
//
extern float gDamageReciveRate;		// 쨔횧쨈횂 쨉짜쨔횑횁철
extern float gDamageRate;			// 횁횜쨈횂 쨉짜쨔횑횁철
extern float gNaeRuykRate;			// 쨀쨩쨌횂쩌횘쨍챨
extern float gUngiSpeed;			// 쩔챤짹창횁쨋쩍횆 쩍쨘횉횉쨉책
extern float gPartyExpRate;			// 횈횆횈쩌째챈횉챔횆징
extern float gGetMoney;				// 쩐챵쨈횂쨉쨌?횉 쨔챔쩌철
extern float gMugongExpRate;	// 무공경험치 배수
// Etc
extern float gShield;				// 횊짙쩍횇째짯짹창
extern float gDefence;				// 쨔챈쩐챤쨌횂

extern float gEventRate[];
extern float gEventRateFile[];
struct PARTYEVENT;
extern PARTYEVENT gPartyEvent[];

#define _SERVER_RESOURCE_FIELD_			// 쩌짯쨔철쨍쨍?쨩 짹쨍쨘횖횉횕쨈횂 쩔쨉쩔짧

#define START_LOGIN			0
#define CHANGE_LOGIN		1

#define MONSTERGROUPUNIT	2
#define NPCGROUPUINT		1


#define OBJ_REGEN_START_INDEX	100001

enum SERVER_KIND
{
	ERROR_SERVER,
	DISTRIBUTE_SERVER,
	AGENT_SERVER,
	MAP_SERVER,
	CHAT_SERVER,
	MURIM_SERVER,
	MONITOR_AGENT_SERVER,
	MONITOR_SERVER,
	BUDDYAUTH_SERVER,
	MAX_SERVER_KIND,
};
#define MAX_IPADDRESS_SIZE	16
#define MAX_MAP_NUM		100
#define MAX_SERVER_COUNT	30

enum CHEAT_LOG
{
	eCHT_Item,
	eCHT_Money,
	eCHT_Hide,
	eCHT_AddMugong,
	eCHT_MugongSung,
	eCHT_LevelUp,
	eCHT_AbilityExp,
	eCHT_Gengol,
	eCHT_Minchub,
	eCHT_Cheryuk,
	eCHT_Simmak,
};

enum eBOSSEVENT
{
	eBOSSEVENT_LIFE,
};

enum ebossstate
{	
	//////////////////////////////////////////////////////////////////////////
	//NORMAL STATE
	eBossState_Stand,
	eBossState_WalkAround,
	eBossState_Pursuit,
	eBossState_RunAway,
	eBossState_Attack,

	//////////////////////////////////////////////////////////////////////////
	//EVENT STATE
	eBossState_Recover,
	eBossState_Summon,
			
	eBossState_Max,
};

enum ebossaction
{
	eBOSSACTION_RECOVER = 1,
	eBOSSACTION_SUMMON,
};

enum ebosscondition
{
	eBOSSCONDITION_LIFE = 1,	
};

enum eGuildLog
{
	// member
	eGuildLog_MemberSecede = 1,
	eGuildLog_MemberBan,
	eGuildLog_MemberAdd,

	// master 
	eGuildLog_MasterChangeRank = 100,
	

	// guild
	eLogGuild_GuildCreate = 200,
	eGuildLog_GuildBreakUp,
	eGuildLog_GuildLevelUp,
	eGuildLog_GuildLevelDown,
	
	// guildunion
	eGuildLog_CreateUnion = 400,
	eGuildLog_DestroyUnion,
	eGuildLog_AddUnion,
	eGuildLog_RemoveUnion,	
	eGuildLog_SecedeUnion,
};

//---KES PUNISH  //주의: 아래의 두 enum의 번호값은 DB로 저장이 된다. 순서를 바꾸지 마시오.
enum ePunishKind			
{
	ePunish_Login = 0,
	ePunish_AutoNoteUse,		//---오토노트 사용제한
	// 밑의 것은 현재 안씀
	ePunish_Chat,
	ePunish_Trade,	
	ePunish_Max,
};

enum ePunishCountKind
{
	ePunishCount_AutoUser = 0,	//---오토사용
	// 밑의 것은 현재 안씀
	ePunishCount_NoManner,
	ePunishCount_TradeCheat,	//---교환 노점등 사기
	ePunishCount_Max,
};

#endif //__SERVERGAMEDEFINE_H__
