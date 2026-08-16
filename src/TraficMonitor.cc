#include "TraficMonitor.h"

std::vector<std::pair<std::string, std::string>> MacosTraficMonitor::getActiveInterfaces()
/**
 * 
 */
{
    std::vector<std::pair<std::string, std::string>> interfaces;

    struct ifaddrs *ifaddr{nullptr};
    if (getifaddrs(&ifaddr) == -1) {
        throw std::runtime_error("can`t getActiveInterfaces");
    }

    for (auto i=ifaddr;i!=nullptr;i=i->ifa_next) {
        if (!i->ifa_addr) continue;
        if (i->ifa_addr->sa_family != AF_INET) continue;
        if (!(i->ifa_flags & IFF_UP)) continue;
        if (i->ifa_flags & IFF_LOOPBACK) continue;

        char ipStr[INET_ADDRSTRLEN];
        auto sa = reinterpret_cast<struct sockaddr_in*>(i->ifa_addr);
        inet_ntop(AF_INET, &sa->sin_addr, ipStr, sizeof(ipStr));

        interfaces.push_back({std::string(i->ifa_name), std::string(ipStr)});
    }

    freeifaddrs(ifaddr);
    return interfaces;
}

MacosTraficMonitor::MacosTraficMonitor()
/**
 * 
 */
{
    auto interfaces = getActiveInterfaces();
    for (auto &i : interfaces) {
        std::cout << i.first << ":" << i.second << std::endl;
    }
    openBPF();
}

void MacosTraficMonitor::openBPF()
/**
 * 
 */
{
    for (size_t i{};i<256;++i) {
        std::string dev = "/dev/bpf" + std::to_string(i);
        bpf_descriptor_ = open(dev.c_str(), O_RDWR);
        if (bpf_descriptor_ != -1) {
            std::cout << "bpf_descriptor open: " << bpf_descriptor_ << std::endl;
            int flags = fcntl(bpf_descriptor_, F_GETFL, 0);
            if (flags == -1 || fcntl(bpf_descriptor_, F_SETFL, flags | O_NONBLOCK) == -1) {
                throw std::runtime_error("Failed to set non-blocking mode");
            }
            return;
        } else {
            std::cerr << "failed to open " << dev << ": " << std::strerror(errno) << std::endl;
        }
    }
    throw std::runtime_error("");
}

void MacosTraficMonitor::setInterface( const std::string &interface )
/**
 * 
 */
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
    if (ioctl(bpf_descriptor_, BIOCSETIF, &ifr) == -1) {
        close(bpf_descriptor_);
        throw std::runtime_error("BIOCSETIF failed: " + std::string(strerror(errno)));
    }

    // -> Разрешаем захват всех пакетов (promiscuous mode)
    int mode = 1;
    if (ioctl(bpf_descriptor_, BIOCPROMISC, &mode) == -1) {
        std::cerr << "Warning: BIOCPROMISC failed" << std::endl;
    }
}