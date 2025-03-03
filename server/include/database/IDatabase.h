#pragma once
#include <string>
#include <vector>
#include <memory>

namespace server {
namespace database {

class IDatabase {
public:
    virtual ~IDatabase() = default;
    
    // 连接数据库
    virtual bool connect(const std::string& host, int port, 
                        const std::string& username, const std::string& password,
                        const std::string& database) = 0;
    
    // 断开连接
    virtual void disconnect() = 0;
    
    // 执行查询
    virtual bool query(const std::string& sql) = 0;
    
    // 执行更新
    virtual bool update(const std::string& sql) = 0;
    
    // 获取结果集
    virtual std::vector<std::vector<std::string>> getResultSet() = 0;
    
    // 开始事务
    virtual bool beginTransaction() = 0;
    
    // 提交事务
    virtual bool commit() = 0;
    
    // 回滚事务
    virtual bool rollback() = 0;
};

} // namespace database
} // namespace server 