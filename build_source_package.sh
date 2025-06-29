#!/bin/bash

# Exit on error
set -e

echo "🔨 Building source package for PPA..."

# Clean previous builds
rm -rf build-area
mkdir -p build-area

# Create orig tarball
tar --exclude='.git' --exclude='debian' --exclude='build-area' -czf build-area/secure-password-manager_1.0.0.orig.tar.gz .

# Build source package
debuild -S -sa -k16C2C3B5B97BD1F946C3FADE2EC89133D0ED9AD8

echo "✅ Source package built!"
echo ""
echo "To upload to your PPA, run:"
echo "dput ppa:ak0983/secure-password-manager ../secure-password-manager_1.0.0-1ubuntu1_source.changes" 