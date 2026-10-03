#pragma once

#include "kv_store.hpp"
#include "thread_pool.hpp"

#include <string>

class NovaKVServer {
public:

    explicit NovaKVServer(
        int port
    );


    NovaKVServer(
        int port,
        const std::string& persistence_file
    );


    void start();


private:

    int port_;

    KeyValueStore store_;

    ThreadPool pool_;


    void handle_client(
        int client_socket
    );


    std::string process_command(
        const std::string& command
    );
};