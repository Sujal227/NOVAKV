#include "server.hpp"

#include <iostream>
#include <string>


int main(
    int argc,
    char* argv[]
) {

    int port = 6379;


    if (argc >= 2) {

        try {

            port =
                std::stoi(
                    argv[1]
                );

        }
        catch (...) {

            std::cerr
                << "Invalid port\n";

            return 1;
        }
    }


    if (
        port <= 0 ||
        port > 65535
    ) {

        std::cerr
            << "Port must be between 1 and 65535\n";

        return 1;
    }


    NovaKVServer server(
        port
    );


    server.start();


    return 0;
}