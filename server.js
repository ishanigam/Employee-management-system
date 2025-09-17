const http = require('http');
const fs = require('fs');
const path = require('path');
const url = require('url');

class EmployeeServer {
    constructor() {
        this.employees = [];
        this.users = [{ username: 'admin', password: 'admin123' }];
        this.tokens = new Map();
        this.nextId = 1;
        this.dataDir = './data';
        this.uploadsDir = './data/uploads';
        
        this.ensureDirectories();
        this.loadData();
    }

    ensureDirectories() {
        if (!fs.existsSync(this.dataDir)) {
            fs.mkdirSync(this.dataDir, { recursive: true });
        }
        if (!fs.existsSync(this.uploadsDir)) {
            fs.mkdirSync(this.uploadsDir, { recursive: true });
        }
    }

    loadData() {
        try {
            const employeesPath = path.join(this.dataDir, 'employees.json');
            if (fs.existsSync(employeesPath)) {
                const data = fs.readFileSync(employeesPath, 'utf8');
                this.employees = JSON.parse(data);
                this.nextId = Math.max(...this.employees.map(e => e.id), 0) + 1;
            }
        } catch (error) {
            console.log('No existing data found, starting fresh');
        }
    }

    saveData() {
        try {
            const employeesPath = path.join(this.dataDir, 'employees.json');
            fs.writeFileSync(employeesPath, JSON.stringify(this.employees, null, 2));
        } catch (error) {
            console.error('Error saving data:', error);
        }
    }

    generateToken() {
        return Math.random().toString(36).substring(2) + Date.now().toString(36);
    }

    isAuthenticated(req) {
        const auth = req.headers.authorization;
        if (!auth || !auth.startsWith('Bearer ')) return false;
        
        const token = auth.substring(7);
        return this.tokens.has(token);
    }

    handleCORS(res) {
        res.setHeader('Access-Control-Allow-Origin', '*');
        res.setHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
        res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');
    }

    sendJSON(res, data, status = 200) {
        this.handleCORS(res);
        res.writeHead(status, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify(data));
    }

    sendError(res, message, status = 400) {
        this.sendJSON(res, { error: message }, status);
    }

    handleLogin(req, res) {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
            try {
                const { username, password } = JSON.parse(body);
                const user = this.users.find(u => u.username === username && u.password === password);
                
                if (user) {
                    const token = this.generateToken();
                    this.tokens.set(token, username);
                    this.sendJSON(res, { token, message: 'Login successful' });
                } else {
                    this.sendError(res, 'Invalid credentials', 401);
                }
            } catch (error) {
                this.sendError(res, 'Invalid JSON', 400);
            }
        });
    }

    handleGetEmployees(req, res) {
        if (!this.isAuthenticated(req)) {
            return this.sendError(res, 'Authentication required', 401);
        }

        const urlParts = url.parse(req.url, true);
        const page = parseInt(urlParts.query.page) || 1;
        const pageSize = parseInt(urlParts.query.pageSize) || 10;
        
        const activeEmployees = this.employees.filter(emp => emp.active !== false);
        const startIndex = (page - 1) * pageSize;
        const endIndex = startIndex + pageSize;
        const paginatedEmployees = activeEmployees.slice(startIndex, endIndex);

        this.sendJSON(res, {
            employees: paginatedEmployees,
            totalCount: activeEmployees.length,
            page,
            pageSize
        });
    }

    handleCreateEmployee(req, res) {
        if (!this.isAuthenticated(req)) {
            return this.sendError(res, 'Authentication required', 401);
        }

        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
            try {
                const employeeData = JSON.parse(body);
                console.log('Creating employee:', employeeData);
                
                if (!employeeData.firstName || !employeeData.lastName || !employeeData.email) {
                    return this.sendError(res, 'First name, last name, and email are required');
                }

                const newEmployee = {
                    id: this.nextId++,
                    firstName: employeeData.firstName.trim(),
                    lastName: employeeData.lastName.trim(),
                    email: employeeData.email.trim(),
                    phone: employeeData.phone ? employeeData.phone.trim() : '',
                    department: employeeData.department ? employeeData.department.trim() : 'General',
                    position: employeeData.position ? employeeData.position.trim() : 'Employee',
                    salary: parseFloat(employeeData.salary) || 0,
                    hireDate: employeeData.hireDate || new Date().toISOString().split('T')[0],
                    profilePhoto: employeeData.profilePhoto || '',
                    active: true
                };

                this.employees.push(newEmployee);
                this.saveData();
                console.log('Employee created successfully:', newEmployee);
                this.sendJSON(res, { 
                    message: 'Employee created successfully', 
                    employee: newEmployee 
                });
            } catch (error) {
                console.error('Error creating employee:', error);
                this.sendError(res, 'Invalid employee data: ' + error.message);
            }
        });
    }

    handleUpdateEmployee(req, res) {
        if (!this.isAuthenticated(req)) {
            return this.sendError(res, 'Authentication required', 401);
        }

        const urlParts = url.parse(req.url);
        const id = parseInt(urlParts.pathname.split('/').pop());

        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
            try {
                const employeeData = JSON.parse(body);
                const index = this.employees.findIndex(emp => emp.id === id);
                
                if (index === -1) {
                    return this.sendError(res, 'Employee not found', 404);
                }

                this.employees[index] = {
                    ...this.employees[index],
                    ...employeeData,
                    id // Preserve ID
                };

                this.saveData();
                this.sendJSON(res, { message: 'Employee updated successfully' });
            } catch (error) {
                this.sendError(res, 'Invalid employee data');
            }
        });
    }

    handleDeleteEmployee(req, res) {
        if (!this.isAuthenticated(req)) {
            return this.sendError(res, 'Authentication required', 401);
        }

        const urlParts = url.parse(req.url);
        const id = parseInt(urlParts.pathname.split('/').pop());
        const index = this.employees.findIndex(emp => emp.id === id);
        
        if (index === -1) {
            return this.sendError(res, 'Employee not found', 404);
        }

        // Soft delete
        this.employees[index].active = false;
        this.saveData();
        this.sendJSON(res, { message: 'Employee deleted successfully' });
    }

    handleSearch(req, res) {
        if (!this.isAuthenticated(req)) {
            return this.sendError(res, 'Authentication required', 401);
        }

        const urlParts = url.parse(req.url, true);
        const query = urlParts.query.q || '';
        const department = urlParts.query.department || '';
        const position = urlParts.query.position || '';
        
        let results = this.employees.filter(emp => emp.active !== false);

        if (query) {
            const lowerQuery = query.toLowerCase();
            results = results.filter(emp => 
                emp.firstName.toLowerCase().includes(lowerQuery) ||
                emp.lastName.toLowerCase().includes(lowerQuery) ||
                emp.email.toLowerCase().includes(lowerQuery) ||
                emp.department.toLowerCase().includes(lowerQuery) ||
                emp.position.toLowerCase().includes(lowerQuery) ||
                emp.id.toString().includes(query)
            );
        }

        if (department) {
            results = results.filter(emp => emp.department.toLowerCase() === department.toLowerCase());
        }

        if (position) {
            results = results.filter(emp => emp.position.toLowerCase() === position.toLowerCase());
        }

        this.sendJSON(res, { employees: results });
    }

    handleGetDepartments(req, res) {
        if (!this.isAuthenticated(req)) {
            return this.sendError(res, 'Authentication required', 401);
        }

        const departments = [...new Set(this.employees
            .filter(emp => emp.active !== false && emp.department)
            .map(emp => emp.department))];
        
        this.sendJSON(res, { departments });
    }

    handleGetPositions(req, res) {
        if (!this.isAuthenticated(req)) {
            return this.sendError(res, 'Authentication required', 401);
        }

        const positions = [...new Set(this.employees
            .filter(emp => emp.active !== false && emp.position)
            .map(emp => emp.position))];
        
        this.sendJSON(res, { positions });
    }

    handleExport(req, res) {
        if (!this.isAuthenticated(req)) {
            return this.sendError(res, 'Authentication required', 401);
        }

        const activeEmployees = this.employees.filter(emp => emp.active !== false);
        const csvHeader = 'ID,First Name,Last Name,Email,Phone,Department,Position,Salary,Hire Date,Active\n';
        const csvData = activeEmployees.map(emp => 
            `${emp.id},"${emp.firstName}","${emp.lastName}","${emp.email}","${emp.phone}","${emp.department}","${emp.position}",${emp.salary},"${emp.hireDate}",${emp.active}`
        ).join('\n');

        this.handleCORS(res);
        res.writeHead(200, {
            'Content-Type': 'text/csv',
            'Content-Disposition': 'attachment; filename="employees_export.csv"'
        });
        res.end(csvHeader + csvData);
    }

    handleImport(req, res) {
        if (!this.isAuthenticated(req)) {
            return this.sendError(res, 'Authentication required', 401);
        }

        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
            try {
                const lines = body.split('\n');
                const headers = lines[0].split(',');
                
                for (let i = 1; i < lines.length; i++) {
                    const line = lines[i].trim();
                    if (!line) continue;

                    const values = line.split(',').map(val => val.replace(/"/g, ''));
                    
                    if (values.length >= 10) {
                        const employee = {
                            id: this.nextId++,
                            firstName: values[1],
                            lastName: values[2],
                            email: values[3],
                            phone: values[4],
                            department: values[5],
                            position: values[6],
                            salary: parseFloat(values[7]) || 0,
                            hireDate: values[8],
                            profilePhoto: '',
                            active: true
                        };
                        this.employees.push(employee);
                    }
                }

                this.saveData();
                this.sendJSON(res, { message: 'Import completed successfully' });
            } catch (error) {
                this.sendError(res, 'Error importing data');
            }
        });
    }

    handleRequest(req, res) {
        const urlParts = url.parse(req.url, true);
        const pathname = urlParts.pathname;

        // Handle CORS preflight
        if (req.method === 'OPTIONS') {
            this.handleCORS(res);
            res.writeHead(200);
            res.end();
            return;
        }

        // Route handling
        if (pathname === '/api/auth/login' && req.method === 'POST') {
            this.handleLogin(req, res);
        } else if (pathname === '/api/employees' && req.method === 'GET') {
            this.handleGetEmployees(req, res);
        } else if (pathname === '/api/employees' && req.method === 'POST') {
            this.handleCreateEmployee(req, res);
        } else if (pathname.startsWith('/api/employees/') && req.method === 'PUT') {
            this.handleUpdateEmployee(req, res);
        } else if (pathname.startsWith('/api/employees/') && req.method === 'DELETE') {
            this.handleDeleteEmployee(req, res);
        } else if (pathname === '/api/employees/search' && req.method === 'GET') {
            this.handleSearch(req, res);
        } else if (pathname === '/api/departments' && req.method === 'GET') {
            this.handleGetDepartments(req, res);
        } else if (pathname === '/api/positions' && req.method === 'GET') {
            this.handleGetPositions(req, res);
        } else if (pathname === '/api/employees/export' && req.method === 'GET') {
            this.handleExport(req, res);
        } else if (pathname === '/api/employees/import' && req.method === 'POST') {
            this.handleImport(req, res);
        } else {
            this.sendError(res, 'Not Found', 404);
        }
    }

    start(port = 8080) {
        const server = http.createServer((req, res) => {
            this.handleRequest(req, res);
        });

        server.listen(port, () => {
            console.log(`Employee Management Server running on http://localhost:${port}`);
            console.log('Default credentials: admin / admin123');
        });
    }
}

// Start the server
const server = new EmployeeServer();
server.start();
