#pragma once

#include <memory>
#include "asio.hpp"
#include <cstdint>
#include <string>
#include <array>
#include <iostream>

namespace multicast_const {
    const std::string k_multicast_addr{"239.255.0.1"};
    const uint16_t k_port{30000};
}

/**
 * -> separate on system: win32, apple, linux
 */
class IClipBoardManager {
public:
    virtual ~IClipBoardManager() = default;
};

class IFileManager {
public:
    virtual ~IFileManager() = default;
};

class MulticastBus : public std::enable_shared_from_this<MulticastBus> {
private:
    std::shared_ptr<asio::ip::udp::socket> socket_{nullptr};
    std::shared_ptr<asio::ip::udp::endpoint> sender_endpoint_{nullptr};
    std::array<char, 1024> recv_buffer_{};
    void doReceive();
private:
    std::shared_ptr<IClipBoardManager> Clipboard_control_{nullptr};
public:
    explicit MulticastBus( asio::io_context &ctx );
    ~MulticastBus();
};