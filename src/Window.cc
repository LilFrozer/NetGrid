#include "Window.h"

void DeviceTable::draw_cell( TableContext context, int row, int col,int x, int y, int w, int h )
/**
 * 
 */
{
    if (context == CONTEXT_CELL) {
        fl_color(FL_WHITE);
        fl_rectf(x, y, w, h, FL_WHITE);
        fl_color(FL_BLACK);
        fl_font(FL_HELVETICA, 14);
        if (row < (int)data.size()) {
            const auto& item = data[row];
            std::string text;
            switch (col) {
                case 0: text = item.addr; break;
                case 1: text = item.hostname; break;
                case 2: text = item.mac; break;
                case 3: text = item.vendor; break;
            }
            fl_draw(text.c_str(), x+5, y, w-10, h, FL_ALIGN_LEFT);
        }
        fl_color(FL_GRAY);
        fl_rect(x, y, w, h);
    }
}

DeviceTable::DeviceTable( const u32 &x, const u32 &y, const u32 &width, const u32 &height, const std::string &text ) :
    Fl_Table(x, y, width, height, text.c_str())
/**
 * 
 */
{
    cols(4);
    col_header(1);
    col_header_height(25);
    for (size_t c{}; c < 4; ++c) {
        col_width(c, colWidths[c]);
    }
    rows(0);
    type(0);
    row_height_all(25);
    end();
}

void DeviceTable::addRow( const DiscoveredDevice &device )
/**
 * 
 */
{
    data.push_back(device);
    rows(data.size());
    redraw();
}

void DeviceTable::clearTable()
/**
 * 
 */
{
    data.clear();
    rows(0);
    redraw();
}

size_t DeviceTable::getRowCount() const
/**
 * 
 */
{
    return data.size();
}

Window::Window( const u32 &W, const u32 &H, const std::string &T, asio::io_context &ctx ) 
    : Fl_Window(W, H, T.c_str())
    , local_finder_(std::make_shared<LocalFinderDevicesTypes>(ctx, "/Users/alekseypodoplelov/Documents/hyita01/etc/macvendor.db"))
    , multicast_bus_(std::make_shared<MulticastBus>(ctx))
/**
 * 
 */
{   
    int btnY = 10;          // отступ сверху
    int btnH = 30;          // высота кнопок
    int spacing = 10;       // промежуток между кнопками
    int x = 10;             // начальная позиция по X
    int btnW = 100;         // ширина каждой кнопки (можно одинаковую)

    wdgt_btn1_ = std::make_unique<Fl_Button>(x, btnY, btnW, btnH, "Кнопка 1");
    x += btnW + spacing;
    wdgt_btn2_ = std::make_unique<Fl_Button>(x, btnY, btnW, btnH, "Кнопка 2");
    x += btnW + spacing;
    wdgt_btn3_ = std::make_unique<Fl_Button>(x, btnY, btnW, btnH, "Кнопка 3");

    wdgt_btn1_->callback(onBtn1, this);
    wdgt_btn2_->callback(onBtn2, this);
    wdgt_btn3_->callback(onBtn3, this);

    int tableY = btnY + btnH + 10; // отступ от кнопок
    int tableH = H - tableY - 10;  // оставшаяся высота
    wdgt_table_ = std::make_unique<DeviceTable>(10, tableY, W - 20, tableH, "000");
    resizable(wdgt_table_.get());

    end();
}

void Window::onBtn1( Fl_Widget* widget, void* data )
/**
 * 
 */
{   
    Window *object = static_cast<Window*>(data);
    object->startScan(pingM::tcp_80);
}

void Window::onBtn2( Fl_Widget* widget, void* data )
/**
 * 
 */
{
    std::cout << std::string("Кнопка 2 нажата") << std::endl;
}

void Window::onBtn3( Fl_Widget* widget, void* data ) 
/**
 * 
 */
{
    std::cout << std::string("Кнопка 3 нажата") << std::endl;
}

void Window::startScan( pingM t )
/**
 * 
 */
{
    
    std::vector<DiscoveredDevice> res{network_scaner_->scanSubnet(t)};
    for (auto &i : res) {
        std::cout << "find! -> " << i.addr << "/" << i.hostname << "/" << i.mac << "/" << i.vendor << std::endl;
    }
}