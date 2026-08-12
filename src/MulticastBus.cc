#include "MulticastBus.h"

MulticastBus::MulticastBus( asio::io_context &ctx )
/**
 * 
 */
{
    socket_ = std::make_shared<asio::ip::udp::socket>(ctx, asio::ip::udp::endpoint(asio::ip::udp::v4(), multicast_const::k_port));
    socket_->set_option(asio::ip::udp::socket::reuse_address(true));
    socket_->set_option(asio::ip::multicast::join_group(asio::ip::make_address(multicast_const::k_multicast_addr)));
    sender_endpoint_ = std::make_shared<asio::ip::udp::endpoint>();
    this->doReceive();
}

MulticastBus::~MulticastBus()
/**
 * 
 */
{
    asio::error_code ec;
    socket_->close(ec);
}

void MulticastBus::doReceive()
/**
 * 
 */
{
    auto self = shared_from_this();
    socket_->async_receive_from(asio::buffer(recv_buffer_), *sender_endpoint_.get(), [this, self](asio::error_code ec, std::size_t bytes_transferred) {
        if (!ec) {
            doReceive();
        } else {
            std::cerr << "error rcv -> " << ec.message() << std::endl;
        }
    });
}