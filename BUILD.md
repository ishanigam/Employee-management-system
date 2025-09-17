# Build Instructions

## Prerequisites

### For Windows:
- **Visual Studio 2019 or later** with C++ development tools
- **Windows SDK** (usually included with Visual Studio)
- **Git** (for cloning dependencies if needed)

### For Linux/macOS:
- **GCC 7+** or **Clang 5+** with C++17 support
- **Make** or **CMake** (optional)
- **pthread** library (usually included)

## Building the Backend

### Windows (Visual Studio Developer Command Prompt)

1. Open **Developer Command Prompt for VS 2019** (or later)

2. Navigate to the backend directory:
```cmd
cd employee-management-system\backend
```

3. Compile the server:
```cmd
cl /EHsc /std:c++17 /I"lib" src\main.cpp src\employee.cpp src\auth.cpp src\database.cpp /Fe:employee_server.exe
```

4. If you encounter linking errors, try:
```cmd
cl /EHsc /std:c++17 /I"lib" src\main.cpp src\employee.cpp src\auth.cpp src\database.cpp /Fe:employee_server.exe ws2_32.lib
```

### Windows (MinGW-w64)

1. Install MinGW-w64 from https://www.mingw-w64.org/

2. Add MinGW-w64 bin directory to your PATH

3. Navigate to the backend directory:
```cmd
cd employee-management-system\backend
```

4. Compile:
```cmd
g++ -std=c++17 -I"lib" -pthread -o employee_server.exe src\main.cpp src\employee.cpp src\auth.cpp src\database.cpp -lws2_32
```

### Linux

1. Navigate to the backend directory:
```bash
cd employee-management-system/backend
```

2. Compile:
```bash
g++ -std=c++17 -Ilib -pthread -o employee_server src/main.cpp src/employee.cpp src/auth.cpp src/database.cpp
```

3. Make executable:
```bash
chmod +x employee_server
```

### macOS

1. Install Xcode Command Line Tools:
```bash
xcode-select --install
```

2. Navigate to the backend directory:
```bash
cd employee-management-system/backend
```

3. Compile:
```bash
clang++ -std=c++17 -Ilib -pthread -o employee_server src/main.cpp src/employee.cpp src/auth.cpp src/database.cpp
```

## Running the Application

### 1. Start the Backend Server

#### Windows:
```cmd
cd employee-management-system\backend
employee_server.exe
```

#### Linux/macOS:
```bash
cd employee-management-system/backend
./employee_server
```

The server will start on `http://localhost:8080` and display:
```
Starting Employee Management Server...
Server will be available at http://localhost:8080
Created default admin user - Username: admin, Password: admin123
```

### 2. Serve the Frontend

You have several options to serve the frontend:

#### Option 1: Python HTTP Server (Recommended)
```bash
cd employee-management-system/frontend
python -m http.server 3000
```
Then open: http://localhost:3000

#### Option 2: Node.js HTTP Server
```bash
cd employee-management-system/frontend
npx http-server -p 3000
```
Then open: http://localhost:3000

#### Option 3: PHP Built-in Server
```bash
cd employee-management-system/frontend
php -S localhost:3000
```
Then open: http://localhost:3000

#### Option 4: Direct File Access (Not Recommended)
You can open `frontend/login.html` directly in your browser, but this may cause CORS issues.

## Default Login Credentials

- **Username:** `admin`
- **Password:** `admin123`

## Importing Sample Data

1. Log into the application
2. Go to the "Employees" section
3. Click "Import CSV"
4. Select the file: `backend/data/seed_data.csv`
5. The system will import 10 sample employees

## Troubleshooting

### Backend Issues

**Error: "Failed to start server"**
- Check if port 8080 is already in use
- Try running as administrator (Windows) or with sudo (Linux/macOS)
- Check firewall settings

**Compilation Errors:**
- Ensure you have C++17 support
- Check that all source files are present
- Verify include paths are correct

**Runtime Errors:**
- Check that the `data` directory is writable
- Ensure proper file permissions

### Frontend Issues

**CORS Errors:**
- Don't open HTML files directly in browser
- Use an HTTP server to serve the frontend
- Ensure backend is running on localhost:8080

**Login Issues:**
- Verify backend server is running
- Check browser developer console for errors
- Ensure correct API endpoint URLs

**File Upload Issues:**
- Check that `backend/data/uploads` directory exists
- Verify proper file permissions
- Ensure backend has write access to uploads directory

## Development Notes

### Code Structure

**Backend (C++):**
- `main.cpp` - HTTP server and API endpoints
- `employee.h/cpp` - Employee data structure and DSA algorithms
- `database.h/cpp` - File-based database operations
- `auth.h/cpp` - Authentication and user management
- `simple_httplib.h` - Lightweight HTTP server implementation

**Frontend (JavaScript):**
- `auth.js` - Authentication management
- `main.js` - Main application logic
- `employee.js` - Employee-specific utilities
- `styles.css` - Responsive CSS styling

### Data Storage

- **Employee data:** `backend/data/employees.json`
- **User data:** `backend/data/users.json`
- **File uploads:** `backend/data/uploads/`
- **CSV exports:** `backend/data/`

### API Endpoints

All endpoints require authentication except `/api/auth/login`:

- `POST /api/auth/login` - User login
- `POST /api/auth/logout` - User logout
- `GET /api/employees` - Get employees (paginated)
- `POST /api/employees` - Create employee
- `GET /api/employees/:id` - Get employee by ID
- `PUT /api/employees/:id` - Update employee
- `DELETE /api/employees/:id` - Delete employee
- `GET /api/employees/search` - Search employees
- `GET /api/departments` - Get all departments
- `GET /api/positions` - Get all positions
- `POST /api/employees/upload` - Upload profile photo
- `GET /api/employees/export` - Export to CSV
- `POST /api/employees/import` - Import from CSV

### Performance Notes

- Binary search is used for employee lookups (O(log n))
- Quick sort and merge sort implementations for data sorting
- Server-side pagination for large datasets
- In-memory data structures for fast operations
- File-based persistence for data durability
