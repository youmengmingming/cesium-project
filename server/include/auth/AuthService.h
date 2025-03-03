#pragma once
#include "User.h"
#include "../database/IDatabase.h"
#include <string>
#include <memory>
#include <optional>

namespace server {
namespace auth {

class AuthService {
public:
    explicit AuthService(std::shared_ptr<database::IDatabase> db);
    
    // 用户登录
    std::optional<User> login(const std::string& username, const std::string& password);
    
    // 用户注册
    bool registerUser(const std::string& username, const std::string& password, const std::string& role);
    
    // 验证token
    bool validateToken(const std::string& token);
    
    // 生成token
    std::string generateToken(const User& user);
    
    // 获取用户信息
    std::optional<User> getUserById(int userId);
    
    // 更新用户最后登录时间
    bool updateLastLogin(int userId);

private:
    // 密码哈希
    std::string hashPassword(const std::string& password, const std::string& salt);
    
    // 生成随机盐值
    std::string generateSalt();
    
    // 验证密码
    bool verifyPassword(const std::string& password, const std::string& hash, const std::string& salt);

    std::shared_ptr<database::IDatabase> db_;
};

} // namespace auth
} // namespace server 