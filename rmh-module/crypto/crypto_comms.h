/**
 * @file crypto_comms.h
 * @brief RMH Crypto-Secure Communications Module
 *
 * Provides encrypted, authenticated communications between Tow Body (TB),
 * MCM USV, and ATAK C2 station. Leverages ATAK's MeshNetCrypto with
 * additional anti-jamming and anti-spoofing protections.
 *
 * @author Primal Logic
 * @date 2025-11-23
 * @classification UNCLASSIFIED
 */

#ifndef RMH_CRYPTO_COMMS_H
#define RMH_CRYPTO_COMMS_H

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <chrono>

namespace rmh {
namespace crypto {

/**
 * @brief Message types in RMH protocol
 */
enum class MessageType : uint8_t {
    // TB -> MCM USV
    SENSOR_TELEMETRY = 0x01,      // Continuous sensor data stream
    CONTACT_REPORT = 0x02,         // Detected mine contact
    STATUS_UPDATE = 0x03,          // TB health and position
    ACKNOWLEDGMENT = 0x04,         // Command acknowledgment

    // MCM USV -> TB
    COMMAND_WAYPOINT = 0x10,       // Navigation waypoint
    COMMAND_MODE_CHANGE = 0x11,    // Change autonomy mode
    COMMAND_EMERGENCY = 0x12,      // Emergency surface/stop
    COMMAND_CONFIG = 0x13,         // Update TB configuration

    // Bidirectional
    KEY_EXCHANGE = 0x20,           // Cryptographic key exchange
    HEARTBEAT = 0x21,              // Keep-alive ping
    TIME_SYNC = 0x22               // Clock synchronization
};

/**
 * @brief Communication link types
 */
enum class LinkType {
    TETHER,                        // Physical tether (high bandwidth)
    ACOUSTIC,                      // Underwater acoustic (low bandwidth)
    RF_SURFACE                     // RF when TB is surfaced
};

/**
 * @brief Security level for message classification
 */
enum class SecurityLevel {
    UNCLASSIFIED,
    CONTROLLED_UNCLASSIFIED,       // CUI
    CONFIDENTIAL,
    SECRET
};

/**
 * @brief Message header (sent in cleartext for routing)
 */
struct MessageHeader {
    uint32_t message_id;           // Unique message identifier
    MessageType type;              // Message type
    uint16_t payload_length;       // Encrypted payload size (bytes)
    uint64_t timestamp_ms;         // Unix timestamp (ms)
    uint8_t sender_id;             // Sender node ID
    uint8_t receiver_id;           // Receiver node ID (0xFF = broadcast)
    uint8_t hop_count;             // For multi-hop routing
    uint8_t priority;              // 0 (low) to 255 (critical)
} __attribute__((packed));

/**
 * @brief Encrypted message envelope
 */
struct SecureMessage {
    MessageHeader header;
    std::vector<uint8_t> encrypted_payload;  // AES-256-GCM encrypted
    std::vector<uint8_t> auth_tag;           // HMAC-SHA256 authentication
    std::vector<uint8_t> iv;                 // Initialization vector
};

/**
 * @brief Cryptographic key material
 */
struct KeyMaterial {
    std::vector<uint8_t> cipher_key;    // 256-bit AES key
    std::vector<uint8_t> auth_key;      // 256-bit HMAC key
    uint64_t key_id;                    // Key identifier
    uint64_t expiry_time_ms;            // Key expiration timestamp
};

/**
 * @brief Communication statistics
 */
struct CommsStats {
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t messages_dropped;          // Failed authentication or crypto
    uint64_t bytes_transmitted;
    uint64_t bytes_received;
    float packet_loss_rate;             // Percentage
    float average_latency_ms;           // Round-trip time
    uint64_t crypto_errors;             // Decryption/auth failures
};

/**
 * @brief Callback for received messages
 */
using MessageCallback = std::function<void(const MessageType type,
                                           const std::vector<uint8_t>& payload,
                                           const MessageHeader& header)>;

/**
 * @brief Main crypto communications engine
 */
class CryptoComms {
public:
    /**
     * @brief Constructor
     * @param node_id This node's ID (unique within network)
     * @param link_type Primary communication link type
     */
    CryptoComms(uint8_t node_id, LinkType link_type);

    /**
     * @brief Destructor
     */
    ~CryptoComms();

    /**
     * @brief Initialize crypto with pre-shared keys
     * @param cipher_key 256-bit encryption key
     * @param auth_key 256-bit authentication key
     * @return true if successful
     */
    bool initialize(const std::vector<uint8_t>& cipher_key,
                   const std::vector<uint8_t>& auth_key);

    /**
     * @brief Start communications (begin listening for messages)
     * @param bind_address Network address to bind to (e.g., "0.0.0.0:5555")
     * @return true if successful
     */
    bool start(const std::string& bind_address);

    /**
     * @brief Stop communications
     */
    void stop();

    /**
     * @brief Send encrypted message
     * @param type Message type
     * @param payload Plaintext payload (will be encrypted)
     * @param receiver_id Destination node ID
     * @param priority Message priority (0-255)
     * @return true if message queued successfully
     */
    bool sendMessage(MessageType type,
                    const std::vector<uint8_t>& payload,
                    uint8_t receiver_id,
                    uint8_t priority = 128);

    /**
     * @brief Register callback for received messages
     * @param type Message type to listen for
     * @param callback Function to call when message received
     */
    void registerCallback(MessageType type, MessageCallback callback);

    /**
     * @brief Perform ECDH key exchange with peer
     * @param peer_id Peer node ID
     * @return true if key exchange successful
     */
    bool performKeyExchange(uint8_t peer_id);

    /**
     * @brief Rotate session keys (should be called periodically)
     * @return true if rotation successful
     */
    bool rotateKeys();

    /**
     * @brief Get communication statistics
     * @return Current statistics
     */
    CommsStats getStatistics() const;

    /**
     * @brief Set security level for outgoing messages
     * @param level Security classification level
     */
    void setSecurityLevel(SecurityLevel level);

    /**
     * @brief Enable/disable frequency hopping (anti-jamming)
     * @param enable true to enable frequency hopping
     * @param hop_interval_ms Time between frequency hops
     */
    void setFrequencyHopping(bool enable, uint32_t hop_interval_ms = 1000);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Encryption utilities (wraps ATAK MeshNetCrypto)
 */
class EncryptionEngine {
public:
    /**
     * @brief Encrypt data with AES-256-GCM
     * @param plaintext Input data
     * @param key 256-bit key
     * @param iv Initialization vector (will be generated if empty)
     * @return Encrypted data + authentication tag
     */
    static std::vector<uint8_t> encrypt(
        const std::vector<uint8_t>& plaintext,
        const std::vector<uint8_t>& key,
        std::vector<uint8_t>& iv);

    /**
     * @brief Decrypt data with AES-256-GCM
     * @param ciphertext Encrypted data + auth tag
     * @param key 256-bit key
     * @param iv Initialization vector
     * @return Decrypted plaintext (empty if authentication fails)
     */
    static std::vector<uint8_t> decrypt(
        const std::vector<uint8_t>& ciphertext,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv);

    /**
     * @brief Compute HMAC-SHA256
     * @param data Input data
     * @param key HMAC key
     * @return HMAC tag
     */
    static std::vector<uint8_t> computeHMAC(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& key);

    /**
     * @brief Verify HMAC-SHA256
     * @param data Input data
     * @param key HMAC key
     * @param tag Expected HMAC tag
     * @return true if tag matches
     */
    static bool verifyHMAC(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& tag);

    /**
     * @brief Generate cryptographically secure random bytes
     * @param length Number of bytes to generate
     * @return Random bytes
     */
    static std::vector<uint8_t> generateRandom(size_t length);
};

/**
 * @brief Key exchange using Elliptic Curve Diffie-Hellman (ECDH)
 */
class KeyExchange {
public:
    KeyExchange();
    ~KeyExchange();

    /**
     * @brief Generate ephemeral key pair
     * @return Public key to send to peer
     */
    std::vector<uint8_t> generateKeyPair();

    /**
     * @brief Derive shared secret from peer's public key
     * @param peer_public_key Peer's public key
     * @return Shared secret (use for deriving session keys)
     */
    std::vector<uint8_t> deriveSharedSecret(
        const std::vector<uint8_t>& peer_public_key);

    /**
     * @brief Derive session keys from shared secret
     * @param shared_secret Output from deriveSharedSecret()
     * @return KeyMaterial with cipher and auth keys
     */
    static KeyMaterial deriveSessionKeys(
        const std::vector<uint8_t>& shared_secret);

private:
    void* ecdh_ctx_;  // Opaque ECDH context
};

/**
 * @brief Anti-jamming spread spectrum modulation
 */
class SpreadSpectrum {
public:
    /**
     * @brief Apply frequency hopping spread spectrum (FHSS)
     * @param data Input data
     * @param hop_sequence Frequency hop sequence
     * @param current_hop Current hop index
     * @return Modulated data
     */
    static std::vector<uint8_t> applyFHSS(
        const std::vector<uint8_t>& data,
        const std::vector<uint32_t>& hop_sequence,
        uint32_t current_hop);

    /**
     * @brief Demodulate FHSS data
     * @param modulated_data FHSS modulated data
     * @param hop_sequence Frequency hop sequence (must match sender)
     * @param current_hop Current hop index
     * @return Demodulated data
     */
    static std::vector<uint8_t> demodFHSS(
        const std::vector<uint8_t>& modulated_data,
        const std::vector<uint32_t>& hop_sequence,
        uint32_t current_hop);

    /**
     * @brief Generate pseudo-random hop sequence
     * @param seed Seed for PRNG (shared secret)
     * @param num_hops Number of frequencies in sequence
     * @return Hop sequence
     */
    static std::vector<uint32_t> generateHopSequence(
        uint64_t seed,
        size_t num_hops);
};

/**
 * @brief Network time synchronization (for replay attack prevention)
 */
class TimeSync {
public:
    /**
     * @brief Synchronize local clock with remote clock
     * @param remote_timestamp Remote timestamp (ms)
     * @param round_trip_time Measured RTT (ms)
     * @return Estimated clock offset (ms)
     */
    static int64_t synchronize(uint64_t remote_timestamp,
                               uint64_t round_trip_time);

    /**
     * @brief Check if timestamp is within acceptable window
     * @param timestamp Timestamp to check
     * @param window_ms Acceptable time window
     * @return true if timestamp is valid (not replayed)
     */
    static bool validateTimestamp(uint64_t timestamp,
                                  uint64_t window_ms = 5000);

    /**
     * @brief Get current synchronized timestamp
     * @return Unix timestamp in milliseconds
     */
    static uint64_t getCurrentTime();
};

} // namespace crypto
} // namespace rmh

#endif // RMH_CRYPTO_COMMS_H
