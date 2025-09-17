#pragma once
#include <string>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace httplib {

struct Request {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> params;
    std::string body;
    
    std::string get_param_value(const std::string& key) const {
        auto it = params.find(key);
        return it != params.end() ? it->second : "";
    }
    
    std::string get_header_value(const std::string& key) const {
        auto it = headers.find(key);
        return it != headers.end() ? it->second : "";
    }
};

struct Response {
    int status = 200;
    std::map<std::string, std::string> headers;
    std::string body;
    
    void set_content(const std::string& content, const std::string& content_type) {
        body = content;
        headers["Content-Type"] = content_type;
        headers["Content-Length"] = std::to_string(content.length());
    }
    
    void set_header(const std::string& key, const std::string& value) {
        headers[key] = value;
    }
};

class Server {
public:
    using Handler = std::function<void(const Request&, Response&)>;
    
    Server() {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }
    
    ~Server() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    Server& Get(const std::string& pattern, Handler handler) {
        routes_["GET:" + pattern] = handler;
        return *this;
    }
    
    Server& Post(const std::string& pattern, Handler handler) {
        routes_["POST:" + pattern] = handler;
        return *this;
    }
    
    Server& Put(const std::string& pattern, Handler handler) {
        routes_["PUT:" + pattern] = handler;
        return *this;
    }
    
    Server& Delete(const std::string& pattern, Handler handler) {
        routes_["DELETE:" + pattern] = handler;
        return *this;
    }
    
    bool listen(const std::string& host, int port) {
        int server_fd;
        struct sockaddr_in address;
        int opt = 1;
        int addrlen = sizeof(address);
        
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            return false;
        }
        
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
            return false;
        }
        
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            return false;
        }
        
        if (::listen(server_fd, 3) < 0) {
            return false;
        }
        
        std::cout << "Server listening on " << host << ":" << port << std::endl;
        
        while (true) {
            int new_socket;
            if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
                continue;
            }
            
            std::thread([this, new_socket]() {
                handle_request(new_socket);
            }).detach();
        }
        
        return true;
    }
    
private:
    std::map<std::string, Handler> routes_;
    
    void handle_request(int socket) {
        char buffer[4096] = {0};
        recv(socket, buffer, 4096, 0);
        
        Request req = parse_request(buffer);
        Response res;
        
        // Add CORS headers
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        
        if (req.method == "OPTIONS") {
            res.status = 200;
            res.set_content("", "text/plain");
        } else {
            std::string route_key = req.method + ":" + req.path;
            auto it = routes_.find(route_key);
            
            if (it != routes_.end()) {
                it->second(req, res);
            } else {
                // Check for parameterized routes
                bool found = false;
                for (const auto& route : routes_) {
                    if (route.first.substr(0, req.method.length() + 1) == req.method + ":") {
                        std::string pattern = route.first.substr(req.method.length() + 1);
                        if (match_route(pattern, req.path, req)) {
                            route.second(req, res);
                            found = true;
                            break;
                        }
                    }
                }
                
                if (!found) {
                    res.status = 404;
                    res.set_content("Not Found", "text/plain");
                }
            }
        }
        
        send_response(socket, res);
        
#ifdef _WIN32
        closesocket(socket);
#else
        close(socket);
#endif
    }
    
    Request parse_request(const std::string& raw_request) {
        Request req;
        std::istringstream stream(raw_request);
        std::string line;
        
        // Parse request line
        if (std::getline(stream, line)) {
            std::istringstream line_stream(line);
            std::string path_with_params;
            line_stream >> req.method >> path_with_params;
            
            // Parse path and query parameters
            size_t query_pos = path_with_params.find('?');
            if (query_pos != std::string::npos) {
                req.path = path_with_params.substr(0, query_pos);
                std::string query = path_with_params.substr(query_pos + 1);
                parse_query_params(query, req.params);
            } else {
                req.path = path_with_params;
            }
        }
        
        // Parse headers
        while (std::getline(stream, line) && line != "\r") {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 2);
                if (value.back() == '\r') value.pop_back();
                req.headers[key] = value;
            }
        }
        
        // Parse body
        std::string body_line;
        while (std::getline(stream, body_line)) {
            req.body += body_line + "\n";
        }
        if (!req.body.empty()) {
            req.body.pop_back(); // Remove last newline
        }
        
        return req;
    }
    
    void parse_query_params(const std::string& query, std::map<std::string, std::string>& params) {
        std::istringstream stream(query);
        std::string param;
        
        while (std::getline(stream, param, '&')) {
            size_t eq_pos = param.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = param.substr(0, eq_pos);
                std::string value = param.substr(eq_pos + 1);
                params[key] = url_decode(value);
            }
        }
    }
    
    bool match_route(const std::string& pattern, const std::string& path, Request& req) {
        if (pattern.find(':') == std::string::npos) {
            return pattern == path;
        }
        
        std::vector<std::string> pattern_parts = split(pattern, '/');
        std::vector<std::string> path_parts = split(path, '/');
        
        if (pattern_parts.size() != path_parts.size()) {
            return false;
        }
        
        for (size_t i = 0; i < pattern_parts.size(); ++i) {
            if (pattern_parts[i][0] == ':') {
                std::string param_name = pattern_parts[i].substr(1);
                req.params[param_name] = path_parts[i];
            } else if (pattern_parts[i] != path_parts[i]) {
                return false;
            }
        }
        
        return true;
    }
    
    std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> parts;
        std::istringstream stream(str);
        std::string part;
        
        while (std::getline(stream, part, delimiter)) {
            if (!part.empty()) {
                parts.push_back(part);
            }
        }
        
        return parts;
    }
    
    std::string url_decode(const std::string& str) {
        std::string result;
        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '%' && i + 2 < str.length()) {
                int hex_value;
                std::istringstream hex_stream(str.substr(i + 1, 2));
                hex_stream >> std::hex >> hex_value;
                result += static_cast<char>(hex_value);
                i += 2;
            } else if (str[i] == '+') {
                result += ' ';
            } else {
                result += str[i];
            }
        }
        return result;
    }
    
    void send_response(int socket, const Response& res) {
        std::ostringstream response_stream;
        response_stream << "HTTP/1.1 " << res.status << " OK\r\n";
        
        for (const auto& header : res.headers) {
            response_stream << header.first << ": " << header.second << "\r\n";
        }
        
        response_stream << "\r\n" << res.body;
        
        std::string response = response_stream.str();
        send(socket, response.c_str(), response.length(), 0);
    }
};

} // namespace httplib
