//#include "database/SQLiteDatabase.h"
//#include <stdexcept>
//#include <iostream>
//
//namespace server {
//namespace database {
//
//SQLiteDatabase::SQLiteDatabase() : db_(nullptr), connected_(false) {
//}
//
//SQLiteDatabase::~SQLiteDatabase() {
//    disconnect();
//}
//
//bool SQLiteDatabase::connect(const std::string& host, int port,
//                           const std::string& username, const std::string& password,
//                           const std::string& database) {
//    // SQLite只需要数据库文件路径，忽略其他参数
//    int rc = sqlite3_open(database.c_str(), &db_);
//    if (rc != SQLITE_OK) {
//        std::cerr << "SQLite open error: " << sqlite3_errmsg(db_) << std::endl;
//        sqlite3_close(db_);
//        db_ = nullptr;
//        return false;
//    }
//    
//    connected_ = true;
//    return true;
//}
//
//void SQLiteDatabase::disconnect() {
//    if (db_) {
//        sqlite3_close(db_);
//        db_ = nullptr;
//        connected_ = false;
//    }
//}
//
//// 用于处理查询结果的回调函数
//static int queryCallback(void* data, int argc, char** argv, char** azColName) {
//    auto* resultSet = static_cast<std::vector<std::vector<std::string>>*>(data);
//    
//    std::vector<std::string> row;
//    for (int i = 0; i < argc; i++) {
//        row.push_back(argv[i] ? argv[i] : "");
//    }
//    
//    resultSet->push_back(row);
//    return 0;
//}
//
//bool SQLiteDatabase::query(const std::string& sql) {
//    if (!connected_) {
//        return false;
//    }
//    
//    resultSet_.clear();
//    
//    char* errMsg = nullptr;
//    int rc = sqlite3_exec(db_, sql.c_str(), queryCallback, &resultSet_, &errMsg);
//    
//    if (rc != SQLITE_OK) {
//        std::cerr << "SQLite query error: " << errMsg << std::endl;
//        sqlite3_free(errMsg);
//        return false;
//    }
//    
//    return true;
//}
//
//bool SQLiteDatabase::update(const std::string& sql) {
//    if (!connected_) {
//        return false;
//    }
//    
//    char* errMsg = nullptr;
//    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
//    
//    if (rc != SQLITE_OK) {
//        std::cerr << "SQLite update error: " << errMsg << std::endl;
//        sqlite3_free(errMsg);
//        return false;
//    }
//    
//    return true;
//}
//
//std::vector<std::vector<std::string>> SQLiteDatabase::getResultSet() {
//    return resultSet_;
//}
//
//bool SQLiteDatabase::beginTransaction() {
//    if (!connected_) {
//        return false;
//    }
//    return update("BEGIN TRANSACTION");
//}
//
//bool SQLiteDatabase::commit() {
//    if (!connected_) {
//        return false;
//    }
//    return update("COMMIT");
//}
//
//bool SQLiteDatabase::rollback() {
//    if (!connected_) {
//        return false;
//    }
//    return update("ROLLBACK");
//}
//
//} // namespace database
//} // namespace server