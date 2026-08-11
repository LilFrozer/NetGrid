#pragma once

#include <memory>
#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>
#include <string>
#include <iostream>
#include <map>

using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

/*
    -> Определение подсети
*/
struct LocalSubnet {
    u32 addr{};
    u32 mask{};
};

enum class pingM : u32 {
    icmp = 0,
    tcp_80 = 1
};

struct DiscoveredDevice {
    std::string addr{};
    std::string mac{};
    std::string hostname{};
    std::string vendor{};
};

class IScaner {
protected:
    virtual std::vector<LocalSubnet> getLocalSubnets() = 0;
    virtual bool pingHost( pingM t = pingM::tcp_80, u32 addr = 0, u32 timeout_ms = 300 ) = 0;
    virtual std::map<u32, std::string> readArpTable() = 0;
public:
    virtual ~IScaner() = default;
    virtual void scanSubnet( pingM t = pingM::tcp_80 ) = 0;
};

class LocalNetworkScaner final : public IScaner, public std::enable_shared_from_this<LocalNetworkScaner> {
private:
    std::vector<LocalSubnet> getLocalSubnets() override;
    bool pingHost( pingM t, u32 addr, u32 timeout_ms ) override;
    std::map<u32, std::string> readArpTable() override;
public:
    ~LocalNetworkScaner() override = default;
    void scanSubnet( pingM t ) override;
};