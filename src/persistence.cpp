#include "persistence.hpp"

#include <fstream>
#include <string>

Persistence::Persistence(
    const std::string& filename
)
    : filename_(filename) {
}


// ============================================================
// SAVE
// ============================================================

bool Persistence::save(
    const std::unordered_map<std::string, std::string>& data
) const {

    std::ofstream file(
        filename_,
        std::ios::trunc
    );

    if (!file.is_open()) {
        return false;
    }


    for (const auto& pair : data) {

        file << pair.first
             << '\t'
             << pair.second
             << '\n';
    }


    return file.good();
}


// ============================================================
// LOAD
// ============================================================

bool Persistence::load(
    std::unordered_map<std::string, std::string>& data
) const {

    std::ifstream file(
        filename_
    );

    if (!file.is_open()) {

        // No database file yet is not an error.
        return true;
    }


    std::string key;
    std::string value;


    while (
        std::getline(
            file,
            key,
            '\t'
        )
    ) {

        if (
            !std::getline(
                file,
                value
            )
        ) {
            break;
        }


        data[key] = value;
    }


    return true;
}