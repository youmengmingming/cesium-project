#pragma once
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <condition_variable>
#include <sw/redis++/redis++.h>

namespace server {
namespace database {

class RedisPool {
public:
    static RedisPool& getInstance();
    
    // 初始化连接池
    void init(const std::string& host = "localhost",
              int port = 6379,
              const std::string& password = "",
              size_t poolSize = 10,
              const std::string& db = "0");
    
    // 获取Redis连接
    std::shared_ptr<sw::redis::Redis> acquire();
    
    // 释放Redis连接
    void release(std::shared_ptr<sw::redis::Redis> conn);
    
    // 基本的缓存操作
    bool set(const std::string& key, const std::string& value, std::chrono::seconds ttl = std::chrono::seconds{0});
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    
    // 列表操作
    bool lpush(const std::string& key, const std::string& value);
    bool rpush(const std::string& key, const std::string& value);
    std::optional<std::string> lpop(const std::string& key);
    std::optional<std::string> rpop(const std::string& key);
    
    // 哈希表操作
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    std::optional<std::string> hget(const std::string& key, const std::string& field);
    bool hdel(const std::string& key, const std::string& field);
    
    // 集合操作
    bool sadd(const std::string& key, const std::string& member);
    bool srem(const std::string& key, const std::string& member);
    bool sismember(const std::string& key, const std::string& member);
    
    // 有序集合操作
    bool zadd(const std::string& key, double score, const std::string& member);
    bool zrem(const std::string& key, const std::string& member);
    std::optional<double> zscore(const std::string& key, const std::string& member);
    
private:
    RedisPool() = default;
    ~RedisPool();
    
    RedisPool(const RedisPool&) = delete;
    RedisPool& operator=(const RedisPool&) = delete;
    
    std::queue<std::shared_ptr<sw::redis::Redis>> connections_;
    std::mutex mutex_;
    std::condition_variable condition_;
    size_t poolSize_ = 0;
    bool initialized_ = false;
    
    // Redis连接配置
    std::string host_;
    int port_;
    std::string password_;
    std::string db_;
};

} // namespace database
} // namespace server