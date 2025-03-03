#include "database/MySQLDatabase.h"
#include <stdexcept>

namespace server {
namespace database {

MySQLDatabase::MySQLDatabase() : mysql_(nullptr), connected_(false) {
    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        throw std::runtime_error("Failed to initialize MySQL");
    }
}

MySQLDatabase::~MySQLDatabase() {
    disconnect();
}

bool MySQLDatabase::connect(const std::string& host, int port,
                          const std::string& username, const std::string& password,
                          const std::string& database) {
    if (!mysql_real_connect(mysql_, host.c_str(), username.c_str(),
                           password.c_str(), database.c_str(), port,
                           nullptr, 0)) {
        return false;
    }
    
    connected_ = true;
    return true;
}

void MySQLDatabase::disconnect() {
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
        connected_ = false;
    }
}

bool MySQLDatabase::query(const std::string& sql) {
    if (!connected_) {
        return false;
    }
    
    if (mysql_query(mysql_, sql.c_str())) {
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(mysql_);
    if (!result) {
        return false;
    }
    
    resultSet_.clear();
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        std::vector<std::string> rowData;
        for (unsigned int i = 0; i < mysql_num_fields(result); ++i) {
            rowData.push_back(row[i] ? row[i] : "");
        }
        resultSet_.push_back(rowData);
    }
    
    mysql_free_result(result);
    return true;
}

bool MySQLDatabase::update(const std::string& sql) {
    if (!connected_) {
        return false;
    }
    
    return mysql_query(mysql_, sql.c_str()) == 0;
}

std::vector<std::vector<std::string>> MySQLDatabase::getResultSet() {
    return resultSet_;
}

bool MySQLDatabase::beginTransaction() {
    if (!connected_) {
        return false;
    }
    return mysql_query(mysql_, "START TRANSACTION") == 0;
}

bool MySQLDatabase::commit() {
    if (!connected_) {
        return false;
    }
    return mysql_query(mysql_, "COMMIT") == 0;
}

bool MySQLDatabase::rollback() {
    if (!connected_) {
        return false;
    }
    return mysql_query(mysql_, "ROLLBACK") == 0;
}

} // namespace database
} // namespace server 