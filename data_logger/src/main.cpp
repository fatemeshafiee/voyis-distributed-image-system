// data_logger: receives feature metadata + image bytes from tcp://localhost:5556
// and stores them in a SQLite database (voyis_frames.db).

#include <iostream>
#include <vector>
#include <string>
#include <cerrno>

#include <zmq.h>
#include <sqlite3.h>
#include <nlohmann/json.hpp>


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
        zmq_msg_t metadata_msg;
        zmq_msg_init (&metadata_msg);
        int rc_meta = zmq_msg_recv(&metadata_msg, pull_socket, 0);
        if(rc_meta == -1){
            std::cerr << "[ERROR] zmq_msg_recv(meta) failed: "
                  << zmq_strerror(errno) << "\n";
            zmq_msg_close(&metadata_msg);
            continue;
        }
        std::string metadata_str(
            static_cast<char*> (zmq_msg_data(&metadata_msg)), 
            zmq_msg_size(&metadata_msg));
        zmq_msg_close(&metadata_msg);
        nlohmann::json metadata;
        try{
            metadata = nlohmann::json::parse(metadata_str);
        } catch (const std::exception& e){
            std::cerr<< "[ERROR] Failed to parse metadata JSON: " << e.what() << "\n";
            continue;
        }
        std::cout << "Received meta: " << metadata.dump() << "\n";
        

        zmq_msg_t img_msg;
        zmq_msg_init(&img_msg);
        int rc_img = zmq_msg_recv(&img_msg, pull_socket, 0);
        if (rc_img == -1){
            std::cerr << "[ERROR] zmq_msg_recv(image) failed: " << zmq_strerror(errno) << "\n";
            zmq_msg_close(&img_msg);
            continue;
        }

        std::vector<unsigned char> buf (
            static_cast<unsigned char*>(zmq_msg_data(&img_msg)),
            static_cast<unsigned char*>(zmq_msg_data(&img_msg)) + zmq_msg_size(&img_msg)
        );
        std::cout<< buf.size()<<"\n";
        zmq_msg_close(&img_msg);
        std::cout << "Received image buffer size: " << buf.size() << "\n";
        int seq_number = metadata.value("seq_number", -1);
        std::string name = metadata.value("image_name", std::string("unknown"));
        int rows = metadata.value("rows", -1);
        int cols = metadata.value("cols", -1);
        int kp_count = metadata.value("keypoint_count", -1);

        sqlite3_reset(insert_stmt);
        sqlite3_clear_bindings(insert_stmt);
        int idx = 1;

        sqlite3_bind_int(insert_stmt, idx++, seq_number);
        sqlite3_bind_text(insert_stmt, idx++,  name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(insert_stmt,   idx++, rows);
        sqlite3_bind_int(insert_stmt,   idx++, cols);
        sqlite3_bind_int(insert_stmt,   idx++, kp_count);
        sqlite3_bind_text(insert_stmt,  idx++, metadata_str.c_str(), -1, SQLITE_TRANSIENT);
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