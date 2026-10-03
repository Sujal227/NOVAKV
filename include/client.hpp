#pragma once

#include <cstdint>
#include <string>

class NovaKVClient {
public:

    NovaKVClient(
        const std::string& host,
        int port
    );


    ~NovaKVClient();


    bool connect_to_server();


    void run();


    void disconnect();


private:

    std::string host_;

    int port_;

    std::uintptr_t socket_;

    bool connected_;

    bool winsock_initialized_;


    bool reconnect_with_retry();

    bool send_all(
        const std::string& data
    );


    bool receive_line(
        std::string& response
    );


    bool send_command(
        const std::string& command,
        std::string& response
    );
};