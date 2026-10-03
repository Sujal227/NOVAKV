#include "client.hpp"

#include <iostream>
#include <string>


int main(
    int argc,
    char* argv[]
) {

    std::string host =
        "127.0.0.1";


    int port =
        6379;


    if (argc >= 2) {

        host =
            argv[1];
    }


    if (argc >= 3) {

        try {

            port =
                std::stoi(
                    argv[2]
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


    NovaKVClient client(
        host,
        port
    );


    if (
        !client.connect_to_server()
    ) {

        return 1;
    }


    client.run();


    return 0;
}