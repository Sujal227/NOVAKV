#pragma once

#include <string>
#include <unordered_map>

class Persistence {
public:
    explicit Persistence(
        const std::string& filename
    );

    bool save(
        const std::unordered_map<std::string, std::string>& data
    ) const;

    bool load(
        std::unordered_map<std::string, std::string>& data
    ) const;

private:
    std::string filename_;
};