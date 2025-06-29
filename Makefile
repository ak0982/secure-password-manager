# Secure Password Manager Makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -DAES_BLOCK_SIZE=16
LDFLAGS = -lssl -lcrypto -lpthread

# Installation paths
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
DOCDIR = $(PREFIX)/share/doc/secure-password-manager

# Source files
SOURCES = main.cpp crypto.cpp vault.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = password_manager

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "✅ Build complete!"

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Install the program
install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(DOCDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/secure-password-manager
	install -m 644 README.md $(DESTDIR)$(DOCDIR)/
	install -m 644 LICENSE $(DESTDIR)$(DOCDIR)/
	install -m 644 demo.md $(DESTDIR)$(DOCDIR)/

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "🧹 Cleaned build artifacts."

# Install dependencies (macOS)
install-deps-mac:
	@echo "Installing OpenSSL via Homebrew..."
	brew install openssl
	@echo "✅ Dependencies installed."

# Install dependencies (Ubuntu/Debian)
install-deps-ubuntu:
	@echo "Installing OpenSSL development libraries..."
	sudo apt-get update
	sudo apt-get install libssl-dev build-essential
	@echo "✅ Dependencies installed."

# Install dependencies (CentOS/RHEL)
install-deps-centos:
	@echo "Installing OpenSSL development libraries..."
	sudo yum install openssl-devel gcc-c++ make
	@echo "✅ Dependencies installed."

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build with optimizations
release: CXXFLAGS += -O3 -DNDEBUG
release: $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Check for memory leaks (requires valgrind)
memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Security check (static analysis)
security-check:
	@echo "Running basic security checks..."
	@grep -n "system\|exec\|popen" *.cpp || echo "No dangerous system calls found."
	@grep -n "strcpy\|strcat\|sprintf" *.cpp || echo "No unsafe string functions found."
	@echo "✅ Basic security check complete."

# Test build on different systems
test-build:
	@echo "Testing build..."
	$(MAKE) clean
	$(MAKE) all
	@echo "✅ Build test passed."

# Create a backup of the project
backup:
	@echo "Creating project backup..."
	tar -czf password_manager_backup_$(shell date +%Y%m%d_%H%M%S).tar.gz *.cpp *.hpp Makefile README.md
	@echo "✅ Backup created."

# Help target
help:
	@echo "Available targets:"
	@echo "  all              - Build the password manager (default)"
	@echo "  clean            - Remove build artifacts"
	@echo "  debug            - Build with debug symbols"
	@echo "  release          - Build optimized release version"
	@echo "  run              - Build and run the program"
	@echo "  install-deps-*   - Install dependencies for different systems"
	@echo "  memcheck         - Run with valgrind memory checker"
	@echo "  security-check   - Basic security analysis"
	@echo "  test-build       - Test the build process"
	@echo "  backup           - Create a backup archive"
	@echo "  help             - Show this help message"

.PHONY: all clean debug release run install install-deps-mac install-deps-ubuntu install-deps-centos memcheck security-check test-build backup help 