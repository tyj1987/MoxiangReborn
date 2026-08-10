#include "mxh/compat/anm_motion.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace mxh::compat {
namespace {
template<class T> bool read(std::span<const std::uint8_t> bytes, std::size_t at, T& out) {
    if (at > bytes.size() || sizeof(T) > bytes.size() - at) return false;
    std::memcpy(&out, bytes.data() + at, sizeof(T)); return true;
}
void fail(std::string* error, const char* value) { if (error) *error = value; }
std::string fixedName(std::span<const std::uint8_t> bytes, std::size_t at) {
    if (at + 128 > bytes.size()) return {};
    std::size_t length = 0;
    while (length < 128 && bytes[at + length]) ++length;
    return {reinterpret_cast<const char*>(bytes.data() + at), length};
}

template<class Key, std::size_t N>
std::array<float, N> sampleLinear(const std::vector<Key>& keys, float frame,
                                  const std::array<float, N>& fallback) noexcept {
    if (keys.empty()) return fallback;
    const auto next = std::lower_bound(keys.begin(), keys.end(), frame,
        [](const Key& key, float value) { return static_cast<float>(key.frame) < value; });
    if (next == keys.begin()) return next->value;
    if (next == keys.end()) return keys.back().value;
    const auto& previous = *(next - 1);
    const float span = static_cast<float>(next->frame - previous.frame);
    if (span <= 0.0f) return next->value;
    const float alpha = (frame - static_cast<float>(previous.frame)) / span;
    std::array<float, N> result{};
    for (std::size_t i = 0; i < N; ++i)
        result[i] = previous.value[i] + (next->value[i] - previous.value[i]) * alpha;
    return result;
}
}

std::array<float, 3> AnmMotionObject::samplePosition(
    float frame, const std::array<float, 3>& fallback) const noexcept {
    return sampleLinear<AnmPositionKey, 3>(positions, frame, fallback);
}

std::array<float, 3> AnmMotionObject::sampleScale(
    float frame, const std::array<float, 3>& fallback) const noexcept {
    return sampleLinear<AnmScaleKey, 3>(scales, frame, fallback);
}

std::array<float, 4> AnmMotionObject::sampleRotation(
    float frame, const std::array<float, 4>& fallback) const noexcept {
    if (rotations.empty()) return fallback;
    const auto next = std::lower_bound(rotations.begin(), rotations.end(), frame,
        [](const AnmRotationKey& key, float value) { return static_cast<float>(key.frame) < value; });
    if (next == rotations.begin()) return next->value;
    if (next == rotations.end()) return rotations.back().value;
    const auto& previous = *(next - 1);
    const float span = static_cast<float>(next->frame - previous.frame);
    if (span <= 0.0f) return next->value;
    const float alpha = (frame - static_cast<float>(previous.frame)) / span;
    auto a = previous.value;
    auto b = next->value;
    float dot = 0.0f;
    for (std::size_t i = 0; i < 4; ++i) dot += a[i] * b[i];
    if (dot < 0.0f) {
        dot = -dot;
        for (auto& value : b) value = -value;
    }
    std::array<float, 4> result{};
    if (dot > 0.9995f) {
        for (std::size_t i = 0; i < 4; ++i) result[i] = a[i] + (b[i] - a[i]) * alpha;
    } else {
        dot = std::clamp(dot, -1.0f, 1.0f);
        const float theta = std::acos(dot);
        const float sine = std::sin(theta);
        const float wa = std::sin((1.0f - alpha) * theta) / sine;
        const float wb = std::sin(alpha * theta) / sine;
        for (std::size_t i = 0; i < 4; ++i) result[i] = a[i] * wa + b[i] * wb;
    }
    float length = 0.0f;
    for (const auto value : result) length += value * value;
    length = std::sqrt(length);
    if (length > 0.0f) for (auto& value : result) value /= length;
    return result;
}

std::optional<AnmMotion> AnmMotion::parse(std::span<const std::uint8_t> bytes,
                                          std::string* error) {
    constexpr std::size_t kMotionHeader = 160, kObjectHeader = 152;
    if (bytes.size() < kMotionHeader) { fail(error, "truncated ANM header"); return std::nullopt; }
    AnmMotion motion;
    std::uint32_t objectCount = 0;
    read(bytes, 0, motion.version); read(bytes, 4, motion.ticks_per_frame);
    read(bytes, 8, motion.first_frame); read(bytes, 12, motion.last_frame);
    read(bytes, 16, motion.frame_speed); read(bytes, 20, objectCount);
    read(bytes, 24, motion.key_frame_step);
    if (motion.version != 1 || objectCount > 65536u || motion.last_frame < motion.first_frame) {
        fail(error, "unsupported ANM header"); return std::nullopt;
    }
    std::size_t cursor = kMotionHeader;
    motion.objects.reserve(objectCount);
    for (std::uint32_t objectIndex = 0; objectIndex < objectCount; ++objectIndex) {
        std::uint32_t type = 0, size = 0;
        if (!read(bytes, cursor, type) || !read(bytes, cursor + 4, size) ||
            size < kObjectHeader || cursor + 8u + size > bytes.size()) {
            fail(error, "truncated ANM object"); return std::nullopt;
        }
        cursor += 8;
        const auto objectEnd = cursor + size;
        AnmMotionObject object;
        std::uint32_t rotations = 0, positions = 0, scales = 0, animated = 0;
        read(bytes, cursor, object.index); read(bytes, cursor + 4, rotations);
        read(bytes, cursor + 8, positions); read(bytes, cursor + 12, scales);
        read(bytes, cursor + 16, animated); object.name = fixedName(bytes, cursor + 20);
        cursor += kObjectHeader;
        if (rotations > 1'000'000u || positions > 1'000'000u || scales > 1'000'000u || animated > 1'000'000u) {
            fail(error, "implausible ANM key count"); return std::nullopt;
        }
        object.positions.resize(positions);
        for (auto& key : object.positions) {
            if (!read(bytes, cursor, key.ticks) || !read(bytes, cursor + 4, key.frame) ||
                cursor + 20 > objectEnd) { fail(error, "truncated ANM position keys"); return std::nullopt; }
            std::memcpy(key.value.data(), bytes.data() + cursor + 8, 12); cursor += 20;
        }
        object.rotations.resize(rotations);
        for (auto& key : object.rotations) {
            if (!read(bytes, cursor, key.ticks) || !read(bytes, cursor + 4, key.frame) ||
                cursor + 24 > objectEnd) { fail(error, "truncated ANM rotation keys"); return std::nullopt; }
            std::memcpy(key.value.data(), bytes.data() + cursor + 8, 16); cursor += 24;
        }
        object.scales.resize(scales);
        for (auto& key : object.scales) {
            if (!read(bytes, cursor, key.ticks) || !read(bytes, cursor + 4, key.frame) ||
                cursor + 36 > objectEnd) { fail(error, "truncated ANM scale keys"); return std::nullopt; }
            std::memcpy(key.value.data(), bytes.data() + cursor + 8, 12); cursor += 36;
        }
        for (std::uint32_t key = 0; key < animated; ++key) {
            std::uint32_t vertices = 0, texcoords = 0;
            if (!read(bytes, cursor + 8, vertices) || !read(bytes, cursor + 12, texcoords) || cursor + 24 > objectEnd) {
                fail(error, "truncated ANM mesh key"); return std::nullopt;
            }
            cursor += 24;
            const auto payload = static_cast<std::size_t>(vertices) * 12u + static_cast<std::size_t>(texcoords) * 8u;
            if (cursor > objectEnd || payload > objectEnd - cursor) { fail(error, "truncated ANM mesh payload"); return std::nullopt; }
            cursor += payload;
        }
        if (cursor != objectEnd) { fail(error, "ANM object size mismatch"); return std::nullopt; }
        motion.objects.push_back(std::move(object));
    }
    return motion;
}

const AnmMotionObject* AnmMotion::find(std::string_view name) const noexcept {
    const auto found = std::find_if(objects.begin(), objects.end(),
        [&](const auto& object) { return object.name == name; });
    return found == objects.end() ? nullptr : &*found;
}

} // namespace mxh::compat
