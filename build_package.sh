#!/bin/bash

# Exit on error
set -e

echo "🔨 Building Secure Password Manager package..."

# Set version
VERSION="1.0.0"
PACKAGE_NAME="secure-password-manager"
PACKAGE_FULL="${PACKAGE_NAME}_${VERSION}"

# Clean previous builds
rm -rf "build" "${PACKAGE_FULL}.deb"
mkdir -p build

# Create package structure
mkdir -p build/debian/DEBIAN
mkdir -p build/debian/usr/local/bin
mkdir -p build/debian/usr/local/share/${PACKAGE_NAME}
mkdir -p build/debian/usr/share/doc/${PACKAGE_NAME}

# Copy control files
cp debian/DEBIAN/control build/debian/DEBIAN/
cp debian/DEBIAN/postinst build/debian/DEBIAN/
cp debian/DEBIAN/prerm build/debian/DEBIAN/

# Set permissions for scripts
chmod 755 build/debian/DEBIAN/postinst
chmod 755 build/debian/DEBIAN/prerm

# Build the binary
echo "🔧 Compiling source..."
make clean
make release

# Copy binary and documentation
cp password_manager build/debian/usr/local/bin/
cp README.md build/debian/usr/share/doc/${PACKAGE_NAME}/
cp LICENSE build/debian/usr/share/doc/${PACKAGE_NAME}/
cp demo.md build/debian/usr/share/doc/${PACKAGE_NAME}/

# Set permissions
chmod 755 build/debian/usr/local/bin/password_manager

# Build the package
echo "📦 Creating Debian package..."
dpkg-deb --build build/debian "${PACKAGE_FULL}.deb"

# Verify the package
echo "✅ Verifying package..."
lintian "${PACKAGE_FULL}.deb" || true

echo "🎉 Package built successfully!"
echo "📝 Package information:"
dpkg -I "${PACKAGE_FULL}.deb"

echo ""
echo "To install the package:"
echo "sudo dpkg -i ${PACKAGE_FULL}.deb"
echo "sudo apt-get install -f  # Install dependencies if needed" 