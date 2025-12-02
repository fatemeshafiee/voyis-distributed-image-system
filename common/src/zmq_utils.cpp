#include "common/zmq_utils.hpp"

#include <cstddef>
#include <cerrno>
#include <iostream>
#include <zmq.h>

namespace zmq_utils {
namespace {

SendResult send_raw(
    void* socket,
    const void* data,
    std::size_t size,
    int flags,
    std::string_view what
) {
    int rc = zmq_send(socket, data, size, flags);
    if (rc == -1) {
        if (errno == EAGAIN) {
            return SendResult::WouldBlock;
        }
        std::cerr << "[ERROR] " << what << " failed: " << zmq_strerror(errno) << "\n";
        return SendResult::Error;
    }
    return SendResult::Ok;
}

}  // namespace

std::optional<std::string> recv_string(
    void* socket,
    int flags,
    std::string_view what
) {
    zmq_msg_t msg;
    zmq_msg_init(&msg);
    int rc = zmq_msg_recv(&msg, socket, flags);
    if (rc == -1) {
        std::cerr << "[ERROR] " << what << " failed: " << zmq_strerror(errno) << "\n";
        zmq_msg_close(&msg);
        return std::nullopt;
    }
    std::string data(
        static_cast<char*>(zmq_msg_data(&msg)),
        static_cast<char*>(zmq_msg_data(&msg)) + zmq_msg_size(&msg)
    );
    zmq_msg_close(&msg);
    return data;
}

std::optional<std::vector<unsigned char>> recv_bytes(
    void* socket,
    int flags,
    std::string_view what
) {
    zmq_msg_t msg;
    zmq_msg_init(&msg);
    int rc = zmq_msg_recv(&msg, socket, flags);
    if (rc == -1) {
        std::cerr << "[ERROR] " << what << " failed: " << zmq_strerror(errno) << "\n";
        zmq_msg_close(&msg);
        return std::nullopt;
    }
    auto* begin = static_cast<unsigned char*>(zmq_msg_data(&msg));
    std::vector<unsigned char> data(begin, begin + zmq_msg_size(&msg));
    zmq_msg_close(&msg);
    return data;
}

SendResult send_string(
    void* socket,
    std::string_view data,
    int flags,
    std::string_view what
) {
    return send_raw(socket, data.data(), data.size(), flags, what);
}

SendResult send_bytes(
    void* socket,
    std::span<const unsigned char> data,
    int flags,
    std::string_view what
) {
    return send_raw(socket, data.data(), data.size(), flags, what);
}

}  // namespace zmq_utils
