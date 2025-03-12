#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "../include/database/RedisPool.h"
#include "../include/thread_pool.h"

using namespace cesium_server;
using namespace std::chrono_literals;

// 基本的CRUD操作示例
void basicOperationsExample() {
    auto& redis = RedisPool::getInstance();
    
    // 设置键值对
    redis.set("user:1:name", "John Doe");
    redis.set("user:1:email", "john@example.com", 3600s); // 1小时过期
    
    // 获取值
    if (auto name = redis.get("user:1:name")) {
        std::cout << "Name: " << *name << std::endl;
    }
    
    // 检查键是否存在
    if (redis.exists("user:1:email")) {
        std::cout << "Email exists" << std::endl;
    }
    
    // 删除键
    redis.del("user:1:name");
}

// 列表操作示例
void listOperationsExample() {
    auto& redis = RedisPool::getInstance();
    
    // 向列表添加元素
    redis.lpush("recent_users", "user1");
    redis.rpush("recent_users", "user2");
    redis.rpush("recent_users", "user3");
    
    // 从列表获取元素
    if (auto user = redis.lpop("recent_users")) {
        std::cout << "Latest user: " << *user << std::endl;
    }
}

// 哈希表操作示例
void hashOperationsExample() {
    auto& redis = RedisPool::getInstance();
    
    // 存储用户信息
    redis.hset("user:1", "name", "John Doe");
    redis.hset("user:1", "email", "john@example.com");
    redis.hset("user:1", "age", "30");
    
    // 获取用户信息
    if (auto name = redis.hget("user:1", "name")) {
        std::cout << "User name: " << *name << std::endl;
    }
    
    // 删除字段
    redis.hdel("user:1", "age");
}

// 集合操作示例
void setOperationsExample() {
    auto& redis = RedisPool::getInstance();
    
    // 添加在线用户
    redis.sadd("online_users", "user1");
    redis.sadd("online_users", "user2");
    redis.sadd("online_users", "user3");
    
    // 检查用户是否在线
    if (redis.sismember("online_users", "user1")) {
        std::cout << "User1 is online" << std::endl;
    }
    
    // 用户下线
    redis.srem("online_users", "user2");
}

// 有序集合操作示例
void sortedSetOperationsExample() {
    auto& redis = RedisPool::getInstance();
    
    // 记录用户积分
    redis.zadd("user_scores", 100.0, "user1");
    redis.zadd("user_scores", 85.5, "user2");
    redis.zadd("user_scores", 95.0, "user3");
    
    // 获取用户分数
    if (auto score = redis.zscore("user_scores", "user1")) {
        std::cout << "User1 score: " << *score << std::endl;
    }
    
    // 移除用户分数
    redis.zrem("user_scores", "user2");
}

// 高并发场景示例
void concurrencyExample() {
    auto& redis = RedisPool::getInstance();
    ThreadPool pool(4); // 创建4个线程的线程池
    std::vector<std::future<void>> futures;
    
    // 模拟多个并发请求
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.enqueue([i, &redis]() {
            // 模拟计数器增加
            std::string key = "counter:" + std::to_string(i % 10);
            auto value = redis.get(key).value_or("0");
            int count = std::stoi(value) + 1;
            redis.set(key, std::to_string(count));
            
            // 模拟一些延迟
            std::this_thread::sleep_for(50ms);
        }));
    }
    
    // 等待所有任务完成
    for (auto& future : futures) {
        future.wait();
    }
}

int main() {
    // 初始化Redis连接池
    RedisPool::getInstance().init("localhost", 6379, "", 10);
    
    std::cout << "=== Basic Operations Example ===" << std::endl;
    basicOperationsExample();
    
    std::cout << "\n=== List Operations Example ===" << std::endl;
    listOperationsExample();
    
    std::cout << "\n=== Hash Operations Example ===" << std::endl;
    hashOperationsExample();
    
    std::cout << "\n=== Set Operations Example ===" << std::endl;
    setOperationsExample();
    
    std::cout << "\n=== Sorted Set Operations Example ===" << std::endl;
    sortedSetOperationsExample();
    
    std::cout << "\n=== Concurrency Example ===" << std::endl;
    concurrencyExample();
    
    return 0;
}