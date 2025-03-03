#include "database/DatabasePool.h"
#include <stdexcept>

namespace server {
namespace database {

DatabasePool& DatabasePool::getInstance() {
    static DatabasePool instance;
    return instance;
}

void DatabasePool::init(DatabaseType type, const std::string& host, int port,
                       const std::string& username, const std::string& password,
                       const std::string& database, size_t poolSize) {
    if (initialized_) {
        return;
    }

    poolSize_ = poolSize;
    
    for (size_t i = 0; i < poolSize_; ++i) {
        auto conn = DatabaseFactory::createDatabase(type);
        if (conn && conn->connect(host, port, username, password, database)) {
            connections_.push(std::shared_ptr<IDatabase>(std::move(conn)));
        }
    }
    
    initialized_ = true;
}

std::shared_ptr<IDatabase> DatabasePool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this] { return !connections_.empty(); });
    
    auto conn = connections_.front();
    connections_.pop();
    return conn;
}

void DatabasePool::release(std::shared_ptr<IDatabase> conn) {
    if (conn) {
        std::unique_lock<std::mutex> lock(mutex_);
        connections_.push(conn);
        condition_.notify_one();
    }
}

DatabasePool::~DatabasePool() {
    while (!connections_.empty()) {
        auto conn = connections_.front();
        connections_.pop();
        if (conn) {
            conn->disconnect();
        }
    }
}

} // namespace database
} // namespace server 