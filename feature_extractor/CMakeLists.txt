add_executable(feature_extractor src/main.cpp)

target_link_libraries(feature_extractor
    PRIVATE
        ${OpenCV_LIBRARIES}
        ${ZMQ_LIBRARIES}
        nlohmann_json::nlohmann_json

)

target_include_directories(feature_extractor
    PRIVATE
        ${OpenCV_LIBRARIES}
        ${ZMQ_LIBRARIES}
        nlohmann_json::nlohmann_json
)