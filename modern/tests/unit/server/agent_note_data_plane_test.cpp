// agent_note_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_note_user +
// classify_note_server (D4.154).
// Augments the legacy 15-test agent_note_test.cpp with deeper coverage of:
//   - note_category constant = 58 (MP_NOTE)
//   - 18 sub-protocol constants (note_sendnote_syn=0 .. note_new_note=17)
//   - 8-value NoteActionKind enum (send_note_filtered, send_note_by_id,
//     forward_to_user, delete_all_with_ack, list_notes, read_note,
//     drop_no_user, drop_with_invalid_filter)
//   - NoteRequest struct defaults
//   - NoteAction struct defaults
//   - classify_note_user truth table:
//       sendnote_syn + !user_found -> drop_no_user
//       sendnote_syn + from_invalid -> drop_with_invalid_filter
//       sendnote_syn + to_invalid -> drop_with_invalid_filter
//       sendnote_syn + note_has_quote -> drop_with_invalid_filter
//       sendnote_syn + all valid -> send_note_filtered
//       sendnoteid_syn -> send_note_by_id
//       receivenote + user_found -> forward_to_user
//       receivenote + !user_found -> drop_no_user
//       delallnote_syn + user_found -> delete_all_with_ack
//       delallnote_syn + !user_found -> drop_no_user
//       notelist_syn -> list_notes (unconditional)
//       readnote_syn + user_found -> read_note
//       readnote_syn + !user_found -> drop_no_user
//       delnote_syn + user_found -> read_note
//       delnote_syn + !user_found -> drop_no_user
//       default -> drop_no_user with protocol preserved
//   - classify_note_server truth table:
//       sendnote_syn + !user_found -> drop_no_user
//       sendnote_syn + from/to_invalid -> drop_with_invalid_filter
//       sendnote_syn + valid -> send_note_filtered
//       default -> drop_no_user
//
// 1:1 invariants (locked):
//   - note_category = 58
//   - 18 protocol constants (0..17, all distinct)
//   - User dispatch: sendnote_syn filter chain (4 gates); 5 forward kinds
//   - Server dispatch: only sendnote_syn path

#pragma once

#include "mxh/server/agent_note.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

namespace {

using mxh::server::classify_note_server;
using mxh::server::classify_note_user;
using mxh::server::NoteAction;
using mxh::server::NoteActionKind;
using mxh::server::NoteRequest;
using mxh::server::note_category;
using mxh::server::note_delallnote_ack;
using mxh::server::note_delallnote_nack;
using mxh::server::note_delallnote_syn;
using mxh::server::note_delnote_ack;
using mxh::server::note_delnote_nack;
using mxh::server::note_delnote_syn;
using mxh::server::note_new_note;
using mxh::server::note_notelist_ack;
using mxh::server::note_notelist_nack;
using mxh::server::note_notelist_syn;
using mxh::server::note_readnote_ack;
using mxh::server::note_readnote_nack;
using mxh::server::note_readnote_syn;
using mxh::server::note_receivenote;
using mxh::server::note_sendnote_ack;
using mxh::server::note_sendnote_nack;
using mxh::server::note_sendnote_syn;
using mxh::server::note_sendnoteid_syn;

}  // namespace


// ===========================================================================
// Constants
// ===========================================================================

TEST(NoteDataPlane, CategoryIsFiftyEight) {
    EXPECT_EQ(note_category, 58u);
}

TEST(NoteDataPlane, ProtocolSendnoteSynIsZero) { EXPECT_EQ(note_sendnote_syn, 0u); }
TEST(NoteDataPlane, ProtocolSendnoteAckIsOne) { EXPECT_EQ(note_sendnote_ack, 1u); }
TEST(NoteDataPlane, ProtocolSendnoteNackIsTwo) { EXPECT_EQ(note_sendnote_nack, 2u); }
TEST(NoteDataPlane, ProtocolSendnoteidSynIsThree) { EXPECT_EQ(note_sendnoteid_syn, 3u); }
TEST(NoteDataPlane, ProtocolReceivenoteIsFour) { EXPECT_EQ(note_receivenote, 4u); }
TEST(NoteDataPlane, ProtocolDelnoteSynIsFive) { EXPECT_EQ(note_delnote_syn, 5u); }
TEST(NoteDataPlane, ProtocolDelallnoteSynIsEight) { EXPECT_EQ(note_delallnote_syn, 8u); }
TEST(NoteDataPlane, ProtocolNotelistSynIsEleven) { EXPECT_EQ(note_notelist_syn, 11u); }
TEST(NoteDataPlane, ProtocolReadnoteSynIsFourteen) { EXPECT_EQ(note_readnote_syn, 14u); }
TEST(NoteDataPlane, ProtocolNewNoteIsSeventeen) { EXPECT_EQ(note_new_note, 17u); }

TEST(NoteDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        note_sendnote_syn, note_sendnote_ack, note_sendnote_nack,
        note_sendnoteid_syn, note_receivenote,
        note_delnote_syn, note_delnote_ack, note_delnote_nack,
        note_delallnote_syn, note_delallnote_ack, note_delallnote_nack,
        note_notelist_syn, note_notelist_ack, note_notelist_nack,
        note_readnote_syn, note_readnote_ack, note_readnote_nack,
        note_new_note,
    };
    EXPECT_EQ(seen.size(), 18u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(NoteDataPlane, ActionKindHasEightValues) {
    auto all = {
        NoteActionKind::send_note_filtered, NoteActionKind::send_note_by_id,
        NoteActionKind::forward_to_user, NoteActionKind::delete_all_with_ack,
        NoteActionKind::list_notes, NoteActionKind::read_note,
        NoteActionKind::drop_no_user, NoteActionKind::drop_with_invalid_filter,
    };
    EXPECT_EQ(all.size(), 8u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(NoteDataPlane, RequestDefaults) {
    NoteRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_EQ(r.target_id, 0u);
    EXPECT_TRUE(r.user_found);
    EXPECT_FALSE(r.from_invalid);
    EXPECT_FALSE(r.to_invalid);
    EXPECT_FALSE(r.note_has_quote);
}

TEST(NoteDataPlane, ActionDefaults) {
    NoteAction a{};
    EXPECT_EQ(a.kind, NoteActionKind::drop_no_user);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
}


// ===========================================================================
// classify_note_user -- sendnote_syn filter chain
// ===========================================================================

TEST(NoteDataPlane, UserSendnoteNoUserDrops) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = false;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.kind, NoteActionKind::drop_no_user);
    EXPECT_EQ(a.protocol, note_sendnote_syn);
}

TEST(NoteDataPlane, UserSendnoteFromInvalidDrops) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = true;
    r.from_invalid = true;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::drop_with_invalid_filter);
}

TEST(NoteDataPlane, UserSendnoteToInvalidDrops) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = true;
    r.to_invalid = true;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::drop_with_invalid_filter);
}

TEST(NoteDataPlane, UserSendnoteNoteHasQuoteDrops) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = true;
    r.note_has_quote = true;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::drop_with_invalid_filter);
}

TEST(NoteDataPlane, UserSendnoteValidSendsFiltered) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = true;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.kind, NoteActionKind::send_note_filtered);
    EXPECT_EQ(a.protocol, note_sendnote_syn);
}

TEST(NoteDataPlane, UserSendnoteUserFoundWinsOverInvalidFilter) {
    // !user_found wins over invalid filter (return early).
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = false;
    r.from_invalid = true;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::drop_no_user);
}


// ===========================================================================
// classify_note_user -- other user paths
// ===========================================================================

TEST(NoteDataPlane, UserSendnoteidSendsById) {
    NoteRequest r;
    r.protocol = note_sendnoteid_syn;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::send_note_by_id);
}

TEST(NoteDataPlane, UserSendnoteidPreservesProtocol) {
    NoteRequest r;
    r.protocol = note_sendnoteid_syn;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.protocol, note_sendnoteid_syn);
}

TEST(NoteDataPlane, UserReceivenoteFoundForwardsToUser) {
    NoteRequest r;
    r.protocol = note_receivenote;
    r.user_found = true;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.kind, NoteActionKind::forward_to_user);
    EXPECT_EQ(a.protocol, note_receivenote);
}

TEST(NoteDataPlane, UserReceivenoteMissingDrops) {
    NoteRequest r;
    r.protocol = note_receivenote;
    r.user_found = false;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::drop_no_user);
}

TEST(NoteDataPlane, UserDelallnoteFoundDeletesAndAcks) {
    NoteRequest r;
    r.protocol = note_delallnote_syn;
    r.user_found = true;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::delete_all_with_ack);
}

TEST(NoteDataPlane, UserDelallnoteMissingDrops) {
    NoteRequest r;
    r.protocol = note_delallnote_syn;
    r.user_found = false;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::drop_no_user);
}

TEST(NoteDataPlane, UserNotelistAlwaysLists) {
    NoteRequest r;
    r.protocol = note_notelist_syn;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::list_notes);
}

TEST(NoteDataPlane, UserNotelistIgnoresUserFound) {
    NoteRequest r;
    r.protocol = note_notelist_syn;
    r.user_found = false;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::list_notes);
}

TEST(NoteDataPlane, UserReadnoteFoundReads) {
    NoteRequest r;
    r.protocol = note_readnote_syn;
    r.user_found = true;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::read_note);
}

TEST(NoteDataPlane, UserReadnoteMissingDrops) {
    NoteRequest r;
    r.protocol = note_readnote_syn;
    r.user_found = false;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::drop_no_user);
}

TEST(NoteDataPlane, UserDelnoteFoundReads) {
    NoteRequest r;
    r.protocol = note_delnote_syn;
    r.user_found = true;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::read_note);
}

TEST(NoteDataPlane, UserDelnoteMissingDrops) {
    NoteRequest r;
    r.protocol = note_delnote_syn;
    r.user_found = false;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::drop_no_user);
}


// ===========================================================================
// classify_note_user -- default path
// ===========================================================================

TEST(NoteDataPlane, UserUnknownProtocolDrops) {
    NoteRequest r;
    r.protocol = 99;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::drop_no_user);
}

TEST(NoteDataPlane, UserUnknownPreservesProtocol) {
    NoteRequest r;
    r.protocol = 99;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.protocol, 99u);
}

TEST(NoteDataPlane, UserProtocol255Drops) {
    NoteRequest r;
    r.protocol = 255;
    EXPECT_EQ(classify_note_user(r).kind, NoteActionKind::drop_no_user);
}


// ===========================================================================
// classify_note_user -- object_id preservation
// ===========================================================================

TEST(NoteDataPlane, UserPreservesObjectIdMaxUint32) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.object_id, 0xDEADBEEFu);
}

TEST(NoteDataPlane, UserPreservesObjectIdOnDrop) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    auto a = classify_note_user(r);
    EXPECT_EQ(a.object_id, 0xCAFEBABEu);
}


// ===========================================================================
// classify_note_server -- sendnote_syn path
// ===========================================================================

TEST(NoteDataPlane, ServerSendnoteNoUserDrops) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = false;
    EXPECT_EQ(classify_note_server(r).kind, NoteActionKind::drop_no_user);
}

TEST(NoteDataPlane, ServerSendnoteFromInvalidDrops) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = true;
    r.from_invalid = true;
    EXPECT_EQ(classify_note_server(r).kind, NoteActionKind::drop_with_invalid_filter);
}

TEST(NoteDataPlane, ServerSendnoteToInvalidDrops) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = true;
    r.to_invalid = true;
    EXPECT_EQ(classify_note_server(r).kind, NoteActionKind::drop_with_invalid_filter);
}

TEST(NoteDataPlane, ServerSendnoteValidSends) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = true;
    EXPECT_EQ(classify_note_server(r).kind, NoteActionKind::send_note_filtered);
}

TEST(NoteDataPlane, ServerSendnoteUserFoundWinsOverInvalid) {
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = false;
    r.from_invalid = true;
    EXPECT_EQ(classify_note_server(r).kind, NoteActionKind::drop_no_user);
}


// ===========================================================================
// classify_note_server -- default
// ===========================================================================

TEST(NoteDataPlane, ServerOtherProtocolsDrop) {
    NoteRequest r;
    r.protocol = note_readnote_syn;
    r.user_found = true;
    EXPECT_EQ(classify_note_server(r).kind, NoteActionKind::drop_no_user);
}

TEST(NoteDataPlane, ServerNotelistDrops) {
    NoteRequest r;
    r.protocol = note_notelist_syn;
    r.user_found = true;
    EXPECT_EQ(classify_note_server(r).kind, NoteActionKind::drop_no_user);
}

TEST(NoteDataPlane, ServerUnknownProtocolDrops) {
    NoteRequest r;
    r.protocol = 99;
    EXPECT_EQ(classify_note_server(r).kind, NoteActionKind::drop_no_user);
}

TEST(NoteDataPlane, ServerUnknownPreservesProtocol) {
    NoteRequest r;
    r.protocol = 99;
    auto a = classify_note_server(r);
    EXPECT_EQ(a.protocol, 99u);
}

TEST(NoteDataPlane, ServerIgnoresNoteHasQuote) {
    // Server path doesn't check note_has_quote (legacy quirk).
    NoteRequest r;
    r.protocol = note_sendnote_syn;
    r.user_found = true;
    r.note_has_quote = true;
    EXPECT_EQ(classify_note_server(r).kind, NoteActionKind::send_note_filtered);
}
