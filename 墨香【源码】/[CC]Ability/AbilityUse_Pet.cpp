#include "stdafx.h"
#include ".\abilityuse_pet.h"
#include "AbilityManager.h"

#ifdef _MHCLIENT_
#include "PetManager.h"
#endif

CAbilityUse_Pet::CAbilityUse_Pet(void)
{
}

CAbilityUse_Pet::~CAbilityUse_Pet(void)
{
}

void CAbilityUse_Pet::Use(BYTE Level,CAbilityInfo* pAbilityInfo)
{
#ifdef _MHCLIENT_
	ySWITCH(pAbilityInfo->GetInfo()->Ability_effect_Param1)
		yCASE(eAUKPET_State)
			//!!!팻상태창 열기
			PETMGR->OpenPetStateDlg();
		yCASE(eAUKPET_Inven)
			//!!!팻인벤창 열기
			PETMGR->OpenPetInvenDlg();
		yCASE(eAUKPET_Skill)
			//!!!팻스킬사용
			PETMGR->CheckRestNSkillUse();
		yCASE(eAUKPET_Rest)
			//!!!팻휴식상태설정
			PETMGR->SendPetRestMsg(TRUE);
		yCASE(eAUKPET_Seal)
			//!!!팻봉인
			PETMGR->SendPetSealMsg();
			yDEFAULT
		yENDSWITCH
#endif
}
