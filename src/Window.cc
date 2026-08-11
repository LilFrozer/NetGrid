#include "Window.h"

Window::Window( const u32 &W, const u32 &H, const std::string &T ) : Fl_Window(W, H, T.c_str())
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

    // Привязываем колбэки (передаём this для доступа к таблице)
    wdgt_btn1_->callback(onBtn1);
    wdgt_btn2_->callback(onBtn2, this);
    wdgt_btn3_->callback(onBtn3, this);

    int tableY = btnY + btnH + 10; // отступ от кнопок
    int tableH = H - tableY - 10;  // оставшаяся высота
    wdgt_table_ = std::make_unique<Fl_Table>(10, tableY, W - 20, tableH);
    resizable(wdgt_table_.get());
    
    // wdgt_table_->addRow({1, "192.168.1.1", "AA:BB:CC:DD:EE:FF", "Intel"});
    // wdgt_table_->addRow({2, "192.168.1.2", "11:22:33:44:55:66", "Realtek"});

    end();
}

void Window::onBtn1( Fl_Widget* widget, void* data )
{   
    std::cout << std::string("Кнопка 1 нажата") << std::endl;
}

void Window::onBtn2( Fl_Widget* widget, void* data )
{
    std::cout << std::string("Кнопка 2 нажата") << std::endl;
}

void Window::onBtn3( Fl_Widget* widget, void* data ) 
{
    std::cout << std::string("Кнопка 3 нажата") << std::endl;
}