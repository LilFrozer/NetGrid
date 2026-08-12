#include "Window.h"

int main( int argc, char* argv[] ) 
{
    try {
        asio::io_context io_context;
        auto work_guard = asio::make_work_guard(io_context);
        std::thread asio_thread([&io_context]() { io_context.run(); });

        std::shared_ptr<Window> app = std::make_shared<Window>(700, 400, "000", io_context);
        app->show(argc, argv);

        int ret = Fl::run();

        io_context.stop();
        asio_thread.join();
        return ret;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}