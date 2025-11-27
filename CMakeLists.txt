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

# add_subdirectory(common)
# add_subdirectory(feature_extractor)
# add_subdirectory(data_logger)