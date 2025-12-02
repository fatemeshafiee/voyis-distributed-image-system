#include "common/sqlite_utils.hpp"

#include <iostream>
#include <string>

namespace sqlite_utils {

void DbDeleter::operator()(sqlite3* db) const noexcept {
    if (db) {
        sqlite3_close(db);
    }
}

void StatementDeleter::operator()(sqlite3_stmt* stmt) const noexcept {
    if (stmt) {
        sqlite3_finalize(stmt);
    }
}

std::optional<DbPtr> open(std::string_view path) {
    sqlite3* raw = nullptr;
    std::string path_str(path);
    int rc = sqlite3_open(path_str.c_str(), &raw);
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] sqlite3_open failed: "
                  << (raw ? sqlite3_errmsg(raw) : "unknown") << "\n";
        if (raw) {
            sqlite3_close(raw);
        }
        return std::nullopt;
    }
    return DbPtr(raw);
}

bool exec(sqlite3* db, std::string_view sql, std::string_view what) {
    char* errmsg = nullptr;
    std::string sql_str(sql);
    int rc = sqlite3_exec(db, sql_str.c_str(), nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] " << what << " failed: "
                  << (errmsg ? errmsg : sqlite3_errmsg(db)) << "\n";
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

std::optional<StatementPtr> prepare(
    sqlite3* db,
    std::string_view sql,
    std::string_view what
) {
    sqlite3_stmt* stmt = nullptr;
    std::string sql_str(sql);
    int rc = sqlite3_prepare_v2(db, sql_str.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] " << what << " failed: "
                  << sqlite3_errmsg(db) << "\n";
        return std::nullopt;
    }
    return StatementPtr(stmt);
}

bool step(sqlite3_stmt* stmt, std::string_view what) {
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE || rc == SQLITE_ROW) {
        return true;
    }
    sqlite3* db = sqlite3_db_handle(stmt);
    std::cerr << "[ERROR] " << what << " failed: "
              << (db ? sqlite3_errmsg(db) : "unknown") << "\n";
    return false;
}

void reset(sqlite3_stmt* stmt) {
    if (!stmt) {
        return;
    }
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
}

}  // namespace sqlite_utils
