#pragma once 

/**
 *  -> logic.h
 */
#include "LocalNetworkScaner.h"
#include "MulticastBus.h"
#include <chrono>

/**
 *  -> gui .h
 */
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Table.H>
#include <FL/Fl_Button.H>
#include <FL/fl_draw.H>

class DeviceTable : public Fl_Table, public std::enable_shared_from_this<DeviceTable> {
private:
    std::vector<DiscoveredDevice> data{};
    int colWidths[4] = {40, 120, 120, 150};
protected:
    void draw_cell( TableContext context, int row, int col,int x, int y, int w, int h ) override;
public:
    explicit DeviceTable( const u32 &x, const u32 &y, const u32 &width, const u32 &height, const std::string &text );
    void addRow( const DiscoveredDevice &device );
    void clearTable();
    size_t getRowCount() const;
};

/**
 * -> for local scanning
 */
struct LocalFinderDevicesTypes : public std::enable_shared_from_this<LocalFinderDevicesTypes> {
    std::shared_ptr<asio::steady_timer> timer_{nullptr};
    std::shared_ptr<IScaner> network_scaner_{nullptr};
    explicit LocalFinderDevicesTypes( asio::io_context &ctx, const std::string &path_db ) : 
        timer_(std::make_shared<asio::steady_timer>(ctx, std::chrono::seconds(1)))
        , network_scaner_(std::make_shared<LocalNetworkScaner>("/Users/alekseypodoplelov/Documents/hyita01/etc/macvendor.db")) {}
};

class Window : public Fl_Window, public std::enable_shared_from_this<Window> {
private:
    std::unique_ptr<DeviceTable> wdgt_table_{nullptr};
    std::unique_ptr<Fl_Button> wdgt_btn1_{nullptr};
    std::unique_ptr<Fl_Button> wdgt_btn2_{nullptr};
    std::unique_ptr<Fl_Button> wdgt_btn3_{nullptr};
    static void onBtn1( Fl_Widget* widget, void* data );
    static void onBtn2( Fl_Widget* widget, void* data );
    static void onBtn3( Fl_Widget* widget, void* data );
private:
    std::shared_ptr<LocalFinderDevicesTypes> local_finder_{nullptr};
    std::shared_ptr<MulticastBus> multicast_bus_{nullptr};
public:
    explicit Window( const u32 &W, const u32 &H, const std::string &T, asio::io_context &ctx );
    void startScan( pingM t );
};