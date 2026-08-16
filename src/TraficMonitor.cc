#include "TraficMonitor.h"

MacosTraficMonitor::MacosTraficMonitor( const std::string& iface ) 
/**
 * 
 */
{
    // 1. Получаем список интерфейсов
    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1) {
        throw std::runtime_error("can't get interfaces");
    }
    for (auto i = ifaddr; i != nullptr; i = i->ifa_next) {
        if (!i->ifa_addr) continue;
        if (i->ifa_addr->sa_family != AF_INET) continue;
        if (!(i->ifa_flags & IFF_UP)) continue;
        if (i->ifa_flags & IFF_LOOPBACK) continue;

        char ipStr[INET_ADDRSTRLEN];
        auto sa = reinterpret_cast<struct sockaddr_in*>(i->ifa_addr);
        inet_ntop(AF_INET, &sa->sin_addr, ipStr, sizeof(ipStr));
        interfaces_.push_back({std::string(i->ifa_name), std::string(ipStr)});
    }
    freeifaddrs(ifaddr);

    if (interfaces_.empty()) {
        throw std::runtime_error("No active non-loopback interfaces found");
    }

    // 2. Выбираем интерфейс
    if (!iface.empty()) {
        // Проверяем, есть ли такой интерфейс
        auto it = std::find_if(interfaces_.begin(), interfaces_.end(),
                               [&iface](const auto& p) { return p.first == iface; });
        if (it == interfaces_.end()) {
            throw std::runtime_error("Interface " + iface + " not found or not active");
        }
        selected_interface_ = iface;
        local_ip_ = it->second;
    } else {
        // Берём первый
        selected_interface_ = interfaces_[0].first;
        local_ip_ = interfaces_[0].second;
    }

    std::cout << "Selected interface: " << selected_interface_
              << " (" << local_ip_ << ")" << std::endl;

    // 3. Открываем BPF
    openBPF();

    configureBPF(selected_interface_);
}

MacosTraficMonitor::~MacosTraficMonitor() 
/**
 * 
 */
{
    stopCapture();
    if (bpf_descriptor_ != -1) {
        close(bpf_descriptor_);
    }
}

void MacosTraficMonitor::openBPF() 
/**
 * 
 */
{
    for (int i = 0; i < 256; ++i) {
        std::string dev = "/dev/bpf" + std::to_string(i);
        bpf_descriptor_ = open(dev.c_str(), O_RDWR);
        if (bpf_descriptor_ != -1) {
            std::cout << "bpf_descriptor open: " << bpf_descriptor_ << std::endl;
            // Устанавливаем неблокирующий режим
            int flags = fcntl(bpf_descriptor_, F_GETFL, 0);
            if (flags == -1 || fcntl(bpf_descriptor_, F_SETFL, flags | O_NONBLOCK) == -1) {
                close(bpf_descriptor_);
                throw std::runtime_error("Failed to set non-blocking mode");
            }
            return;
        }
        // else: продолжаем пробовать
    }
    throw std::runtime_error("Cannot open /dev/bpf* (are you root?)");
}

void MacosTraficMonitor::configureBPF( const std::string& interface ) {
    // 1. Устанавливаем снаплен (максимальный размер пакета)
    int snap = 65536;
    if (ioctl(bpf_descriptor_, BIOCSBLEN, &snap) == -1) {
        std::cerr << "Warning: BIOCSBLEN failed: " << strerror(errno) << std::endl;
        if (ioctl(bpf_descriptor_, BIOCGBLEN, &snap) == 0) {
            std::cerr << "Current snap length: " << snap << std::endl;
        }
    } else {
        std::cerr << "Snap length set to " << snap << std::endl;
    }

    // 2. Привязываем интерфейс
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
    if (ioctl(bpf_descriptor_, BIOCSETIF, &ifr) == -1) {
        close(bpf_descriptor_);
        throw std::runtime_error("BIOCSETIF failed: " + std::string(strerror(errno)));
    }
    std::cout << "BIOCSETIF ok" << std::endl;

    // 3. Promiscuous mode (необязательно)
    int mode = 1;
    if (ioctl(bpf_descriptor_, BIOCPROMISC, &mode) == -1) {
        std::cerr << "Warning: BIOCPROMISC failed" << std::endl;
    }

    // 4. Отключаем сжатие заголовков
    int hdr_complete = 0;
    if (ioctl(bpf_descriptor_, BIOCSHDRCMPLT, &hdr_complete) == -1) {
        std::cerr << "Warning: BIOCSHDRCMPLT failed" << std::endl;
    }
}

void MacosTraficMonitor::startCapture() 
/**
 * 
 */
{
    is_running_.store(true);

    thread_ = std::thread([this]() {
        std::cout << "started capture..." << std::endl;
        fd_set readfds;
        struct timeval tv;
        char buffer[65536];

        while (is_running_.load()) {
            FD_ZERO(&readfds);
            FD_SET(bpf_descriptor_, &readfds);
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            int ret = select(bpf_descriptor_ + 1, &readfds, nullptr, nullptr, &tv);
            if (ret < 0) {
                if (errno == EINTR) continue;
                perror("select");
                break;
            }
            if (ret == 0) continue;

            ssize_t bytes_read = read(bpf_descriptor_, buffer, sizeof(buffer));
            if (bytes_read < 0) {
                if (errno == EINTR) continue;
                perror("read");
                break;
            }

            // Разбор пакетов (как в вашем коде)
            char* ptr = buffer;
            while (ptr < buffer + bytes_read) {
                struct bpf_hdr* bpf_hdr = reinterpret_cast<struct bpf_hdr*>(ptr);
                u_char* packet_data = reinterpret_cast<u_char*>(ptr + bpf_hdr->bh_hdrlen);
                uint32_t captured_len = bpf_hdr->bh_caplen;

                if (captured_len < sizeof(struct ether_header)) {
                    ptr += BPF_WORDALIGN(bpf_hdr->bh_hdrlen + bpf_hdr->bh_caplen);
                    continue;
                }
                struct ether_header* eth = reinterpret_cast<struct ether_header*>(packet_data);
                if (ntohs(eth->ether_type) != ETHERTYPE_IP) {
                    ptr += BPF_WORDALIGN(bpf_hdr->bh_hdrlen + bpf_hdr->bh_caplen);
                    continue;
                }

                uint32_t ip_offset = sizeof(struct ether_header);
                if (captured_len < ip_offset + sizeof(struct ip)) {
                    ptr += BPF_WORDALIGN(bpf_hdr->bh_hdrlen + bpf_hdr->bh_caplen);
                    continue;
                }
                struct ip* ip_hdr = reinterpret_cast<struct ip*>(packet_data + ip_offset);

                DirectionPacketInfo dir = getDirection(ip_hdr);
                if (dir == DirectionPacketInfo::Unknown) {
                    ptr += BPF_WORDALIGN(bpf_hdr->bh_hdrlen + bpf_hdr->bh_caplen);
                    continue;
                }

                uint32_t pkt_len = ntohs(ip_hdr->ip_len);
                if (dir == DirectionPacketInfo::InComing) {
                    // stats_.cnt_incoming_packets++;
                    // stats_.cnt_incoming_bytes += pkt_len;
                } else {
                    // stats_.cnt_outcoming_packets++;
                    // stats_.cnt_outcoming_bytes += pkt_len;
                }

                printPacketInfo(ip_hdr, dir);

                ptr += BPF_WORDALIGN(bpf_hdr->bh_hdrlen + bpf_hdr->bh_caplen);
            }
        }
    });
}

void MacosTraficMonitor::stopCapture() 
/**
 * 
 */
{
    is_running_.store(false);
    if (thread_.joinable()) {
        thread_.join();
    }
}

DirectionPacketInfo MacosTraficMonitor::getDirection( struct ip* ip_hdr ) 
/**
 * 
 */
{
    char src[INET_ADDRSTRLEN], dst[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_hdr->ip_src, src, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip_hdr->ip_dst, dst, INET_ADDRSTRLEN);
    if (local_ip_ == src) return DirectionPacketInfo::OutComing;
    if (local_ip_ == dst) return DirectionPacketInfo::InComing;
    return DirectionPacketInfo::Unknown;
}

void MacosTraficMonitor::printPacketInfo( struct ip* ip_hdr, DirectionPacketInfo dir ) 
/**
 * 
 */
{
    char src[INET_ADDRSTRLEN], dst[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_hdr->ip_src, src, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip_hdr->ip_dst, dst, INET_ADDRSTRLEN);

    std::string dirStr = (dir == DirectionPacketInfo::InComing) ? "IN " : "OUT";
    std::string proto;
    switch (ip_hdr->ip_p) {
        case IPPROTO_TCP: proto = "TCP"; break;
        case IPPROTO_UDP: proto = "UDP"; break;
        case IPPROTO_ICMP: proto = "ICMP"; break;
        default: proto = "Other(" + std::to_string(ip_hdr->ip_p) + ")";
    }

    uint16_t src_port = 0, dst_port = 0;
    if (ip_hdr->ip_p == IPPROTO_TCP) {
        struct tcphdr* tcp = (struct tcphdr*)((u_char*)ip_hdr + (ip_hdr->ip_hl << 2));
        src_port = ntohs(tcp->th_sport);
        dst_port = ntohs(tcp->th_dport);
    } else if (ip_hdr->ip_p == IPPROTO_UDP) {
        struct udphdr* udp = (struct udphdr*)((u_char*)ip_hdr + (ip_hdr->ip_hl << 2));
        src_port = ntohs(udp->uh_sport);
        dst_port = ntohs(udp->uh_dport);
    }

    std::cout << "[" << dirStr << "] " << proto << " "
              << src << (src_port ? ":" + std::to_string(src_port) : "") << " -> "
              << dst << (dst_port ? ":" + std::to_string(dst_port) : "")
              << "  len=" << ntohs(ip_hdr->ip_len)
              << std::endl;
}