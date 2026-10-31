#pragma once

#include <cstdint>
#include <optional>

namespace mxh::client {

// 96 DPI is one physical pixel per game pixel. 120 is 125%, 144 is 150%,
// 192 is 200%. The game client and the swap-chain stay at the requested
// pixel size. Only the non-client frame grows with the monitor DPI, so
// Windows does not bitmap-stretch the back buffer.
inline constexpr int kDpiBaseline = 96;

struct DpiSize {
    int width = 0;
    int height = 0;
};

struct DpiPoint {
    int x = 0;
    int y = 0;
};

inline int normalize_dpi(int dpi) noexcept {
    return dpi > 0 ? dpi : kDpiBaseline;
}

// MulDiv-style rounding of a 96-DPI length onto `dpi`.
inline int scale_length_for_dpi(int length_at_96, int dpi) noexcept {
    dpi = normalize_dpi(dpi);
    const long long scaled = static_cast<long long>(length_at_96) * dpi;
    const int bias = length_at_96 < 0 ? -(kDpiBaseline / 2) : (kDpiBaseline / 2);
    return static_cast<int>((scaled + bias) / kDpiBaseline);
}

// Requested client pixels are the back-buffer pixels at every DPI.
inline DpiSize render_client_size(int requested_width, int requested_height,
                                  int dpi) noexcept {
    (void)dpi;
    if (requested_width < 0) requested_width = 0;
    if (requested_height < 0) requested_height = 0;
    return DpiSize{requested_width, requested_height};
}

// Outer window size whose client area stays `client_*` pixels when the
// non-client thicknesses were measured at 96 DPI.
inline DpiSize outer_window_size(int client_width, int client_height,
                                 int frame_left, int frame_top,
                                 int frame_right, int frame_bottom,
                                 int dpi) noexcept {
    const DpiSize client = render_client_size(client_width, client_height, dpi);
    if (frame_left < 0) frame_left = 0;
    if (frame_top < 0) frame_top = 0;
    if (frame_right < 0) frame_right = 0;
    if (frame_bottom < 0) frame_bottom = 0;
    return DpiSize{
        client.width + scale_length_for_dpi(frame_left, dpi) +
            scale_length_for_dpi(frame_right, dpi),
        client.height + scale_length_for_dpi(frame_top, dpi) +
            scale_length_for_dpi(frame_bottom, dpi),
    };
}

// A mouse position is already in client pixels once the process is
// per-monitor DPI aware. It is not multiplied by 125% or 150%.
inline std::optional<DpiPoint> mouse_to_game_client(int x, int y,
                                                    int client_width,
                                                    int client_height) noexcept {
    if (client_width <= 0 || client_height <= 0) return std::nullopt;
    if (x < 0 || y < 0 || x >= client_width || y >= client_height) return std::nullopt;
    return DpiPoint{x, y};
}

}  // namespace mxh::client
