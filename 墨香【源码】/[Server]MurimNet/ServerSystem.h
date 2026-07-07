// ServerSystem.h: interface for the CServerSystem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVERSYSTEM_H__6D60EE32_C8B5_44A7_883C_8EF113DD06B4__INCLUDED_)
#define AFX_SERVERSYSTEM_H__6D60EE32_C8B5_44A7_883C_8EF113DD06B4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CServerSystem  
{
protected:

	void SetNetworkParser();//---��Ʈ��ũ �ļ� ����

public:
	CServerSystem();
	virtual ~CServerSystem();

	void Start(WORD ServerNum);
	void End();
	void Process();

// Phase 7.6: GetMapNum() stub added so [CC]ServerModule/DataBase.cpp and
// MiniDumper.cpp can compile. MurimNet is a PvP arena server — it doesn't
// manage maps, but the shared [CC]ServerModule code references g_pServerSystem->
// GetMapNum() inside #ifdef _MAPSERVER_. We return 0 as a sentinel for MurimNet.
// Tracked in docs/KNOWN_BUGS.md Bug D-8.
WORD GetMapNum() { return 0; }

//...?
	void ReleaseAuthKey(DWORD key);
};

void GameProcess();

void ProcessDBMessage();
void ReceivedMsgFromServer(DWORD dwConnectionIndex,char* pMsg,DWORD dwLength);
void ReceivedMsgFromUser(DWORD dwConnectionIndex,char* pMsg,DWORD dwLength);

void ButtonProc1();
void ButtonProc2();
void ButtonProc3();
void OnCommand( char* szCommand );

void OnAcceptServer(DWORD dwConnectionIndex);
void OnDisconnectServer(DWORD dwConnectionIndex);
void OnAcceptUser(DWORD dwConnectionIndex);
void OnDisconnectUser(DWORD dwConnectionIndex);

void OnConnectServerSuccess(DWORD dwConnectionIndex, void* pVoid);
void OnConnectServerFail(void* pVoid);

extern CServerSystem * g_pServerSystem;

#endif // !defined(AFX_SERVERSYSTEM_H__6D60EE32_C8B5_44A7_883C_8EF113DD06B4__INCLUDED_)
