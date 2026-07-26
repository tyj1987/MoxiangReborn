#include "mxh/server/agent_note.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(NoteUser, SendNoteNoUserDrops){NoteRequest r;r.protocol=note_sendnote_syn;r.user_found=false;auto a=classify_note_user(r);EXPECT_EQ(a.kind,NoteActionKind::drop_no_user);EXPECT_EQ(a.protocol,note_sendnote_syn);}
TEST(NoteUser, SendNoteFromNameInvalidDrops){NoteRequest r;r.protocol=note_sendnote_syn;r.user_found=true;r.from_invalid=true;auto a=classify_note_user(r);EXPECT_EQ(a.kind,NoteActionKind::drop_with_invalid_filter);}
TEST(NoteUser, SendNoteToNameInvalidDrops){NoteRequest r;r.protocol=note_sendnote_syn;r.user_found=true;r.to_invalid=true;EXPECT_EQ(classify_note_user(r).kind,NoteActionKind::drop_with_invalid_filter);}
TEST(NoteUser, SendNoteWithQuoteDrops){NoteRequest r;r.protocol=note_sendnote_syn;r.user_found=true;r.note_has_quote=true;EXPECT_EQ(classify_note_user(r).kind,NoteActionKind::drop_with_invalid_filter);}
TEST(NoteUser, SendNoteValidSendsFiltered){NoteRequest r;r.protocol=note_sendnote_syn;r.user_found=true;auto a=classify_note_user(r);EXPECT_EQ(a.kind,NoteActionKind::send_note_filtered);EXPECT_EQ(a.protocol,note_sendnote_syn);}
TEST(NoteUser, SendNoteIdSendsById){NoteRequest r;r.protocol=note_sendnoteid_syn;EXPECT_EQ(classify_note_user(r).kind,NoteActionKind::send_note_by_id);}
TEST(NoteUser, ReceiveNoteFoundForwardsToUser){NoteRequest r;r.protocol=note_receivenote;r.user_found=true;auto a=classify_note_user(r);EXPECT_EQ(a.kind,NoteActionKind::forward_to_user);EXPECT_EQ(a.protocol,note_receivenote);}
TEST(NoteUser, ReceiveNoteMissingDrops){NoteRequest r;r.protocol=note_receivenote;r.user_found=false;EXPECT_EQ(classify_note_user(r).kind,NoteActionKind::drop_no_user);}
TEST(NoteUser, DelAllNoteFoundDeletesAndAcks){NoteRequest r;r.protocol=note_delallnote_syn;r.user_found=true;EXPECT_EQ(classify_note_user(r).kind,NoteActionKind::delete_all_with_ack);}
TEST(NoteUser, NoteListAlwaysLists){NoteRequest r;r.protocol=note_notelist_syn;EXPECT_EQ(classify_note_user(r).kind,NoteActionKind::list_notes);}
TEST(NoteUser, ReadNoteFoundReads){NoteRequest r;r.protocol=note_readnote_syn;r.user_found=true;EXPECT_EQ(classify_note_user(r).kind,NoteActionKind::read_note);}
TEST(NoteServer, SendNoteNoUserDrops){NoteRequest r;r.protocol=note_sendnote_syn;r.user_found=false;EXPECT_EQ(classify_note_server(r).kind,NoteActionKind::drop_no_user);}
TEST(NoteServer, SendNoteToInvalidDrops){NoteRequest r;r.protocol=note_sendnote_syn;r.user_found=true;r.to_invalid=true;EXPECT_EQ(classify_note_server(r).kind,NoteActionKind::drop_with_invalid_filter);}
TEST(NoteServer, SendNoteValidSends){NoteRequest r;r.protocol=note_sendnote_syn;r.user_found=true;EXPECT_EQ(classify_note_server(r).kind,NoteActionKind::send_note_filtered);}
TEST(NoteServer, OtherProtocolsDrop){NoteRequest r;r.protocol=note_readnote_syn;r.user_found=true;EXPECT_EQ(classify_note_server(r).kind,NoteActionKind::drop_no_user);}