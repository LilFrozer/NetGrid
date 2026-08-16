#pragma once 

#include "DeviceFinderScaner.h"
#include "KeyScaner.h"
#include "MulticastBus.h"
#include "TraficMonitor.h"
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
 * -> Сканирование всех устройств в локальной сети
 */
struct LocalFinderDevicesTypes : public std::enable_shared_from_this<LocalFinderDevicesTypes> {
    bool is_active = false;
    std::unique_ptr<asio::steady_timer> timer{nullptr};
    std::unique_ptr<IDeviceScaner> device_scaner{nullptr};
    explicit LocalFinderDevicesTypes( asio::io_context &ctx, const std::string &path_db ) : 
        timer(std::make_unique<asio::steady_timer>(ctx))
        , device_scaner(std::make_unique<DeviceFinderScaner>("/Users/alekseypodoplelov/Documents/hyita01/etc/macvendor.db")) {}
};

/**
 * -> Общий буфер обмена в локальной сети
 */
struct LocalSharedClipboard : public std::enable_shared_from_this<LocalSharedClipboard> {
    bool is_active = false;
    std::shared_ptr<MulticastBus> multicast_bus{nullptr};
    std::shared_ptr<IKeyScaner> key_scaner{nullptr};
    explicit LocalSharedClipboard( asio::io_context &ctx ) : 
        multicast_bus(std::make_shared<MulticastBus>(ctx))
        , key_scaner(std::make_shared<MacKeyScaner>()) {}
};

/**
 * -> Cканер входящего и выходящего трафика
 */
struct LocalTraficMonitor : public std::enable_shared_from_this<LocalTraficMonitor> {
    bool is_active = false;
    std::shared_ptr<ITraficMonitor> trafic_monitor{nullptr};
    explicit LocalTraficMonitor() : trafic_monitor(std::make_shared<MacosTraficMonitor>()) {}
};

class Window : public std::enable_shared_from_this<Window> {
private:
    std::unique_ptr<DeviceTable> table_{nullptr};
private:
    std::unique_ptr<LocalFinderDevicesTypes> local_scanning_service_{nullptr};
    std::shared_ptr<LocalSharedClipboard> local_clipboard_service_{nullptr};
    std::shared_ptr<LocalTraficMonitor> local_trafic_service_{nullptr};
    void onClipboardSlot();
public:
    explicit Window( asio::io_context &ctx );
    void draw();
    void doLocalScanningService();
    void doLocalClipboardService();
    void doTraficMonitorService();
};