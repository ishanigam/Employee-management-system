# Employee Management System

A full-stack Employee Management System with C++ REST backend and vanilla JavaScript frontend.

## Features

- **Authentication**: Simple username/password login system
- **CRUD Operations**: Create, Read, Update, Delete employees
- **Search & Filter**: Binary search implementation with sorting
- **Pagination**: Server-side pagination for efficient data handling
- **File Upload**: Profile photo upload with path storage
- **Data Export/Import**: CSV export and import functionality
- **Responsive Design**: Mobile-friendly HTML/CSS interface
- **Data Structures**: Efficient in-memory structures with DSA algorithms

## Tech Stack

### Backend
- **Language**: C++ 17
- **HTTP Library**: cpp-httplib (single-file header-only library)
- **Data Storage**: JSON and CSV files
- **Algorithms**: Binary search, merge sort, quick sort

### Frontend
- **HTML5**: Semantic markup with responsive design
- **CSS3**: Modern styling with flexbox/grid
- **JavaScript**: Vanilla ES6+ (no frameworks)

## Project Structure

```
employee-management-system/
├── backend/
│   ├── src/
│   │   ├── main.cpp
│   │   ├── employee.h
│   │   ├── employee.cpp
│   │   ├── auth.h
│   │   ├── auth.cpp
│   │   ├── database.h
│   │   └── database.cpp
│   ├── lib/
│   │   └── httplib.h
│   ├── data/
│   │   ├── employees.json
│   │   ├── users.json
│   │   └── seed_data.csv
│   └── uploads/
├── frontend/
│   ├── index.html
│   ├── login.html
│   ├── css/
│   │   └── styles.css
│   ├── js/
│   │   ├── main.js
│   │   ├── auth.js
│   │   └── employee.js
│   └── assets/
└── build/
```

## Build Instructions

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Make or CMake (optional)

### Building the Backend

#### Windows (MSVC)
```bash
cd backend
cl /EHsc /std:c++17 src/main.cpp src/employee.cpp src/auth.cpp src/database.cpp /Fe:employee_server.exe
```

#### Linux/macOS (GCC/Clang)
```bash
cd backend
g++ -std=c++17 -pthread -o employee_server src/main.cpp src/employee.cpp src/auth.cpp src/database.cpp
```

### Running the Application

1. Start the backend server:
```bash
cd backend
./employee_server
```

2. Open frontend in browser:
```bash
# Navigate to frontend/index.html or serve via HTTP server
cd frontend
python -m http.server 8080  # Python 3
# or
python -m SimpleHTTPServer 8080  # Python 2
```

3. Access the application at `http://localhost:8080`

## API Endpoints

- `POST /api/auth/login` - User authentication
- `GET /api/employees` - Get employees (with pagination)
- `POST /api/employees` - Create new employee
- `GET /api/employees/:id` - Get employee by ID
- `PUT /api/employees/:id` - Update employee
- `DELETE /api/employees/:id` - Delete employee
- `GET /api/employees/search` - Search employees
- `POST /api/employees/upload` - Upload profile photo
- `GET /api/employees/export` - Export to CSV
- `POST /api/employees/import` - Import from CSV

## Default Credentials

- Username: `admin`
- Password: `admin123`

## Sample Data

The system comes with 10 sample employees with various departments and roles for testing purposes.
