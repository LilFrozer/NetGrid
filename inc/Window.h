#pragma once 

/**
 *  -> logic.h
 */
#include "LocalNetworkScaner.h"
#include "MulticastBus.h"
#include <chrono>
#include <mutex>

class DeviceTable {
public:
    void addRow(const DiscoveredDevice& device);
    void clearTable();
    size_t getRowCount() const;
    void drawTable();
private:
    std::vector<DiscoveredDevice> data_;
    mutable std::mutex mutex_;
};

/**
 * -> for local scanning
 */
struct LocalFinderDevicesTypes : public std::enable_shared_from_this<LocalFinderDevicesTypes> {
    bool timer_active = false;
    std::unique_ptr<asio::steady_timer> timer{nullptr};
    std::unique_ptr<IScaner> network_scaner{nullptr};
    explicit LocalFinderDevicesTypes( asio::io_context &ctx, const std::string &path_db ) : 
        timer(std::make_unique<asio::steady_timer>(ctx))
        , network_scaner(std::make_unique<LocalNetworkScaner>("/Users/alekseypodoplelov/Documents/hyita01/etc/macvendor.db")) {
            std::cout << "LocalFinderDevicesTypes!" << std::endl;
            // LocalNetworkScaner *scaner = static_cast<LocalNetworkScaner*>(network_scaner.get());
            // std::vector<DiscoveredDevice> list_devices{scaner->scanAllSubnet()};
            // for (auto &i : list_devices) {
            //     std::cout << "find! -> " << i.addr << "/" << i.hostname << "/" << i.mac << "/" << i.vendor << std::endl;
            // }
        }
};

class Window : public std::enable_shared_from_this<Window> {
private:
    std::unique_ptr<DeviceTable> table_{nullptr};
private:
    std::unique_ptr<LocalFinderDevicesTypes> local_finder_{nullptr};
    std::unique_ptr<MulticastBus> multicast_bus_{nullptr};
public:
    explicit Window( asio::io_context &ctx );
    void draw();
    void startScan();
};