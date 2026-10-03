#pragma once

#include "persistence.hpp"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class KeyValueStore {
public:

    explicit KeyValueStore(
        const std::string& persistence_file = ""
    );

    void set(
        const std::string& key,
        const std::string& value
    );

    void set(
        const std::string& key,
        const std::string& value,
        int ttl_seconds
    );

    std::optional<std::string> get(
        const std::string& key
    ) const;

    bool exists(
        const std::string& key
    ) const;

    bool remove(
        const std::string& key
    );

    std::size_t size() const;

    /*
        TTL return values:

        -2 = key doesn't exist / expired
        -1 = key exists permanently
        >=0 = remaining TTL in seconds
    */
    std::int64_t ttl(
        const std::string& key
    ) const;

private:

    struct Entry {
        std::string value;

        std::chrono::steady_clock::time_point expires_at;
    };


    mutable std::mutex mutex_;
    mutable std::mutex persistence_mutex_;

    std::unordered_map<
        std::string,
        Entry
    > data_;

    std::string persistence_file_;


    bool persistence_enabled() const;

    bool is_expired(
        const Entry& entry
    ) const;

    void persist();
};