#pragma once
#include "IDatabase.h"
//#include <sqlite3.h>
#include <string>
#include <vector>

namespace server {
namespace database {

class SQLiteDatabase : public IDatabase {
public:
    SQLiteDatabase();
    ~SQLiteDatabase() override;

    bool connect(const std::string& host, int port,
                const std::string& username, const std::string& password,
                const std::string& database) override;
    
    void disconnect() override;
    
    bool query(const std::string& sql) override;
    
    bool update(const std::string& sql) override;
    
    std::vector<std::vector<std::string>> getResultSet() override;
    
    bool beginTransaction() override;
    
    bool commit() override;
    
    bool rollback() override;

private:
    //sqlite3* db_;
    bool connected_;
    std::vector<std::vector<std::string>> resultSet_;
};

} // namespace database
} // namespace server