// Employee-specific utilities and helpers
class EmployeeUtils {
    static formatSalary(salary) {
        return new Intl.NumberFormat('en-US', {
            style: 'currency',
            currency: 'USD'
        }).format(salary);
    }

    static formatDate(dateString) {
        if (!dateString) return '';
        const date = new Date(dateString);
        return date.toLocaleDateString('en-US');
    }

    static validateEmployee(employee) {
        const errors = [];

        if (!employee.firstName || employee.firstName.trim().length === 0) {
            errors.push('First name is required');
        }

        if (!employee.lastName || employee.lastName.trim().length === 0) {
            errors.push('Last name is required');
        }

        if (!employee.email || employee.email.trim().length === 0) {
            errors.push('Email is required');
        } else if (!this.isValidEmail(employee.email)) {
            errors.push('Please enter a valid email address');
        }

        if (employee.phone && !this.isValidPhone(employee.phone)) {
            errors.push('Please enter a valid phone number');
        }

        if (employee.salary && (isNaN(employee.salary) || employee.salary < 0)) {
            errors.push('Salary must be a valid positive number');
        }

        return errors;
    }

    static isValidEmail(email) {
        const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        return emailRegex.test(email);
    }

    static isValidPhone(phone) {
        const phoneRegex = /^[\+]?[1-9][\d]{0,15}$/;
        return phoneRegex.test(phone.replace(/[\s\-\(\)]/g, ''));
    }

    static generateEmployeeCard(employee) {
        return `
            <div class="employee-card" data-id="${employee.id}">
                <div class="employee-photo-container">
                    ${employee.profilePhoto ? 
                        `<img src="/uploads/${employee.profilePhoto}" alt="Profile" class="employee-photo">` : 
                        '<div class="employee-photo-placeholder">No Photo</div>'
                    }
                </div>
                <div class="employee-info">
                    <h3>${employee.firstName} ${employee.lastName}</h3>
                    <p class="employee-title">${employee.position}</p>
                    <p class="employee-department">${employee.department}</p>
                    <p class="employee-email">${employee.email}</p>
                    <p class="employee-salary">${this.formatSalary(employee.salary)}</p>
                    <p class="employee-hire-date">Hired: ${this.formatDate(employee.hireDate)}</p>
                </div>
                <div class="employee-actions">
                    <button class="btn btn-small btn-primary" onclick="app.editEmployee(${employee.id})">
                        Edit
                    </button>
                    <button class="btn btn-small btn-danger" onclick="app.deleteEmployee(${employee.id})">
                        Delete
                    </button>
                </div>
            </div>
        `;
    }

    static exportToCSV(employees) {
        const headers = [
            'ID', 'First Name', 'Last Name', 'Email', 'Phone', 
            'Department', 'Position', 'Salary', 'Hire Date', 'Active'
        ];

        const csvContent = [
            headers.join(','),
            ...employees.map(emp => [
                emp.id,
                `"${emp.firstName}"`,
                `"${emp.lastName}"`,
                `"${emp.email}"`,
                `"${emp.phone || ''}"`,
                `"${emp.department || ''}"`,
                `"${emp.position || ''}"`,
                emp.salary || 0,
                `"${emp.hireDate || ''}"`,
                emp.active ? 'true' : 'false'
            ].join(','))
        ].join('\n');

        return csvContent;
    }

    static downloadCSV(csvContent, filename) {
        const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
        const link = document.createElement('a');
        
        if (link.download !== undefined) {
            const url = URL.createObjectURL(blob);
            link.setAttribute('href', url);
            link.setAttribute('download', filename);
            link.style.visibility = 'hidden';
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        }
    }

    static parseCSV(csvText) {
        const lines = csvText.split('\n');
        const headers = lines[0].split(',').map(h => h.replace(/"/g, '').trim());
        const employees = [];

        for (let i = 1; i < lines.length; i++) {
            const line = lines[i].trim();
            if (!line) continue;

            const values = this.parseCSVLine(line);
            if (values.length >= headers.length) {
                const employee = {};
                headers.forEach((header, index) => {
                    const value = values[index] ? values[index].replace(/"/g, '').trim() : '';
                    
                    switch (header.toLowerCase()) {
                        case 'id':
                            employee.id = parseInt(value) || 0;
                            break;
                        case 'first name':
                            employee.firstName = value;
                            break;
                        case 'last name':
                            employee.lastName = value;
                            break;
                        case 'email':
                            employee.email = value;
                            break;
                        case 'phone':
                            employee.phone = value;
                            break;
                        case 'department':
                            employee.department = value;
                            break;
                        case 'position':
                            employee.position = value;
                            break;
                        case 'salary':
                            employee.salary = parseFloat(value) || 0;
                            break;
                        case 'hire date':
                            employee.hireDate = value;
                            break;
                        case 'active':
                            employee.active = value.toLowerCase() === 'true';
                            break;
                    }
                });
                employees.push(employee);
            }
        }

        return employees;
    }

    static parseCSVLine(line) {
        const values = [];
        let current = '';
        let inQuotes = false;

        for (let i = 0; i < line.length; i++) {
            const char = line[i];
            
            if (char === '"') {
                inQuotes = !inQuotes;
            } else if (char === ',' && !inQuotes) {
                values.push(current);
                current = '';
            } else {
                current += char;
            }
        }
        
        values.push(current);
        return values;
    }

    static searchEmployees(employees, query) {
        if (!query) return employees;
        
        const searchTerm = query.toLowerCase();
        return employees.filter(emp => 
            emp.firstName.toLowerCase().includes(searchTerm) ||
            emp.lastName.toLowerCase().includes(searchTerm) ||
            emp.email.toLowerCase().includes(searchTerm) ||
            emp.department.toLowerCase().includes(searchTerm) ||
            emp.position.toLowerCase().includes(searchTerm) ||
            emp.id.toString().includes(searchTerm)
        );
    }

    static sortEmployees(employees, field, direction = 'asc') {
        return [...employees].sort((a, b) => {
            let aVal = a[field];
            let bVal = b[field];

            // Handle different data types
            if (field === 'salary') {
                aVal = parseFloat(aVal) || 0;
                bVal = parseFloat(bVal) || 0;
            } else if (field === 'hireDate') {
                aVal = new Date(aVal);
                bVal = new Date(bVal);
            } else {
                aVal = String(aVal).toLowerCase();
                bVal = String(bVal).toLowerCase();
            }

            if (direction === 'desc') {
                return aVal < bVal ? 1 : aVal > bVal ? -1 : 0;
            } else {
                return aVal > bVal ? 1 : aVal < bVal ? -1 : 0;
            }
        });
    }

    static filterEmployees(employees, filters) {
        return employees.filter(emp => {
            if (filters.department && emp.department !== filters.department) {
                return false;
            }
            
            if (filters.position && emp.position !== filters.position) {
                return false;
            }
            
            if (filters.minSalary && parseFloat(emp.salary) < parseFloat(filters.minSalary)) {
                return false;
            }
            
            if (filters.maxSalary && parseFloat(emp.salary) > parseFloat(filters.maxSalary)) {
                return false;
            }
            
            if (filters.active !== undefined && emp.active !== filters.active) {
                return false;
            }
            
            return true;
        });
    }

    static getEmployeeStats(employees) {
        const stats = {
            total: employees.length,
            active: employees.filter(emp => emp.active).length,
            departments: [...new Set(employees.map(emp => emp.department))].length,
            positions: [...new Set(employees.map(emp => emp.position))].length,
            averageSalary: 0,
            totalSalary: 0,
            departmentBreakdown: {},
            positionBreakdown: {},
            salaryRanges: {
                '0-30k': 0,
                '30k-50k': 0,
                '50k-70k': 0,
                '70k-100k': 0,
                '100k+': 0
            }
        };

        if (employees.length > 0) {
            stats.totalSalary = employees.reduce((sum, emp) => sum + parseFloat(emp.salary || 0), 0);
            stats.averageSalary = stats.totalSalary / employees.length;

            // Department breakdown
            employees.forEach(emp => {
                stats.departmentBreakdown[emp.department] = 
                    (stats.departmentBreakdown[emp.department] || 0) + 1;
            });

            // Position breakdown
            employees.forEach(emp => {
                stats.positionBreakdown[emp.position] = 
                    (stats.positionBreakdown[emp.position] || 0) + 1;
            });

            // Salary ranges
            employees.forEach(emp => {
                const salary = parseFloat(emp.salary || 0);
                if (salary < 30000) stats.salaryRanges['0-30k']++;
                else if (salary < 50000) stats.salaryRanges['30k-50k']++;
                else if (salary < 70000) stats.salaryRanges['50k-70k']++;
                else if (salary < 100000) stats.salaryRanges['70k-100k']++;
                else stats.salaryRanges['100k+']++;
            });
        }

        return stats;
    }

    static debounce(func, wait) {
        let timeout;
        return function executedFunction(...args) {
            const later = () => {
                clearTimeout(timeout);
                func(...args);
            };
            clearTimeout(timeout);
            timeout = setTimeout(later, wait);
        };
    }

    static throttle(func, limit) {
        let inThrottle;
        return function() {
            const args = arguments;
            const context = this;
            if (!inThrottle) {
                func.apply(context, args);
                inThrottle = true;
                setTimeout(() => inThrottle = false, limit);
            }
        };
    }
}

// Form validation helper
class FormValidator {
    constructor(formElement) {
        this.form = formElement;
        this.errors = {};
    }

    validateField(fieldName, value, rules) {
        this.errors[fieldName] = [];

        rules.forEach(rule => {
            switch (rule.type) {
                case 'required':
                    if (!value || value.trim().length === 0) {
                        this.errors[fieldName].push(rule.message || `${fieldName} is required`);
                    }
                    break;
                case 'email':
                    if (value && !EmployeeUtils.isValidEmail(value)) {
                        this.errors[fieldName].push(rule.message || 'Please enter a valid email address');
                    }
                    break;
                case 'phone':
                    if (value && !EmployeeUtils.isValidPhone(value)) {
                        this.errors[fieldName].push(rule.message || 'Please enter a valid phone number');
                    }
                    break;
                case 'number':
                    if (value && (isNaN(value) || parseFloat(value) < 0)) {
                        this.errors[fieldName].push(rule.message || 'Please enter a valid positive number');
                    }
                    break;
                case 'minLength':
                    if (value && value.length < rule.value) {
                        this.errors[fieldName].push(rule.message || `Minimum length is ${rule.value} characters`);
                    }
                    break;
                case 'maxLength':
                    if (value && value.length > rule.value) {
                        this.errors[fieldName].push(rule.message || `Maximum length is ${rule.value} characters`);
                    }
                    break;
            }
        });

        return this.errors[fieldName].length === 0;
    }

    showFieldError(fieldName) {
        const field = this.form.querySelector(`[name="${fieldName}"]`);
        if (!field) return;

        // Remove existing error display
        const existingError = field.parentNode.querySelector('.field-error');
        if (existingError) {
            existingError.remove();
        }

        if (this.errors[fieldName] && this.errors[fieldName].length > 0) {
            const errorDiv = document.createElement('div');
            errorDiv.className = 'field-error';
            errorDiv.textContent = this.errors[fieldName][0];
            errorDiv.style.color = '#dc3545';
            errorDiv.style.fontSize = '12px';
            errorDiv.style.marginTop = '4px';
            
            field.parentNode.appendChild(errorDiv);
            field.style.borderColor = '#dc3545';
        } else {
            field.style.borderColor = '';
        }
    }

    clearErrors() {
        this.errors = {};
        this.form.querySelectorAll('.field-error').forEach(error => error.remove());
        this.form.querySelectorAll('input, select, textarea').forEach(field => {
            field.style.borderColor = '';
        });
    }

    isValid() {
        return Object.values(this.errors).every(fieldErrors => fieldErrors.length === 0);
    }
}

// Export utilities for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { EmployeeUtils, FormValidator };
}
