/**
 * @file example_crypto_comms.cpp
 * @brief Example demonstrating RMH crypto-secure communications
 */

#include "crypto_comms.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace rmh::crypto;

void onMessageReceived(MessageType type,
                       const std::vector<uint8_t>& payload,
                       const MessageHeader& header) {
    std::cout << "Message received!" << std::endl;
    std::cout << "  Type: " << static_cast<int>(type) << std::endl;
    std::cout << "  From: Node " << static_cast<int>(header.sender_id) << std::endl;
    std::cout << "  Payload size: " << payload.size() << " bytes" << std::endl;
    std::cout << "  Message: " << std::string(payload.begin(), payload.end()) << std::endl;
}

int main() {
    std::cout << "=== RMH Crypto Communications Example ===" << std::endl;
    std::cout << std::endl;

    // Generate 256-bit keys
    std::cout << "Generating cryptographic keys..." << std::endl;
    std::vector<uint8_t> cipher_key = EncryptionEngine::generateRandom(32);
    std::vector<uint8_t> auth_key = EncryptionEngine::generateRandom(32);
    std::cout << "  Cipher key: " << cipher_key.size() << " bytes" << std::endl;
    std::cout << "  Auth key: " << auth_key.size() << " bytes" << std::endl;
    std::cout << std::endl;

    // Initialize node 1 (Tow Body)
    std::cout << "Initializing Node 1 (Tow Body)..." << std::endl;
    CryptoComms tb_comms(1, LinkType::TETHER);
    if (!tb_comms.initialize(cipher_key, auth_key)) {
        std::cerr << "Failed to initialize TB comms" << std::endl;
        return 1;
    }
    std::cout << "  Node ID: 1" << std::endl;
    std::cout << "  Link type: Tether" << std::endl;
    std::cout << std::endl;

    // Initialize node 2 (MCM USV)
    std::cout << "Initializing Node 2 (MCM USV)..." << std::endl;
    CryptoComms usv_comms(2, LinkType::TETHER);
    if (!usv_comms.initialize(cipher_key, auth_key)) {
        std::cerr << "Failed to initialize USV comms" << std::endl;
        return 1;
    }
    std::cout << "  Node ID: 2" << std::endl;
    std::cout << "  Link type: Tether" << std::endl;
    std::cout << std::endl;

    // Register message callback
    usv_comms.registerCallback(MessageType::SENSOR_TELEMETRY, onMessageReceived);
    usv_comms.registerCallback(MessageType::CONTACT_REPORT, onMessageReceived);

    // Start communications
    std::cout << "Starting communications..." << std::endl;
    tb_comms.start("0.0.0.0:5555");
    usv_comms.start("0.0.0.0:5556");
    std::cout << std::endl;

    // Send test messages
    std::cout << "Sending encrypted messages..." << std::endl;

    // Message 1: Sensor telemetry
    std::string msg1 = "TB position: 37.8N, 122.4W, depth 10m";
    std::vector<uint8_t> payload1(msg1.begin(), msg1.end());
    tb_comms.sendMessage(MessageType::SENSOR_TELEMETRY, payload1, 2, 128);
    std::cout << "  Sent: Sensor telemetry" << std::endl;

    // Message 2: Contact report
    std::string msg2 = "Mine detected at 37.85N, 122.45W, confidence 95%";
    std::vector<uint8_t> payload2(msg2.begin(), msg2.end());
    tb_comms.sendMessage(MessageType::CONTACT_REPORT, payload2, 2, 255);
    std::cout << "  Sent: Contact report (high priority)" << std::endl;
    std::cout << std::endl;

    // Wait for messages to be processed
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Display statistics
    std::cout << "TB Communications Statistics:" << std::endl;
    auto tb_stats = tb_comms.getStatistics();
    std::cout << "  Messages sent: " << tb_stats.messages_sent << std::endl;
    std::cout << "  Bytes transmitted: " << tb_stats.bytes_transmitted << std::endl;
    std::cout << "  Crypto errors: " << tb_stats.crypto_errors << std::endl;
    std::cout << std::endl;

    std::cout << "USV Communications Statistics:" << std::endl;
    auto usv_stats = usv_comms.getStatistics();
    std::cout << "  Messages received: " << usv_stats.messages_received << std::endl;
    std::cout << "  Bytes received: " << usv_stats.bytes_received << std::endl;
    std::cout << "  Messages dropped: " << usv_stats.messages_dropped << std::endl;
    std::cout << std::endl;

    // Test encryption/decryption directly
    std::cout << "Testing direct encryption/decryption..." << std::endl;
    std::string plaintext = "This is a secret mine hunting message";
    std::vector<uint8_t> plain(plaintext.begin(), plaintext.end());

    std::vector<uint8_t> iv;
    std::vector<uint8_t> encrypted = EncryptionEngine::encrypt(plain, cipher_key, iv);
    std::cout << "  Plaintext size: " << plain.size() << " bytes" << std::endl;
    std::cout << "  Encrypted size: " << encrypted.size() << " bytes" << std::endl;

    std::vector<uint8_t> decrypted = EncryptionEngine::decrypt(encrypted, cipher_key, iv);
    std::string recovered(decrypted.begin(), decrypted.end());
    std::cout << "  Decrypted: " << recovered << std::endl;
    std::cout << "  Match: " << (plaintext == recovered ? "YES" : "NO") << std::endl;
    std::cout << std::endl;

    // Stop communications
    std::cout << "Stopping communications..." << std::endl;
    tb_comms.stop();
    usv_comms.stop();
    std::cout << std::endl;

    std::cout << "=== Example Complete ===" << std::endl;

    return 0;
}
