#include "auth.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <chrono>
#include <filesystem>

// Simple hash function (in production, use bcrypt or similar)
#include <functional>

std::string User::toJson() const {
    std::ostringstream json;
    json << "{"
         << "\"username\":\"" << username << "\","
         << "\"passwordHash\":\"" << passwordHash << "\","
         << "\"role\":\"" << role << "\","
         << "\"active\":" << (active ? "true" : "false")
         << "}";
    return json.str();
}

User User::fromJson(const std::string& json) {
    User user;
    std::string cleanJson = json;
    
    // Remove braces and quotes
    cleanJson.erase(std::remove(cleanJson.begin(), cleanJson.end(), '{'), cleanJson.end());
    cleanJson.erase(std::remove(cleanJson.begin(), cleanJson.end(), '}'), cleanJson.end());
    cleanJson.erase(std::remove(cleanJson.begin(), cleanJson.end(), '\"'), cleanJson.end());
    
    std::istringstream stream(cleanJson);
    std::string pair;
    
    while (std::getline(stream, pair, ',')) {
        size_t colonPos = pair.find(':');
        if (colonPos != std::string::npos) {
            std::string key = pair.substr(0, colonPos);
            std::string value = pair.substr(colonPos + 1);
            
            if (key == "username") user.username = value;
            else if (key == "passwordHash") user.passwordHash = value;
            else if (key == "role") user.role = value;
            else if (key == "active") user.active = (value == "true");
        }
    }
    
    return user;
}

AuthManager::AuthManager(const std::string& dataDir) : dataDirectory_(dataDir) {
    usersFilePath_ = dataDirectory_ + "/users.json";
    std::filesystem::create_directories(dataDirectory_);
    loadUsers();
    initializeDefaultUser();
}

AuthManager::~AuthManager() {
    saveUsers();
}

std::string AuthManager::login(const std::string& username, const std::string& password) {
    auto userIt = users_.find(username);
    if (userIt == users_.end() || !userIt->second.active) {
        return ""; // User not found or inactive
    }
    
    if (verifyPassword(password, userIt->second.passwordHash)) {
        std::string token = generateToken(username);
        return token;
    }
    
    return ""; // Invalid password
}

bool AuthManager::logout(const std::string& token) {
    auto tokenIt = tokens_.find(token);
    if (tokenIt != tokens_.end()) {
        tokens_.erase(tokenIt);
        return true;
    }
    return false;
}

bool AuthManager::validateToken(const std::string& token) {
    auto tokenIt = tokens_.find(token);
    if (tokenIt == tokens_.end()) {
        return false;
    }
    
    if (isTokenExpired(token)) {
        tokens_.erase(tokenIt);
        return false;
    }
    
    return true;
}

std::string AuthManager::getUserFromToken(const std::string& token) {
    auto tokenIt = tokens_.find(token);
    if (tokenIt != tokens_.end() && !isTokenExpired(token)) {
        return tokenIt->second.first;
    }
    return "";
}

bool AuthManager::createUser(const std::string& username, const std::string& password, const std::string& role) {
    if (users_.find(username) != users_.end()) {
        return false; // User already exists
    }
    
    std::string hashedPassword = hashPassword(password);
    users_[username] = User(username, hashedPassword, role);
    
    return saveUsers();
}

bool AuthManager::changePassword(const std::string& username, const std::string& oldPassword, const std::string& newPassword) {
    auto userIt = users_.find(username);
    if (userIt == users_.end()) {
        return false;
    }
    
    if (!verifyPassword(oldPassword, userIt->second.passwordHash)) {
        return false;
    }
    
    userIt->second.passwordHash = hashPassword(newPassword);
    return saveUsers();
}

std::string AuthManager::generateToken(const std::string& username) {
    std::string token = generateRandomString(32);
    long long expiry = getCurrentTimestamp() + (24 * 60 * 60 * 1000); // 24 hours
    
    tokens_[token] = std::make_pair(username, expiry);
    return token;
}

bool AuthManager::isTokenExpired(const std::string& token) {
    auto tokenIt = tokens_.find(token);
    if (tokenIt == tokens_.end()) {
        return true;
    }
    
    return getCurrentTimestamp() > tokenIt->second.second;
}

std::string AuthManager::hashPassword(const std::string& password) {
    // Simple hash (in production, use proper password hashing like bcrypt)
    std::hash<std::string> hasher;
    return std::to_string(hasher(password + "salt_string"));
}

bool AuthManager::verifyPassword(const std::string& password, const std::string& hash) {
    return hashPassword(password) == hash;
}

std::string AuthManager::generateRandomString(int length) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    
    std::string result;
    for (int i = 0; i < length; ++i) {
        result += chars[dis(gen)];
    }
    
    return result;
}

long long AuthManager::getCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool AuthManager::loadUsers() {
    std::ifstream file(usersFilePath_);
    if (!file.is_open()) {
        return true; // File doesn't exist, start with empty users
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    if (content.empty() || content == "[]") {
        return true;
    }
    
    // Simple JSON array parsing
    if (content.front() == '[') content.erase(0, 1);
    if (content.back() == ']') content.pop_back();
    
    std::string currentObject;
    int braceCount = 0;
    
    for (char c : content) {
        currentObject += c;
        
        if (c == '{') {
            braceCount++;
        } else if (c == '}') {
            braceCount--;
            
            if (braceCount == 0) {
                try {
                    User user = User::fromJson(currentObject);
                    users_[user.username] = user;
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing user JSON: " << currentObject << std::endl;
                }
                currentObject.clear();
            }
        }
    }
    
    return true;
}

bool AuthManager::saveUsers() {
    std::ofstream file(usersFilePath_);
    if (!file.is_open()) {
        return false;
    }
    
    file << "[";
    bool first = true;
    for (const auto& userPair : users_) {
        if (!first) {
            file << ",";
        }
        file << userPair.second.toJson();
        first = false;
    }
    file << "]";
    
    file.close();
    return true;
}

void AuthManager::initializeDefaultUser() {
    if (users_.empty()) {
        createUser("admin", "admin123", "admin");
        std::cout << "Created default admin user - Username: admin, Password: admin123" << std::endl;
    }
}
