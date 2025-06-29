# 🚀 Quick Demo Guide

## How to Run
```bash
./password_manager
```

## First Time Setup
1. The application will detect no vault exists
2. You'll be prompted to create a master password
3. Password strength will be validated
4. Confirm your master password

## Sample Commands to Try

### 1. Add a credential
```
🔐 > add
Service name: Gmail
Username: your.email@gmail.com
Password (leave empty to generate): [press Enter]
Generate password? (Y/n): y
Password length (default 16): 20
Include symbols? (Y/n): y
```

### 2. List all services
```
🔐 > list
```

### 3. Retrieve a password
```
🔐 > get
Service name: Gmail
```

### 4. Generate a random password
```
🔐 > generate
Password length (default 16): 16
Include symbols? (Y/n): y
```

### 5. Check vault status
```
🔐 > status
```

### 6. Exit securely
```
🔐 > exit
```

## Security Features in Action

- **Encryption**: All data is encrypted with AES-256
- **Auto-lock**: Vault locks after 2 minutes of inactivity
- **Secure Input**: Master password input is hidden
- **Memory Security**: Sensitive data is cleared from memory

## Testing Auto-Lock
1. Start the application and unlock your vault
2. Run a command like `status` to see the auto-lock timer
3. Wait 2 minutes without typing anything
4. Try running another command - you'll be prompted to re-authenticate

## File Created
- `vault.dat` - Your encrypted password vault (backup this file!)

## Clipboard Integration (macOS)
When retrieving passwords, you'll be offered to copy them to clipboard with automatic clearing after 30 seconds. 