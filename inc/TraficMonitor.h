#pragma once

#include <memory>
#include <vector>
#include <string>
#include <iostream>

class ITraficMonitor {
protected:
    virtual std::vector<std::pair<std::string, std::string>> getActiveInterfaces() = 0;
public:
    virtual ~ITraficMonitor() = default;
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

class MacosTraficMonitor final : public std::enable_shared_from_this<MacosTraficMonitor>, public ITraficMonitor {
private:
    std::vector<std::pair<std::string, std::string>> getActiveInterfaces() override;
    int bpf_descriptor_ = -1;
    void openBPF();
    void setInterface( const std::string &interface );
public:
    explicit MacosTraficMonitor();
    ~MacosTraficMonitor() override = default;
};