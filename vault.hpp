#ifndef VAULT_HPP
#define VAULT_HPP

#include "crypto.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Vault {
    // Structure to represent a credential entry
    struct Credential {
        std::string service;
        std::string username;
        std::string password;
        
        Credential() = default;
        Credential(const std::string& srv, const std::string& user, const std::string& pass)
            : service(srv), username(user), password(pass) {}
    };

    // Password Manager class
    class PasswordManager {
    private:
        std::string vaultFilePath;
        std::string masterPassword;
        std::map<std::string, Credential> credentials;
        bool isLocked;
        Crypto::EncryptedData authData; // Used to verify master password

        /**
         * Serialize credentials to JSON-like string format
         * @return String representation of all credentials
         */
        std::string serializeCredentials() const;

        /**
         * Deserialize credentials from JSON-like string format
         * @param data String containing serialized credentials
         */
        void deserializeCredentials(const std::string& data);

        /**
         * Create authentication data for password verification
         * @param password Master password
         * @return Encrypted authentication data
         */
        Crypto::EncryptedData createAuthData(const std::string& password) const;

    public:
        /**
         * Constructor
         * @param vaultPath Path to the vault file (default: "vault.dat")
         */
        explicit PasswordManager(const std::string& vaultPath = "vault.dat");

        /**
         * Initialize vault with master password (for new vault)
         * @param password Master password
         * @return true if successful, false if vault already exists
         */
        bool initializeVault(const std::string& password);

        /**
         * Unlock vault with master password
         * @param password Master password
         * @return true if successful, false if incorrect password
         */
        bool unlock(const std::string& password);

        /**
         * Lock the vault (clear sensitive data from memory)
         */
        void lock();

        /**
         * Check if vault is currently locked
         * @return true if locked, false if unlocked
         */
        bool isVaultLocked() const { return isLocked; }

        /**
         * Check if vault file exists
         * @return true if vault file exists
         */
        bool vaultExists() const;

        /**
         * Add or update a credential
         * @param service Service name
         * @param username Username
         * @param password Password
         * @return true if successful
         */
        bool addCredential(const std::string& service, 
                          const std::string& username, 
                          const std::string& password);

        /**
         * Get a credential by service name
         * @param service Service name
         * @return Credential if found, empty credential if not found
         */
        Credential getCredential(const std::string& service) const;

        /**
         * Get all service names
         * @return Vector of service names
         */
        std::vector<std::string> getServices() const;

        /**
         * Remove a credential by service name
         * @param service Service name
         * @return true if removed, false if not found
         */
        bool removeCredential(const std::string& service);

        /**
         * Save current credentials to encrypted vault file
         * @return true if successful
         */
        bool saveVault();

        /**
         * Load credentials from encrypted vault file
         * @return true if successful
         */
        bool loadVault();

        /**
         * Get total number of credentials stored
         * @return Number of credentials
         */
        size_t getCredentialCount() const { return credentials.size(); }

        /**
         * Clear all sensitive data from memory (called on lock)
         */
        void clearSensitiveData();

        /**
         * Validate password strength
         * @param password Password to validate
         * @return Strength score (0-100) and description
         */
        static std::pair<int, std::string> validatePasswordStrength(const std::string& password);
    };

    // Utility functions
    namespace Utils {
        /**
         * Generate a secure random password
         * @param length Password length
         * @param includeSymbols Include special symbols
         * @return Generated password
         */
        std::string generatePassword(int length = 16, bool includeSymbols = true);

        /**
         * Securely clear string from memory
         * @param str String to clear
         */
        void secureErase(std::string& str);

        /**
         * Get hidden password input from user
         * @param prompt Prompt message
         * @return Password entered by user
         */
        std::string getHiddenInput(const std::string& prompt);
    }
}

#endif // VAULT_HPP 