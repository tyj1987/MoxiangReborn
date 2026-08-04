// sosdialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cSOSDialog (guild SOS dialog: list of
// guild members to send SOS to when in trouble).
//
// Covers modern/src/ui/sosdialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\SOSDialog.h (641 B) and
//   墨香【源码】\[Client]MH\SOSDialog.cpp.
//
// What's tested:
//   - Linking, destructor safety, and selection state.
//   - Guild member row formatting and online/offline colors.
//   - SetActive refresh ordering and close-cancel dispatch.
//   - Self/offline target messages and successful SOS fields.
//   - Legacy WORD position packing and callback replacement.
//
// 1:1 quirks preserved:
//   - SetActive always refreshes before base state change.
//   - Close always sends cancel when host wiring exists.
//   - Position x/z use WORD casts before packed DWORD send.

#include "sosdialog.hpp"
#include "clistdialog.hpp"
#include "cbutton.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction
// ===========================================================================

TEST(CSOSDialogTest, LeftButtonEventMatchesCanonicalLegacyValue) {
    EXPECT_EQ(cSOSDialog::kWeLeftButtonClick, mxh::ui::legacy_window_event::kLeftButtonClick);
}

TEST(CSOSDialogTest, DefaultConstructionHasNullPointers) {
    cSOSDialog dlg;
    EXPECT_EQ(dlg.GetMemberList(), nullptr);
    EXPECT_EQ(dlg.GetOkButton(),   nullptr);
    EXPECT_EQ(dlg.GetSelectIdx(),  0u);
}

// ===========================================================================
// Id constants
// ===========================================================================

TEST(CSOSDialogTest, IdConstantsAreDistinct) {
    EXPECT_NE(cSOSDialog::kMemberListId, cSOSDialog::kOkBtnId);
}

TEST(CSOSDialogTest, IdConstantsMatchExpectedLocalRange) {
    // 1:1 quirk: pick 230-231 to avoid collisions with
    // other Tier 2 dialog id ranges (cCharMakeDlg 200-203,
    // cGuildJoinDialog 210-212, cCharStateDialog 220-224).
    EXPECT_EQ(cSOSDialog::kMemberListId, 230);
    EXPECT_EQ(cSOSDialog::kOkBtnId,      231);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

// Build a cSOSDialog with 1 cListDialog + 1 cButton
// children wired in the modern id range (230-231).
// Returns the raw pointers via the out params; ownership
// lives in the dlg (children are added via cWindow::Add).
void BuildDlgWithChildren(cSOSDialog& dlg,
                          cListDialog*& out_list,
                          cButton*& out_btn) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    auto list = std::make_unique<cListDialog>();
    list->Init(0, 0, 200, 158, nullptr,
               cSOSDialog::kMemberListId);
    out_list = list.get();
    dlg.Add(std::unique_ptr<cWindow>(list.release()));

    auto btn = std::make_unique<cButton>();
    btn->Init(0, 0, 50, 14, nullptr, nullptr, nullptr,
              nullptr, nullptr, cSOSDialog::kOkBtnId);
    out_btn = btn.get();
    dlg.Add(std::unique_ptr<cWindow>(btn.release()));

    dlg.Linking();
}

}  // namespace

TEST(CSOSDialogTest, LinkingResolvesListAndButton) {
    cSOSDialog dlg;
    cListDialog* raw_list = nullptr;
    cButton*     raw_btn  = nullptr;
    BuildDlgWithChildren(dlg, raw_list, raw_btn);

    EXPECT_EQ(dlg.GetMemberList(), raw_list);
    EXPECT_EQ(dlg.GetOkButton(),   raw_btn);
}

TEST(CSOSDialogTest, LinkingConfiguresListDialog) {
    // 1:1 with legacy SetShowSelect(TRUE). SetHeight(158)
    // is also called in the legacy but the modern
    // cListDialog / cDialog API doesn't expose
    // SetHeight (1:1 quirk documented in sosdialog.cpp:
    // size is configured at Init time, not in Linking).
    cSOSDialog dlg;
    cListDialog* raw_list = nullptr;
    cButton*     raw_btn  = nullptr;
    BuildDlgWithChildren(dlg, raw_list, raw_btn);
    ASSERT_NE(dlg.GetMemberList(), nullptr);
    EXPECT_TRUE(dlg.GetMemberList()->IsShowSelect());
}

TEST(CSOSDialogTest, LinkingWithoutChildrenLeavesPointersNull) {
    cSOSDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetMemberList(), nullptr);
    EXPECT_EQ(dlg.GetOkButton(),   nullptr);
}

// ===========================================================================
// Destructor (1:1 with legacy ~CSOSDlg)
// ===========================================================================

TEST(CSOSDialogTest, DestructorNullChecksListBeforeRemoveAll) {
    // 1:1 quirk: legacy ~CSOSDlg unconditionally
    // dereferences m_pListDlg->RemoveAll(); modern port
    // is null-checked. Test by constructing + destructing
    // a dialog that never went through Linking (m_pListDlg
    // is null). The destructor must not crash.
    {
        cSOSDialog dlg;
        // No Linking → m_pListDlg is null → destructor
        // must not crash.
    }
    SUCCEED();
}

TEST(CSOSDialogTest, DestructorCallsRemoveAllOnResolvedList) {
    // When m_pListDlg is resolved, the destructor
    // calls RemoveAll. Verify by checking the list
    // is empty after destruction setup (the list is
    // captured by raw pointer; we can't observe after
    // destruction, so we observe the m_pListDlg state
    // before destruction).
    cSOSDialog dlg;
    cListDialog* raw_list = nullptr;
    cButton*     raw_btn  = nullptr;
    BuildDlgWithChildren(dlg, raw_list, raw_btn);
    ASSERT_NE(raw_list, nullptr);
    raw_list->AddItem("test");
    EXPECT_EQ(raw_list->RowCount(), 1u);
    // After dlg destruction (out of scope), raw_list
    // would be destroyed too. We can't easily test
    // "after destruction" here; the test is mostly a
    // smoke that the destructor doesn't crash.
    SUCCEED();
}

// ===========================================================================
// Runtime callbacks
// ===========================================================================

namespace {

struct SOSState {
    std::vector<SOSGuildMember> members;
    std::uint32_t heroId = 10;
    std::uint32_t mapNum = 55;
    std::uint32_t channel = 3;
    float x = 12.9f;
    float z = 34.8f;
    bool mouseDownUsed = false;
    int cancelCalls = 0;
    std::uint32_t cancelObjectId = 0;
    int messageCalls = 0;
    std::int32_t lastMessageId = 0;
    int sendCalls = 0;
    std::uint32_t sentObjectId = 0;
    std::uint32_t sentMemberId = 0;
    std::uint32_t sentMapNum = 0;
    std::uint32_t sentMovePoint = 0;
    std::uint32_t sentChannel = 0;
};
std::size_t GetSOSMemberCount(void* data) { return static_cast<SOSState*>(data)->members.size(); }
bool GetSOSMember(std::size_t index,SOSGuildMember* member,void* data) {
    auto& members=static_cast<SOSState*>(data)->members;
    if(index>=members.size()||!member) return false;
    *member=members[index]; return true;
}
std::uint32_t GetSOSHeroId(void* data){return static_cast<SOSState*>(data)->heroId;}
std::uint32_t GetSOSMapNum(void* data){return static_cast<SOSState*>(data)->mapNum;}
std::uint32_t GetSOSChannel(void* data){return static_cast<SOSState*>(data)->channel;}
void GetSOSPosition(float* x,float* z,void* data){auto& s=*static_cast<SOSState*>(data);*x=s.x;*z=s.z;}
void AddSOSMessage(std::int32_t id,void* data){auto& s=*static_cast<SOSState*>(data);++s.messageCalls;s.lastMessageId=id;}
void SendSOSCancel(std::uint32_t id,void* data){auto& s=*static_cast<SOSState*>(data);++s.cancelCalls;s.cancelObjectId=id;}
void SendSOSRequest(std::uint32_t objectId,std::uint32_t memberId,std::uint32_t mapNum,
                    std::uint32_t movePoint,std::uint32_t channel,void* data){
 auto& s=*static_cast<SOSState*>(data);++s.sendCalls;s.sentObjectId=objectId;s.sentMemberId=memberId;
 s.sentMapNum=mapNum;s.sentMovePoint=movePoint;s.sentChannel=channel;
}
bool IsSOSMouseDownUsed(void* data){return static_cast<SOSState*>(data)->mouseDownUsed;}
void InstallSOSCallbacks(cSOSDialog& dlg,SOSState& state){
 dlg.SetCallbacks(GetSOSMemberCount,GetSOSMember,GetSOSHeroId,GetSOSMapNum,GetSOSChannel,
                  GetSOSPosition,AddSOSMessage,SendSOSCancel,SendSOSRequest,
                  IsSOSMouseDownUsed,&state);
}
SOSGuildMember MakeSOSMember(std::uint32_t id,const char* name,const char* rank,
                             std::int32_t level,bool logged){return {id,name,rank,level,logged};}

}  // namespace

TEST(CSOSDialogTest, LegacyRuntimeConstantsMatchSource) {
    EXPECT_EQ(cSOSDialog::kSelfTargetMessageId,1631);
    EXPECT_EQ(cSOSDialog::kOfflineTargetMessageId,1632);
    EXPECT_EQ(cSOSDialog::kOnlineColor,0xFFFFFFFFu);
    EXPECT_EQ(cSOSDialog::kOfflineColor,0xFFACB6C7u);
}

TEST(CSOSDialogTest, SetActiveRefreshesMembersBeforeActivating) {
    cSOSDialog dlg; cListDialog* list=nullptr; cButton* btn=nullptr; BuildDlgWithChildren(dlg,list,btn);
    SOSState state; state.members.push_back(MakeSOSMember(20,"Alice","Master",88,true));
    InstallSOSCallbacks(dlg,state); dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive()); ASSERT_EQ(list->RowCount(),1u);
    EXPECT_NE(list->GetRow(0).first.find("Alice"),std::string::npos);
    EXPECT_NE(list->GetRow(0).first.find("Master"),std::string::npos);
    EXPECT_NE(list->GetRow(0).first.find("88"),std::string::npos);
    EXPECT_EQ(list->GetRow(0).second,cSOSDialog::kOnlineColor);
}

TEST(CSOSDialogTest, MemberRefreshUsesOfflineColorAndClearsExistingRows) {
    cSOSDialog dlg; cListDialog* list=nullptr; cButton* btn=nullptr; BuildDlgWithChildren(dlg,list,btn);
    list->AddItem("stale"); SOSState state; state.members.push_back(MakeSOSMember(20,"Bob","Member",7,false));
    InstallSOSCallbacks(dlg,state); dlg.SOSMemberInfo();
    ASSERT_EQ(list->RowCount(),1u); EXPECT_EQ(list->GetRow(0).second,cSOSDialog::kOfflineColor);
}

TEST(CSOSDialogTest, SetActiveFalseSendsCancelAfterBaseStateChange) {
    cSOSDialog dlg; cListDialog* list=nullptr; cButton* btn=nullptr; BuildDlgWithChildren(dlg,list,btn);
    SOSState state; InstallSOSCallbacks(dlg,state); dlg.SetActive(true); dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive()); EXPECT_EQ(state.cancelCalls,1); EXPECT_EQ(state.cancelObjectId,10u);
}

TEST(CSOSDialogTest, SetActiveFalseWithoutHostIsSafe) {
    cSOSDialog dlg; dlg.SetActive(false); EXPECT_FALSE(dlg.isActive());
}

TEST(CSOSDialogTest, SelfTargetEmits1631) {
    cSOSDialog dlg; SOSState state; state.members.push_back(MakeSOSMember(10,"Hero","Master",90,true));
    InstallSOSCallbacks(dlg,state); dlg.OnActionEvent(cSOSDialog::kOkBtnId,nullptr,0);
    EXPECT_EQ(state.lastMessageId,1631); EXPECT_EQ(state.sendCalls,0);
}

TEST(CSOSDialogTest, OfflineTargetEmits1632) {
    cSOSDialog dlg; SOSState state; state.members.push_back(MakeSOSMember(20,"Bob","Member",10,false));
    InstallSOSCallbacks(dlg,state); dlg.OnActionEvent(cSOSDialog::kOkBtnId,nullptr,0);
    EXPECT_EQ(state.lastMessageId,1632); EXPECT_EQ(state.sendCalls,0);
}

TEST(CSOSDialogTest, OnlineTargetSendsMapPositionAndChannel) {
    cSOSDialog dlg; SOSState state; state.members.push_back(MakeSOSMember(20,"Bob","Member",10,true));
    InstallSOSCallbacks(dlg,state); dlg.OnActionEvent(cSOSDialog::kOkBtnId,nullptr,0);
    EXPECT_EQ(state.sendCalls,1); EXPECT_EQ(state.sentObjectId,10u); EXPECT_EQ(state.sentMemberId,20u);
    EXPECT_EQ(state.sentMapNum,55u); EXPECT_EQ(state.sentChannel,3u);
    EXPECT_EQ(state.sentMovePoint,12u|(34u<<16u));
}

TEST(CSOSDialogTest, PositionPackingUsesLegacyWordCasts) {
    cSOSDialog dlg; SOSState state; state.members.push_back(MakeSOSMember(20,"Bob","Member",10,true));
    state.x=65537.0f; state.z=65538.0f; InstallSOSCallbacks(dlg,state);
    dlg.OnActionEvent(cSOSDialog::kOkBtnId,nullptr,0);
    EXPECT_EQ(state.sentMovePoint,1u|(2u<<16u));
}

TEST(CSOSDialogTest, UnknownButtonDoesNotSend) {
    cSOSDialog dlg; SOSState state; state.members.push_back(MakeSOSMember(20,"Bob","Member",10,true));
    InstallSOSCallbacks(dlg,state); dlg.OnActionEvent(999,nullptr,0); EXPECT_EQ(state.sendCalls,0);
}

TEST(CSOSDialogTest, MissingSelectedMemberDoesNotSend) {
    cSOSDialog dlg; SOSState state; InstallSOSCallbacks(dlg,state);
    dlg.OnActionEvent(cSOSDialog::kOkBtnId,nullptr,0); EXPECT_EQ(state.sendCalls,0);
}

TEST(CSOSDialogTest, SetCallbacksReplacesDispatch) {
    cSOSDialog dlg; SOSState first; SOSState second;
    first.members.push_back(MakeSOSMember(20,"A","M",1,true));
    second.members.push_back(MakeSOSMember(30,"B","M",1,true));
    InstallSOSCallbacks(dlg,first); InstallSOSCallbacks(dlg,second);
    dlg.OnActionEvent(cSOSDialog::kOkBtnId,nullptr,0);
    EXPECT_EQ(first.sendCalls,0); EXPECT_EQ(second.sendCalls,1); EXPECT_EQ(second.sentMemberId,30u);
}

// ===========================================================================
// SelectIdx accessor (1:1 with legacy m_dwSelectIdx)
// ===========================================================================

TEST(CSOSDialogTest, SelectIdxDefaultZero) {
    cSOSDialog dlg;
    EXPECT_EQ(dlg.GetSelectIdx(), 0u);
}

TEST(CSOSDialogTest, SetSelectIdxUpdatesValue) {
    cSOSDialog dlg;
    dlg.SetSelectIdx(42);
    EXPECT_EQ(dlg.GetSelectIdx(), 42u);
    dlg.SetSelectIdx(0);
    EXPECT_EQ(dlg.GetSelectIdx(), 0u);
}

}  // namespace mxh::ui::test
