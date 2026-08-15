#include "KeyScaner.h"

void IKeyScaner::setCallBack( std::function<void()> signal )
/**
 * 
 */
{
    signal_ = std::move(signal);
}

std::string MacKeyScaner::get_text()
/**
 * 
 */
{
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("pbpaste", "r"), pclose);
    if( !pipe ) {
        throw std::runtime_error("error pbpaste");
    }
    char buffer[128]{};
    std::string text{""};
    while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
        text += buffer;
    }
    return text;
}

void MacKeyScaner::startScan()
/**
 * 
 */
{
    is_running_.store(true);

    scanning_thread_ = std::thread([this]() {
        event_tap_ = CGEventTapCreate(
            kCGSessionEventTap,
            kCGHeadInsertEventTap,
            kCGEventTapOptionDefault,
            CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp),
            &MacKeyScaner::systemScaning,
            this
        );

         if (!event_tap_) {
            std::cerr << "error -> failed to create event tap" << std::endl;
            is_running_.store(false);
            return;
        }

        run_loop_source_ = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, event_tap_, 0);

        run_loop_ = CFRunLoopGetCurrent();
        CFRunLoopAddSource(run_loop_, run_loop_source_, kCFRunLoopCommonModes);

        while (is_running_.load()) {
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, true);
        }

        // -> exit
        if (run_loop_source_) {
            CFRunLoopRemoveSource(run_loop_, run_loop_source_, kCFRunLoopCommonModes);
            CFRelease(run_loop_source_);
            run_loop_source_ = nullptr;
        }
        if (event_tap_) {
            CFRelease(event_tap_);
            event_tap_ = nullptr;
        }
        run_loop_ = nullptr;
    });
}

void MacKeyScaner::stopScan()
/**
 * 
 */
{
    is_running_.store(false);
    if (run_loop_) {
        CFRunLoopWakeUp(run_loop_);
    }
    if (scanning_thread_.joinable()) {
        scanning_thread_.join();
    }
}

CGEventRef MacKeyScaner::systemScaning( CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon )
/**
 * 
 */
{
    auto ptr = static_cast<MacKeyScaner*>(refcon);
    if (!ptr || !ptr->is_running_.load()) {
        return event;
    }

    /*
        -> Игнорируем все, что не связано с клавиатурой!
    */
    if (type != kCGEventKeyDown && type != kCGEventKeyUp) {
        return event;
    }

    /*
        -> Что нажали, код клавиши + модификатор
    */
    CGKeyCode key_code = static_cast<CGKeyCode>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
    CGEventFlags flags = CGEventGetFlags(event);

    bool is_cmd_pressed = (flags & kCGEventFlagMaskCommand) != 0;
    bool is_keyC_pressed = (key_code == 8);
    if (is_keyC_pressed && is_cmd_pressed && type == kCGEventKeyDown) {
        ptr->signal_();
    }

    return event;
}