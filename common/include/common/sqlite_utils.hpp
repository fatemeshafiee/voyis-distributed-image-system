#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <sqlite3.h>

namespace sqlite_utils {

struct DbDeleter {
    void operator()(sqlite3* db) const noexcept;
};

using DbPtr = std::unique_ptr<sqlite3, DbDeleter>;

struct StatementDeleter {
    void operator()(sqlite3_stmt* stmt) const noexcept;
};

using StatementPtr = std::unique_ptr<sqlite3_stmt, StatementDeleter>;

std::optional<DbPtr> open(std::string_view path);

bool exec(sqlite3* db, std::string_view sql, std::string_view what);

std::optional<StatementPtr> prepare(
    sqlite3* db,
    std::string_view sql,
    std::string_view what
);

bool step(sqlite3_stmt* stmt, std::string_view what);

void reset(sqlite3_stmt* stmt);

}  // namespace sqlite_utils
