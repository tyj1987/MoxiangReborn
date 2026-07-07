#ifndef __STREETSTALL__
#define __STREETSTALL__


#define MAX_STREETSTALL_CELLNUM 25
#define MAX_STREETBUYSTALL_CELLNUM 5

enum STALL_KIND
{
	eSK_NULL,
	eSK_SELL,
	eSK_BUY,
};

//#define DEFAULT_USABLE_INVENTORY	3

struct sCELLINFO 
{
	void Init()
	{
		wVolume = 0;
		dwMoney = 0;
		bLock = FALSE;
		bFill = FALSE;
		memset(&sItemBase, 0, sizeof(ITEMBASE));
	}

	ITEMBASE	sItemBase;
	DWORD		dwMoney;
	WORD		wVolume;
	BOOL		bLock;
	BOOL		bFill;
};

class CPlayer;

class cStreetStall 
{
protected:
	sCELLINFO	m_sArticles[MAX_STREETSTALL_CELLNUM];		// 潞赂脌炉赂帽路脧
	CPlayer*	m_pOwner;									// 禄贸脕隆 驴卯驴碌脌脷
	cPtrList	m_GuestList;								// 禄贸脕隆脌脟 录脮麓脭
	int			m_nCurRegistItemNum;
	WORD		m_wStallKind;
//	WORD		m_nUsable;
	DWORD		m_nTotalMoney;

public:
	cStreetStall();
	virtual ~cStreetStall();
	void Init();

	DWORD	GetTotalMoney() { return m_nTotalMoney; };
//	WORD	GetUsable() { return m_nUsable; };
//	void	SetMaxUsable() { m_nUsable = MAX_STREETSTALL_CELLNUM; };
//	void	SetExtraUsable(WORD num) { m_nUsable += num; };
//	void	SetDefaultUsable() { m_nUsable = DEFAULT_USABLE_INVENTORY; };

	// 禄贸脕隆驴卯驴碌 脕娄戮卯 脟脭录枚
	BOOL FillCell(ITEMBASE* pBase, DWORD money, BOOL bLock = FALSE, DWORD Volume = 0, WORD wAbsPosition = 0);
	void EmptyCell( ITEMBASE* pBase, eITEMTABLE tableIdx );
	void EmptyCell( POSTYPE pos );
	void EmptyCellAll();
	void ChangeCellState( WORD pos, BOOL bLock );

	// 掳茫脛隆卤芒 戮脝脌脤脜脹驴隆 麓毛脟脩 脙鲁赂庐
	void UpdateCell( WORD pos, DURTYPE dur );

	void SetMoney( WORD pos, DWORD money );
	void SetVolume( WORD pos, WORD Volume );

	// LYJ 备涝畴痢 眠啊
	WORD GetStallKind() { return m_wStallKind; }
	void SetStallKind(WORD wStallKind) { m_wStallKind = wStallKind; }

	// 禄贸脕隆 驴卯驴碌脌脷驴隆 麓毛脟脩 脟脭录枚 
	CPlayer* GetOwner() { return m_pOwner; }
	void SetOwner( CPlayer* pOwner ) { m_pOwner = pOwner; }
	
	// 禄贸脕隆驴隆 麓毛脟脩 脌眉脙录 脕陇潞赂 脟脭录枚
	void GetStreetStallInfo( STREETSTALL_INFO& stall );
	sCELLINFO* GetCellInfo( POSTYPE pos ) { return &m_sArticles[pos];}

	// 禄贸脕隆 录脮麓脭驴隆 麓毛脟脩 脕娄戮卯 脟脭录枚
	void AddGuest( CPlayer* pGuest );
	void DeleteGuest( CPlayer* pGuest );
	void DeleteGuestAll();
	void SendMsgGuestAll( MSGBASE* pMsg, int nMsgLen, BOOL bChangeState = FALSE );

	int GetCurRegistItemNum() { return m_nCurRegistItemNum; }
	
	BOOL IsFull();

	BOOL CheckItemDBIdx(DWORD idx);
	BOOL CheckItemIdx(DWORD idx);// 脕脽潞鹿 掳脣禄莽 (脟脢脠梅 DB驴隆 脌脰麓脗 脌脦碌娄陆潞驴隆 麓毛脟脩 潞帽卤鲁赂娄 脟脩麓脵.)
};

#endif //__STREETSTALL__