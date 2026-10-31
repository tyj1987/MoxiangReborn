#include "mxh/audio/audio_contract.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

// Synthetic inputs only. This file does not open a pak, PlayDH tree, or
// SoundList. Passing it does not play audio.

namespace {

using mxh::audio::contract::BgmAction;
using mxh::audio::contract::Channel;
using mxh::audio::contract::CueReject;
using mxh::audio::contract::EngineCueKind;
using mxh::audio::contract::MissingBgm;

float nan() { return std::numeric_limits<float>::quiet_NaN(); }
float pos_inf() { return std::numeric_limits<float>::infinity(); }
float neg_inf() { return -std::numeric_limits<float>::infinity(); }

}  // namespace

TEST(AudioContract, BgmVolumeAndEntryFallback) {
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_bus_volume(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_bus_volume(0.25f), 0.25f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_bus_volume(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_bus_volume(2.5f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_bus_volume(-0.5f), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_bus_volume(nan()), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_bus_volume(pos_inf()), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_bus_volume(neg_inf()), 0.0f);

    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_entry_volume(0.4f), 0.4f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_entry_volume(2.0f), 2.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_entry_volume(0.0f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_entry_volume(-1.0f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_entry_volume(nan()), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_entry_volume(pos_inf()), 1.0f);

    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_output_gain(0.5f, 0.5f), 0.25f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_output_gain(1.0f, 2.0f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::bgm_output_gain(0.0f, 0.4f), 0.0f);
    EXPECT_FLOAT_EQ(
        mxh::audio::contract::bgm_output_gain(
            mxh::audio::contract::bgm_bus_volume(nan()), 0.5f),
        0.0f);
}

TEST(AudioContract, SfxVolumeBusAndEntryFallback) {
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_bus_volume(0.25f), 0.25f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_bus_volume(2.5f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_bus_volume(-0.5f), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_bus_volume(nan()), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_bus_volume(pos_inf()), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_bus_volume(neg_inf()), 1.0f);

    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_entry_volume(0.4f), 0.4f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_entry_volume(0.0f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_entry_volume(-2.0f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_entry_volume(nan()), 1.0f);
    EXPECT_TRUE(std::isinf(mxh::audio::contract::sfx_entry_volume(pos_inf())));
    EXPECT_FLOAT_EQ(
        mxh::audio::contract::sfx_output_gain(
            1.0f, 1.0f, mxh::audio::contract::sfx_entry_volume(pos_inf()), 1.0f),
        1.0f);

    EXPECT_FALSE(mxh::audio::contract::sfx_bus_gain_finite(nan()));
    EXPECT_FALSE(mxh::audio::contract::sfx_bus_gain_finite(pos_inf()));
    EXPECT_TRUE(mxh::audio::contract::sfx_bus_gain_finite(0.0f));
    EXPECT_TRUE(mxh::audio::contract::sfx_bus_gain_finite(1.5f));
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_clamped_bus_gain(1.5f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_clamped_bus_gain(-0.2f), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_clamped_bus_gain(0.3f), 0.3f);
}

TEST(AudioContract, DistanceGainIsLinearBetweenMinAndMax) {
    EXPECT_FLOAT_EQ(mxh::audio::contract::distance_gain(0.0f, 10.0f, 100.0f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::distance_gain(10.0f, 10.0f, 100.0f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::distance_gain(100.0f, 10.0f, 100.0f), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::distance_gain(140.0f, 10.0f, 100.0f), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::distance_gain(55.0f, 10.0f, 100.0f), 0.5f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::distance_gain(1000.0f, 50.0f, 50.0f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::distance_gain(10.0f, 80.0f, 20.0f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::distance_gain(nan(), 10.0f, 100.0f), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::distance_gain(10.0f, pos_inf(), 100.0f), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::distance_gain(10.0f, 10.0f, neg_inf()), 0.0f);
}

TEST(AudioContract, SfxMixClampsAndVoicesRoundRobin) {
    EXPECT_EQ(mxh::audio::contract::kSfxVoiceCount, 16u);
    EXPECT_EQ(mxh::audio::contract::sfx_voice_slot(0), 0u);
    EXPECT_EQ(mxh::audio::contract::sfx_voice_slot(1), 1u);
    EXPECT_EQ(mxh::audio::contract::sfx_voice_slot(15), 15u);
    EXPECT_EQ(mxh::audio::contract::sfx_voice_slot(16), 0u);
    EXPECT_EQ(mxh::audio::contract::sfx_voice_slot(17), 1u);
    EXPECT_EQ(mxh::audio::contract::sfx_voice_slot(31), 15u);
    EXPECT_EQ(mxh::audio::contract::sfx_voice_slot(32), 0u);

    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_voice_mix(1.0f, 0.5f, 0.5f), 0.25f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_voice_mix(1.0f, 2.0f, 1.0f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_voice_mix(0.0f, 1.0f, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_output_gain(0.5f, 1.0f, 1.0f, 1.0f), 0.5f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_output_gain(0.0f, 1.0f, 1.0f, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::sfx_output_gain(1.0f, 0.5f, 0.5f, 0.5f), 0.125f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::scale_stored_mix(0.5f, 0.5f), 0.25f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::scale_stored_mix(1.0f, 1.0f), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::scale_stored_mix(0.0f, 0.8f), 0.0f);
}

TEST(AudioContract, ChannelAndMissingFileReject) {
    EXPECT_TRUE(mxh::audio::contract::channel_accepts_entry(Channel::Bgm, true, true));
    EXPECT_FALSE(mxh::audio::contract::channel_accepts_entry(Channel::Bgm, true, false));
    EXPECT_FALSE(mxh::audio::contract::channel_accepts_entry(Channel::Bgm, false, true));
    EXPECT_TRUE(mxh::audio::contract::channel_accepts_entry(Channel::Sfx, true, false));
    EXPECT_FALSE(mxh::audio::contract::channel_accepts_entry(Channel::Sfx, true, true));
    EXPECT_FALSE(mxh::audio::contract::channel_accepts_entry(Channel::Sfx, false, false));

    EXPECT_FALSE(mxh::audio::contract::relative_stays_in_root(true, false, false));
    EXPECT_FALSE(mxh::audio::contract::relative_stays_in_root(false, true, false));
    EXPECT_FALSE(mxh::audio::contract::relative_stays_in_root(false, false, true));
    EXPECT_TRUE(mxh::audio::contract::relative_stays_in_root(false, false, false));
    EXPECT_FALSE(mxh::audio::contract::cue_file_present(true, false));
    EXPECT_FALSE(mxh::audio::contract::cue_file_present(false, true));
    EXPECT_TRUE(mxh::audio::contract::cue_file_present(true, true));

    EXPECT_EQ(mxh::audio::contract::classify_cue(
                  false, Channel::Bgm, true, true, false, false, false, true),
              CueReject::OutOfRange);
    EXPECT_EQ(mxh::audio::contract::classify_cue(
                  true, Channel::Sfx, true, true, false, false, false, true),
              CueReject::WrongEntry);
    EXPECT_EQ(mxh::audio::contract::classify_cue(
                  true, Channel::Bgm, true, true, false, false, true, true),
              CueReject::EscapesRoot);
    EXPECT_EQ(mxh::audio::contract::classify_cue(
                  true, Channel::Sfx, true, false, false, false, false, false),
              CueReject::MissingFile);
    EXPECT_EQ(mxh::audio::contract::classify_cue(
                  true, Channel::Bgm, true, true, false, false, false, true),
              CueReject::Playable);
}

TEST(AudioContract, BgmSelectionRollbackAndMissingPolicy) {
    EXPECT_EQ(mxh::audio::contract::kNoSound, 0xffffu);
    EXPECT_EQ(mxh::audio::contract::kLoginBgmSoundId, 1667u);
    const auto login = mxh::audio::contract::login_bgm();
    EXPECT_EQ(login.action, BgmAction::Play);
    EXPECT_EQ(login.sound_id, 1667u);

    const auto field = mxh::audio::contract::map_bgm(1658u);
    EXPECT_EQ(field.action, BgmAction::Play);
    EXPECT_EQ(field.sound_id, 1658u);
    const auto unset = mxh::audio::contract::map_bgm(0u);
    EXPECT_EQ(unset.action, BgmAction::Play);
    EXPECT_EQ(unset.sound_id, 0u);

    const auto same = mxh::audio::contract::restore_previous_bgm(1667u, 1667u);
    EXPECT_EQ(same.action, BgmAction::Keep);
    EXPECT_EQ(same.sound_id, 1667u);
    const auto stopped = mxh::audio::contract::restore_previous_bgm(
        1667u, mxh::audio::contract::kNoSound);
    EXPECT_EQ(stopped.action, BgmAction::Stop);
    EXPECT_EQ(stopped.sound_id, mxh::audio::contract::kNoSound);
    const auto restored = mxh::audio::contract::restore_previous_bgm(
        mxh::audio::contract::kNoSound, 1663u);
    EXPECT_EQ(restored.action, BgmAction::Play);
    EXPECT_EQ(restored.sound_id, 1663u);

    EXPECT_EQ(mxh::audio::contract::missing_bgm_policy(false), MissingBgm::FailLoad);
    EXPECT_EQ(mxh::audio::contract::missing_bgm_policy(true),
              MissingBgm::WarnAndContinue);
}

TEST(AudioContract, FocusMuteAndOptionPercent) {
    EXPECT_FLOAT_EQ(mxh::audio::contract::focused_volume(0.8f, true), 0.8f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::focused_volume(0.8f, false), 0.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::focused_volume(0.0f, true), 0.0f);

    EXPECT_FALSE(mxh::audio::contract::checkbox_from_positive_volume(0.0f));
    EXPECT_FALSE(mxh::audio::contract::checkbox_from_positive_volume(-0.1f));
    EXPECT_TRUE(mxh::audio::contract::checkbox_from_positive_volume(0.01f));

    EXPECT_EQ(mxh::audio::contract::volume_to_percent(0.0f), 0);
    EXPECT_EQ(mxh::audio::contract::volume_to_percent(0.5f), 50);
    EXPECT_EQ(mxh::audio::contract::volume_to_percent(1.0f), 100);
    EXPECT_EQ(mxh::audio::contract::volume_to_percent(0.004f), 0);
    EXPECT_EQ(mxh::audio::contract::volume_to_percent(0.005f), 1);
    EXPECT_FLOAT_EQ(mxh::audio::contract::percent_to_volume(50), 0.5f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::percent_to_volume(150), 1.0f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::percent_to_volume(-20), 0.0f);

    EXPECT_FLOAT_EQ(mxh::audio::contract::muted_or_volume(true, 0.8f), 0.8f);
    EXPECT_FLOAT_EQ(mxh::audio::contract::muted_or_volume(false, 0.8f), 0.0f);
    const float applied = mxh::audio::contract::focused_volume(
        mxh::audio::contract::muted_or_volume(false, 0.8f), true);
    EXPECT_FLOAT_EQ(applied, 0.0f);
}

TEST(AudioContract, EffectSoundIdLoopAndEngineCue) {
    EXPECT_FALSE(mxh::audio::contract::effect_sound_id_accepted(0u));
    EXPECT_TRUE(mxh::audio::contract::effect_sound_id_accepted(1u));
    EXPECT_TRUE(mxh::audio::contract::effect_sound_id_accepted(0xffffu));
    EXPECT_FALSE(mxh::audio::contract::effect_sound_id_accepted(0x10000u));

    EXPECT_TRUE(mxh::audio::contract::entry_loops(true));
    EXPECT_FALSE(mxh::audio::contract::entry_loops(false));

    EXPECT_TRUE(mxh::audio::contract::generic_engine_cue_plays(EngineCueKind::Attack));
    EXPECT_TRUE(mxh::audio::contract::generic_engine_cue_plays(EngineCueKind::Pickup));
    EXPECT_FALSE(mxh::audio::contract::generic_engine_cue_plays(EngineCueKind::UiClick));
    EXPECT_FALSE(mxh::audio::contract::generic_engine_cue_plays(EngineCueKind::Skill));
    EXPECT_TRUE(mxh::audio::contract::spatial_engine_cue_plays(EngineCueKind::Attack));
    EXPECT_TRUE(mxh::audio::contract::spatial_engine_cue_plays(EngineCueKind::Pickup));
    EXPECT_TRUE(mxh::audio::contract::spatial_engine_cue_plays(EngineCueKind::UiClick));
    EXPECT_FALSE(mxh::audio::contract::spatial_engine_cue_plays(EngineCueKind::Skill));
}
