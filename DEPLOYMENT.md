# Deployment Guide

## Quick Start

### 1. Build and Run Backend
```bash
# Windows
cd employee-management-system\backend
cl /EHsc /std:c++17 /I"lib" src\main.cpp src\employee.cpp src\auth.cpp src\database.cpp /Fe:employee_server.exe
employee_server.exe

# Linux/macOS
cd employee-management-system/backend
g++ -std=c++17 -Ilib -pthread -o employee_server src/main.cpp src/employee.cpp src/auth.cpp src/database.cpp
./employee_server
```

### 2. Serve Frontend
```bash
cd employee-management-system/frontend
python -m http.server 3000
```

### 3. Access Application
- Open browser to: http://localhost:3000/login.html
- Login with: admin / admin123
- Import sample data from: `backend/data/seed_data.csv`

## Production Deployment

### Backend Deployment

#### Option 1: Systemd Service (Linux)

1. Create service file `/etc/systemd/system/employee-server.service`:
```ini
[Unit]
Description=Employee Management Server
After=network.target

[Service]
Type=simple
User=www-data
WorkingDirectory=/opt/employee-management/backend
ExecStart=/opt/employee-management/backend/employee_server
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

2. Enable and start:
```bash
sudo systemctl enable employee-server
sudo systemctl start employee-server
```

#### Option 2: Docker Container

Create `Dockerfile` in backend directory:
```dockerfile
FROM ubuntu:20.04

RUN apt-get update && apt-get install -y \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN g++ -std=c++17 -Ilib -pthread -o employee_server \
    src/main.cpp src/employee.cpp src/auth.cpp src/database.cpp

EXPOSE 8080

CMD ["./employee_server"]
```

Build and run:
```bash
docker build -t employee-server .
docker run -p 8080:8080 -v $(pwd)/data:/app/data employee-server
```

### Frontend Deployment

#### Option 1: Nginx
```nginx
server {
    listen 80;
    server_name your-domain.com;
    
    location / {
        root /var/www/employee-management/frontend;
        index login.html;
        try_files $uri $uri/ /login.html;
    }
    
    location /api/ {
        proxy_pass http://localhost:8080/api/;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

#### Option 2: Apache
```apache
<VirtualHost *:80>
    ServerName your-domain.com
    DocumentRoot /var/www/employee-management/frontend
    
    ProxyPreserveHost On
    ProxyPass /api/ http://localhost:8080/api/
    ProxyPassReverse /api/ http://localhost:8080/api/
</VirtualHost>
```

## Security Considerations

### Backend Security
- Change default admin credentials immediately
- Use HTTPS in production
- Implement rate limiting
- Add input validation and sanitization
- Use proper password hashing (bcrypt)
- Implement JWT token expiration
- Add CORS configuration for specific domains

### Frontend Security
- Serve over HTTPS
- Implement Content Security Policy (CSP)
- Validate all user inputs
- Sanitize data before display
- Use secure cookie settings

## Monitoring and Logging

### Backend Logging
Add logging to `main.cpp`:
```cpp
#include <fstream>
#include <ctime>

void logRequest(const std::string& method, const std::string& path, int status) {
    std::ofstream logFile("logs/access.log", std::ios::app);
    time_t now = time(0);
    char* timeStr = ctime(&now);
    timeStr[strlen(timeStr)-1] = '\0'; // Remove newline
    
    logFile << "[" << timeStr << "] " << method << " " << path 
            << " - " << status << std::endl;
}
```

### System Monitoring
- Monitor CPU and memory usage
- Track disk space for data directory
- Monitor network connections
- Set up alerts for service failures

## Backup Strategy

### Data Backup
```bash
#!/bin/bash
# backup-data.sh
DATE=$(date +%Y%m%d_%H%M%S)
BACKUP_DIR="/backups/employee-management"

mkdir -p $BACKUP_DIR
tar -czf "$BACKUP_DIR/data_backup_$DATE.tar.gz" -C /opt/employee-management/backend data/
```

### Automated Backups
Add to crontab:
```bash
# Daily backup at 2 AM
0 2 * * * /opt/scripts/backup-data.sh
```

## Performance Optimization

### Backend Optimization
- Implement connection pooling
- Add caching for frequently accessed data
- Use database indexing for large datasets
- Implement request compression
- Add connection keep-alive

### Frontend Optimization
- Minify CSS and JavaScript
- Implement lazy loading for large tables
- Add client-side caching
- Optimize images and assets
- Use CDN for static assets

## Scaling Considerations

### Horizontal Scaling
- Use load balancer (nginx, HAProxy)
- Implement session sharing
- Use external database (PostgreSQL, MySQL)
- Add Redis for caching
- Implement microservices architecture

### Database Migration
For production use, consider migrating to a proper database:
```cpp
// Example PostgreSQL integration
#include <libpq-fe.h>

class PostgreSQLDatabase {
    PGconn* conn;
public:
    bool connect(const std::string& connStr) {
        conn = PQconnectdb(connStr.c_str());
        return PQstatus(conn) == CONNECTION_OK;
    }
    // Implement CRUD operations...
};
```

## Health Checks

### Backend Health Endpoint
Add to `main.cpp`:
```cpp
server_.Get("/health", [](const httplib::Request&, httplib::Response& res) {
    res.set_content("{\"status\":\"healthy\",\"timestamp\":\"" + 
                    std::to_string(time(nullptr)) + "\"}", 
                    "application/json");
});
```

### Frontend Health Check
```javascript
// Add to main.js
async function checkBackendHealth() {
    try {
        const response = await fetch(`${this.baseURL}/health`);
        return response.ok;
    } catch (error) {
        return false;
    }
}
```

## Troubleshooting Production Issues

### Common Issues
1. **Port conflicts**: Change port in main.cpp
2. **File permissions**: Ensure data directory is writable
3. **Memory leaks**: Monitor memory usage over time
4. **CORS errors**: Configure proper CORS headers
5. **SSL certificate**: Use Let's Encrypt for free SSL

### Debug Mode
Add debug flag to backend:
```cpp
#ifdef DEBUG
    std::cout << "Debug: " << message << std::endl;
#endif
```

Compile with debug:
```bash
g++ -DDEBUG -std=c++17 -Ilib -pthread -o employee_server_debug src/*.cpp
```
