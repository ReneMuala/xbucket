#include "server.hpp"
#include <stdexcept>
#include <crow/logging.h>
int main(int argc, char** argv) {
    try {
        server::run();
    } catch (std::exception &e) {
        CROW_LOG_CRITICAL << argv[0] << " (xbucket) failed to serve:" << e.what();
    }
    return 0;
}
