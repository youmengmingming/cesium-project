#pragma once
#include "IDatabase.h"
#include "DatabaseFactory.h"
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>

namespace server {
namespace database {

class DatabasePool {
public:
    static DatabasePool& getInstance();
    
    // 初始化连接池
    void init(DatabaseType type, const std::string& host, int port,
              const std::string& username, const std::string& password,
              const std::string& database, size_t poolSize = 10);
    
    // 获取数据库连接
    std::shared_ptr<IDatabase> acquire();
    
    // 释放数据库连接
    void release(std::shared_ptr<IDatabase> conn);
    
private:
    DatabasePool() = default;
    ~DatabasePool();
    
    DatabasePool(const DatabasePool&) = delete;
    DatabasePool& operator=(const DatabasePool&) = delete;
    
    std::queue<std::shared_ptr<IDatabase>> connections_;
    std::mutex mutex_;
    std::condition_variable condition_;
    size_t poolSize_ = 0;
    bool initialized_ = false;
};

} // namespace database
} // namespace server 