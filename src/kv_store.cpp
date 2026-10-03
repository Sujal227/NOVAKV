#include "kv_store.hpp"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>

KeyValueStore::KeyValueStore(
    const std::string& persistence_file
)
    : persistence_file_(persistence_file) {

    if (!persistence_enabled()) {
        return;
    }


    Persistence persistence(
        persistence_file_
    );


    std::unordered_map<
        std::string,
        std::string
    > loaded_data;


    if (
        persistence.load(
            loaded_data
        )
    ) {

        std::lock_guard<std::mutex> lock(
            mutex_
        );


        for (
            const auto& pair :
            loaded_data
        ) {

            data_[pair.first] = Entry{
                pair.second,
                std::chrono::steady_clock::time_point::max()
            };
        }
    }
}


// ============================================================
// Persistence
// ============================================================

bool KeyValueStore::persistence_enabled() const {

    return !persistence_file_.empty();
}


// ============================================================
// Expiration
// ============================================================

bool KeyValueStore::is_expired(
    const Entry& entry
) const {

    return std::chrono::steady_clock::now()
        >= entry.expires_at;
}


// ============================================================
// Persist
// ============================================================

void KeyValueStore::persist() {

    if (!persistence_enabled()) {
        return;
    }

    // Serialize disk writes. Multiple concurrent SET/REMOVE calls
    // may call persist() at the same time.
    std::lock_guard<std::mutex> persistence_lock(
        persistence_mutex_
    );


    std::unordered_map<
        std::string,
        std::string
    > snapshot;


    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        for (
            const auto& pair :
            data_
        ) {

            if (
                !is_expired(
                    pair.second
                )
            ) {

                snapshot[
                    pair.first
                ] = pair.second.value;
            }
        }
    }


    Persistence persistence(
        persistence_file_
    );


    persistence.save(
        snapshot
    );
}


// ============================================================
// SET
// ============================================================

void KeyValueStore::set(
    const std::string& key,
    const std::string& value
) {

    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        data_[key] = Entry{
            value,
            std::chrono::steady_clock::time_point::max()
        };
    }


    if (persistence_enabled()) {
        persist();
    }
}


// ============================================================
// SET with TTL
// ============================================================

void KeyValueStore::set(
    const std::string& key,
    const std::string& value,
    int ttl_seconds
) {

    auto expiration =
        std::chrono::steady_clock::now()
        + std::chrono::seconds(
            ttl_seconds
        );


    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        data_[key] = Entry{
            value,
            expiration
        };
    }


    if (persistence_enabled()) {
        persist();
    }
}


// ============================================================
// GET
// ============================================================

std::optional<std::string>
KeyValueStore::get(
    const std::string& key
) const {

    std::lock_guard<std::mutex> lock(
        mutex_
    );


    auto it =
        data_.find(key);


    if (
        it ==
        data_.end()
    ) {
        return std::nullopt;
    }


    if (
        is_expired(
            it->second
        )
    ) {
        return std::nullopt;
    }


    return it->second.value;
}


// ============================================================
// EXISTS
// ============================================================

bool KeyValueStore::exists(
    const std::string& key
) const {

    std::lock_guard<std::mutex> lock(
        mutex_
    );


    auto it =
        data_.find(key);


    if (
        it ==
        data_.end()
    ) {
        return false;
    }


    if (
        is_expired(
            it->second
        )
    ) {
        return false;
    }


    return true;
}


// ============================================================
// REMOVE
// ============================================================

bool KeyValueStore::remove(
    const std::string& key
) {

    bool removed = false;


    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        auto it =
            data_.find(key);


        if (
            it !=
            data_.end()
        ) {

            data_.erase(it);

            removed = true;
        }
    }


    if (
        removed &&
        persistence_enabled()
    ) {
        persist();
    }


    return removed;
}


// ============================================================
// SIZE
// ============================================================

std::size_t KeyValueStore::size() const {

    std::lock_guard<std::mutex> lock(
        mutex_
    );


    std::size_t count = 0;


    for (
        const auto& pair :
        data_
    ) {

        if (
            !is_expired(
                pair.second
            )
        ) {

            ++count;
        }
    }


    return count;
}


// ============================================================
// TTL
// ============================================================

std::int64_t KeyValueStore::ttl(
    const std::string& key
) const {

    std::lock_guard<std::mutex> lock(
        mutex_
    );


    auto it =
        data_.find(key);


    if (
        it ==
        data_.end()
    ) {
        return -2;
    }


    if (
        is_expired(
            it->second
        )
    ) {
        return -2;
    }


    if (
        it->second.expires_at ==
        std::chrono::steady_clock::time_point::max()
    ) {
        return -1;
    }


    auto remaining =
        std::chrono::duration_cast<
            std::chrono::seconds
        >(
            it->second.expires_at -
            std::chrono::steady_clock::now()
        ).count();


    if (remaining < 0) {
        return -2;
    }


    return remaining;
}