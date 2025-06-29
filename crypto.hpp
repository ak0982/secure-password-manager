#ifndef CRYPTO_HPP
#define CRYPTO_HPP

#include <string>
#include <vector>
#include <cstdint>

namespace Crypto {
    // Constants for encryption
    const int AES_KEY_SIZE = 32;  // 256 bits
    const int AES_IV_SIZE = 16;   // 128 bits
    const int SALT_SIZE = 16;     // 128 bits for PBKDF2
    const int PBKDF2_ITERATIONS = 100000;

    // Structure to hold encrypted data with metadata
    struct EncryptedData {
        std::vector<uint8_t> salt;
        std::vector<uint8_t> iv;
        std::vector<uint8_t> ciphertext;
    };

    /**
     * Derive encryption key from master password using PBKDF2
     * @param password Master password
     * @param salt Random salt for key derivation
     * @param iterations Number of PBKDF2 iterations
     * @return 256-bit derived key
     */
    std::vector<uint8_t> deriveKey(const std::string& password, 
                                  const std::vector<uint8_t>& salt, 
                                  int iterations = PBKDF2_ITERATIONS);

    /**
     * Generate cryptographically secure random bytes
     * @param size Number of bytes to generate
     * @return Vector containing random bytes
     */
    std::vector<uint8_t> generateRandomBytes(int size);

    /**
     * Encrypt plaintext using AES-256-CBC
     * @param plaintext Data to encrypt
     * @param password Master password for key derivation
     * @return EncryptedData structure containing salt, IV, and ciphertext
     */
    EncryptedData encrypt(const std::string& plaintext, const std::string& password);

    /**
     * Decrypt ciphertext using AES-256-CBC
     * @param encData EncryptedData structure containing encrypted data
     * @param password Master password for key derivation
     * @return Decrypted plaintext string
     */
    std::string decrypt(const EncryptedData& encData, const std::string& password);

    /**
     * Serialize encrypted data to binary format for file storage
     * @param encData EncryptedData to serialize
     * @return Binary data suitable for file storage
     */
    std::vector<uint8_t> serialize(const EncryptedData& encData);

    /**
     * Deserialize binary data back to EncryptedData structure
     * @param data Binary data from file
     * @return EncryptedData structure
     */
    EncryptedData deserialize(const std::vector<uint8_t>& data);

    /**
     * Verify if a password is correct by attempting to decrypt a test block
     * @param encData Encrypted data to test against
     * @param password Password to verify
     * @return true if password is correct, false otherwise
     */
    bool verifyPassword(const EncryptedData& encData, const std::string& password);
}

#endif // CRYPTO_HPP 