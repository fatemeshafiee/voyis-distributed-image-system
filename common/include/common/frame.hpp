#pragma once
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

struct FrameMetadata {
    int         seq_number{};
    std::string image_name;
    int         rows{};
    int         cols{};
    std::string encoding;  
    std::size_t data_bytes{};
    int         keypoint_count{}; 

    nlohmann::json to_json() const;
    static std::optional<FrameMetadata> from_json(const std::string& s);
};