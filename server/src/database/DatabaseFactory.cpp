#include "database/DatabaseFactory.h"
#include "database/MySQLDatabase.h"
//#include "database/MongoDBDatabase.h"
//#include "database/DMDatabase.h"
//#include "database/SQLiteDatabase.h"

namespace server {
namespace database {

std::unique_ptr<IDatabase> DatabaseFactory::createDatabase(DatabaseType type) {
    switch (type) {
        case DatabaseType::MYSQL:
            return std::make_unique<MySQLDatabase>();
        case DatabaseType::MONGODB:
            //return std::make_unique<MongoDBDatabase>();
        case DatabaseType::DM:
            //return std::make_unique<DMDatabase>();
        case DatabaseType::SQLITE:
            //return std::make_unique<SQLiteDatabase>();
        default:
            return nullptr;
    }
}

} // namespace database
} // namespace server