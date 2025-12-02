// common/src/frame.cpp
#include "common/frame.hpp"

#include <optional>
#include <string>

nlohmann::json FrameMetadata::to_json() const {
    nlohmann::json j;
    j["seq_number"]      = seq_number;
    j["image_name"]      = image_name;
    j["rows"]            = rows;
    j["cols"]            = cols;
    j["encoding"]        = encoding;
    j["data_bytes"]      = data_bytes;
    j["keypoint_count"]  = keypoint_count; 
    return j;
}

std::optional<FrameMetadata> FrameMetadata::from_json(const std::string& s) {
    try {
        auto j = nlohmann::json::parse(s);

        FrameMetadata meta;
        meta.seq_number     = j.value("seq_number", -1);
        meta.image_name     = j.value("image_name", std::string{});
        meta.rows           = j.value("rows", 0);
        meta.cols           = j.value("cols", 0);
        meta.encoding       = j.value("encoding", std::string{});
        meta.data_bytes     = j.value("data_bytes", static_cast<std::size_t>(0));
        meta.keypoint_count = j.value("keypoint_count", 0);

        return meta;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}