// data_logger: receives feature metadata + image bytes from tcp://localhost:5556
// and stores them in a SQLite database (voyis_frames.db).

#include <iostream>
#include <vector>
#include <string>
#include <cerrno>

#include <zmq.h>
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include "common/frame.hpp"
#include "common/zmq_utils.hpp"


bool open_db(sqlite3** db){
    int rc = sqlite3_open("voyis_frames.db", db);
    if(rc != SQLITE_OK){
        std::cerr << "[ERROR] Failed to open database: "
        << sqlite3_errmsg(*db) << "\n";
        sqlite3_close(*db);
        *db = nullptr;
        return false;
    }
    return true;
}
bool create_table(sqlite3* db){

    const char* sql = "CREATE TABLE IF NOT EXISTS frames ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  seq_number INTEGER,"
        "  image_name TEXT,"
        "  rows INTEGER,"
        "  cols INTEGER,"
        "  keypoint_count INTEGER,"
        "  meta_json TEXT,"
        "  image_bytes BLOB"
        ");";

        char *errmsg = nullptr;
        int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errmsg);
        if (rc != SQLITE_OK){
            std::cerr << "[ERROR] failed to create table " 
            << (errmsg ? errmsg : "unknown") << "\n";

            sqlite3_free(errmsg);
            return false;
        }


    return true;
}
bool prepare_insert(sqlite3* db, sqlite3_stmt** stmt){

    const char* sql = 
        "INSERT INTO frames ("
        "  seq_number, image_name, rows, cols, keypoint_count, meta_json, image_bytes"
        ") VALUES (?, ?, ?, ?, ?, ?, ?);";

    int rc = sqlite3_prepare_v2(db, sql, -1, stmt, nullptr);
    if(rc!= SQLITE_OK){
        std::cerr << "[ERROR] Failed to prepare insert statement: "
                  << sqlite3_errmsg(db) << "\n";  
        return false;
    }
    return true;

}


int main(){
    sqlite3* db = nullptr;
    if(!open_db(&db))
        return 1;

    if(!create_table(db)){
        sqlite3_close(db);
        return 1;
    }
    sqlite3_stmt * insert_stmt = nullptr;
    if(!prepare_insert(db, &insert_stmt)){
        sqlite3_close(db);
        return 1;

    }

    void* context = zmq_ctx_new();
    void* pull_socket = zmq_socket(context, ZMQ_PULL);
    int rc_pull = zmq_connect(pull_socket, "tcp://localhost:5556");
    if(rc_pull != 0){
        std::cerr << "Failed to connect to the ZMQ PULL socket: " << zmq_strerror(errno) << "\n";
        zmq_close(pull_socket);
        zmq_ctx_term(context);
        sqlite3_finalize(insert_stmt);
        sqlite3_close(db);
        return 0; 
    }
    std::cout << "Connected the ZMQ PULL socket. " << "\n";

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

        sqlite3_reset(insert_stmt);
        sqlite3_clear_bindings(insert_stmt);
        int idx = 1;

        sqlite3_bind_int(insert_stmt, idx++, seq_number);
        sqlite3_bind_text(insert_stmt, idx++,  name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(insert_stmt,   idx++, rows);
        sqlite3_bind_int(insert_stmt,   idx++, cols);
        sqlite3_bind_int(insert_stmt,   idx++, kp_count);
        sqlite3_bind_text(insert_stmt,  idx++, metadata_str_opt->c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_blob(insert_stmt,  idx++, buf.data(),
                          static_cast<int>(buf.size()), SQLITE_TRANSIENT);

        int step_rc = sqlite3_step(insert_stmt);
        if (step_rc != SQLITE_DONE) {
            std::cerr << "[ERROR] sqlite3_step failed: "
                      << sqlite3_errmsg(db) << "\n";
            continue;
        }

        std::cout << "Inserted frame seq=" << seq_number
                  << " with " << kp_count
                  << " keypoints into database.\n";
    }


    zmq_close(pull_socket);
    zmq_ctx_term(context);
    sqlite3_finalize(insert_stmt);
    sqlite3_close(db); 

    return 0;
}
