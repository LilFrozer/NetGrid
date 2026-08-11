#pragma once 

/**
 *  -> logic.h
 */
#include "LocalNetworkScaner.h"

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
    std::shared_ptr<IScaner> network_scaner_{nullptr};
public:
    explicit Window( const u32 &W, const u32 &H, const std::string &T );
    void startScan( pingM t );
};