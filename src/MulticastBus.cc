#include "MulticastBus.h"

MulticastBus::MulticastBus( asio::io_context &ctx )
/**
 * 
 */
{
    socket_ = std::make_shared<asio::ip::udp::socket>(ctx, asio::ip::udp::endpoint(asio::ip::udp::v4(), multicast_const::k_port));
    socket_->set_option(asio::ip::udp::socket::reuse_address(true));
    socket_->set_option(asio::ip::multicast::join_group(asio::ip::make_address(multicast_const::k_multicast_addr)));
}

MulticastBus::~MulticastBus()
/**
 * 
 */
{
    stopListen();
}

void MulticastBus::startListen()
/**
 * 
 */
{
    doReceive();
}

void MulticastBus::stopListen()
/**
 * 
 */
{
    asio::error_code ec;
    socket_->close(ec);
}

void MulticastBus::handlePacket( const std::vector<char> &src )
/**
 * 
 */
{
    try {
        proto_project::Packet packet = proto_project::Packet::doDeserialise(src);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void MulticastBus::doReceive()
/**
 * 
 */
{
    auto self = shared_from_this();
    socket_->async_receive_from(asio::buffer(recv_buffer_), sender_endpoint_, [this, self](asio::error_code ec, std::size_t bytes_transferred) {
        if (!ec) {
            std::vector<char> data(recv_buffer_.data(), recv_buffer_.data() + bytes_transferred);
            handlePacket(data);
            self->doReceive();
        } else {
            std::cerr << "error multicast rcv -> " << ec.message() << std::endl;
        }
    });
}

void MulticastBus::doSend( const proto_project::Packet &packet ) 
/**
 * 
 */
{
    std::vector<char> src = proto_project::Packet::doSerialize(packet);
    asio::ip::udp::endpoint multicast_endpoint(asio::ip::make_address(multicast_const::k_multicast_addr), multicast_const::k_port);
    socket_->async_send_to(asio::buffer(src), multicast_endpoint, [](asio::error_code ec, std::size_t) {
        if (!ec) {
            std::cerr << "error multicast send -> " << ec.message() << std::endl;
        }
    });
}