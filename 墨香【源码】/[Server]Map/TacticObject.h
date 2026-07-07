// TacticObject.h: interface for the CTacticObject class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TACTICOBJECT_H__AC699872_BF6E_424A_84CB_D71C5CC7017C__INCLUDED_)
#define AFX_TACTICOBJECT_H__AC699872_BF6E_424A_84CB_D71C5CC7017C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Object.h"

class CTacticStartInfo;

enum TacticAttackType
{
	eTacTicAttack_Physical,
	eTacTicAttack_LifeRecover,
	eTacTicAttack_Buff,

	eTacTicAttack_Max,
};

class CTacticObject  : public CObject
{
private:
	CTacticStartInfo* m_pTacticInfo;
	TACTIC_TOTALINFO m_TotalInfo;
	DWORD m_PartyID;									// 陆脙脌眉脕脽脌脦 脝脛脝录脌脟 ID
	DWORD m_LastNaeryukReduceTime;
	int m_TacticLevel;

	DWORD m_JoinLvl; // 曼咯 某腐磐狼 饭骇
	BYTE m_OperatorAbilityLevel;
	DWORD m_bExecute;	
	
public:
	CTacticObject();
	virtual ~CTacticObject();

	void InitTacticObject(CTacticStartInfo* pInfo,CPlayer* pOperator,DIRINDEX Direction,BYTE TacticAbilityLevel);
	BOOL TacticJoin(CPlayer* pObject,int Position);
	
	BOOL CheckFail();
	BOOL CheckExecute();

	void FailTactic(int tfr);
	BOOL ExecuteTactic();
	
	void GetTacticTotalInfo(TACTIC_TOTALINFO* pRtInfo);
	DWORD GetTacticStartTime();

	/////////////////////////////////////////////////////////////////////
	// for calc 
	void AddJoinLevel(LEVELTYPE JoinLevel);
	DWORD GetJoinLevel();
	BYTE GetOperatorAbilityLevel();
	/////////////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////////////
	// Overrided Func
	void DoSetInited();
	void SetAddMsg(char* pAddMsg, WORD* pMsgLen, DWORD dwReceiverID,BOOL bLogin);
	void OnStartObjectState(BYTE State, DWORD dwParam);
	void OnEndObjectState(BYTE State);
	
};

#endif // !defined(AFX_TACTICOBJECT_H__AC699872_BF6E_424A_84CB_D71C5CC7017C__INCLUDED_)
