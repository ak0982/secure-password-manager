#!/bin/bash

echo "🚀 Setting up PPA for Secure Password Manager"
echo "-------------------------------------------"

# Check for required tools
check_tool() {
    if ! command -v $1 &> /dev/null; then
        echo "❌ $1 is required but not installed."
        echo "Please install it with: sudo apt install $2"
        exit 1
    fi
}

check_tool "dput" "dput"
check_tool "debuild" "devscripts"
check_tool "gpg" "gnupg"

# Check if GPG key exists
if ! gpg --list-secret-keys | grep -q "amar.edu09@gmail.com"; then
    echo "⚠️  No GPG key found for amar.edu09@gmail.com"
    echo "Would you like to create one? (y/N)"
    read -r response
    if [[ "$response" =~ ^([yY][eE][sS]|[yY])+$ ]]; then
        echo "Creating GPG key..."
        gpg --gen-key
    else
        echo "❌ GPG key is required for PPA uploads. Exiting."
        exit 1
    fi
fi

# Create source package
echo "📦 Creating source package..."
debuild -S -sa

# Instructions for PPA setup
echo """
🎉 Source package created!

To upload to Launchpad:

1. First, create a Launchpad account at https://launchpad.net/
2. Upload your GPG key to Launchpad:
   gpg --keyserver keyserver.ubuntu.com --send-keys YOUR_KEY_ID

3. Create a PPA on Launchpad:
   - Go to https://launchpad.net/~your-username
   - Click 'Create a new PPA'
   - Name it 'secure-password-manager'

4. Upload the package:
   dput ppa:your-username/secure-password-manager ../secure-password-manager_1.0.0-1_source.changes

After upload, wait for Launchpad to build and publish your package.
Users can then install it with:

sudo add-apt-repository ppa:your-username/secure-password-manager
sudo apt update
sudo apt install secure-password-manager

Would you like to proceed with the upload now? (y/N)"""
read -r response

if [[ "$response" =~ ^([yY][eE][sS]|[yY])+$ ]]; then
    echo "Please enter your Launchpad username:"
    read -r username
    
    echo "Uploading to PPA..."
    dput ppa:$username/secure-password-manager ../secure-password-manager_1.0.0-1_source.changes
    
    echo """
    ✅ Package uploaded!
    
    Check your PPA page for build status:
    https://launchpad.net/~$username/+archive/ubuntu/secure-password-manager
    """
fi 