#pragma once

#include <memory>
#include <thread>
#include <functional>
#include <atomic>
#include <iostream>
#include <string>

class IKeyScaner {
protected:
    std::thread scanning_thread_;
    std::atomic<bool> is_running_{false};
    std::function<void()> signal_;
public:
    virtual ~IKeyScaner() = default;
    virtual void startScan() = 0;
    virtual void stopScan() = 0;
    virtual std::string get_text() = 0;
    void setCallBack( std::function<void()> signal );
};

#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>

class MacKeyScaner final : public std::enable_shared_from_this<MacKeyScaner>, public IKeyScaner {
private:
    CFMachPortRef event_tap_{nullptr};
    CFRunLoopSourceRef run_loop_source_{nullptr};
    CFRunLoopRef run_loop_{nullptr};
    static CGEventRef systemScaning( CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon );
public: 
    ~MacKeyScaner() override = default;
    void startScan() override;
    void stopScan() override;
    std::string get_text() override;
};