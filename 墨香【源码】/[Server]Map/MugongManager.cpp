// MugongManager.cpp: implementation of the CMugongManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MugongManager.h"
#include "MapDBMsgParser.h"
#include "Player.h"
#include "UserTable.h"
#include "SkillManager_server.h"
#include "ItemManager.h"
#include "..\[CC]Header\GameResourceManager.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMugongManager::CMugongManager()
{

}

CMugongManager::~CMugongManager()
{

}
void CMugongManager::NetworkMsgParse( BYTE Protocol, void* pMsg )
{
	switch(Protocol)
	{
	case MP_MUGONG_MOVE_SYN:
		{
			MSG_MUGONG_MOVE_SYN * msg = (MSG_MUGONG_MOVE_SYN *)pMsg;
			CPlayer * pPlayer = (CPlayer *)g_pUserTable->FindUser(msg->dwObjectID);			
			if(!pPlayer) return;
			if(MoveMugong(pPlayer, msg->FromPos, msg->ToPos))
			{
				//횇짭쨋처?횑쩐챨횈짰쩔징째횚쨉쨉 쨘쨍쨀쨩횁횥
				MSG_MUGONG_MOVE_ACK msg2;
				memcpy(&msg2, msg, sizeof(MSG_MUGONG_MOVE_SYN));
				msg2.Protocol = MP_MUGONG_MOVE_ACK;
				pPlayer->SendMsg(&msg2, sizeof(MSG_MUGONG_MOVE_ACK));
			}
			else
			{
				MSGBASE Msg;
				Msg.Category = MP_MUGONG;
				Msg.Protocol = MP_MUGONG_MOVE_NACK;
				pPlayer->SendMsg(&Msg, sizeof(Msg));
			}
		}
		break;
/*
			case MP_MUGONG_ADD_SYN:
				{
					MSG_MUGONG_ADD * msg = (MSG_MUGONG_ADD *)pMsg;
					CPlayer* pPlayer = (CPlayer *)g_pUserTable->FindUser(msg->dwObjectID);
					if(pPlayer == NULL)
						return;
					MUGONGMGR_OBJ->AddMugong(pPlayer, msg);
				}
				break;*/
	case MP_MUGONG_REM_SYN:
		{
			MSG_MUGONG_REM_SYN * msg = (MSG_MUGONG_REM_SYN *)pMsg;
			CPlayer* pPlayer = (CPlayer *)g_pUserTable->FindUser(msg->dwObjectID);
			if(pPlayer == NULL)
				return;
			if(RemMugong(pPlayer, msg->wMugongIdx, msg->TargetPos,eLog_MugongDiscard))
			{
				MSG_MUGONG_REM_ACK ToMsg;
				ToMsg.Category = MP_MUGONG;
				ToMsg.Protocol = MP_MUGONG_REM_ACK;
				ToMsg.dwObjectID = msg->dwObjectID;
				ToMsg.wMugongIdx = msg->wMugongIdx;
				ToMsg.TargetPos = msg->TargetPos;

				pPlayer->SendMsg(&ToMsg, sizeof(ToMsg));
			}
			else
			{
				MSGBASE Msg;
				Msg.Category = MP_MUGONG;
				Msg.Protocol = MP_MUGONG_REM_NACK;
				pPlayer->SendMsg(&Msg, sizeof(Msg));
			}
		}
		break;
	case MP_MUGONG_OPTION_SYN:
		{
			MSG_WORD4* pmsg = (MSG_WORD4*)pMsg;

			CPlayer* pPlayer = (CPlayer *)g_pUserTable->FindUser(pmsg->dwObjectID);
			if(pPlayer == NULL)
				return;

			MSG_WORD4 msg;
			msg.Category = MP_MUGONG;
			msg.wData1 = pmsg->wData1;
			msg.wData2 = pmsg->wData2;
			msg.wData3 = pmsg->wData3;
			msg.wData4 = pmsg->wData4;

			if( SetOption(pPlayer, pmsg->wData1, pmsg->wData2, pmsg->wData3, pmsg->wData4) )
				msg.Protocol = MP_MUGONG_OPTION_ACK;
			else
				msg.Protocol = MP_MUGONG_OPTION_NACK;
			
			pPlayer->SendMsg(&msg, sizeof(msg));
		}
		break;
	case MP_MUGONG_OPTION_CLEAR_SYN:
		{
			MSG_WORD4* pmsg = (MSG_WORD4*)pMsg;

			CPlayer* pPlayer = (CPlayer *)g_pUserTable->FindUser(pmsg->dwObjectID);
			if(pPlayer == NULL)
				return;

			MSG_WORD4 msg;
			msg.Category = MP_MUGONG;
			msg.wData1 = pmsg->wData1;
			msg.wData2 = pmsg->wData2;
			msg.wData3 = pmsg->wData3;
			msg.wData4 = pmsg->wData4;

			if( ClearOption(pPlayer, pmsg->wData1, pmsg->wData2, pmsg->wData3, pmsg->wData4) )
				msg.Protocol = MP_MUGONG_OPTION_CLEAR_ACK;
			else
				msg.Protocol = MP_MUGONG_OPTION_CLEAR_NACK;

			pPlayer->SendMsg(&msg, sizeof(msg));
		}
		break;
	default:
		break;
	} 
}
BOOL CMugongManager::RemMugong(CPlayer * pPlayer, WORD wMugongIdx, POSTYPE TargetPos, BYTE bType)
{
	MUGONGBASE * pMugong = pPlayer->GetMugongBase(TargetPos);
	if(!pMugong || pMugong->wIconIdx != wMugongIdx)
		return FALSE;

	// magi82 - Titan(070611) 타이탄 무공변환 주석처리
	//MUGONGBASE* pTitanMugong = GetTitanMugongBase(pPlayer, pMugong->wIconIdx);
	//if( pTitanMugong )
	//{
	//	MugongDeleteToDB(pTitanMugong->dwDBIdx);
	//	InsertLogMugong( bType, pPlayer->GetID(), pTitanMugong->wIconIdx, pTitanMugong->dwDBIdx, pTitanMugong->Sung, pTitanMugong->ExpPoint );
	//	memset(pTitanMugong, 0, sizeof(MUGONGBASE));		
	//}

	// db
	MugongDeleteToDB(pMugong->dwDBIdx);

	// Log 쨔짬째첩 쨩챔횁짝
	InsertLogMugong( bType, pPlayer->GetID(), pMugong->wIconIdx, pMugong->dwDBIdx, pMugong->Sung, pMugong->ExpPoint );

	// 쨔짬째첩횁짚쨘쨍 쨩챔횁짝
	memset(pMugong, 0, sizeof(MUGONGBASE));

	return TRUE;
}

BOOL CMugongManager::MoveMugong(CPlayer * pPlayer, POSTYPE FromPos, POSTYPE ToPos)
{	
	if(FromPos == ToPos)
		return FALSE;

#ifdef _JAPAN_LOCAL_

		if( FromPos < TP_JINBUB_START )
		{
			DWORD EndPos = TP_MUGONG_START + GIVEN_MUGONG_SLOT + ( MUGONG_SLOT_ADDCOUNT*pPlayer->GetExtraMugongSlot() );
			if( ToPos > EndPos )
				return FALSE;
		}

#elif defined _HK_LOCAL_
		if( FromPos < TP_JINBUB_START )
		{
			DWORD EndPos = TP_MUGONG_START + GIVEN_MUGONG_SLOT + ( MUGONG_SLOT_ADDCOUNT*pPlayer->GetExtraMugongSlot() );
			if( ToPos > EndPos )
				return FALSE;
		}

#elif defined _TL_LOCAL_
		if( FromPos < TP_JINBUB_START )
		{
			DWORD EndPos = TP_MUGONG_START + GIVEN_MUGONG_SLOT + ( MUGONG_SLOT_ADDCOUNT*pPlayer->GetExtraMugongSlot() );
			if( ToPos > EndPos )
				return FALSE;
		}

#else

	// magi82 - Titan(070910) 타이탄 무공업데이트
	if( FromPos >= TP_JINBUB_START && FromPos < TP_JINBUB_END )
	{
		if( ToPos >= TP_MUGONG1_START && ToPos < TP_MUGONG2_END )
			return FALSE;
		if( ToPos >= TP_TITANMUGONG_START && ToPos < TP_TITANMUGONG_END )
			return FALSE;
	}
	else if( FromPos >= TP_MUGONG1_START && FromPos < TP_MUGONG2_END )
	{
		if( ToPos >= TP_JINBUB_START && ToPos < TP_JINBUB_END )
			return FALSE;
		if( ToPos >= TP_TITANMUGONG_START && ToPos < TP_TITANMUGONG_END )
			return FALSE;
	}
	else if( FromPos >= TP_TITANMUGONG_START && FromPos < TP_TITANMUGONG_END )
	{
		if( ToPos >= TP_MUGONG1_START && ToPos < TP_MUGONG2_END )
			return FALSE;
		if( ToPos >= TP_JINBUB_START && ToPos < TP_JINBUB_END )
			return FALSE;
	}

#endif	// _JAPAN_LOCAL_	
	
	MUGONGBASE FromMugong = *pPlayer->GetMugongBase(FromPos);
	MUGONGBASE ToMugong = *pPlayer->GetMugongBase(ToPos);

	if( ToMugong.Position == 0 ) // 쩔횇짹챈 ?횣쨍짰째징 쨘챰쩐챤 ?횜쨈횂째챈쩔챙
	{
		FromMugong.Position = ToPos;
		pPlayer->SetMugongBase(FromPos, &ToMugong); // 짹창횁쨍 ?횣쨍짰 쨘챰쩔챙짹창
		pPlayer->SetMugongBase(FromMugong.Position, &FromMugong); // 쨩천쨌횓쩔챤 ?횣쨍짰쩔징 쨀횜짹창

		MugongUpdateToDB(&FromMugong,"MOVE_NULL");


		// magi82 - Titan(070611) 타이탄 무공변환 주석처리
		/*
		// 스킬정보
		MUGONGBASE* pTitanMugong = GetTitanMugongBase(pPlayer, FromMugong.wIconIdx);
		if( pTitanMugong )
		{
			pTitanMugong->Position = ToPos;
			pPlayer->SetMugongBase(pTitanMugong->Position, pTitanMugong);

			MugongUpdateToDB(pTitanMugong,"MOVE_NULL");
		}
		*/
	}
	else
	{
		if( FromMugong.dwDBIdx == 0 )
			return FALSE;
		
		SWAPVALUE(FromMugong.Position, ToMugong.Position)
		ASSERT(FromMugong.Position);
		pPlayer->SetMugongBase(FromMugong.Position, &FromMugong);
		pPlayer->SetMugongBase(ToMugong.Position, &ToMugong);

		MugongMoveUpdateToDB(FromMugong.dwDBIdx, FromMugong.Position, ToMugong.dwDBIdx, ToMugong.Position, "MOVE_FROM");



		// magi82 - Titan(070611) 타이탄 무공변환 주석처리
		/*
		// From TitanMugong
		MUGONGBASE* pTitanFromMugong = GetTitanMugongBase(pPlayer, FromMugong.wIconIdx);
		MUGONGBASE* pTitanToMugong = GetTitanMugongBase(pPlayer, ToMugong.wIconIdx);

		if( pTitanFromMugong && pTitanToMugong )	// From과 To 모두 타이탄 무공으로 변환을 한 경우
		{
			MUGONGBASE Temp;
			CopyMemory(&Temp, pTitanToMugong, sizeof(MUGONGBASE));

			pTitanFromMugong->Position = FromMugong.Position;
			pPlayer->SetMugongBase(pTitanFromMugong->Position, pTitanFromMugong);
			MugongUpdateToDB(pTitanFromMugong,"MOVE_NULL");


			Temp.Position = ToMugong.Position;
			pPlayer->SetMugongBase(Temp.Position, &Temp);
			MugongUpdateToDB(&Temp,"MOVE_NULL");
		}
		else	// 둘중하나만 변환을 했거나 둘다 하지 않았을경우
		{
			if( pTitanFromMugong )
			{
				pTitanFromMugong->Position = FromMugong.Position;
				pPlayer->SetMugongBase(pTitanFromMugong->Position, pTitanFromMugong);

				MugongUpdateToDB(pTitanFromMugong,"MOVE_NULL");
			}

			// To TitanMugong
			if( pTitanToMugong )
			{
				pTitanToMugong->Position = ToMugong.Position;
				pPlayer->SetMugongBase(pTitanToMugong->Position, pTitanToMugong);

				MugongUpdateToDB(pTitanToMugong,"MOVE_NULL");
			}
		}
		*/
	}

	//CharacterMugongUpdate(FromMugong);
	//CharacterMugongUpdate(ToMugong);

	return TRUE;
}

// magi82 - Titan(070611) 타이탄 무공변환 주석처리
/*
MUGONGBASE* CMugongManager::GetTitanMugongBase( CPlayer * pPlayer, WORD idx )
{
	CSkillInfo* pInfo = SKILLMGR->GetSkillInfo(idx);
	if( pInfo == NULL )
		return NULL;

	WORD mugongIdx = pInfo->GetSkillInfo()->LinkSkillIdx;
	// 이미 배웠으면 처리안함
	MUGONGBASE* pTitanMugong = pPlayer->GetMugongBaseByMugongIdx(mugongIdx);
	if( pTitanMugong )
        return pTitanMugong;

	return NULL;
}
*/

void CMugongManager::AddMugongDBResult(CPlayer * pPlayer, MUGONGBASE * pMugongBase)
{
	MSG_MUGONG_ADD_ACK msg;
	msg.Category = MP_MUGONG;
	msg.Protocol = MP_MUGONG_ADD_ACK;
	msg.dwObjectID = pPlayer->GetID();
	msg.MugongBase = *pMugongBase;

	pPlayer->SetMugongBase(pMugongBase->Position, pMugongBase);
	pPlayer->SendMsg(&msg, sizeof(MSG_MUGONG_ADD_ACK));

	if(pMugongBase->QuickPosition)
	{
		MSG_QUICK_ADD_ACK msg;
		msg.Category = MP_QUICK;
		msg.Protocol = MP_QUICK_ADD_ACK;
		msg.OldSrcItemIdx = 0;
		msg.OldSrcPos = 0;
		msg.SrcPos = pMugongBase->Position;
		msg.SrcItemIdx = pMugongBase->wIconIdx;
		msg.QuickPos = pMugongBase->QuickPosition;
		pPlayer->SendMsg(&msg, sizeof(msg));
	}
	// Log 쨔짬째첩 횊쨔쨉챈..
	InsertLogMugong( eLog_MugongLearn, pPlayer->GetID(), pMugongBase->wIconIdx, pMugongBase->dwDBIdx, pMugongBase->Sung, pMugongBase->ExpPoint );
}
BOOL CMugongManager::AddMugong( CPlayer * pPlayer, WORD wMugongIdx, WORD ItemKind, POSTYPE QuickPos, BYTE bSung, WORD Option)
{
	// 횁횩쨘쨔 횄쩌횇짤
	if(pPlayer->GetMugongBaseByMugongIdx(wMugongIdx))
		return FALSE;
	
	// 횈짰쨍짰횄쩌횇짤
	CSkillInfo* pInfo = SKILLMGR->GetSkillInfo(wMugongIdx);
	if( pInfo == NULL )
		return FALSE;


	// 절초 검색 hs
	SKILL_CHANGE_INFO * pChangeInfo = SKILLMGR->GetSkillChangeInfo(wMugongIdx);
	if(pChangeInfo)
	{
		if(pPlayer->GetMugongBaseByMugongIdx(pChangeInfo->wTargetMugongIdx))
			return FALSE;
	}

	// 쨘처째첩째짙
	POSTYPE StartPos,EndPos;
	if(ItemKind == eMUGONG_ITEM_JINBUB)
	{
		StartPos = TP_JINBUB_START;
		EndPos = TP_JINBUB_END;
	}
	// magi82 - Titan(070910) 타이탄 무공업데이트
	else if(ItemKind == eMUGONG_ITEM_TITAN)
	{
		StartPos = TP_TITANMUGONG_START;
		EndPos = TP_TITANMUGONG_END;
	}
	else
	{		
#ifdef _JAPAN_LOCAL_
		StartPos = TP_MUGONG_START;
		EndPos = TP_MUGONG_START + GIVEN_MUGONG_SLOT + ( MUGONG_SLOT_ADDCOUNT*pPlayer->GetExtraMugongSlot() );
#elif defined _HK_LOCAL_
		StartPos = TP_MUGONG_START;
		EndPos = TP_MUGONG_START + GIVEN_MUGONG_SLOT + ( MUGONG_SLOT_ADDCOUNT*pPlayer->GetExtraMugongSlot() );
#elif defined _TL_LOCAL_
		StartPos = TP_MUGONG_START;
		EndPos = TP_MUGONG_START + GIVEN_MUGONG_SLOT + ( MUGONG_SLOT_ADDCOUNT*pPlayer->GetExtraMugongSlot() );
#else
		StartPos = TP_MUGONG1_START;
		EndPos = TP_MUGONG1_END;
		if( pPlayer->GetShopItemManager()->GetUsingItemInfo( eIncantation_MugongExtend ) )
			EndPos = TP_MUGONG2_END;
#endif	// _JAPAN_LOCAL_
	}

	POSTYPE ToPos = EndPos;
	MUGONGBASE* pMugong;
	for( POSTYPE n = StartPos ; n < EndPos ; ++n )
	{
		pMugong = pPlayer->GetMugongBase(n);
		if(pMugong->dwDBIdx == 0)
		{
			ToPos = n;
			break;
		}
	}
	if(ToPos == EndPos)
		return FALSE;

	BYTE weared = 0;
	switch(ConvAbsPos2MugongPos(ToPos))
	{
#ifdef _JAPAN_LOCAL_
	case TP_MUGONG_START:
#elif defined _HK_LOCAL_
	case TP_MUGONG_START:
#elif defined _TL_LOCAL_
	case TP_MUGONG_START:
#else
	case TP_MUGONG1_START:
#endif
	case TP_JINBUB_START:
		weared = 1;
		break;
	}

	MUGONGBASE MugongBase;
	MugongBase.Position			= ToPos;
	MugongBase.dwDBIdx			= 9;
	MugongBase.ExpPoint			= 9;
	MugongBase.QuickPosition	= 9;
	MugongBase.Sung				= 9;
    MugongBase.wIconIdx			= wMugongIdx;
	MugongBase.OptionIndex		= Option;

	pPlayer->SetMugongBase(MugongBase.Position, &MugongBase);

	ASSERTMSG(ToPos, "0쨔첩 횈첨횁철쩌횉쩔징 쨔짬째첩?쨩 쨀횜?쨍쨌횁 횉횛쨈횕쨈횢.");
	//db
	
	if( pInfo->GetNeedExp( 0 ) == (DWORD)(-1) )
	{
		bSung = 12;
	}

	// magi82 - Titan(070914) 타이탄 무공업데이트
	// 서적이 타이탄 무공이면 무조건 1성부터
	if( pInfo->GetSkillKind() == SKILLKIND_TITAN )
	{
		bSung = 1;
	}
	
	//2007. 7. 2. CBH - 전문기술 스킬 관련 추가
	//서적이 전문기술이면 bSung 1성으로 셋팅 
	WORD wSkillKind = pInfo->GetSkillKind();
	if( SKILLMGR->CheckSkillKind(wSkillKind) == TRUE )
	{
		WORD wSkillIndex = pInfo->GetSkillIndex();
		if( (wSkillIndex == 6001) || (wSkillIndex == 6003) || (wSkillIndex == 6005) )
		{
			bSung = 13;
		}
		else
		{
			bSung = 1;
		}
	}


	MugongInsertToDB( pPlayer->GetID(), wMugongIdx, ToPos, weared, bSung, Option );

	return TRUE;
}

// magi82 - Titan(070611) 타이탄 무공변환 주석처리
/*
BOOL CMugongManager::AddTitanMugong( CPlayer * pPlayer, WORD wMugongIdx, POSTYPE QuickPos, BYTE bSung, WORD Option)
{
	// 스킬정보
	CSkillInfo* pInfo = SKILLMGR->GetSkillInfo(wMugongIdx);
	if( pInfo == NULL )
		return FALSE;

	// 플레이어가 배운 스킬정보
	MUGONGBASE* pMugongInfo = pPlayer->GetMugongBaseByMugongIdx(wMugongIdx);
	if(!pMugongInfo)
		return FALSE;

	WORD mugongIdx = pInfo->GetSkillInfo()->LinkSkillIdx;
	// 이미 배웠으면 처리안함
	if(pPlayer->GetMugongBaseByMugongIdx(mugongIdx))
		return FALSE;

	BYTE weared = 0;

	MUGONGBASE MugongBase;
	MugongBase.Position			= pMugongInfo->Position;
	MugongBase.dwDBIdx			= 9; 
	MugongBase.ExpPoint			= 9;
	MugongBase.QuickPosition	= 9;
	MugongBase.Sung				= 9;
	MugongBase.wIconIdx			= mugongIdx;
	MugongBase.OptionIndex		= Option;

	pPlayer->SetMugongBase(MugongBase.Position, &MugongBase);

	ASSERTMSG(pMugongInfo->Position, "MugongManager.cpp -> Func : AddTitanMugong()");
	//db

	if( pInfo->GetNeedExp( 0 ) == (EXPTYPE)(-1) )
	{
		bSung = 12;
	}


	MugongInsertToDB( pPlayer->GetID(), mugongIdx, pMugongInfo->Position, weared, bSung, Option );

	return TRUE;
}
*/

void CMugongManager::ChangeMugong(CPlayer* pPlayer, WORD wRemMugongIdx, POSTYPE TargetPos, WORD wAddMugongIdx, WORD LogType)
{
	MUGONGBASE* pMugong = pPlayer->GetMugongBase(TargetPos);
	POSTYPE QPos = pMugong->QuickPosition;
	WORD Option = pMugong->OptionIndex;
	if(RemMugong(pPlayer, wRemMugongIdx, TargetPos, (BYTE)LogType))
	{
		MSG_MUGONG_REM_ACK ToMsg;
		ToMsg.Category = MP_MUGONG;
		ToMsg.Protocol = MP_MUGONG_REM_ACK;
		ToMsg.dwObjectID = pPlayer->GetID();
		ToMsg.wMugongIdx = wRemMugongIdx;
		ToMsg.TargetPos = TargetPos;
		
		pPlayer->SendMsg(&ToMsg, sizeof(ToMsg));
	}
	AddMugong(pPlayer, wAddMugongIdx, eMUGONG_ITEM, 0, 12, Option);
}

#ifdef _JAPAN_LOCAL_
BOOL CMugongManager::DeleteSkill( CPlayer* pPlayer, WORD wSkillIdx )
{
	MUGONGBASE* pMugong = pPlayer->GetMugongBaseByMugongIdx( wSkillIdx );
	if( !pMugong || pMugong->wIconIdx != wSkillIdx )	return FALSE;

	// db & Log 
	MugongDeleteToDB( pMugong->dwDBIdx );	
	InsertLogMugong( eLog_MugongDestroyByGetNextLevel, pPlayer->GetID(), pMugong->wIconIdx, pMugong->dwDBIdx,
					 pMugong->Sung, pMugong->ExpPoint );

	MSG_MUGONG_REM_ACK Msg;
	Msg.Category = MP_MUGONG;
	Msg.Protocol = MP_MUGONG_REM_ACK;
	Msg.dwObjectID = pPlayer->GetID();
	Msg.wMugongIdx = wSkillIdx;
	Msg.TargetPos = pMugong->Position;
	pPlayer->SendMsg( &Msg, sizeof(Msg) );

	memset( pMugong, 0, sizeof(MUGONGBASE) );	

	return TRUE;
}
#endif

BOOL CMugongManager::SetOption(CPlayer * pPlayer, WORD wMugongIdx, POSTYPE TargetPos, WORD ItemIdx, WORD ItemPos)
{
	MUGONGBASE * pMugong = pPlayer->GetMugongBase(TargetPos);
	if(!pMugong || pMugong->wIconIdx != wMugongIdx)
		return FALSE;

	CSkillInfo* pInfo = NULL;
	SKILLINFO* pSInfo = NULL;
	SKILLOPTION* pOption = NULL;
	SKILLOPTION* pOldOption = NULL;

	// 스킬 정보를 가져온다
	pInfo = SKILLMGR->GetSkillInfo(wMugongIdx);
	if(!pInfo)	return FALSE;
 	pSInfo = pInfo->GetSkillInfo();
	if(!pSInfo)	return FALSE;

	// 변환 불가 스킬이면 실패
	if(pSInfo->ChangeKind == eSkillChange_None) return FALSE;

	// 아이템 정보를 가져온다
	CItemSlot* pInven = (CItemSlot*)pPlayer->GetSlot(ItemPos);
	if(!pInven) return FALSE;

	const ITEMBASE* pItem = pInven->GetItemInfoAbs(ItemPos);
	if(!pItem) return FALSE;
	if(pItem->wIconIdx != ItemIdx) return FALSE;

	pOption = SKILLMGR->GetSkillOptionByItemIndex(ItemIdx);
	if(!pOption) return FALSE;

	ITEM_INFO* pItemInfo = ITEMMGR->GetItemInfo(ItemIdx);
	if(!pItemInfo) return FALSE;

	// 레벨 제한
	if(pPlayer->GetLevel() < pItemInfo->LimitLevel)
		return FALSE;

	// 경지 제한
	switch(pOption->OptionGrade)
	{
	case 1:
		// 1단계는 일반
		break;
	case 2:
		// 2단계는 화경 극마 이상
		if(!(pPlayer->GetStage() & eStage_Hwa || pPlayer->GetStage() & eStage_Geuk))
			return FALSE;
		break;
	case 3:
		// 3단계는 현경 탈마 이상
		if(!(pPlayer->GetStage() == eStage_Hyun || pPlayer->GetStage() == eStage_Tal))
			return FALSE;
		break;
	}

	// 모든 변환 가능 스킬이 아니면 변환 가능 종류와 아이템의 종류가 다르면 실패한다.
	if(pSInfo->ChangeKind != eSkillChange_All && pSInfo->ChangeKind != pOption->SkillKind)
		return FALSE;

	// 이미 변환된 무공인지 확인한다
	pOldOption = SKILLMGR->GetSkillOption(pMugong->OptionIndex);
	if(pOldOption)
	{
		// 이미 적용된 변환의 종류와 다르면 실패한다.
		if(pOldOption->OptionKind != pOption->OptionKind)
			return FALSE;

		// 이미 적용된 변환보다 낮거나 같은 등급이면 실패한다.
		if(pOldOption->OptionGrade >= pOption->OptionGrade)
			return FALSE;
	}

	if( EI_TRUE != ITEMMGR->DiscardItem(pPlayer, ItemPos, ItemIdx, 1) )
		return FALSE; 

	pMugong->OptionIndex = pOption->Index;

	InsertLogMugong( eLog_MugongOption, pPlayer->GetID(), pMugong->wIconIdx, pMugong->dwDBIdx, pMugong->OptionIndex, pMugong->ExpPoint );
	MugongUpdateToDB(pMugong, "OPTION");

	return TRUE;
}

BOOL CMugongManager::ClearOption(CPlayer * pPlayer, WORD wMugongIdx, POSTYPE TargetPos, WORD ItemIdx, WORD ItemPos)
{
	MUGONGBASE * pMugong = pPlayer->GetMugongBase(TargetPos);
	if(!pMugong || pMugong->wIconIdx != wMugongIdx)
		return FALSE;

	// 아이템 정보를 가져온다
	CItemSlot* pInven = (CItemSlot*)pPlayer->GetSlot(ItemPos);
	if(!pInven) return FALSE;

	const ITEMBASE* pItem = pInven->GetItemInfoAbs(ItemPos);
	if(!pItem) return FALSE;
	if(pItem->wIconIdx != ItemIdx) return FALSE;

	// 이미 변환된 무공인지 확인한다
	if(pMugong->OptionIndex == eSkillOption_None)
		return FALSE;

	ITEMBASE itembase = *pItem;
	
	if( EI_TRUE != ITEMMGR->DiscardItem(pPlayer, ItemPos, ItemIdx, 1) )
		return FALSE;

	if(ItemIdx == 10750)
	{		
		pPlayer->MugongLevelDown(pMugong->wIconIdx, eLog_MugongDestroyByOptionClear);

		InsertLogMugong( eLog_MugongOptionClear, pPlayer->GetID(), pMugong->wIconIdx, pMugong->dwDBIdx, pMugong->OptionIndex, pMugong->ExpPoint );
	}
	// 06.09.15 RaMa - 무공변환초기화아이템 추가(아이템몰)
	else if( ItemIdx == eIncantation_MugongOptionReset )
	{
		InsertLogMugong( eLog_MugongOptionClearbyShopItem, pPlayer->GetID(), pMugong->wIconIdx, pMugong->dwDBIdx, pMugong->OptionIndex, pMugong->ExpPoint );

		//
		SEND_SHOPITEM_BASEINFO usemsg;
		SetProtocol( &usemsg, MP_ITEM, MP_ITEM_SHOPITEM_USE_ACK );
		usemsg.ShopItemPos = ItemPos;
		usemsg.ShopItemIdx = ItemIdx;
		pPlayer->SendMsg( &usemsg, sizeof(usemsg) );

		LogItemMoney( pPlayer->GetID(), pPlayer->GetObjectName(), 0, "",
			eLog_ShopItemUse, pPlayer->GetMoney(eItemTable_Inventory), 0, 0,
			ItemIdx, itembase.dwDBIdx, ItemPos, 0, itembase.Durability, pPlayer->GetPlayerExpPoint());
	}

	pMugong->OptionIndex = 0;	
	MugongUpdateToDB(pMugong, "OPTION");
	return TRUE;
}
