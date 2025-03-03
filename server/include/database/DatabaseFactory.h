#pragma once
#include "IDatabase.h"
#include <memory>

namespace server {
namespace database {

enum class DatabaseType {
    MYSQL,
    MONGODB,
    DM  // 达梦数据库
};

class DatabaseFactory {
public:
    static std::unique_ptr<IDatabase> createDatabase(DatabaseType type);
    
private:
    DatabaseFactory() = delete;
    ~DatabaseFactory() = delete;
};

} // namespace database
} // namespace server 