#include "auth/AuthService.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace server {
namespace auth {

AuthService::AuthService(std::shared_ptr<database::IDatabase> db) : db_(db) {}

std::optional<User> AuthService::login(const std::string& username, const std::string& password) {
    // 构建查询SQL
    std::string sql = "SELECT id, username, password_hash, salt, role, is_active, last_login "
                      "FROM users WHERE username = '" + username + "'";
    
    if (!db_->query(sql)) {
        return std::nullopt;
    }
    
    auto results = db_->getResultSet();
    if (results.empty()) {
        return std::nullopt;
    }
    
    // 验证密码
    if (!verifyPassword(password, results[0][2], results[0][3])) {
        return std::nullopt;
    }
    
    // 检查用户状态
    if (results[0][5] != "1") {
        return std::nullopt;
    }
    
    // 构建用户对象
    User user;
    user.id = std::stoi(results[0][0]);
    user.username = results[0][1];
    user.password_hash = results[0][2];
    user.salt = results[0][3];
    user.role = results[0][4];
    user.is_active = true;
    user.last_login = results[0][6];
    
    // 更新最后登录时间
    updateLastLogin(user.id);
    
    return user;
}

bool AuthService::registerUser(const std::string& username, const std::string& password, const std::string& role) {
    // 检查用户名是否已存在
    std::string checkSql = "SELECT id FROM users WHERE username = '" + username + "'";
    if (!db_->query(checkSql)) {
        return false;
    }
    
    if (!db_->getResultSet().empty()) {
        return false;
    }
    
    // 生成盐值和密码哈希
    std::string salt = generateSalt();
    std::string passwordHash = hashPassword(password, salt);
    
    // 获取当前时间
    auto now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    
    // 构建插入SQL
    std::string sql = "INSERT INTO users (username, password_hash, salt, role, is_active, last_login) "
                      "VALUES ('" + username + "', '" + passwordHash + "', '" + salt + "', '" + 
                      role + "', 1, '" + ss.str() + "')";
    
    return db_->update(sql);
}

bool AuthService::validateToken(const std::string& token) {
    // TODO: 实现token验证逻辑
    return true;
}

std::string AuthService::generateToken(const User& user) {
    // TODO: 实现token生成逻辑
    return "dummy_token";
}

std::optional<User> AuthService::getUserById(int userId) {
    std::string sql = "SELECT id, username, password_hash, salt, role, is_active, last_login "
                      "FROM users WHERE id = " + std::to_string(userId);
    
    if (!db_->query(sql)) {
        return std::nullopt;
    }
    
    auto results = db_->getResultSet();
    if (results.empty()) {
        return std::nullopt;
    }
    
    User user;
    user.id = std::stoi(results[0][0]);
    user.username = results[0][1];
    user.password_hash = results[0][2];
    user.salt = results[0][3];
    user.role = results[0][4];
    user.is_active = (results[0][5] == "1");
    user.last_login = results[0][6];
    
    return user;
}

bool AuthService::updateLastLogin(int userId) {
    auto now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    
    std::string sql = "UPDATE users SET last_login = '" + ss.str() + 
                      "' WHERE id = " + std::to_string(userId);
    
    return db_->update(sql);
}

std::string AuthService::hashPassword(const std::string& password, const std::string& salt) {
    std::string input = password + salt;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.length());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

std::string AuthService::generateSalt() {
    unsigned char salt[16];
    if (RAND_bytes(salt, sizeof(salt)) != 1) {
        throw std::runtime_error("Failed to generate salt");
    }
    
    std::stringstream ss;
    for(int i = 0; i < 16; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(salt[i]);
    }
    
    return ss.str();
}

bool AuthService::verifyPassword(const std::string& password, const std::string& hash, const std::string& salt) {
    std::string computedHash = hashPassword(password, salt);
    return computedHash == hash;
}

} // namespace auth
} // namespace server 