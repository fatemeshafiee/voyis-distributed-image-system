add_executable(data_logger src/main.cpp)

target_link_libraries(data_logger
    PRIVATE
        SQLite::SQLite3
        ${ZMQ_LIBRARIES}
        nlohmann_json::nlohmann_json)

target_include_directories(data_logger
    PRIVATE
    ${ZMQ_INCLUDE_DIRS}

)