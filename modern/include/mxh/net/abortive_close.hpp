#pragma once

namespace mxh::net {

// SO_LINGER value that makes closesocket abortive (TCP RST) rather than
// a graceful FIN. l_onoff=1 enables linger; l_linger=0 discards unsent
// data and resets the connection.
struct LingerOption {
    int l_onoff = 1;
    int l_linger = 0;
};

inline constexpr LingerOption kAbortiveLinger{1, 0};

// Set SO_LINGER {1,0} and then close. There is no shutdown step: a
// shutdown() would send FIN before the reset. If linger cannot be set,
// the socket is left open so the caller can decide; this function does
// not close it and does not call shutdown.
template <typename SetLinger, typename CloseSocket>
bool abortive_rst_close(SetLinger&& set_linger, CloseSocket&& close_socket) {
    if (!set_linger(kAbortiveLinger)) return false;
    close_socket();
    return true;
}

}  // namespace mxh::net
