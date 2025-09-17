// Main Application Module
class EmployeeManagementApp {
    constructor() {
        this.baseURL = 'http://localhost:8080/api';
        this.currentPage = 1;
        this.pageSize = 10;
        this.currentSection = 'dashboard';
        this.employees = [];
        this.departments = [];
        this.positions = [];
        
        this.init();
    }

    init() {
        this.setupEventListeners();
        this.setupUserInfo();
        this.loadInitialData();
        this.showSection('dashboard');
    }

    setupEventListeners() {
        // Navigation
        document.querySelectorAll('.nav-link').forEach(link => {
            link.addEventListener('click', (e) => {
                e.preventDefault();
                const section = e.target.getAttribute('data-section');
                this.showSection(section);
            });
        });

        // Logout
        document.getElementById('logoutBtn').addEventListener('click', () => {
            window.authManager.logout();
        });

        // Search and Filter
        document.getElementById('searchBtn').addEventListener('click', () => this.searchEmployees());
        document.getElementById('searchInput').addEventListener('keypress', (e) => {
            if (e.key === 'Enter') this.searchEmployees();
        });
        document.getElementById('filterBtn').addEventListener('click', () => this.filterEmployees());
        document.getElementById('clearFiltersBtn').addEventListener('click', () => this.clearFilters());

        // Pagination
        document.getElementById('prevPageBtn').addEventListener('click', () => this.previousPage());
        document.getElementById('nextPageBtn').addEventListener('click', () => this.nextPage());

        // Export/Import
        document.getElementById('exportBtn').addEventListener('click', () => this.exportEmployees());
        document.getElementById('importBtn').addEventListener('click', () => {
            document.getElementById('importFile').click();
        });
        document.getElementById('importFile').addEventListener('change', (e) => this.importEmployees(e));

        // Add Employee Form
        document.getElementById('addEmployeeForm').addEventListener('submit', (e) => this.addEmployee(e));

        // Modal
        document.querySelector('.close').addEventListener('click', () => this.closeModal());
        document.querySelector('.modal-close').addEventListener('click', () => this.closeModal());
        document.getElementById('editEmployeeForm').addEventListener('submit', (e) => this.updateEmployee(e));

        // Click outside modal to close
        window.addEventListener('click', (e) => {
            const modal = document.getElementById('employeeModal');
            if (e.target === modal) {
                this.closeModal();
            }
        });
    }

    setupUserInfo() {
        const userInfo = document.getElementById('userInfo');
        userInfo.textContent = `Welcome, ${window.authManager.getUsername()}`;
    }

    async loadInitialData() {
        this.showLoading(true);
        try {
            await Promise.all([
                this.loadEmployees(),
                this.loadDepartments(),
                this.loadPositions()
            ]);
            this.updateDashboard();
        } catch (error) {
            console.error('Error loading initial data:', error);
            this.showToast('Error loading data', 'error');
        } finally {
            this.showLoading(false);
        }
    }

    async loadEmployees(page = 1, pageSize = 10) {
        try {
            const response = await fetch(`${this.baseURL}/employees?page=${page}&pageSize=${pageSize}`, {
                headers: window.authManager.getAuthHeaders()
            });

            if (!response.ok) {
                throw new Error('Failed to load employees');
            }

            const data = await response.json();
            this.employees = data.employees || [];
            this.currentPage = data.page || 1;
            this.totalCount = data.totalCount || 0;
            
            this.renderEmployeeTable();
            this.updatePagination();
            
            return data;
        } catch (error) {
            console.error('Error loading employees:', error);
            this.showToast('Error loading employees', 'error');
            return { employees: [], totalCount: 0, page: 1 };
        }
    }

    async loadDepartments() {
        try {
            const response = await fetch(`${this.baseURL}/departments`, {
                headers: window.authManager.getAuthHeaders()
            });

            if (response.ok) {
                const data = await response.json();
                this.departments = data.departments || [];
                this.populateDepartmentFilter();
            }
        } catch (error) {
            console.error('Error loading departments:', error);
        }
    }

    async loadPositions() {
        try {
            const response = await fetch(`${this.baseURL}/positions`, {
                headers: window.authManager.getAuthHeaders()
            });

            if (response.ok) {
                const data = await response.json();
                this.positions = data.positions || [];
                this.populatePositionFilter();
            }
        } catch (error) {
            console.error('Error loading positions:', error);
        }
    }

    showSection(sectionName) {
        // Hide all sections
        document.querySelectorAll('.content-section').forEach(section => {
            section.classList.remove('active');
        });

        // Remove active class from all nav links
        document.querySelectorAll('.nav-link').forEach(link => {
            link.classList.remove('active');
        });

        // Show selected section
        const section = document.getElementById(sectionName);
        if (section) {
            section.classList.add('active');
        }

        // Add active class to corresponding nav link
        const navLink = document.querySelector(`[data-section="${sectionName}"]`);
        if (navLink) {
            navLink.classList.add('active');
        }

        this.currentSection = sectionName;

        // Load section-specific data
        if (sectionName === 'employees') {
            this.loadEmployees(this.currentPage, this.pageSize);
        } else if (sectionName === 'dashboard') {
            this.updateDashboard();
        } else if (sectionName === 'reports') {
            this.updateReports();
        }
    }

    renderEmployeeTable() {
        const tbody = document.getElementById('employeeTableBody');
        tbody.innerHTML = '';

        this.employees.forEach(employee => {
            const row = document.createElement('tr');
            row.innerHTML = `
                <td>${employee.id}</td>
                <td>
                    ${employee.profilePhoto ? 
                        `<img src="${this.baseURL.replace('/api', '')}/uploads/${employee.profilePhoto}" alt="Profile" class="employee-photo">` : 
                        '<div class="employee-photo" style="background: #ddd; display: flex; align-items: center; justify-content: center; color: #666;">No Photo</div>'
                    }
                </td>
                <td>${employee.firstName} ${employee.lastName}</td>
                <td>${employee.email}</td>
                <td>${employee.department}</td>
                <td>${employee.position}</td>
                <td>$${parseFloat(employee.salary).toLocaleString()}</td>
                <td>
                    <div class="action-buttons">
                        <button class="btn btn-small btn-primary" onclick="app.editEmployee(${employee.id})">Edit</button>
                        <button class="btn btn-small btn-danger" onclick="app.deleteEmployee(${employee.id})">Delete</button>
                    </div>
                </td>
            `;
            tbody.appendChild(row);
        });
    }

    updatePagination() {
        const totalPages = Math.ceil(this.totalCount / this.pageSize);
        document.getElementById('pageInfo').textContent = `Page ${this.currentPage} of ${totalPages}`;
        
        document.getElementById('prevPageBtn').disabled = this.currentPage <= 1;
        document.getElementById('nextPageBtn').disabled = this.currentPage >= totalPages;
    }

    populateDepartmentFilter() {
        const select = document.getElementById('departmentFilter');
        select.innerHTML = '<option value="">All Departments</option>';
        
        this.departments.forEach(dept => {
            const option = document.createElement('option');
            option.value = dept;
            option.textContent = dept;
            select.appendChild(option);
        });
    }

    populatePositionFilter() {
        const select = document.getElementById('positionFilter');
        select.innerHTML = '<option value="">All Positions</option>';
        
        this.positions.forEach(pos => {
            const option = document.createElement('option');
            option.value = pos;
            option.textContent = pos;
            select.appendChild(option);
        });
    }

    async searchEmployees() {
        const query = document.getElementById('searchInput').value.trim();
        if (!query) {
            this.loadEmployees(1, this.pageSize);
            return;
        }

        this.showLoading(true);
        try {
            const response = await fetch(`${this.baseURL}/employees/search?q=${encodeURIComponent(query)}&page=1&pageSize=${this.pageSize}`, {
                headers: window.authManager.getAuthHeaders()
            });

            if (response.ok) {
                const data = await response.json();
                this.employees = data.employees || [];
                this.currentPage = 1;
                this.totalCount = data.employees.length;
                this.renderEmployeeTable();
                this.updatePagination();
            }
        } catch (error) {
            console.error('Error searching employees:', error);
            this.showToast('Error searching employees', 'error');
        } finally {
            this.showLoading(false);
        }
    }

    async filterEmployees() {
        const department = document.getElementById('departmentFilter').value;
        const position = document.getElementById('positionFilter').value;
        const minSalary = document.getElementById('minSalary').value;
        const maxSalary = document.getElementById('maxSalary').value;

        const params = new URLSearchParams({
            page: '1',
            pageSize: this.pageSize.toString()
        });

        if (department) params.append('department', department);
        if (position) params.append('position', position);
        if (minSalary) params.append('minSalary', minSalary);
        if (maxSalary) params.append('maxSalary', maxSalary);

        this.showLoading(true);
        try {
            const response = await fetch(`${this.baseURL}/employees/search?${params}`, {
                headers: window.authManager.getAuthHeaders()
            });

            if (response.ok) {
                const data = await response.json();
                this.employees = data.employees || [];
                this.currentPage = 1;
                this.totalCount = data.employees.length;
                this.renderEmployeeTable();
                this.updatePagination();
            }
        } catch (error) {
            console.error('Error filtering employees:', error);
            this.showToast('Error filtering employees', 'error');
        } finally {
            this.showLoading(false);
        }
    }

    clearFilters() {
        document.getElementById('searchInput').value = '';
        document.getElementById('departmentFilter').value = '';
        document.getElementById('positionFilter').value = '';
        document.getElementById('minSalary').value = '';
        document.getElementById('maxSalary').value = '';
        
        this.loadEmployees(1, this.pageSize);
    }

    previousPage() {
        if (this.currentPage > 1) {
            this.loadEmployees(this.currentPage - 1, this.pageSize);
        }
    }

    nextPage() {
        const totalPages = Math.ceil(this.totalCount / this.pageSize);
        if (this.currentPage < totalPages) {
            this.loadEmployees(this.currentPage + 1, this.pageSize);
        }
    }

    async addEmployee(event) {
        event.preventDefault();
        
        const formData = new FormData(event.target);
        const employeeData = {
            firstName: formData.get('firstName'),
            lastName: formData.get('lastName'),
            email: formData.get('email'),
            phone: formData.get('phone'),
            department: formData.get('department'),
            position: formData.get('position'),
            salary: parseFloat(formData.get('salary')) || 0,
            hireDate: formData.get('hireDate'),
            profilePhoto: '',
            active: true
        };

        // Handle file upload if present
        const photoFile = formData.get('profilePhoto');
        if (photoFile && photoFile.size > 0) {
            try {
                const uploadResponse = await fetch(`${this.baseURL}/employees/upload`, {
                    method: 'POST',
                    headers: {
                        'Authorization': `Bearer ${window.authManager.token}`
                    },
                    body: photoFile
                });

                if (uploadResponse.ok) {
                    const uploadData = await uploadResponse.json();
                    employeeData.profilePhoto = uploadData.filename;
                }
            } catch (error) {
                console.error('Error uploading photo:', error);
            }
        }

        this.showLoading(true);
        try {
            const response = await fetch(`${this.baseURL}/employees`, {
                method: 'POST',
                headers: window.authManager.getAuthHeaders(),
                body: JSON.stringify(employeeData)
            });

            if (response.ok) {
                this.showToast('Employee added successfully', 'success');
                event.target.reset();
                this.loadEmployees(this.currentPage, this.pageSize);
                this.loadDepartments();
                this.loadPositions();
            } else {
                const error = await response.json();
                this.showToast(error.error || 'Error adding employee', 'error');
            }
        } catch (error) {
            console.error('Error adding employee:', error);
            this.showToast('Error adding employee', 'error');
        } finally {
            this.showLoading(false);
        }
    }

    async editEmployee(id) {
        try {
            const response = await fetch(`${this.baseURL}/employees/${id}`, {
                headers: window.authManager.getAuthHeaders()
            });

            if (response.ok) {
                const employee = await response.json();
                this.populateEditForm(employee);
                this.showModal();
            } else {
                this.showToast('Error loading employee details', 'error');
            }
        } catch (error) {
            console.error('Error loading employee:', error);
            this.showToast('Error loading employee details', 'error');
        }
    }

    populateEditForm(employee) {
        document.getElementById('editEmployeeId').value = employee.id;
        document.getElementById('editFirstName').value = employee.firstName;
        document.getElementById('editLastName').value = employee.lastName;
        document.getElementById('editEmail').value = employee.email;
        document.getElementById('editPhone').value = employee.phone;
        document.getElementById('editDepartment').value = employee.department;
        document.getElementById('editPosition').value = employee.position;
        document.getElementById('editSalary').value = employee.salary;
        document.getElementById('editHireDate').value = employee.hireDate;
    }

    async updateEmployee(event) {
        event.preventDefault();
        
        const formData = new FormData(event.target);
        const id = document.getElementById('editEmployeeId').value;
        
        const employeeData = {
            firstName: formData.get('firstName'),
            lastName: formData.get('lastName'),
            email: formData.get('email'),
            phone: formData.get('phone'),
            department: formData.get('department'),
            position: formData.get('position'),
            salary: parseFloat(formData.get('salary')) || 0,
            hireDate: formData.get('hireDate'),
            active: true
        };

        this.showLoading(true);
        try {
            const response = await fetch(`${this.baseURL}/employees/${id}`, {
                method: 'PUT',
                headers: window.authManager.getAuthHeaders(),
                body: JSON.stringify(employeeData)
            });

            if (response.ok) {
                this.showToast('Employee updated successfully', 'success');
                this.closeModal();
                this.loadEmployees(this.currentPage, this.pageSize);
                this.loadDepartments();
                this.loadPositions();
            } else {
                const error = await response.json();
                this.showToast(error.error || 'Error updating employee', 'error');
            }
        } catch (error) {
            console.error('Error updating employee:', error);
            this.showToast('Error updating employee', 'error');
        } finally {
            this.showLoading(false);
        }
    }

    async deleteEmployee(id) {
        if (!confirm('Are you sure you want to delete this employee?')) {
            return;
        }

        this.showLoading(true);
        try {
            const response = await fetch(`${this.baseURL}/employees/${id}`, {
                method: 'DELETE',
                headers: window.authManager.getAuthHeaders()
            });

            if (response.ok) {
                this.showToast('Employee deleted successfully', 'success');
                this.loadEmployees(this.currentPage, this.pageSize);
            } else {
                const error = await response.json();
                this.showToast(error.error || 'Error deleting employee', 'error');
            }
        } catch (error) {
            console.error('Error deleting employee:', error);
            this.showToast('Error deleting employee', 'error');
        } finally {
            this.showLoading(false);
        }
    }

    async exportEmployees() {
        this.showLoading(true);
        try {
            const response = await fetch(`${this.baseURL}/employees/export`, {
                headers: window.authManager.getAuthHeaders()
            });

            if (response.ok) {
                const blob = await response.blob();
                const url = window.URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `employees_export_${new Date().toISOString().split('T')[0]}.csv`;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);
                window.URL.revokeObjectURL(url);
                
                this.showToast('Export completed successfully', 'success');
            } else {
                this.showToast('Error exporting data', 'error');
            }
        } catch (error) {
            console.error('Error exporting employees:', error);
            this.showToast('Error exporting data', 'error');
        } finally {
            this.showLoading(false);
        }
    }

    async importEmployees(event) {
        const file = event.target.files[0];
        if (!file) return;

        this.showLoading(true);
        try {
            const response = await fetch(`${this.baseURL}/employees/import`, {
                method: 'POST',
                headers: {
                    'Authorization': `Bearer ${window.authManager.token}`
                },
                body: file
            });

            if (response.ok) {
                this.showToast('Import completed successfully', 'success');
                this.loadEmployees(this.currentPage, this.pageSize);
                this.loadDepartments();
                this.loadPositions();
            } else {
                const error = await response.json();
                this.showToast(error.error || 'Error importing data', 'error');
            }
        } catch (error) {
            console.error('Error importing employees:', error);
            this.showToast('Error importing data', 'error');
        } finally {
            this.showLoading(false);
            event.target.value = ''; // Reset file input
        }
    }

    updateDashboard() {
        const totalEmployees = this.totalCount || 0;
        const totalDepartments = this.departments.length;
        const totalPositions = this.positions.length;
        
        let averageSalary = 0;
        if (this.employees.length > 0) {
            const totalSalary = this.employees.reduce((sum, emp) => sum + parseFloat(emp.salary), 0);
            averageSalary = totalSalary / this.employees.length;
        }

        document.getElementById('totalEmployees').textContent = totalEmployees;
        document.getElementById('totalDepartments').textContent = totalDepartments;
        document.getElementById('totalPositions').textContent = totalPositions;
        document.getElementById('averageSalary').textContent = `$${averageSalary.toLocaleString()}`;
    }

    updateReports() {
        // Simple department distribution
        const departmentChart = document.getElementById('departmentChart');
        const departmentCounts = {};
        
        this.employees.forEach(emp => {
            departmentCounts[emp.department] = (departmentCounts[emp.department] || 0) + 1;
        });

        let chartHTML = '<h4>Department Distribution</h4>';
        Object.entries(departmentCounts).forEach(([dept, count]) => {
            chartHTML += `<div style="margin: 10px 0;"><strong>${dept}:</strong> ${count} employees</div>`;
        });

        departmentChart.innerHTML = chartHTML;

        // Simple salary distribution
        const salaryChart = document.getElementById('salaryChart');
        const salaryRanges = {
            '0-30k': 0,
            '30k-50k': 0,
            '50k-70k': 0,
            '70k+': 0
        };

        this.employees.forEach(emp => {
            const salary = parseFloat(emp.salary);
            if (salary < 30000) salaryRanges['0-30k']++;
            else if (salary < 50000) salaryRanges['30k-50k']++;
            else if (salary < 70000) salaryRanges['50k-70k']++;
            else salaryRanges['70k+']++;
        });

        let salaryHTML = '<h4>Salary Distribution</h4>';
        Object.entries(salaryRanges).forEach(([range, count]) => {
            salaryHTML += `<div style="margin: 10px 0;"><strong>$${range}:</strong> ${count} employees</div>`;
        });

        salaryChart.innerHTML = salaryHTML;
    }

    showModal() {
        document.getElementById('employeeModal').style.display = 'block';
    }

    closeModal() {
        document.getElementById('employeeModal').style.display = 'none';
    }

    showLoading(show) {
        const spinner = document.getElementById('loadingSpinner');
        spinner.style.display = show ? 'flex' : 'none';
    }

    showToast(message, type = 'info') {
        const container = document.getElementById('toastContainer');
        const toast = document.createElement('div');
        toast.className = `toast ${type}`;
        toast.textContent = message;
        
        container.appendChild(toast);
        
        setTimeout(() => {
            toast.remove();
        }, 5000);
    }
}

// Initialize the app when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    console.log('DOM loaded, initializing app...');
    
    // Check if we're on the main app page
    if (!window.location.pathname.includes('login.html')) {
        // Initialize auth manager first
        if (!window.authManager) {
            window.authManager = new AuthManager();
        }
        
        // Wait a bit for auth manager to initialize
        setTimeout(() => {
            console.log('Auth manager ready, checking authentication...');
            if (window.authManager && window.authManager.isAuthenticated()) {
                console.log('User authenticated, starting app...');
                window.app = new EmployeeManagementApp();
            } else {
                console.log('User not authenticated, redirecting to login...');
                window.location.href = 'login.html';
            }
        }, 200);
    }
});
