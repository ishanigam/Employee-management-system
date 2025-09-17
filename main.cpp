#include "../lib/simple_httplib.h"
#include "database.h"
#include "auth.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

class EmployeeServer {
private:
    Database db_;
    AuthManager auth_;
    httplib::Server server_;
    
public:
    EmployeeServer() : db_("data"), auth_("data") {
        setupRoutes();
    }
    
    void setupRoutes() {
        // Authentication routes
        server_.Post("/api/auth/login", [this](const httplib::Request& req, httplib::Response& res) {
            handleLogin(req, res);
        });
        
        server_.Post("/api/auth/logout", [this](const httplib::Request& req, httplib::Response& res) {
            handleLogout(req, res);
        });
        
        // Employee CRUD routes
        server_.Get("/api/employees", [this](const httplib::Request& req, httplib::Response& res) {
            handleGetEmployees(req, res);
        });
        
        server_.Get("/api/employees/:id", [this](const httplib::Request& req, httplib::Response& res) {
            handleGetEmployee(req, res);
        });
        
        server_.Post("/api/employees", [this](const httplib::Request& req, httplib::Response& res) {
            handleCreateEmployee(req, res);
        });
        
        server_.Put("/api/employees/:id", [this](const httplib::Request& req, httplib::Response& res) {
            handleUpdateEmployee(req, res);
        });
        
        server_.Delete("/api/employees/:id", [this](const httplib::Request& req, httplib::Response& res) {
            handleDeleteEmployee(req, res);
        });
        
        // Search and filter routes
        server_.Get("/api/employees/search", [this](const httplib::Request& req, httplib::Response& res) {
            handleSearchEmployees(req, res);
        });
        
        // Utility routes
        server_.Get("/api/departments", [this](const httplib::Request& req, httplib::Response& res) {
            handleGetDepartments(req, res);
        });
        
        server_.Get("/api/positions", [this](const httplib::Request& req, httplib::Response& res) {
            handleGetPositions(req, res);
        });
        
        // File operations
        server_.Post("/api/employees/upload", [this](const httplib::Request& req, httplib::Response& res) {
            handleFileUpload(req, res);
        });
        
        server_.Get("/api/employees/export", [this](const httplib::Request& req, httplib::Response& res) {
            handleExportCsv(req, res);
        });
        
        server_.Post("/api/employees/import", [this](const httplib::Request& req, httplib::Response& res) {
            handleImportCsv(req, res);
        });
        
        // Serve static files
        server_.Get("/uploads/.*", [this](const httplib::Request& req, httplib::Response& res) {
            handleStaticFile(req, res);
        });
    }
    
    bool start(const std::string& host = "localhost", int port = 8080) {
        std::cout << "Starting Employee Management Server..." << std::endl;
        std::cout << "Server will be available at http://" << host << ":" << port << std::endl;
        return server_.listen(host, port);
    }
    
private:
    bool isAuthenticated(const httplib::Request& req) {
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            return false;
        }
        
        std::string token = authHeader.substr(7);
        return auth_.validateToken(token);
    }
    
    void sendError(httplib::Response& res, int status, const std::string& message) {
        res.status = status;
        res.set_content("{\"error\":\"" + message + "\"}", "application/json");
    }
    
    void sendSuccess(httplib::Response& res, const std::string& data) {
        res.status = 200;
        res.set_content(data, "application/json");
    }
    
    void handleLogin(const httplib::Request& req, httplib::Response& res) {
        try {
            // Parse JSON body (simple parsing)
            std::string body = req.body;
            std::string username, password;
            
            // Extract username and password from JSON
            size_t userPos = body.find("\"username\":");
            size_t passPos = body.find("\"password\":");
            
            if (userPos != std::string::npos && passPos != std::string::npos) {
                // Extract username
                size_t userStart = body.find("\"", userPos + 11) + 1;
                size_t userEnd = body.find("\"", userStart);
                username = body.substr(userStart, userEnd - userStart);
                
                // Extract password
                size_t passStart = body.find("\"", passPos + 11) + 1;
                size_t passEnd = body.find("\"", passStart);
                password = body.substr(passStart, passEnd - passStart);
            }
            
            if (username.empty() || password.empty()) {
                sendError(res, 400, "Username and password required");
                return;
            }
            
            std::string token = auth_.login(username, password);
            if (token.empty()) {
                sendError(res, 401, "Invalid credentials");
                return;
            }
            
            sendSuccess(res, "{\"token\":\"" + token + "\",\"message\":\"Login successful\"}");
            
        } catch (const std::exception& e) {
            sendError(res, 500, "Internal server error");
        }
    }
    
    void handleLogout(const httplib::Request& req, httplib::Response& res) {
        std::string authHeader = req.get_header_value("Authorization");
        if (!authHeader.empty() && authHeader.substr(0, 7) == "Bearer ") {
            std::string token = authHeader.substr(7);
            auth_.logout(token);
        }
        
        sendSuccess(res, "{\"message\":\"Logout successful\"}");
    }
    
    void handleGetEmployees(const httplib::Request& req, httplib::Response& res) {
        if (!isAuthenticated(req)) {
            sendError(res, 401, "Authentication required");
            return;
        }
        
        try {
            int page = std::stoi(req.get_param_value("page").empty() ? "1" : req.get_param_value("page"));
            int pageSize = std::stoi(req.get_param_value("pageSize").empty() ? "10" : req.get_param_value("pageSize"));
            
            std::vector<Employee> employees = db_.getAllEmployees(page, pageSize);
            int totalCount = db_.getTotalEmployeeCount();
            
            std::ostringstream json;
            json << "{\"employees\":[";
            for (size_t i = 0; i < employees.size(); ++i) {
                json << employees[i].toJson();
                if (i < employees.size() - 1) json << ",";
            }
            json << "],\"totalCount\":" << totalCount;
            json << ",\"page\":" << page;
            json << ",\"pageSize\":" << pageSize << "}";
            
            sendSuccess(res, json.str());
            
        } catch (const std::exception& e) {
            sendError(res, 500, "Internal server error");
        }
    }
    
    void handleGetEmployee(const httplib::Request& req, httplib::Response& res) {
        if (!isAuthenticated(req)) {
            sendError(res, 401, "Authentication required");
            return;
        }
        
        try {
            int id = std::stoi(req.get_param_value("id"));
            Employee emp = db_.getEmployee(id);
            
            if (emp.id == 0) {
                sendError(res, 404, "Employee not found");
                return;
            }
            
            sendSuccess(res, emp.toJson());
            
        } catch (const std::exception& e) {
            sendError(res, 400, "Invalid employee ID");
        }
    }
    
    void handleCreateEmployee(const httplib::Request& req, httplib::Response& res) {
        if (!isAuthenticated(req)) {
            sendError(res, 401, "Authentication required");
            return;
        }
        
        try {
            Employee emp = Employee::fromJson(req.body);
            
            // Validation
            if (emp.firstName.empty() || emp.lastName.empty() || emp.email.empty()) {
                sendError(res, 400, "First name, last name, and email are required");
                return;
            }
            
            if (db_.createEmployee(emp)) {
                sendSuccess(res, "{\"message\":\"Employee created successfully\"}");
            } else {
                sendError(res, 500, "Failed to create employee");
            }
            
        } catch (const std::exception& e) {
            sendError(res, 400, "Invalid employee data");
        }
    }
    
    void handleUpdateEmployee(const httplib::Request& req, httplib::Response& res) {
        if (!isAuthenticated(req)) {
            sendError(res, 401, "Authentication required");
            return;
        }
        
        try {
            int id = std::stoi(req.get_param_value("id"));
            Employee emp = Employee::fromJson(req.body);
            
            if (db_.updateEmployee(id, emp)) {
                sendSuccess(res, "{\"message\":\"Employee updated successfully\"}");
            } else {
                sendError(res, 404, "Employee not found");
            }
            
        } catch (const std::exception& e) {
            sendError(res, 400, "Invalid employee data");
        }
    }
    
    void handleDeleteEmployee(const httplib::Request& req, httplib::Response& res) {
        if (!isAuthenticated(req)) {
            sendError(res, 401, "Authentication required");
            return;
        }
        
        try {
            int id = std::stoi(req.get_param_value("id"));
            
            if (db_.deleteEmployee(id)) {
                sendSuccess(res, "{\"message\":\"Employee deleted successfully\"}");
            } else {
                sendError(res, 404, "Employee not found");
            }
            
        } catch (const std::exception& e) {
            sendError(res, 400, "Invalid employee ID");
        }
    }
    
    void handleSearchEmployees(const httplib::Request& req, httplib::Response& res) {
        if (!isAuthenticated(req)) {
            sendError(res, 401, "Authentication required");
            return;
        }
        
        try {
            std::string query = req.get_param_value("q");
            std::string department = req.get_param_value("department");
            std::string position = req.get_param_value("position");
            double minSalary = req.get_param_value("minSalary").empty() ? 0.0 : std::stod(req.get_param_value("minSalary"));
            double maxSalary = req.get_param_value("maxSalary").empty() ? 0.0 : std::stod(req.get_param_value("maxSalary"));
            int page = std::stoi(req.get_param_value("page").empty() ? "1" : req.get_param_value("page"));
            int pageSize = std::stoi(req.get_param_value("pageSize").empty() ? "10" : req.get_param_value("pageSize"));
            
            std::vector<Employee> employees;
            
            if (!query.empty()) {
                employees = db_.searchEmployees(query, page, pageSize);
            } else {
                employees = db_.filterEmployees(department, position, minSalary, maxSalary, page, pageSize);
            }
            
            std::ostringstream json;
            json << "{\"employees\":[";
            for (size_t i = 0; i < employees.size(); ++i) {
                json << employees[i].toJson();
                if (i < employees.size() - 1) json << ",";
            }
            json << "],\"page\":" << page;
            json << ",\"pageSize\":" << pageSize << "}";
            
            sendSuccess(res, json.str());
            
        } catch (const std::exception& e) {
            sendError(res, 500, "Internal server error");
        }
    }
    
    void handleGetDepartments(const httplib::Request& req, httplib::Response& res) {
        if (!isAuthenticated(req)) {
            sendError(res, 401, "Authentication required");
            return;
        }
        
        std::vector<std::string> departments = db_.getDepartments();
        
        std::ostringstream json;
        json << "{\"departments\":[";
        for (size_t i = 0; i < departments.size(); ++i) {
            json << "\"" << departments[i] << "\"";
            if (i < departments.size() - 1) json << ",";
        }
        json << "]}";
        
        sendSuccess(res, json.str());
    }
    
    void handleGetPositions(const httplib::Request& req, httplib::Response& res) {
        if (!isAuthenticated(req)) {
            sendError(res, 401, "Authentication required");
            return;
        }
        
        std::vector<std::string> positions = db_.getPositions();
        
        std::ostringstream json;
        json << "{\"positions\":[";
        for (size_t i = 0; i < positions.size(); ++i) {
            json << "\"" << positions[i] << "\"";
            if (i < positions.size() - 1) json << ",";
        }
        json << "]}";
        
        sendSuccess(res, json.str());
    }
    
    void handleFileUpload(const httplib::Request& req, httplib::Response& res) {
        if (!isAuthenticated(req)) {
            sendError(res, 401, "Authentication required");
            return;
        }
        
        // Simple file upload handling
        std::string filename = "profile_" + std::to_string(std::time(nullptr)) + ".jpg";
        std::string filepath = "data/uploads/" + filename;
        
        std::ofstream file(filepath, std::ios::binary);
        if (file.is_open()) {
            file.write(req.body.c_str(), req.body.length());
            file.close();
            
            sendSuccess(res, "{\"filename\":\"" + filename + "\",\"path\":\"/uploads/" + filename + "\"}");
        } else {
            sendError(res, 500, "Failed to save file");
        }
    }
    
    void handleExportCsv(const httplib::Request& req, httplib::Response& res) {
        if (!isAuthenticated(req)) {
            sendError(res, 401, "Authentication required");
            return;
        }
        
        std::string filename = "employees_export_" + std::to_string(std::time(nullptr)) + ".csv";
        
        if (db_.exportToCsv(filename)) {
            res.set_header("Content-Disposition", "attachment; filename=" + filename);
            res.set_header("Content-Type", "text/csv");
            
            std::ifstream file("data/" + filename);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                file.close();
                res.set_content(content, "text/csv");
            } else {
                sendError(res, 500, "Failed to read export file");
            }
        } else {
            sendError(res, 500, "Failed to export data");
        }
    }
    
    void handleImportCsv(const httplib::Request& req, httplib::Response& res) {
        if (!isAuthenticated(req)) {
            sendError(res, 401, "Authentication required");
            return;
        }
        
        std::string filename = "import_" + std::to_string(std::time(nullptr)) + ".csv";
        std::string filepath = "data/" + filename;
        
        std::ofstream file(filepath);
        if (file.is_open()) {
            file.write(req.body.c_str(), req.body.length());
            file.close();
            
            if (db_.importFromCsv(filepath)) {
                std::filesystem::remove(filepath); // Clean up temp file
                sendSuccess(res, "{\"message\":\"Data imported successfully\"}");
            } else {
                sendError(res, 500, "Failed to import data");
            }
        } else {
            sendError(res, 500, "Failed to save import file");
        }
    }
    
    void handleStaticFile(const httplib::Request& req, httplib::Response& res) {
        std::string filepath = "data" + req.path;
        
        std::ifstream file(filepath, std::ios::binary);
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            file.close();
            
            // Determine content type based on file extension
            std::string contentType = "application/octet-stream";
            if (filepath.ends_with(".jpg") || filepath.ends_with(".jpeg")) {
                contentType = "image/jpeg";
            } else if (filepath.ends_with(".png")) {
                contentType = "image/png";
            } else if (filepath.ends_with(".gif")) {
                contentType = "image/gif";
            }
            
            res.set_content(content, contentType);
        } else {
            sendError(res, 404, "File not found");
        }
    }
};

int main() {
    EmployeeServer server;
    
    if (!server.start("localhost", 8080)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    return 0;
}
