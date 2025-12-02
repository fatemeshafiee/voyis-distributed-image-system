cmake_minimum_required(VERSION 3.16)
project(voyis_distributed_image_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
find_package(nlohmann_json REQUIRED)


add_subdirectory(image_generator)
add_subdirectory(data_logger)
add_subdirectory(feature_extractor)
add_subdirectory(common)

target_link_libraries(image_generator
    PRIVATE
        voyis_common
        ${OpenCV_LIBRARIES}
        ${ZMQ_LIBRARIES}
        nlohmann_json::nlohmann_json
)

target_link_libraries(feature_extractor
    PRIVATE
        voyis_common
        ${OpenCV_LIBRARIES}
        ${ZMQ_LIBRARIES}
        nlohmann_json::nlohmann_json
)

target_link_libraries(data_logger
    PRIVATE
        voyis_common
        ${ZMQ_LIBRARIES}
        SQLite::SQLite3
        nlohmann_json::nlohmann_json
)