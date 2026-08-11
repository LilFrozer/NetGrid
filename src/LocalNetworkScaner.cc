#include "LocalNetworkScaner.h"
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <netdb.h>
#include <sys/sysctl.h>
#include <net/if_dl.h>
#include <net/route.h>

LocalNetworkScaner::LocalNetworkScaner( const std::string& vendor_db_path )
/**
 * 
 */
{
    if (sqlite3_open(vendor_db_path.c_str(), &vendor_db_) != SQLITE_OK) {
        throw std::runtime_error("Can't open database: " + std::string(sqlite3_errmsg(vendor_db_)));
    }
}

LocalNetworkScaner::~LocalNetworkScaner()
/**
 * 
 */
{
    sqlite3_close(vendor_db_);
}

std::string LocalNetworkScaner::findVendor( const std::string &mac )
/**
 * 
 */
{
    std::string cleaned_mac{};
    for (char c : mac) {
        if (c != ':' && c != '-') {
            cleaned_mac.push_back(static_cast<char>(std::toupper(c))); // приводим к верхнему регистру
        }
    }
    if (cleaned_mac.length() < 6) {
        return ""; // -> некорректный MAC
    }

    std::string oui = cleaned_mac.substr(0, 6); // -> берём первые 6 символов

    // -> Запрос с точным совпадением
    std::string sql = "SELECT vendor FROM macvendor WHERE oui = ?;";
    sqlite3_stmt* stmt = nullptr;
    std::string vendor;

    if (sqlite3_prepare_v2(vendor_db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, oui.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* text = sqlite3_column_text(stmt, 0);
            if (text) vendor = reinterpret_cast<const char*>(text);
        }
        sqlite3_finalize(stmt);
    }

    return vendor;
}

std::vector<LocalSubnet> LocalNetworkScaner::getLocalSubnets() 
/**
 * -> ищем все локальные подсети
 */
{
    std::vector<LocalSubnet> res{};

    struct ifaddrs* ifaddr{nullptr};
    if (getifaddrs(&ifaddr) != 0) {
        return res;
    }

    for (auto ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
        if (!(ifa->ifa_flags & IFF_UP)) continue;
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;

        auto sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
        auto maskSin = reinterpret_cast<sockaddr_in*>(ifa->ifa_netmask);

        uint32_t ip = ntohl(sin->sin_addr.s_addr);
        uint32_t mask = ntohl(maskSin->sin_addr.s_addr);

        res.emplace_back(LocalSubnet{ip, mask});
    }

    freeifaddrs(ifaddr);
    return res;
}

bool LocalNetworkScaner::pingHost( pingM t, u32 addr, u32 timeout_ms ) 
/*
 *  -> Перебираем все ip в подсети(192.168.1.1-192.168.1.254), кто ответил - жив
 */
{
    bool alive = false;

    switch (static_cast<pingM>(t)) {
    case pingM::tcp_80: {
        int sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            return false;
        }

        // -> неблокирующий режим, чтобы не ждать таймаут TCP по умолчанию
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        sockaddr_in _addr{};
        _addr.sin_family = AF_INET;
        _addr.sin_port = htons(80);
        _addr.sin_addr.s_addr = htonl(addr);

        ::connect(sock, (sockaddr*)&_addr, sizeof(_addr)); // -> вернёт EINPROGRESS сразу

        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);
        timeval tv{timeout_ms / 1000, static_cast<__darwin_suseconds_t>((timeout_ms % 1000) * 1000)};

        if (select(sock + 1, nullptr, &writeSet, nullptr, &tv) > 0) {
            int err = 0;
            socklen_t len = sizeof(err);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len);
            // -> ECONNREFUSED тоже значит "хост жив", просто порт закрыт
            alive = (err == 0 || err == ECONNREFUSED);
        }

        close(sock);
        break;
    }
    case pingM::icmp: {
        break;
    }
    default: {
        throw std::runtime_error("error mode pingHost");
        break;
    }
    }

    return alive;
}

std::vector<DiscoveredDevice> LocalNetworkScaner::scanSubnet( pingM t = pingM::tcp_80 ) 
/**
 * -> Проверяем локальные подсети
 */
{
    std::vector<DiscoveredDevice> devices{};
    std::vector<LocalSubnet> local_subnets{this->getLocalSubnets()};

    auto func = [this, &devices]( pingM t, LocalSubnet &subnet ) -> void {
        u32 host_bits{~subnet.mask};
        u32 net_addr{subnet.addr & subnet.mask};
        u32 broadcast_addr{net_addr | host_bits};

        std::vector<std::thread> workers;
        std::atomic<u32> next_ip{net_addr + 1};

        u32 thread_count{std::min(64u, std::thread::hardware_concurrency() * 8)};

        for (auto i = 0; i < thread_count; ++i) {
            workers.emplace_back([&]() {
                u32 addr{};
                while ((addr = next_ip.fetch_add(1)) < broadcast_addr) {
                    if (pingHost(t, addr, 300)) {
                        auto resolveDevice = []( DiscoveredDevice &dd, u32 addr ) -> void {
                            sockaddr_in sa{};
                            sa.sin_family = AF_INET;
                            sa.sin_addr.s_addr = htonl(addr);

                            // -> dns
                            char host[NI_MAXHOST] = {};
                            int ret = getnameinfo(reinterpret_cast<sockaddr*>(&sa), sizeof(sa),
                                                host, sizeof(host), nullptr, 0, NI_NAMEREQD);
                            if (ret == 0) {
                                dd.hostname = host;
                            } else {
                                dd.hostname = "null";
                            }

                            // -> addr
                            char _addr[INET_ADDRSTRLEN]{""};
                            inet_ntop(AF_INET, &sa.sin_addr, _addr, sizeof(_addr));
                            dd.addr = _addr;
                        };
                        devices.push_back({});
                        resolveDevice(devices.back(), addr);
                    }
                }
            });
        }

        for (auto& w : workers) {
            w.join();
        }
    };

    for (auto &i : local_subnets) {
        func(t, i);
    }

    std::map<std::string, std::string> arp_table{readArpTable()};
    for (auto &i : devices) {
        i.mac = arp_table[i.addr];
        i.vendor = findVendor(i.mac);
    }

    return devices;
}

std::map<std::string, std::string> LocalNetworkScaner::readArpTable() 
/**
 * -> Выявляем mac из arp таблицы
 */
{
    std::map<std::string, std::string> res{};

    int mib[6] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, RTF_LLINFO};
    size_t needed = 0;
    if (sysctl(mib, 6, nullptr, &needed, nullptr, 0) < 0) return res;

    std::vector<u8> buf(needed);
    if (sysctl(mib, 6, buf.data(), &needed, nullptr, 0) < 0) return res;

    u8* next = buf.data();
    u8* end = buf.data() + needed;

    while (next < end) {
        auto rtm = reinterpret_cast<rt_msghdr*>(next);
        auto sin = reinterpret_cast<sockaddr_in*>(rtm + 1);
        auto sdl = reinterpret_cast<sockaddr_dl*>(reinterpret_cast<u8*>(sin) + sin->sin_len);

        if (sdl->sdl_alen == 6) {
            u8* mac = reinterpret_cast<u8*>(LLADDR(sdl));
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

            sockaddr_in sa = *(reinterpret_cast<sockaddr_in*>(sin));
            char tmp[INET_ADDRSTRLEN]{""};
            inet_ntop(AF_INET, &sa.sin_addr, tmp, sizeof(tmp));
            res[std::string(tmp)] = macStr;
        }

        next += rtm->rtm_msglen;
    }

    return res;
}