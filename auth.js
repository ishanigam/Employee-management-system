// Authentication Module
class AuthManager {
    constructor() {
        this.baseURL = 'http://localhost:8080/api';
        this.token = localStorage.getItem('authToken');
        this.init();
    }

    init() {
        // Check if we're on login page
        if (window.location.pathname.includes('login.html')) {
            this.initLoginPage();
        } else {
            // Check authentication for other pages
            if (!this.isAuthenticated()) {
                this.redirectToLogin();
            }
        }
    }

    initLoginPage() {
        const loginForm = document.getElementById('loginForm');
        if (loginForm) {
            loginForm.addEventListener('submit', (e) => this.handleLogin(e));
        }

        // If already authenticated, redirect to main app
        if (this.isAuthenticated()) {
            window.location.href = 'index.html';
        }
    }

    async handleLogin(event) {
        event.preventDefault();
        
        const username = document.getElementById('username').value;
        const password = document.getElementById('password').value;
        const errorMessage = document.getElementById('errorMessage');

        try {
            const response = await fetch(`${this.baseURL}/auth/login`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ username, password })
            });

            const data = await response.json();

            if (response.ok && data.token) {
                this.token = data.token;
                localStorage.setItem('authToken', this.token);
                localStorage.setItem('username', username);
                window.location.href = 'index.html';
            } else {
                this.showError(errorMessage, data.error || 'Login failed');
            }
        } catch (error) {
            console.error('Login error:', error);
            this.showError(errorMessage, 'Connection error. Please check if the server is running.');
        }
    }

    async logout() {
        try {
            if (this.token) {
                await fetch(`${this.baseURL}/auth/logout`, {
                    method: 'POST',
                    headers: {
                        'Authorization': `Bearer ${this.token}`,
                        'Content-Type': 'application/json'
                    }
                });
            }
        } catch (error) {
            console.error('Logout error:', error);
        } finally {
            this.token = null;
            localStorage.removeItem('authToken');
            localStorage.removeItem('username');
            this.redirectToLogin();
        }
    }

    isAuthenticated() {
        return this.token !== null && this.token !== '';
    }

    getAuthHeaders() {
        return {
            'Authorization': `Bearer ${this.token}`,
            'Content-Type': 'application/json'
        };
    }

    redirectToLogin() {
        window.location.href = 'login.html';
    }

    showError(element, message) {
        element.textContent = message;
        element.style.display = 'block';
        setTimeout(() => {
            element.style.display = 'none';
        }, 5000);
    }

    getUsername() {
        return localStorage.getItem('username') || 'User';
    }
}

// Initialize auth manager when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    console.log('Initializing AuthManager...');
    window.authManager = new AuthManager();
    console.log('AuthManager initialized');
});
