// data_logger: receives feature metadata + image bytes from ipc:///tmp/voyis-feature-stream.ipc
// and stores them in a SQLite database (voyis_frames.db).

#include <iostream>
#include <vector>
#include <string>
#include <cerrno>

#include <zmq.h>
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include "common/frame.hpp"
#include "common/sqlite_utils.hpp"
#include "common/zmq_utils.hpp"

namespace {
constexpr char kFeatureStreamEndpoint[] = "ipc:///tmp/voyis-feature-stream.ipc";
}


int main(){
    auto db_opt = sqlite_utils::open("voyis_frames.db");
    if(!db_opt)
        return 1;
    sqlite_utils::DbPtr db = std::move(*db_opt);

    const char* create_frames_sql = "CREATE TABLE IF NOT EXISTS frames ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  seq_number INTEGER,"
        "  image_name TEXT,"
        "  rows INTEGER,"
        "  cols INTEGER,"
        "  keypoint_count INTEGER,"
        "  meta_json TEXT,"
        "  image_bytes BLOB"
        ");";

    if(!sqlite_utils::exec(db.get(), create_frames_sql, "create frames table")){
        return 1;
    }

    const char* insert_sql = 
        "INSERT INTO frames ("
        "  seq_number, image_name, rows, cols, keypoint_count, meta_json, image_bytes"
        ") VALUES (?, ?, ?, ?, ?, ?, ?);";

    auto insert_stmt_opt = sqlite_utils::prepare(db.get(), insert_sql, "prepare insert");
    if(!insert_stmt_opt){
        return 1;
    }
    sqlite_utils::StatementPtr insert_stmt = std::move(*insert_stmt_opt);

    void* context = zmq_ctx_new();
    void* pull_socket = zmq_socket(context, ZMQ_PULL);
    int rc_pull = zmq_connect(pull_socket, kFeatureStreamEndpoint);
    if(rc_pull != 0){
        std::cerr << "Failed to connect to the ZMQ PULL socket: " << zmq_strerror(errno) << "\n";
        zmq_close(pull_socket);
        zmq_ctx_term(context);
        return 0; 
    }
    std::cout << "Connected the ZMQ PULL socket to " << kFeatureStreamEndpoint << "\n";

    while(true){
        auto metadata_str_opt = zmq_utils::recv_string(
            pull_socket,
            0,
            "zmq_msg_recv(meta)"
        );
        if (!metadata_str_opt) {
            continue;
        }
        auto meta_opt = FrameMetadata::from_json(*metadata_str_opt);
        if (!meta_opt) {
            std::cerr << "[ERROR] Failed to parse metadata JSON\n";
            continue;
        }
        FrameMetadata meta = *meta_opt;
        std::cout << "Received meta: " << meta.to_json().dump() << "\n";

        auto buf_opt = zmq_utils::recv_bytes(
            pull_socket,
            0,
            "zmq_msg_recv(image)"
        );
        if (!buf_opt) {
            continue;
        }
        std::vector<unsigned char> buf = std::move(*buf_opt);
        std::cout<< buf.size()<<"\n";
        std::cout << "Received image buffer size: " << buf.size() << "\n";
        int seq_number = meta.seq_number;
        std::string name = meta.image_name;
        int rows = meta.rows;
        int cols = meta.cols;
        int kp_count = meta.keypoint_count;

        sqlite_utils::reset(insert_stmt.get());
        int idx = 1;

        sqlite3_bind_int(insert_stmt.get(), idx++, seq_number);
        sqlite3_bind_text(insert_stmt.get(), idx++,  name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(insert_stmt.get(),   idx++, rows);
        sqlite3_bind_int(insert_stmt.get(),   idx++, cols);
        sqlite3_bind_int(insert_stmt.get(),   idx++, kp_count);
        sqlite3_bind_text(insert_stmt.get(),  idx++, metadata_str_opt->c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_blob(insert_stmt.get(),  idx++, buf.data(),
                          static_cast<int>(buf.size()), SQLITE_TRANSIENT);

        if (!sqlite_utils::step(insert_stmt.get(), "sqlite3_step(insert frame)")) {
            continue;
        }

        std::cout << "Inserted frame seq=" << seq_number
                  << " with " << kp_count
                  << " keypoints into database.\n";
    }


    zmq_close(pull_socket);
    zmq_ctx_term(context);

    return 0;
}
