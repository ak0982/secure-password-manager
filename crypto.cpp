#include "crypto.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/kdf.h>
#include <stdexcept>
#include <cstring>
#include <iostream>

#ifndef AES_BLOCK_SIZE
#define AES_BLOCK_SIZE 16
#endif

namespace Crypto {

std::vector<uint8_t> deriveKey(const std::string& password, 
                              const std::vector<uint8_t>& salt, 
                              int iterations) {
    std::vector<uint8_t> key(AES_KEY_SIZE);
    
    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                          salt.data(), salt.size(),
                          iterations, EVP_sha256(),
                          AES_KEY_SIZE, key.data()) != 1) {
        throw std::runtime_error("Key derivation failed");
    }
    
    return key;
}

std::vector<uint8_t> generateRandomBytes(int size) {
    std::vector<uint8_t> bytes(size);
    if (RAND_bytes(bytes.data(), size) != 1) {
        throw std::runtime_error("Random byte generation failed");
    }
    return bytes;
}

EncryptedData encrypt(const std::string& plaintext, const std::string& password) {
    EncryptedData result;
    
    // Generate random salt and IV
    result.salt = generateRandomBytes(SALT_SIZE);
    result.iv = generateRandomBytes(AES_IV_SIZE);
    
    // Derive key from password
    std::vector<uint8_t> key = deriveKey(password, result.salt);
    
    // Initialize encryption context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create cipher context");
    }
    
    try {
        // Initialize encryption
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), result.iv.data()) != 1) {
            throw std::runtime_error("Encryption initialization failed");
        }
        
        // Calculate maximum ciphertext length
        int max_len = plaintext.length() + AES_BLOCK_SIZE;
        result.ciphertext.resize(max_len);
        
        int len = 0;
        int total_len = 0;
        
        // Encrypt the plaintext
        if (EVP_EncryptUpdate(ctx, result.ciphertext.data(), &len,
                             reinterpret_cast<const unsigned char*>(plaintext.c_str()),
                             plaintext.length()) != 1) {
            throw std::runtime_error("Encryption update failed");
        }
        total_len += len;
        
        // Finalize encryption (add padding)
        if (EVP_EncryptFinal_ex(ctx, result.ciphertext.data() + total_len, &len) != 1) {
            throw std::runtime_error("Encryption finalization failed");
        }
        total_len += len;
        
        // Resize to actual length
        result.ciphertext.resize(total_len);
        
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }
    
    EVP_CIPHER_CTX_free(ctx);
    return result;
}

std::string decrypt(const EncryptedData& encData, const std::string& password) {
    // Derive key from password using stored salt
    std::vector<uint8_t> key = deriveKey(password, encData.salt);
    
    // Initialize decryption context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create cipher context");
    }
    
    try {
        // Initialize decryption
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), encData.iv.data()) != 1) {
            throw std::runtime_error("Decryption initialization failed");
        }
        
        // Allocate buffer for plaintext
        std::vector<uint8_t> plaintext(encData.ciphertext.size() + AES_BLOCK_SIZE);
        
        int len = 0;
        int total_len = 0;
        
        // Decrypt the ciphertext
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                             encData.ciphertext.data(), encData.ciphertext.size()) != 1) {
            throw std::runtime_error("Decryption update failed - incorrect password?");
        }
        total_len += len;
        
        // Finalize decryption (remove padding)
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + total_len, &len) != 1) {
            throw std::runtime_error("Decryption finalization failed - incorrect password?");
        }
        total_len += len;
        
        EVP_CIPHER_CTX_free(ctx);
        
        // Convert to string
        return std::string(reinterpret_cast<char*>(plaintext.data()), total_len);
        
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }
}

std::vector<uint8_t> serialize(const EncryptedData& encData) {
    std::vector<uint8_t> result;
    
    // Format: [salt_size][salt][iv_size][iv][ciphertext_size][ciphertext]
    // Using 4-byte little-endian integers for sizes
    
    auto writeSize = [&](uint32_t size) {
        result.push_back(size & 0xFF);
        result.push_back((size >> 8) & 0xFF);
        result.push_back((size >> 16) & 0xFF);
        result.push_back((size >> 24) & 0xFF);
    };
    
    // Write salt
    writeSize(encData.salt.size());
    result.insert(result.end(), encData.salt.begin(), encData.salt.end());
    
    // Write IV
    writeSize(encData.iv.size());
    result.insert(result.end(), encData.iv.begin(), encData.iv.end());
    
    // Write ciphertext
    writeSize(encData.ciphertext.size());
    result.insert(result.end(), encData.ciphertext.begin(), encData.ciphertext.end());
    
    return result;
}

EncryptedData deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 12) { // At least 3 size headers
        throw std::runtime_error("Invalid encrypted data format");
    }
    
    EncryptedData result;
    size_t offset = 0;
    
    auto readSize = [&]() -> uint32_t {
        if (offset + 4 > data.size()) {
            throw std::runtime_error("Invalid encrypted data format");
        }
        uint32_t size = data[offset] | (data[offset + 1] << 8) | 
                       (data[offset + 2] << 16) | (data[offset + 3] << 24);
        offset += 4;
        return size;
    };
    
    auto readBytes = [&](uint32_t size) -> std::vector<uint8_t> {
        if (offset + size > data.size()) {
            throw std::runtime_error("Invalid encrypted data format");
        }
        std::vector<uint8_t> bytes(data.begin() + offset, data.begin() + offset + size);
        offset += size;
        return bytes;
    };
    
    // Read salt
    uint32_t saltSize = readSize();
    result.salt = readBytes(saltSize);
    
    // Read IV
    uint32_t ivSize = readSize();
    result.iv = readBytes(ivSize);
    
    // Read ciphertext
    uint32_t cipherSize = readSize();
    result.ciphertext = readBytes(cipherSize);
    
    return result;
}

bool verifyPassword(const EncryptedData& encData, const std::string& password) {
    try {
        decrypt(encData, password);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace Crypto 