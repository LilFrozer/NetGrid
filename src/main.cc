#include "Window.h"

int main( int argc, char* argv[] ) 
{
    std::shared_ptr<Window> app = std::make_shared<Window>(700, 400, "000");
    app->show(argc, argv);
    return Fl::run();
}