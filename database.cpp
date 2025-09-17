#include "database.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

Database::Database(const std::string& dataDir) 
    : dataDirectory_(dataDir), nextId_(1) {
    jsonFilePath_ = dataDirectory_ + "/employees.json";
    initializeDataDirectory();
    loadFromFile();
    updateNextId();
}

Database::~Database() {
    saveToFile();
}

void Database::initializeDataDirectory() {
    std::filesystem::create_directories(dataDirectory_);
    std::filesystem::create_directories(dataDirectory_ + "/uploads");
}

bool Database::createEmployee(const Employee& employee) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    Employee newEmployee = employee;
    newEmployee.id = nextId_++;
    
    employees_.push_back(newEmployee);
    
    // Keep employees sorted by ID for binary search
    EmployeeSorter::quickSort(employees_, EmployeeSorter::ID, EmployeeSorter::ASCENDING);
    
    return saveToFile();
}

Employee Database::getEmployee(int id) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    int index = EmployeeSearcher::binarySearchById(employees_, id);
    if (index != -1) {
        return employees_[index];
    }
    
    return Employee(); // Return empty employee if not found
}

std::vector<Employee> Database::getAllEmployees(int page, int pageSize) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<Employee> activeEmployees;
    for (const auto& emp : employees_) {
        if (emp.active) {
            activeEmployees.push_back(emp);
        }
    }
    
    return paginate(activeEmployees, page, pageSize);
}

bool Database::updateEmployee(int id, const Employee& employee) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    int index = EmployeeSearcher::binarySearchById(employees_, id);
    if (index != -1) {
        Employee updatedEmployee = employee;
        updatedEmployee.id = id; // Preserve the ID
        employees_[index] = updatedEmployee;
        return saveToFile();
    }
    
    return false;
}

bool Database::deleteEmployee(int id) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    int index = EmployeeSearcher::binarySearchById(employees_, id);
    if (index != -1) {
        // Soft delete - mark as inactive
        employees_[index].active = false;
        return saveToFile();
    }
    
    return false;
}

std::vector<Employee> Database::searchEmployees(const std::string& query, int page, int pageSize) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<Employee> results = EmployeeSearcher::linearSearch(employees_, query);
    
    // Filter out inactive employees
    results.erase(std::remove_if(results.begin(), results.end(),
                                [](const Employee& emp) { return !emp.active; }),
                 results.end());
    
    return paginate(results, page, pageSize);
}

std::vector<Employee> Database::filterEmployees(const std::string& department,
                                              const std::string& position,
                                              double minSalary,
                                              double maxSalary,
                                              int page, int pageSize) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<Employee> results = EmployeeSearcher::searchWithFilters(
        employees_, "", department, position, minSalary, maxSalary);
    
    return paginate(results, page, pageSize);
}

int Database::getNextId() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return nextId_;
}

int Database::getTotalEmployeeCount() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    int count = 0;
    for (const auto& emp : employees_) {
        if (emp.active) {
            count++;
        }
    }
    
    return count;
}

std::vector<std::string> Database::getDepartments() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<std::string> departments;
    for (const auto& emp : employees_) {
        if (emp.active && std::find(departments.begin(), departments.end(), emp.department) == departments.end()) {
            departments.push_back(emp.department);
        }
    }
    
    std::sort(departments.begin(), departments.end());
    return departments;
}

std::vector<std::string> Database::getPositions() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<std::string> positions;
    for (const auto& emp : employees_) {
        if (emp.active && std::find(positions.begin(), positions.end(), emp.position) == positions.end()) {
            positions.push_back(emp.position);
        }
    }
    
    std::sort(positions.begin(), positions.end());
    return positions;
}

bool Database::exportToCsv(const std::string& filename) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::ofstream file(dataDirectory_ + "/" + filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Write header
    file << Employee::getCsvHeader() << std::endl;
    
    // Write employee data
    for (const auto& emp : employees_) {
        if (emp.active) {
            file << emp.toCsv() << std::endl;
        }
    }
    
    file.close();
    return true;
}

bool Database::importFromCsv(const std::string& filename) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    bool isFirstLine = true;
    
    while (std::getline(file, line)) {
        if (isFirstLine) {
            isFirstLine = false; // Skip header
            continue;
        }
        
        if (!line.empty()) {
            try {
                Employee emp = Employee::fromCsv(line);
                emp.id = nextId_++;
                employees_.push_back(emp);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing CSV line: " << line << std::endl;
            }
        }
    }
    
    file.close();
    
    // Sort employees by ID
    EmployeeSorter::quickSort(employees_, EmployeeSorter::ID, EmployeeSorter::ASCENDING);
    
    return saveToFile();
}

bool Database::loadFromFile() {
    std::ifstream file(jsonFilePath_);
    if (!file.is_open()) {
        // File doesn't exist, start with empty database
        return true;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    if (content.empty() || content == "[]") {
        return true;
    }
    
    // Simple JSON array parsing
    // Remove brackets and split by employee objects
    if (content.front() == '[') content.erase(0, 1);
    if (content.back() == ']') content.pop_back();
    
    std::istringstream stream(content);
    std::string employeeJson;
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
                    Employee emp = Employee::fromJson(currentObject);
                    employees_.push_back(emp);
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing employee JSON: " << currentObject << std::endl;
                }
                currentObject.clear();
            }
        }
    }
    
    // Sort employees by ID for binary search
    EmployeeSorter::quickSort(employees_, EmployeeSorter::ID, EmployeeSorter::ASCENDING);
    
    return true;
}

bool Database::saveToFile() {
    std::ofstream file(jsonFilePath_);
    if (!file.is_open()) {
        return false;
    }
    
    file << "[";
    for (size_t i = 0; i < employees_.size(); ++i) {
        file << employees_[i].toJson();
        if (i < employees_.size() - 1) {
            file << ",";
        }
    }
    file << "]";
    
    file.close();
    return true;
}

void Database::sortEmployees(EmployeeSorter::SortField field, EmployeeSorter::SortOrder order) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    EmployeeSorter::quickSort(employees_, field, order);
}

std::vector<Employee> Database::paginate(const std::vector<Employee>& data, int page, int pageSize) {
    if (page < 1) page = 1;
    if (pageSize < 1) pageSize = 10;
    
    int startIndex = (page - 1) * pageSize;
    int endIndex = std::min(startIndex + pageSize, static_cast<int>(data.size()));
    
    if (startIndex >= data.size()) {
        return std::vector<Employee>();
    }
    
    return std::vector<Employee>(data.begin() + startIndex, data.begin() + endIndex);
}

void Database::updateNextId() {
    if (employees_.empty()) {
        nextId_ = 1;
        return;
    }
    
    int maxId = 0;
    for (const auto& emp : employees_) {
        if (emp.id > maxId) {
            maxId = emp.id;
        }
    }
    
    nextId_ = maxId + 1;
}
