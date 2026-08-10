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

void LocalNetworkScaner::scanSubnet( pingM t = pingM::tcp_80 ) 
/**
 * -> Проверяем локальные подсети
 */
{
    std::vector<LocalSubnet> local_subnets{this->getLocalSubnets()};

    auto func = [this]( pingM t, LocalSubnet &subnet ) -> void {
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
                        DiscoveredDevice dev{};
                        resolveDevice(dev, addr);
                        std::cout << "find! -> " << dev.addr << " " << dev.hostname << std::endl;
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
}