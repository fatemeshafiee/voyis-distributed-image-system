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


    sqlite3_finalize(insert_stmt);
    sqlite3_close(db); 

    return 0;
}