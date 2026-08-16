#pragma once

#include <memory>
#include "asio.hpp"
#include <cstdint>
#include <string>
#include <array>
#include <iostream>
#include "proto.h"

namespace multicast_const {
    const std::string k_multicast_addr{"239.255.0.1"};
    const uint16_t k_port{30000};
}

class MulticastBus final : public std::enable_shared_from_this<MulticastBus> {
private:
    std::shared_ptr<asio::ip::udp::socket> socket_{nullptr};
    asio::ip::udp::endpoint sender_endpoint_;
    std::array<char, 1472> recv_buffer_{};
    void doReceive();
    void handlePacket( const std::vector<char> &src );
public:
    explicit MulticastBus( asio::io_context &ctx );
    ~MulticastBus();
    void startListen();
    void stopListen();
    void doSend( const proto_project::Packet &packet );
};