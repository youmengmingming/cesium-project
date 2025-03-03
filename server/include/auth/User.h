#pragma once
#include <string>

namespace server {
namespace auth {

struct User {
    int id;
    std::string username;
    std::string password_hash;  // 存储密码的哈希值
    std::string salt;          // 密码加盐
    std::string role;          // 用户角色
    bool is_active;            // 用户状态
    std::string last_login;    // 最后登录时间
};

} // namespace auth
} // namespace server 