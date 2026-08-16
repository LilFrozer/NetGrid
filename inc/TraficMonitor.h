#pragma once

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <atomic>

struct TraficStatistic {
    std::atomic<uint32_t> cnt_incoming_packets{0};
    std::atomic<uint32_t> cnt_incoming_bytes{0};
    std::atomic<uint32_t> cnt_outcoming_packets{0};
    std::atomic<uint32_t> cnt_outcoming_bytes{0};
};

enum class DirectionPacketInfo : uint8_t {
    InComing = 0,
    OutComing,
    Unknown
};

class ITraficMonitor {
protected:
    std::vector<std::pair<std::string, std::string>> interfaces_{};
    std::thread thread_;
    std::atomic<bool> is_running_{false};
public:
    virtual ~ITraficMonitor() = default;
    virtual void startCapture() = 0;
    virtual void stopCapture() = 0;
};

#include <sys/ioctl.h>
#include <sys/select.h>
#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

class MacosTraficMonitor final : public std::enable_shared_from_this<MacosTraficMonitor>, public ITraficMonitor {
private:
    int bpf_descriptor_ = -1;
    std::string selected_interface_{""};
    std::string local_ip_{""};
    void openBPF();
    void configureBPF( const std::string& interface );
    DirectionPacketInfo getDirection( struct ip* ip_hdr );
    void printPacketInfo( struct ip* ip_hdr, DirectionPacketInfo dir );
public:
    explicit MacosTraficMonitor( const std::string& iface = "" );
    ~MacosTraficMonitor() override;
    void startCapture() override;
    void stopCapture() override;
};