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


class Window : public Fl_Window, public std::enable_shared_from_this<Window> {
private:
    std::unique_ptr<Fl_Table> wdgt_table_{nullptr};
    std::unique_ptr<Fl_Button> wdgt_btn1_{nullptr};
    std::unique_ptr<Fl_Button> wdgt_btn2_{nullptr};
    std::unique_ptr<Fl_Button> wdgt_btn3_{nullptr};
    static void onBtn1( Fl_Widget* widget, void* data );
    static void onBtn2( Fl_Widget* widget, void* data );
    static void onBtn3( Fl_Widget* widget, void* data );
public:
    explicit Window( const u32 &W, const u32 &H, const std::string &T );
    ~Window() = default;
};