#include "vault.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <regex>

namespace Vault {

// PasswordManager Implementation
PasswordManager::PasswordManager(const std::string& vaultPath) 
    : vaultFilePath(vaultPath), isLocked(true) {}

bool PasswordManager::initializeVault(const std::string& password) {
    if (vaultExists()) {
        return false; // Vault already exists
    }
    
    masterPassword = password;
    isLocked = false;
    
    // Create authentication data for password verification
    authData = createAuthData(password);
    
    // Save initial empty vault
    return saveVault();
}

bool PasswordManager::unlock(const std::string& password) {
    if (!vaultExists()) {
        return false;
    }
    
    if (!loadVault()) {
        return false;
    }
    
    // Verify password using authentication data
    if (!Crypto::verifyPassword(authData, password)) {
        return false;
    }
    
    masterPassword = password;
    isLocked = false;
    
    // Load and decrypt all credentials
    return loadVault();
}

void PasswordManager::lock() {
    clearSensitiveData();
    isLocked = true;
}

bool PasswordManager::vaultExists() const {
    std::ifstream file(vaultFilePath);
    return file.good();
}

bool PasswordManager::addCredential(const std::string& service, 
                                  const std::string& username, 
                                  const std::string& password) {
    if (isLocked) return false;
    
    credentials[service] = Credential(service, username, password);
    return saveVault();
}

Credential PasswordManager::getCredential(const std::string& service) const {
    if (isLocked) return Credential();
    
    auto it = credentials.find(service);
    if (it != credentials.end()) {
        return it->second;
    }
    return Credential();
}

std::vector<std::string> PasswordManager::getServices() const {
    std::vector<std::string> services;
    if (isLocked) return services;
    
    for (const auto& pair : credentials) {
        services.push_back(pair.first);
    }
    std::sort(services.begin(), services.end());
    return services;
}

bool PasswordManager::removeCredential(const std::string& service) {
    if (isLocked) return false;
    
    auto it = credentials.find(service);
    if (it != credentials.end()) {
        credentials.erase(it);
        return saveVault();
    }
    return false;
}

std::string PasswordManager::serializeCredentials() const {
    std::ostringstream oss;
    oss << "AUTH_DATA_START\n";
    
    // Serialize authentication data
    auto authSerialized = Crypto::serialize(authData);
    oss << authSerialized.size() << "\n";
    for (uint8_t byte : authSerialized) {
        oss << static_cast<int>(byte) << " ";
    }
    oss << "\nAUTH_DATA_END\n";
    
    oss << "CREDENTIALS_START\n";
    oss << credentials.size() << "\n";
    
    for (const auto& pair : credentials) {
        const Credential& cred = pair.second;
        oss << "SERVICE:" << cred.service << "\n";
        oss << "USERNAME:" << cred.username << "\n";
        oss << "PASSWORD:" << cred.password << "\n";
        oss << "---\n";
    }
    oss << "CREDENTIALS_END\n";
    
    return oss.str();
}

void PasswordManager::deserializeCredentials(const std::string& data) {
    std::istringstream iss(data);
    std::string line;
    
    credentials.clear();
    
    // Parse authentication data
    while (std::getline(iss, line) && line != "AUTH_DATA_START") {}
    
    if (std::getline(iss, line)) {
        size_t authSize = std::stoul(line);
        std::vector<uint8_t> authBytes;
        
        if (std::getline(iss, line)) {
            std::istringstream authStream(line);
            int byte;
            while (authStream >> byte) {
                authBytes.push_back(static_cast<uint8_t>(byte));
            }
        }
        
        if (authBytes.size() == authSize) {
            authData = Crypto::deserialize(authBytes);
        }
    }
    
    // Skip to credentials section
    while (std::getline(iss, line) && line != "CREDENTIALS_START") {}
    
    if (std::getline(iss, line)) {
        size_t credCount = std::stoul(line);
        
        for (size_t i = 0; i < credCount; ++i) {
            Credential cred;
            
            if (std::getline(iss, line) && line.substr(0, 8) == "SERVICE:") {
                cred.service = line.substr(8);
            }
            if (std::getline(iss, line) && line.substr(0, 9) == "USERNAME:") {
                cred.username = line.substr(9);
            }
            if (std::getline(iss, line) && line.substr(0, 9) == "PASSWORD:") {
                cred.password = line.substr(9);
            }
            
            if (!cred.service.empty()) {
                credentials[cred.service] = cred;
            }
            
            std::getline(iss, line); // Skip separator
        }
    }
}

Crypto::EncryptedData PasswordManager::createAuthData(const std::string& password) const {
    // Create a known plaintext to verify password correctness
    const std::string authPlaintext = "VAULT_AUTH_CHECK";
    return Crypto::encrypt(authPlaintext, password);
}

bool PasswordManager::saveVault() {
    if (isLocked) return false;
    
    try {
        std::string serialized = serializeCredentials();
        Crypto::EncryptedData encrypted = Crypto::encrypt(serialized, masterPassword);
        std::vector<uint8_t> fileData = Crypto::serialize(encrypted);
        
        std::ofstream file(vaultFilePath, std::ios::binary);
        if (!file) return false;
        
        file.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
        return file.good();
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving vault: " << e.what() << std::endl;
        return false;
    }
}

bool PasswordManager::loadVault() {
    try {
        std::ifstream file(vaultFilePath, std::ios::binary);
        if (!file) return false;
        
        // Read entire file
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> fileData(fileSize);
        file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
        
        if (!file.good()) return false;
        
        // Deserialize and decrypt
        Crypto::EncryptedData encrypted = Crypto::deserialize(fileData);
        
        if (isLocked) {
            // Just load auth data for password verification
            authData = encrypted;
            return true;
        }
        
        std::string decrypted = Crypto::decrypt(encrypted, masterPassword);
        deserializeCredentials(decrypted);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading vault: " << e.what() << std::endl;
        return false;
    }
}

void PasswordManager::clearSensitiveData() {
    Utils::secureErase(masterPassword);
    credentials.clear();
}

std::pair<int, std::string> PasswordManager::validatePasswordStrength(const std::string& password) {
    int score = 0;
    std::string feedback;
    
    if (password.length() >= 8) score += 20;
    else feedback += "Use at least 8 characters. ";
    
    if (password.length() >= 12) score += 10;
    
    if (std::regex_search(password, std::regex("[a-z]"))) score += 15;
    else feedback += "Add lowercase letters. ";
    
    if (std::regex_search(password, std::regex("[A-Z]"))) score += 15;
    else feedback += "Add uppercase letters. ";
    
    if (std::regex_search(password, std::regex("[0-9]"))) score += 15;
    else feedback += "Add numbers. ";
    
    if (std::regex_search(password, std::regex("[^a-zA-Z0-9]"))) score += 25;
    else feedback += "Add special characters. ";
    
    std::string strength;
    if (score < 40) strength = "Weak";
    else if (score < 70) strength = "Moderate";
    else if (score < 90) strength = "Strong";
    else strength = "Very Strong";
    
    if (feedback.empty()) feedback = "Good password!";
    
    return {score, strength + ": " + feedback};
}

// Utility Functions Implementation
namespace Utils {

std::string generatePassword(int length, bool includeSymbols) {
    const std::string lowercase = "abcdefghijklmnopqrstuvwxyz";
    const std::string uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const std::string numbers = "0123456789";
    const std::string symbols = "!@#$%^&*()_+-=[]{}|;:,.<>?";
    
    std::string charset = lowercase + uppercase + numbers;
    if (includeSymbols) {
        charset += symbols;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, charset.size() - 1);
    
    std::string password;
    password.reserve(length);
    
    for (int i = 0; i < length; ++i) {
        password += charset[dis(gen)];
    }
    
    return password;
}

void secureErase(std::string& str) {
    std::fill(str.begin(), str.end(), '\0');
    str.clear();
}

std::string getHiddenInput(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();
    
    // Disable echo
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    std::string input;
    std::getline(std::cin, input);
    
    // Restore echo
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
    std::cout << std::endl;
    return input;
}

} // namespace Utils

} // namespace Vault 