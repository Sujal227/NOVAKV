#include "client.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

namespace {

std::string to_upper(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::toupper(character));
        }
    );

    return value;
}

std::string first_word(const std::string& command) {
    std::istringstream input(command);
    std::string word;
    input >> word;
    return to_upper(word);
}

} // namespace

NovaKVClient::NovaKVClient(
    const std::string& host,
    int port
)
    : host_(host),
      port_(port),
      socket_(static_cast<std::uintptr_t>(INVALID_SOCKET)),
      connected_(false),
      winsock_initialized_(false) {
}

NovaKVClient::~NovaKVClient() {
    disconnect();
}

bool NovaKVClient::connect_to_server() {
    if (connected_) {
        return true;
    }

    if (!winsock_initialized_) {
        WSADATA wsa_data{};

        const int result = WSAStartup(
            MAKEWORD(2, 2),
            &wsa_data
        );

        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << '\n';
            return false;
        }

        winsock_initialized_ = true;
    }

    SOCKET socket_handle = ::socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (socket_handle == INVALID_SOCKET) {
        std::cerr << "Failed to create socket.\n";
        WSACleanup();
        winsock_initialized_ = false;
        return false;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(
        static_cast<u_short>(port_)
    );

    int address_result = inet_pton(
        AF_INET,
        host_.c_str(),
        &server_address.sin_addr
    );

    if (address_result != 1) {
        addrinfo hints{};
        addrinfo* result_info = nullptr;

        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        const int dns_result = getaddrinfo(
            host_.c_str(),
            nullptr,
            &hints,
            &result_info
        );

        if (dns_result != 0 || result_info == nullptr) {
            closesocket(socket_handle);
            std::cerr << "Could not resolve host: " << host_ << '\n';
            return false;
        }

        auto* resolved = reinterpret_cast<sockaddr_in*>(
            result_info->ai_addr
        );

        server_address.sin_addr = resolved->sin_addr;
        freeaddrinfo(result_info);
    }

    const int result = connect(
        socket_handle,
        reinterpret_cast<sockaddr*>(&server_address),
        sizeof(server_address)
    );

    if (result == SOCKET_ERROR) {
        closesocket(socket_handle);
        return false;
    }

    socket_ = static_cast<std::uintptr_t>(socket_handle);
    connected_ = true;

    return true;
}

bool NovaKVClient::reconnect_with_retry() {
    disconnect();

    std::cout << "Connection lost. Waiting for NovaKV server...\n";

    while (true) {
        std::this_thread::sleep_for(
            std::chrono::seconds(2)
        );

        std::cout << "Attempting to reconnect...\n";

        if (connect_to_server()) {
            std::cout << "Reconnected to NovaKV\n";
            return true;
        }
    }
}

bool NovaKVClient::send_all(const std::string& data) {
    if (!connected_) {
        return false;
    }

    SOCKET socket_handle = static_cast<SOCKET>(socket_);
    std::size_t total_sent = 0;

    while (total_sent < data.size()) {
        const int sent = send(
            socket_handle,
            data.data() + total_sent,
            static_cast<int>(data.size() - total_sent),
            0
        );

        if (sent == SOCKET_ERROR || sent == 0) {
            return false;
        }

        total_sent += static_cast<std::size_t>(sent);
    }

    return true;
}

bool NovaKVClient::receive_line(std::string& response) {
    if (!connected_) {
        return false;
    }

    response.clear();
    SOCKET socket_handle = static_cast<SOCKET>(socket_);
    char character = '\0';

    while (true) {
        const int received = recv(
            socket_handle,
            &character,
            1,
            0
        );

        if (received <= 0) {
            return false;
        }

        if (character == '\n') {
            break;
        }

        if (character != '\r') {
            response += character;
        }

        if (response.size() > 1024 * 1024) {
            return false;
        }
    }

    return true;
}

bool NovaKVClient::send_command(
    const std::string& command,
    std::string& response
) {
    if (!connected_) {
        return false;
    }

    if (!send_all(command + "\n")) {
        return false;
    }

    return receive_line(response);
}

void NovaKVClient::run() {
    if (!connected_) {
        return;
    }

    std::cout
        << "Commands:\n"
        << "  SET key value\n"
        << "  SET key value ttl\n"
        << "  GET key\n"
        << "  EXISTS key\n"
        << "  REMOVE key\n"
        << "  SIZE\n"
        << "  TTL key\n"
        << "  PING\n"
        << "  EXIT\n\n";

    std::string command;

    while (true) {
        std::cout << "> " << std::flush;

        if (!std::getline(std::cin, command)) {
            break;
        }

        if (command.empty()) {
            continue;
        }

        if (first_word(command) == "EXIT") {
            break;
        }

        std::string response;

        if (!send_command(command, response)) {
            if (!reconnect_with_retry()) {
                break;
            }

            // Retry the command once after the connection has been restored.
            if (!send_command(command, response)) {
                std::cerr << "Command failed after reconnect.\n";
                continue;
            }
        }

        std::cout << response<<std::endl;
    }

    disconnect();
}

void NovaKVClient::disconnect() {
    if (socket_ != static_cast<std::uintptr_t>(INVALID_SOCKET)) {
        closesocket(static_cast<SOCKET>(socket_));
        socket_ = static_cast<std::uintptr_t>(INVALID_SOCKET);
    }

    connected_ = false;

    if (winsock_initialized_) {
        WSACleanup();
        winsock_initialized_ = false;
    }
}
