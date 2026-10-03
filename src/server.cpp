#include "server.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")


// ============================================================
// Helpers
// ============================================================

namespace {

std::string to_upper(
    std::string value
) {

    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](
            unsigned char character
        ) {
            return static_cast<char>(
                std::toupper(character)
            );
        }
    );


    return value;
}


bool send_all(
    SOCKET socket,
    const std::string& data
) {

    std::size_t total_sent = 0;


    while (
        total_sent <
        data.size()
    ) {

        int sent = send(
            socket,
            data.data() + total_sent,
            static_cast<int>(
                data.size() - total_sent
            ),
            0
        );


        if (
            sent ==
            SOCKET_ERROR
        ) {
            return false;
        }


        if (sent == 0) {
            return false;
        }


        total_sent +=
            static_cast<std::size_t>(
                sent
            );
    }


    return true;
}

}


// ============================================================
// Constructors
// ============================================================

NovaKVServer::NovaKVServer(
    int port
)
    : port_(port),
      store_("novakv.data"),
      pool_(4) {
}


NovaKVServer::NovaKVServer(
    int port,
    const std::string& persistence_file
)
    : port_(port),
      store_(persistence_file),
      pool_(4) {
}


// ============================================================
// START
// ============================================================

void NovaKVServer::start() {

    WSADATA wsa_data{};


    int result =
        WSAStartup(
            MAKEWORD(2, 2),
            &wsa_data
        );


    if (result != 0) {

        std::cerr
            << "WSAStartup failed: "
            << result
            << '\n';

        return;
    }


    SOCKET server_socket =
        socket(
            AF_INET,
            SOCK_STREAM,
            IPPROTO_TCP
        );


    if (
        server_socket ==
        INVALID_SOCKET
    ) {

        std::cerr
            << "Failed to create socket\n";

        WSACleanup();

        return;
    }


    // Allow quick restart after shutdown.

    BOOL reuse = TRUE;

    setsockopt(
        server_socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<const char*>(
            &reuse
        ),
        sizeof(reuse)
    );


    sockaddr_in server_address{};


    server_address.sin_family =
        AF_INET;


    // Listen on every network interface.
    // This allows LAN clients to connect.

    server_address.sin_addr.s_addr =
        htonl(INADDR_ANY);


    server_address.sin_port =
        htons(
            static_cast<u_short>(
                port_
            )
        );


    result =
        bind(
            server_socket,
            reinterpret_cast<sockaddr*>(
                &server_address
            ),
            sizeof(server_address)
        );


    if (
        result ==
        SOCKET_ERROR
    ) {

        std::cerr
            << "Bind failed. Port may already be in use.\n";

        closesocket(
            server_socket
        );

        WSACleanup();

        return;
    }


    result =
        listen(
            server_socket,
            SOMAXCONN
        );


    if (
        result ==
        SOCKET_ERROR
    ) {

        std::cerr
            << "Listen failed\n";

        closesocket(
            server_socket
        );

        WSACleanup();

        return;
    }


    std::cout
        << "\n=====================================\n"
        << "           NovaKV Server\n"
        << "=====================================\n"
        << "Port: "
        << port_
        << '\n'
        << "Listening on all interfaces\n"
        << "=====================================\n\n";


    while (true) {

        SOCKET client_socket =
            accept(
                server_socket,
                nullptr,
                nullptr
            );


        if (
            client_socket ==
            INVALID_SOCKET
        ) {

            std::cerr
                << "Accept failed\n";

            continue;
        }


        std::cout
            << "Client connected\n";


        bool accepted =
            pool_.enqueue(
                [this, client_socket]() {

                    handle_client(
                        static_cast<int>(
                            client_socket
                        )
                    );
                }
            );


        if (!accepted) {

            closesocket(
                client_socket
            );
        }
    }


    closesocket(
        server_socket
    );


    WSACleanup();
}


// ============================================================
// CLIENT HANDLER
// ============================================================

void NovaKVServer::handle_client(
    int client_socket
) {

    SOCKET socket =
        static_cast<SOCKET>(
            client_socket
        );


    std::string buffer;

    char temp[4096];


    while (true) {

        int received =
            recv(
                socket,
                temp,
                sizeof(temp),
                0
            );


        if (received <= 0) {
            break;
        }


        buffer.append(
            temp,
            received
        );


        while (true) {

            std::size_t newline =
                buffer.find('\n');


            if (
                newline ==
                std::string::npos
            ) {
                break;
            }


            std::string command =
                buffer.substr(
                    0,
                    newline
                );


            buffer.erase(
                0,
                newline + 1
            );


            if (
                !command.empty() &&
                command.back() == '\r'
            ) {

                command.pop_back();
            }


            std::string response =
                process_command(
                    command
                );


            if (
                !send_all(
                    socket,
                    response
                )
            ) {

                break;
            }
        }
    }


    closesocket(
        socket
    );


    std::cout
        << "Client disconnected\n";
}


// ============================================================
// COMMAND PROCESSOR
// ============================================================

std::string NovaKVServer::process_command(
    const std::string& command
) {

    std::istringstream input(
        command
    );


    std::string operation;

    input >> operation;


    operation =
        to_upper(
            operation
        );


    // ========================================================
    // SET
    //
    // SET key value
    // SET key value ttl
    // ========================================================

    if (
        operation ==
        "SET"
    ) {

        std::string key;

        std::string value;

        std::string ttl_text;


        input >> key;
        input >> value;


        if (
            key.empty() ||
            value.empty()
        ) {

            return "ERROR invalid_set\n";
        }


        // Optional TTL.

        if (
            input >>
            ttl_text
        ) {

            try {

                int ttl =
                    std::stoi(
                        ttl_text
                    );


                if (ttl <= 0) {

                    return "ERROR invalid_ttl\n";
                }


                store_.set(
                    key,
                    value,
                    ttl
                );


                return "OK\n";

            }
            catch (...) {

                return "ERROR invalid_ttl\n";
            }
        }


        store_.set(
            key,
            value
        );


        return "OK\n";
    }


    // ========================================================
    // GET
    // ========================================================

    if (
        operation ==
        "GET"
    ) {

        std::string key;

        input >> key;


        if (key.empty()) {
            return "ERROR invalid_get\n";
        }


        auto value =
            store_.get(
                key
            );


        if (
            !value.has_value()
        ) {

            return "NOT_FOUND\n";
        }


        return "VALUE " +
               value.value() +
               "\n";
    }


    // ========================================================
    // EXISTS
    // ========================================================

    if (
        operation ==
        "EXISTS"
    ) {

        std::string key;

        input >> key;


        if (key.empty()) {

            return "ERROR invalid_exists\n";
        }


        if (
            store_.exists(
                key
            )
        ) {

            return "EXISTS 1\n";
        }


        return "EXISTS 0\n";
    }


    // ========================================================
    // REMOVE
    // ========================================================

    if (
        operation ==
        "REMOVE"
    ) {

        std::string key;

        input >> key;


        if (key.empty()) {

            return "ERROR invalid_remove\n";
        }


        bool removed =
            store_.remove(
                key
            );


        if (removed) {

            return "REMOVED 1\n";
        }


        return "REMOVED 0\n";
    }


    // ========================================================
    // SIZE
    // ========================================================

    if (
        operation ==
        "SIZE"
    ) {

        return
            "SIZE " +
            std::to_string(
                store_.size()
            ) +
            "\n";
    }


    // ========================================================
    // TTL
    // ========================================================

    if (
        operation ==
        "TTL"
    ) {

        std::string key;

        input >> key;


        if (key.empty()) {

            return "ERROR invalid_ttl\n";
        }


        return
            "TTL " +
            std::to_string(
                store_.ttl(
                    key
                )
            ) +
            "\n";
    }


    // ========================================================
    // PING
    // ========================================================

    if (
        operation ==
        "PING"
    ) {

        return "PONG\n";
    }


    // ========================================================
    // Unknown command
    // ========================================================

    return "ERROR unknown_command\n";
}