add_executable(image_generator src/main.cpp)

target_link_libraries(image_generator 
    PRIVATE 
        ${OpenCV_LIBRARIES} 
        ${SQLite3_LIBRARIES} 
        nlohmann_json::nlohmann_json 
        ${ZMQ_LIBRARIES})
target_include_directories(image_generator
    PRIVATE
        ${OpenCV_INCLUDE_DIRS}
        ${SQLite3_INCLUDE_DIRS}
        ${ZMQ_INCLUDE_DIRS}
)