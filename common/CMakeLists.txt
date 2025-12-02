add_library(voyis_common
    src/frame.cpp
    src/zmq_utils.cpp

)
# #     src/zmq_utils.cpp
# src/image_utils.cpp
# src/db.cpp

target_include_directories(voyis_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(voyis_common
    PRIVATE
      ${OpenCV_LIBRARIES}
      ${ZMQ_LIBRARIES}
      SQLite::SQLite3
      nlohmann_json::nlohmann_json
)
