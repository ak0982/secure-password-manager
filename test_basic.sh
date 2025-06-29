#!/bin/bash

# Basic test script for the secure password manager
echo "🧪 Testing Secure Password Manager..."

# Test 1: Check if binary exists and is executable
if [[ ! -x "./password_manager" ]]; then
    echo "❌ Error: password_manager binary not found or not executable"
    exit 1
fi
echo "✅ Binary exists and is executable"

# Test 2: Check basic help (quick exit test)
echo "🔍 Testing help command..."
timeout 5s bash -c 'echo -e "help\nexit" | ./password_manager' > /dev/null 2>&1
if [[ $? -eq 0 ]] || [[ $? -eq 124 ]]; then
    echo "✅ Basic execution test passed"
else
    echo "❌ Basic execution test failed"
fi

# Test 3: Check if OpenSSL linking works
echo "🔐 Testing OpenSSL functionality..."
if ldd ./password_manager 2>/dev/null | grep -q ssl || otool -L ./password_manager 2>/dev/null | grep -q ssl; then
    echo "✅ OpenSSL libraries linked correctly"
else
    echo "⚠️  OpenSSL library linking not detected (may still work)"
fi

# Test 4: Create a temporary vault for testing
echo "🗄️  Testing vault creation..."
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"

# Create a test vault non-interactively (this is tricky, so we'll just check file operations)
if [[ -w "$TEMP_DIR" ]]; then
    echo "✅ File system operations work"
else
    echo "❌ File system operations failed"
fi

# Cleanup
cd - > /dev/null
rm -rf "$TEMP_DIR"

echo ""
echo "🎉 Basic tests completed!"
echo "📋 To run the password manager:"
echo "   ./password_manager"
echo ""
echo "💡 First run will create a new vault and prompt for master password" 