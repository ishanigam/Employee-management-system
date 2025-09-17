#pragma once
#include <string>
#include <map>
#include <vector>

struct User {
    std::string username;
    std::string passwordHash;
    std::string role;
    bool active;
    
    User() : active(true) {}
    User(const std::string& username, const std::string& passwordHash, 
         const std::string& role = "admin", bool active = true)
        : username(username), passwordHash(passwordHash), role(role), active(active) {}
    
    std::string toJson() const;
    static User fromJson(const std::string& json);
};

class AuthManager {
public:
    AuthManager(const std::string& dataDir = "data");
    ~AuthManager();
    
    // Authentication
    std::string login(const std::string& username, const std::string& password);
    bool logout(const std::string& token);
    bool validateToken(const std::string& token);
    std::string getUserFromToken(const std::string& token);
    
    // User management
    bool createUser(const std::string& username, const std::string& password, const std::string& role = "admin");
    bool changePassword(const std::string& username, const std::string& oldPassword, const std::string& newPassword);
    
    // Token management
    std::string generateToken(const std::string& username);
    bool isTokenExpired(const std::string& token);
    
private:
    std::map<std::string, User> users_;
    std::map<std::string, std::pair<std::string, long long>> tokens_; // token -> (username, expiry)
    std::string dataDirectory_;
    std::string usersFilePath_;
    
    // Helper methods
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
    std::string generateRandomString(int length);
    long long getCurrentTimestamp();
    bool loadUsers();
    bool saveUsers();
    void initializeDefaultUser();
};
