# Makefile for Employee Management System Backend

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
INCLUDES = -Ilib
LIBS = -pthread

# Windows specific settings
ifeq ($(OS),Windows_NT)
    CXX = cl
    CXXFLAGS = /EHsc /std:c++17 /O2
    INCLUDES = /I"lib"
    LIBS = ws2_32.lib
    TARGET = employee_server.exe
    RM = del /Q
else
    TARGET = employee_server
    RM = rm -f
    # Linux/macOS specific libraries
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        LIBS += -lrt
    endif
endif

# Source files
SRCDIR = src
SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/employee.cpp $(SRCDIR)/auth.cpp $(SRCDIR)/database.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Build targets
.PHONY: all clean debug release install

all: release

release: CXXFLAGS += -DNDEBUG
release: $(TARGET)

debug: CXXFLAGS += -DDEBUG -g
debug: $(TARGET)

$(TARGET): $(SOURCES)
ifeq ($(OS),Windows_NT)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) /Fe:$(TARGET) $(LIBS)
else
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) -o $(TARGET) $(SOURCES)
endif

# Create necessary directories
setup:
	mkdir -p data
	mkdir -p data/uploads
	mkdir -p logs

# Install (copy to system directory)
install: $(TARGET) setup
ifeq ($(OS),Windows_NT)
	copy $(TARGET) C:\Program Files\EmployeeManagement\
	xcopy data C:\Program Files\EmployeeManagement\data\ /E /I
else
	sudo mkdir -p /opt/employee-management
	sudo cp $(TARGET) /opt/employee-management/
	sudo cp -r data /opt/employee-management/
	sudo chown -R www-data:www-data /opt/employee-management
endif

# Clean build artifacts
clean:
ifeq ($(OS),Windows_NT)
	$(RM) $(TARGET) *.obj
else
	$(RM) $(TARGET) $(OBJECTS)
endif

# Run the server
run: $(TARGET) setup
	./$(TARGET)

# Run with sample data
run-with-data: $(TARGET) setup
	@echo "Starting server with sample data..."
	@echo "Import data/seed_data.csv after login"
	./$(TARGET)

# Development helpers
format:
	clang-format -i $(SOURCES) $(SRCDIR)/*.h

lint:
	cppcheck --enable=all --std=c++17 $(SOURCES)

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build release version (default)"
	@echo "  debug      - Build debug version"
	@echo "  release    - Build release version"
	@echo "  clean      - Remove build artifacts"
	@echo "  setup      - Create necessary directories"
	@echo "  install    - Install to system directory"
	@echo "  run        - Build and run the server"
	@echo "  run-with-data - Run with instructions for sample data"
	@echo "  format     - Format source code"
	@echo "  lint       - Run static analysis"
	@echo "  help       - Show this help message"
