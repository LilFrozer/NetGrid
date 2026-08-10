#include "LocalNetworkScaner.h"

int main(int argc, char* argv[]) {
    std::shared_ptr<IScaner> ptr{std::make_shared<LocalNetworkScaner>()};
    ptr->scanSubnet(pingM::tcp_80);
    return 0;
}