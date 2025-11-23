/**
 * @file secure_comms.h
 * @brief Crypto-Secure Communications for RMH Tow Body <-> MCM USV
 *
 * Provides end-to-end encrypted, authenticated communications between
 * the Tow Body and MCM USV with:
 * - AES-256-GCM encryption
 * - HMAC-SHA256 authentication
 * - Perfect forward secrecy via ECDH key exchange
 * - Anti-replay protection with sequence numbers
 * - Low-latency design for real-time telemetry
 *
 * Integrates with ATAK CommonCommo crypto infrastructure.
 *
 * @author Primal Logic
 * @date 2025-11-23
 * @classification UNCLASSIFIED
 */

#ifndef RMH_SECURE_COMMS_H
#define RMH_SECURE_COMMS_H

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <functional>

namespace rmh {
namespace crypto {

/**
 * @brief Message types for RMH communications
 */
enum class MessageType : uint8_t {
    TELEMETRY          = 0x01,  ///< Tow Body telemetry (position, status)
    SENSOR_DATA        = 0x02,  ///< Raw sensor data (sonar, magnetometer)
    MINE_CONTACT       = 0x03,  ///< Mine detection/classification report
    COMMAND            = 0x04,  ///< Command from USV to TB
    ACKNOWLEDGMENT     = 0x05,  ///< ACK/NACK response
    KEY_EXCHANGE       = 0x06,  ///< Cryptographic key exchange
    HEARTBEAT          = 0x07,  ///< Keep-alive heartbeat
    STEALTH_STATUS     = 0x08,  ///< Primal Logic stealth status
    EMERGENCY          = 0xFF   ///< Emergency message (highest priority)
};

/**
 * @brief Encryption algorithm options
 */
enum class EncryptionAlgorithm {
    AES_256_GCM,        ///< AES-256 in GCM mode (recommended)
    AES_256_CBC_HMAC,   ///< AES-256-CBC + HMAC-SHA256
    CHACHA20_POLY1305   ///< ChaCha20-Poly1305 (alternative)
};

/**
 * @brief Key exchange protocol
 */
enum class KeyExchangeProtocol {
    ECDH_P256,          ///< ECDH with P-256 curve
    ECDH_CURVE25519,    ///< ECDH with Curve25519
    STATIC_PSK          ///< Pre-shared key (for testing)
};

/**
 * @brief Security level configuration
 */
enum class SecurityLevel {
    TESTING,            ///< Minimal security for testing
    STANDARD,           ///< Standard NAVSEA requirements
    HIGH_ASSURANCE      ///< High-assurance mode with additional checks
};

/**
 * @brief Crypto configuration
 */
struct CryptoConfig {
    EncryptionAlgorithm encryption;
    KeyExchangeProtocol key_exchange;
    SecurityLevel security_level;
    uint32_t key_rotation_interval_sec;  ///< Rekey interval
    uint32_t max_replay_window;          ///< Anti-replay window size
    bool enable_compression;             ///< Compress before encrypt
    bool enable_forward_secrecy;         ///< Enable PFS

    CryptoConfig()
        : encryption(EncryptionAlgorithm::AES_256_GCM)
        , key_exchange(KeyExchangeProtocol::ECDH_P256)
        , security_level(SecurityLevel::STANDARD)
        , key_rotation_interval_sec(3600)  // 1 hour
        , max_replay_window(1000)
        , enable_compression(false)
        , enable_forward_secrecy(true)
    {}
};

/**
 * @brief Encrypted message structure
 */
struct SecureMessage {
    MessageType type;
    uint64_t sequence_number;
    uint64_t timestamp_ms;
    std::vector<uint8_t> payload;      ///< Encrypted payload
    std::vector<uint8_t> auth_tag;     ///< Authentication tag (GCM/HMAC)
    std::vector<uint8_t> iv;           ///< Initialization vector
    uint8_t version;                   ///< Protocol version

    SecureMessage()
        : type(MessageType::TELEMETRY)
        , sequence_number(0)
        , timestamp_ms(0)
        , version(1)
    {}
};

/**
 * @brief Session key material
 */
struct SessionKeys {
    std::vector<uint8_t> encryption_key;   ///< AES encryption key
    std::vector<uint8_t> mac_key;          ///< HMAC key (if needed)
    std::vector<uint8_t> iv_seed;          ///< IV generation seed
    uint64_t creation_timestamp;           ///< Key creation time
    uint32_t key_id;                       ///< Unique key identifier

    bool is_expired(uint64_t current_time_ms, uint32_t lifetime_sec) const {
        uint64_t age_ms = current_time_ms - creation_timestamp;
        return age_ms > (lifetime_sec * 1000ULL);
    }
};

/**
 * @brief Key Exchange Engine
 *
 * Handles cryptographic key establishment using ECDH.
 */
class KeyExchangeEngine {
public:
    explicit KeyExchangeEngine(const CryptoConfig& config);
    ~KeyExchangeEngine();

    /**
     * @brief Generate local key pair
     * @return Public key bytes
     */
    std::vector<uint8_t> generate_keypair();

    /**
     * @brief Compute shared secret from peer's public key
     * @param peer_public_key Peer's public key
     * @return Shared secret
     */
    std::vector<uint8_t> compute_shared_secret(const std::vector<uint8_t>& peer_public_key);

    /**
     * @brief Derive session keys from shared secret
     * @param shared_secret Computed shared secret
     * @param key_id Unique key identifier
     * @return Session keys
     */
    SessionKeys derive_session_keys(const std::vector<uint8_t>& shared_secret, uint32_t key_id);

    /**
     * @brief Set pre-shared key (for testing)
     */
    void set_psk(const std::vector<uint8_t>& psk);

private:
    CryptoConfig config_;
    std::vector<uint8_t> private_key_;
    std::vector<uint8_t> public_key_;
    std::vector<uint8_t> psk_;  ///< Pre-shared key (testing only)

    void kdf_hkdf(const std::vector<uint8_t>& input,
                  const std::vector<uint8_t>& salt,
                  const std::vector<uint8_t>& info,
                  std::vector<uint8_t>& output,
                  size_t output_len);
};

/**
 * @brief Encryption Engine
 *
 * Handles message encryption and decryption.
 */
class EncryptionEngine {
public:
    explicit EncryptionEngine(const CryptoConfig& config);
    ~EncryptionEngine();

    /**
     * @brief Encrypt plaintext message
     * @param plaintext Raw message data
     * @param keys Session keys
     * @param iv Initialization vector
     * @param ciphertext Output encrypted data
     * @param auth_tag Output authentication tag
     * @return True on success
     */
    bool encrypt(const std::vector<uint8_t>& plaintext,
                 const SessionKeys& keys,
                 const std::vector<uint8_t>& iv,
                 std::vector<uint8_t>& ciphertext,
                 std::vector<uint8_t>& auth_tag);

    /**
     * @brief Decrypt ciphertext message
     * @param ciphertext Encrypted data
     * @param keys Session keys
     * @param iv Initialization vector
     * @param auth_tag Authentication tag
     * @param plaintext Output decrypted data
     * @return True on success (includes auth verification)
     */
    bool decrypt(const std::vector<uint8_t>& ciphertext,
                 const SessionKeys& keys,
                 const std::vector<uint8_t>& iv,
                 const std::vector<uint8_t>& auth_tag,
                 std::vector<uint8_t>& plaintext);

private:
    CryptoConfig config_;

    bool encrypt_aes_gcm(const std::vector<uint8_t>& plaintext,
                         const std::vector<uint8_t>& key,
                         const std::vector<uint8_t>& iv,
                         std::vector<uint8_t>& ciphertext,
                         std::vector<uint8_t>& auth_tag);

    bool decrypt_aes_gcm(const std::vector<uint8_t>& ciphertext,
                         const std::vector<uint8_t>& key,
                         const std::vector<uint8_t>& iv,
                         const std::vector<uint8_t>& auth_tag,
                         std::vector<uint8_t>& plaintext);
};

/**
 * @brief Anti-Replay Filter
 *
 * Prevents replay attacks using sliding window.
 */
class AntiReplayFilter {
public:
    explicit AntiReplayFilter(uint32_t window_size);

    /**
     * @brief Check if sequence number is valid (not replayed)
     * @param sequence_number Message sequence number
     * @return True if valid (not seen before)
     */
    bool check_and_update(uint64_t sequence_number);

    /**
     * @brief Reset filter state
     */
    void reset();

private:
    uint32_t window_size_;
    uint64_t highest_sequence_;
    std::vector<bool> window_;
};

/**
 * @brief Secure Communications Channel
 *
 * Main interface for encrypted RMH communications.
 */
class SecureChannel {
public:
    explicit SecureChannel(const CryptoConfig& config);
    ~SecureChannel();

    /**
     * @brief Initialize secure channel
     * @param is_initiator True if initiating key exchange, false if responding
     * @return True on success
     */
    bool initialize(bool is_initiator);

    /**
     * @brief Perform key exchange with peer
     * @param peer_public_key Peer's public key (if available)
     * @return Local public key to send to peer
     */
    std::vector<uint8_t> key_exchange(const std::vector<uint8_t>& peer_public_key);

    /**
     * @brief Send encrypted message
     * @param type Message type
     * @param payload Raw payload data
     * @param encrypted_message Output encrypted message
     * @return True on success
     */
    bool send_message(MessageType type,
                      const std::vector<uint8_t>& payload,
                      SecureMessage& encrypted_message);

    /**
     * @brief Receive and decrypt message
     * @param encrypted_message Received encrypted message
     * @param payload Output decrypted payload
     * @param verified Output: true if authentication verified
     * @return True on success
     */
    bool receive_message(const SecureMessage& encrypted_message,
                        std::vector<uint8_t>& payload,
                        bool& verified);

    /**
     * @brief Check if channel is ready for secure comms
     */
    bool is_ready() const { return keys_established_; }

    /**
     * @brief Get channel statistics
     */
    struct ChannelStats {
        uint64_t messages_sent;
        uint64_t messages_received;
        uint64_t authentication_failures;
        uint64_t replay_attacks_blocked;
        uint64_t bytes_encrypted;
        uint64_t bytes_decrypted;
    };
    ChannelStats get_stats() const { return stats_; }

    /**
     * @brief Force key rotation
     */
    void rotate_keys();

private:
    CryptoConfig config_;
    std::unique_ptr<KeyExchangeEngine> key_exchange_;
    std::unique_ptr<EncryptionEngine> encryption_;
    std::unique_ptr<AntiReplayFilter> replay_filter_;

    bool keys_established_;
    SessionKeys current_keys_;
    uint64_t send_sequence_;
    uint64_t last_key_rotation_ms_;
    uint32_t next_key_id_;

    ChannelStats stats_;

    std::vector<uint8_t> generate_iv();
    uint64_t get_timestamp_ms();
    bool should_rotate_keys();
};

/**
 * @brief Message Serializer
 *
 * Serializes/deserializes SecureMessage to/from wire format.
 */
class MessageSerializer {
public:
    /**
     * @brief Serialize SecureMessage to byte stream
     * @param message Message to serialize
     * @param buffer Output buffer
     * @return True on success
     */
    static bool serialize(const SecureMessage& message, std::vector<uint8_t>& buffer);

    /**
     * @brief Deserialize byte stream to SecureMessage
     * @param buffer Input buffer
     * @param message Output message
     * @return True on success
     */
    static bool deserialize(const std::vector<uint8_t>& buffer, SecureMessage& message);

private:
    static void write_uint8(std::vector<uint8_t>& buffer, uint8_t value);
    static void write_uint64(std::vector<uint8_t>& buffer, uint64_t value);
    static void write_bytes(std::vector<uint8_t>& buffer, const std::vector<uint8_t>& data);

    static uint8_t read_uint8(const std::vector<uint8_t>& buffer, size_t& offset);
    static uint64_t read_uint64(const std::vector<uint8_t>& buffer, size_t& offset);
    static void read_bytes(const std::vector<uint8_t>& buffer, size_t& offset,
                          size_t length, std::vector<uint8_t>& output);
};

} // namespace crypto
} // namespace rmh

#endif // RMH_SECURE_COMMS_H
