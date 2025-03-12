#include "database/RedisPool.h"
#include <stdexcept>

namespace server {
namespace database {

RedisPool& RedisPool::getInstance() {
    static RedisPool instance;
    return instance;
}

void RedisPool::init(const std::string& host, int port, const std::string& password,
                     size_t poolSize, const std::string& db) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        throw std::runtime_error("Redis pool already initialized");
    }

    host_ = host;
    port_ = port;
    password_ = password;
    db_ = db;
    poolSize_ = poolSize;

    // 创建连接池
    for (size_t i = 0; i < poolSize_; ++i) {
        sw::redis::ConnectionOptions connection_options;
        connection_options.host = host_;
        connection_options.port = port_;
        if (!password_.empty()) {
            connection_options.password = password_;
        }
        connection_options.db = std::stoi(db_);

        sw::redis::ConnectionPoolOptions pool_options;
        pool_options.size = 1;  // 每个连接对象只包含一个实际连接

        auto redis = std::make_shared<sw::redis::Redis>(connection_options, pool_options);
        connections_.push(redis);
    }

    initialized_ = true;
}

std::shared_ptr<sw::redis::Redis> RedisPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!initialized_) {
        throw std::runtime_error("Redis pool not initialized");
    }

    while (connections_.empty()) {
        condition_.wait(lock);
    }

    auto conn = connections_.front();
    connections_.pop();
    return conn;
}

void RedisPool::release(std::shared_ptr<sw::redis::Redis> conn) {
    if (conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.push(conn);
        condition_.notify_one();
    }
}

bool RedisPool::set(const std::string& key, const std::string& value, std::chrono::seconds ttl) {
    try {
        auto redis = acquire();
        if (ttl.count() > 0) {
            redis->set(key, value, ttl);
        } else {
            redis->set(key, value);
        }
        release(redis);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::optional<std::string> RedisPool::get(const std::string& key) {
    try {
        auto redis = acquire();
        auto value = redis->get(key);
        release(redis);
        if (value) {
            return *value;
        }
    } catch (const std::exception&) {}
    return std::nullopt;
}

bool RedisPool::del(const std::string& key) {
    try {
        auto redis = acquire();
        redis->del(key);
        release(redis);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool RedisPool::exists(const std::string& key) {
    try {
        auto redis = acquire();
        bool exists = redis->exists(key);
        release(redis);
        return exists;
    } catch (const std::exception&) {
        return false;
    }
}

bool RedisPool::lpush(const std::string& key, const std::string& value) {
    try {
        auto redis = acquire();
        redis->lpush(key, value);
        release(redis);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool RedisPool::rpush(const std::string& key, const std::string& value) {
    try {
        auto redis = acquire();
        redis->rpush(key, value);
        release(redis);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::optional<std::string> RedisPool::lpop(const std::string& key) {
    try {
        auto redis = acquire();
        auto value = redis->lpop(key);
        release(redis);
        if (value) {
            return *value;
        }
    } catch (const std::exception&) {}
    return std::nullopt;
}

std::optional<std::string> RedisPool::rpop(const std::string& key) {
    try {
        auto redis = acquire();
        auto value = redis->rpop(key);
        release(redis);
        if (value) {
            return *value;
        }
    } catch (const std::exception&) {}
    return std::nullopt;
}

bool RedisPool::hset(const std::string& key, const std::string& field, const std::string& value) {
    try {
        auto redis = acquire();
        redis->hset(key, field, value);
        release(redis);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::optional<std::string> RedisPool::hget(const std::string& key, const std::string& field) {
    try {
        auto redis = acquire();
        auto value = redis->hget(key, field);
        release(redis);
        if (value) {
            return *value;
        }
    } catch (const std::exception&) {}
    return std::nullopt;
}

bool RedisPool::hdel(const std::string& key, const std::string& field) {
    try {
        auto redis = acquire();
        redis->hdel(key, field);
        release(redis);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool RedisPool::sadd(const std::string& key, const std::string& member) {
    try {
        auto redis = acquire();
        redis->sadd(key, member);
        release(redis);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool RedisPool::srem(const std::string& key, const std::string& member) {
    try {
        auto redis = acquire();
        redis->srem(key, member);
        release(redis);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool RedisPool::sismember(const std::string& key, const std::string& member) {
    try {
        auto redis = acquire();
        bool is_member = redis->sismember(key, member);
        release(redis);
        return is_member;
    } catch (const std::exception&) {
        return false;
    }
}

bool RedisPool::zadd(const std::string& key, double score, const std::string& member) {
    try {
        auto redis = acquire();
        redis->zadd(key, member, score);
        release(redis);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool RedisPool::zrem(const std::string& key, const std::string& member) {
    try {
        auto redis = acquire();
        redis->zrem(key, member);
        release(redis);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::optional<double> RedisPool::zscore(const std::string& key, const std::string& member) {
    try {
        auto redis = acquire();
        auto score = redis->zscore(key, member);
        release(redis);
        if (score) {
            return *score;
        }
    } catch (const std::exception&) {}
    return std::nullopt;
}

RedisPool::~RedisPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty()) {
        connections_.pop();
    }
}

} // namespace database
} // namespace server