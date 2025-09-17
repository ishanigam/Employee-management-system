#pragma once
#include "employee.h"
#include <vector>
#include <string>
#include <memory>
#include <mutex>

class Database {
public:
    Database(const std::string& dataDir = "data");
    ~Database();
    
    // CRUD operations
    bool createEmployee(const Employee& employee);
    Employee getEmployee(int id);
    std::vector<Employee> getAllEmployees(int page = 1, int pageSize = 10);
    bool updateEmployee(int id, const Employee& employee);
    bool deleteEmployee(int id);
    
    // Search operations
    std::vector<Employee> searchEmployees(const std::string& query, int page = 1, int pageSize = 10);
    std::vector<Employee> filterEmployees(const std::string& department = "",
                                        const std::string& position = "",
                                        double minSalary = 0.0,
                                        double maxSalary = 0.0,
                                        int page = 1, int pageSize = 10);
    
    // Utility operations
    int getNextId();
    int getTotalEmployeeCount();
    std::vector<std::string> getDepartments();
    std::vector<std::string> getPositions();
    
    // File operations
    bool exportToCsv(const std::string& filename);
    bool importFromCsv(const std::string& filename);
    
    // Data management
    bool loadFromFile();
    bool saveToFile();
    
    // Sorting
    void sortEmployees(EmployeeSorter::SortField field, EmployeeSorter::SortOrder order);
    
private:
    std::vector<Employee> employees_;
    std::string dataDirectory_;
    std::string jsonFilePath_;
    std::mutex dataMutex_;
    int nextId_;
    
    // Helper methods
    void initializeDataDirectory();
    std::string readFile(const std::string& filepath);
    bool writeFile(const std::string& filepath, const std::string& content);
    std::vector<Employee> paginate(const std::vector<Employee>& data, int page, int pageSize);
    void updateNextId();
};
