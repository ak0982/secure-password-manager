#include "vault.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <signal.h>
#include <sstream>
#include <iomanip>

class PasswordManagerCLI {
private:
    Vault::PasswordManager vault;
    std::atomic<bool> running{true};
    std::atomic<std::chrono::steady_clock::time_point> lastActivity;
    static constexpr int AUTO_LOCK_MINUTES = 2;
    
    void printWelcome() {
        std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
        std::cout << "║            🔐 Secure Password Manager 🔐             ║\n";
        std::cout << "║                                                      ║\n";
        std::cout << "║  Your passwords are encrypted with AES-256 and      ║\n";
        std::cout << "║  protected by PBKDF2 key derivation.                ║\n";
        std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    }
    
    void printCommands() {
        std::cout << "\n📋 Available Commands:\n";
        std::cout << "  add     - Add a new service credential\n";
        std::cout << "  get     - Retrieve password for a service\n";
        std::cout << "  list    - List all saved services\n";
        std::cout << "  remove  - Remove a service credential\n";
        std::cout << "  generate- Generate a secure password\n";
        std::cout << "  status  - Show vault status\n";
        std::cout << "  help    - Show this help message\n";
        std::cout << "  exit    - Exit and lock the vault\n";
        std::cout << "\n⏰ Auto-lock: " << AUTO_LOCK_MINUTES << " minutes of inactivity\n\n";
    }
    
    void updateActivity() {
        lastActivity.store(std::chrono::steady_clock::now());
    }
    
    bool authenticate() {
        updateActivity();
        
        if (vault.vaultExists()) {
            std::cout << "🔒 Vault found. Please enter your master password.\n";
            std::string password = Vault::Utils::getHiddenInput("Master Password: ");
            
            if (vault.unlock(password)) {
                std::cout << "✅ Vault unlocked successfully!\n";
                Vault::Utils::secureErase(password);
                return true;
            } else {
                std::cout << "❌ Incorrect password!\n";
                Vault::Utils::secureErase(password);
                return false;
            }
        } else {
            std::cout << "🆕 No vault found. Creating a new vault.\n";
            std::string password, confirmPassword;
            
            do {
                password = Vault::Utils::getHiddenInput("Create Master Password: ");
                confirmPassword = Vault::Utils::getHiddenInput("Confirm Master Password: ");
                
                if (password != confirmPassword) {
                    std::cout << "❌ Passwords don't match. Please try again.\n";
                    Vault::Utils::secureErase(password);
                    Vault::Utils::secureErase(confirmPassword);
                } else {
                    auto [score, feedback] = Vault::PasswordManager::validatePasswordStrength(password);
                    std::cout << "Password Strength: " << feedback << "\n";
                    
                    if (score < 40) {
                        std::cout << "⚠️  Weak password detected. Continue anyway? (y/N): ";
                        std::string choice;
                        std::getline(std::cin, choice);
                        if (choice != "y" && choice != "Y") {
                            Vault::Utils::secureErase(password);
                            Vault::Utils::secureErase(confirmPassword);
                            continue;
                        }
                    }
                    break;
                }
            } while (true);
            
            if (vault.initializeVault(password)) {
                std::cout << "✅ Vault created successfully!\n";
                Vault::Utils::secureErase(password);
                Vault::Utils::secureErase(confirmPassword);
                return true;
            } else {
                std::cout << "❌ Failed to create vault!\n";
                Vault::Utils::secureErase(password);
                Vault::Utils::secureErase(confirmPassword);
                return false;
            }
        }
    }
    
    void handleAddCommand() {
        updateActivity();
        
        std::string service, username, password;
        
        std::cout << "Service name: ";
        std::getline(std::cin, service);
        
        if (service.empty()) {
            std::cout << "❌ Service name cannot be empty!\n";
            return;
        }
        
        // Check if service already exists
        auto existing = vault.getCredential(service);
        if (!existing.service.empty()) {
            std::cout << "⚠️  Service '" << service << "' already exists. Update? (y/N): ";
            std::string choice;
            std::getline(std::cin, choice);
            if (choice != "y" && choice != "Y") {
                return;
            }
        }
        
        std::cout << "Username: ";
        std::getline(std::cin, username);
        
        std::cout << "Password (leave empty to generate): ";
        password = Vault::Utils::getHiddenInput("");
        
        if (password.empty()) {
            std::cout << "Generate password? (Y/n): ";
            std::string choice;
            std::getline(std::cin, choice);
            if (choice != "n" && choice != "N") {
                std::cout << "Password length (default 16): ";
                std::string lengthStr;
                std::getline(std::cin, lengthStr);
                int length = lengthStr.empty() ? 16 : std::stoi(lengthStr);
                
                std::cout << "Include symbols? (Y/n): ";
                std::getline(std::cin, choice);
                bool includeSymbols = (choice != "n" && choice != "N");
                
                password = Vault::Utils::generatePassword(length, includeSymbols);
                std::cout << "Generated password: " << password << "\n";
                std::cout << "Press Enter to continue...";
                std::cin.get();
            }
        }
        
        if (vault.addCredential(service, username, password)) {
            std::cout << "✅ Credential added successfully!\n";
        } else {
            std::cout << "❌ Failed to add credential!\n";
        }
        
        Vault::Utils::secureErase(password);
    }
    
    void handleGetCommand() {
        updateActivity();
        
        std::string service;
        std::cout << "Service name: ";
        std::getline(std::cin, service);
        
        auto credential = vault.getCredential(service);
        if (credential.service.empty()) {
            std::cout << "❌ Service '" << service << "' not found!\n";
            return;
        }
        
        std::cout << "\n📋 Credential Details:\n";
        std::cout << "Service:  " << credential.service << "\n";
        std::cout << "Username: " << credential.username << "\n";
        std::cout << "Password: " << credential.password << "\n";
        
        // Optional: Copy to clipboard (platform-dependent)
        #ifdef __APPLE__
        std::cout << "\n📋 Copy password to clipboard? (y/N): ";
        std::string choice;
        std::getline(std::cin, choice);
        if (choice == "y" || choice == "Y") {
            std::string command = "echo '" + credential.password + "' | pbcopy";
            system(command.c_str());
            std::cout << "✅ Password copied to clipboard!\n";
            
            // Clear clipboard after 30 seconds
            std::thread([]{
                std::this_thread::sleep_for(std::chrono::seconds(30));
                system("echo '' | pbcopy");
                std::cout << "\n🔒 Clipboard cleared after 30 seconds.\n";
            }).detach();
        }
        #endif
    }
    
    void handleListCommand() {
        updateActivity();
        
        auto services = vault.getServices();
        if (services.empty()) {
            std::cout << "📭 No services stored in vault.\n";
            return;
        }
        
        std::cout << "\n📋 Stored Services (" << services.size() << " total):\n";
        std::cout << "╔═══════════════════════════════════════════════════════╗\n";
        
        for (const auto& service : services) {
            auto cred = vault.getCredential(service);
            std::cout << "║ " << std::left << std::setw(20) << service 
                     << " │ " << std::setw(25) << cred.username << " ║\n";
        }
        
        std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    }
    
    void handleRemoveCommand() {
        updateActivity();
        
        std::string service;
        std::cout << "Service name to remove: ";
        std::getline(std::cin, service);
        
        auto credential = vault.getCredential(service);
        if (credential.service.empty()) {
            std::cout << "❌ Service '" << service << "' not found!\n";
            return;
        }
        
        std::cout << "⚠️  Are you sure you want to remove '" << service << "'? (y/N): ";
        std::string choice;
        std::getline(std::cin, choice);
        
        if (choice == "y" || choice == "Y") {
            if (vault.removeCredential(service)) {
                std::cout << "✅ Service '" << service << "' removed successfully!\n";
            } else {
                std::cout << "❌ Failed to remove service!\n";
            }
        }
    }
    
    void handleGenerateCommand() {
        updateActivity();
        
        std::cout << "Password length (default 16): ";
        std::string lengthStr;
        std::getline(std::cin, lengthStr);
        int length = lengthStr.empty() ? 16 : std::stoi(lengthStr);
        
        std::cout << "Include symbols? (Y/n): ";
        std::string choice;
        std::getline(std::cin, choice);
        bool includeSymbols = (choice != "n" && choice != "N");
        
        std::string password = Vault::Utils::generatePassword(length, includeSymbols);
        auto [score, feedback] = Vault::PasswordManager::validatePasswordStrength(password);
        
        std::cout << "\n🎲 Generated Password: " << password << "\n";
        std::cout << "Strength: " << feedback << "\n";
    }
    
    void handleStatusCommand() {
        updateActivity();
        
        std::cout << "\n📊 Vault Status:\n";
        std::cout << "Vault File: " << (vault.vaultExists() ? "✅ Exists" : "❌ Not Found") << "\n";
        std::cout << "Status: " << (vault.isVaultLocked() ? "🔒 Locked" : "🔓 Unlocked") << "\n";
        std::cout << "Total Credentials: " << vault.getCredentialCount() << "\n";
        
        auto now = std::chrono::steady_clock::now();
        auto timeSinceActivity = std::chrono::duration_cast<std::chrono::seconds>(
            now - lastActivity.load()).count();
        
        int remainingTime = (AUTO_LOCK_MINUTES * 60) - timeSinceActivity;
        if (remainingTime > 0) {
            std::cout << "Auto-lock in: " << remainingTime << " seconds\n";
        }
    }
    
    void checkAutoLock() {
        auto now = std::chrono::steady_clock::now();
        auto timeSinceActivity = std::chrono::duration_cast<std::chrono::minutes>(
            now - lastActivity.load()).count();
        
        if (timeSinceActivity >= AUTO_LOCK_MINUTES && !vault.isVaultLocked()) {
            std::cout << "\n⏰ Auto-locking vault due to inactivity...\n";
            vault.lock();
            std::cout << "🔒 Vault locked. Please authenticate to continue.\n";
        }
    }
    
    void autoLockWorker() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            if (running) {
                checkAutoLock();
            }
        }
    }

public:
    PasswordManagerCLI() : vault("vault.dat") {
        lastActivity.store(std::chrono::steady_clock::now());
        
        // Handle Ctrl+C gracefully
        signal(SIGINT, [](int) {
            std::cout << "\n🔒 Locking vault and exiting...\n";
            exit(0);
        });
    }
    
    void run() {
        printWelcome();
        
        if (!authenticate()) {
            return;
        }
        
        // Start auto-lock thread
        std::thread autoLockThread(&PasswordManagerCLI::autoLockWorker, this);
        
        printCommands();
        
        std::string command;
        while (running) {
            // Check if vault is locked
            if (vault.isVaultLocked()) {
                std::cout << "🔒 Vault is locked. Please authenticate.\n";
                if (!authenticate()) {
                    std::cout << "❌ Authentication failed. Exiting...\n";
                    break;
                }
            }
            
            std::cout << "🔐 > ";
            if (!std::getline(std::cin, command)) {
                break; // EOF
            }
            
            updateActivity();
            
            if (command.empty()) continue;
            
            // Parse command
            std::istringstream iss(command);
            std::string cmd;
            iss >> cmd;
            
            if (cmd == "add") {
                handleAddCommand();
            } else if (cmd == "get") {
                handleGetCommand();
            } else if (cmd == "list") {
                handleListCommand();
            } else if (cmd == "remove") {
                handleRemoveCommand();
            } else if (cmd == "generate") {
                handleGenerateCommand();
            } else if (cmd == "status") {
                handleStatusCommand();
            } else if (cmd == "help") {
                printCommands();
            } else if (cmd == "exit") {
                running = false;
            } else {
                std::cout << "❓ Unknown command. Type 'help' for available commands.\n";
            }
            
            std::cout << "\n";
        }
        
        running = false;
        if (autoLockThread.joinable()) {
            autoLockThread.join();
        }
        
        vault.lock();
        std::cout << "🔒 Vault locked. Goodbye!\n";
    }
};

int main() {
    try {
        PasswordManagerCLI app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 