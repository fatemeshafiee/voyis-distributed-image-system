#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace zmq_utils {

enum class SendResult {
    Ok,
    WouldBlock,
    Error
};

std::optional<std::string> recv_string(
    void* socket,
    int flags = 0,
    std::string_view what = "zmq_msg_recv"
);

std::optional<std::vector<unsigned char>> recv_bytes(
    void* socket,
    int flags = 0,
    std::string_view what = "zmq_msg_recv"
);

SendResult send_string(
    void* socket,
    std::string_view data,
    int flags,
    std::string_view what = "zmq_send"
);

SendResult send_bytes(
    void* socket,
    std::span<const unsigned char> data,
    int flags,
    std::string_view what = "zmq_send"
);

}  // namespace zmq_utils
