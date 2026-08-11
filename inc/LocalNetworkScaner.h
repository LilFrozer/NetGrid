#pragma once

#include <memory>
#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>
#include <string>
#include <iostream>
#include <map>
#include <sqlite3.h>

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
    virtual std::map<std::string, std::string> readArpTable() = 0;
    sqlite3 *vendor_db_{nullptr};
    virtual std::string findVendor( const std::string &mac ) = 0;
public:
    virtual ~IScaner() = default;
    virtual std::vector<DiscoveredDevice> scanSubnet( pingM t = pingM::tcp_80 ) = 0;
};

class LocalNetworkScaner final : public IScaner, public std::enable_shared_from_this<LocalNetworkScaner> {
private:
    std::vector<LocalSubnet> getLocalSubnets() override;
    bool pingHost( pingM t, u32 addr, u32 timeout_ms ) override;
    std::map<std::string, std::string> readArpTable() override;
    std::string findVendor( const std::string &mac ) override;
public:
    LocalNetworkScaner( const std::string& vendor_db_path );
    ~LocalNetworkScaner() override;
    std::vector<DiscoveredDevice> scanSubnet( pingM t ) override;
};