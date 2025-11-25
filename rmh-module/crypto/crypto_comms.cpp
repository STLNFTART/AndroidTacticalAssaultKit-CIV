/**
 * @file crypto_comms.cpp
 * @brief Implementation of RMH Crypto-Secure Communications
 */

#include "crypto_comms.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/ec.h>
#include <openssl/ecdh.h>
#include <cstring>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

namespace rmh {
namespace crypto {

// CryptoComms Implementation
class CryptoComms::Impl {
public:
    Impl(uint8_t node_id, LinkType link_type)
        : node_id_(node_id)
        , link_type_(link_type)
        , running_(false)
        , message_counter_(0)
    {
        std::memset(&stats_, 0, sizeof(stats_));
    }

    ~Impl() {
        stop();
    }

    bool initialize(const std::vector<uint8_t>& cipher_key,
                   const std::vector<uint8_t>& auth_key)
    {
        if (cipher_key.size() != 32 || auth_key.size() != 32) {
            return false;
        }

        cipher_key_ = cipher_key;
        auth_key_ = auth_key;
        key_material_.cipher_key = cipher_key;
        key_material_.auth_key = auth_key;
        key_material_.key_id = 1;
        key_material_.expiry_time_ms = getCurrentTimeMs() + (15 * 60 * 1000);  // 15 min

        return true;
    }

    bool start(const std::string& bind_address) {
        if (running_) return false;

        running_ = true;

        // Start sender and receiver threads
        sender_thread_ = std::thread(&Impl::senderLoop, this);
        receiver_thread_ = std::thread(&Impl::receiverLoop, this);

        return true;
    }

    void stop() {
        if (!running_) return;

        running_ = false;

        send_cv_.notify_all();
        receive_cv_.notify_all();

        if (sender_thread_.joinable()) sender_thread_.join();
        if (receiver_thread_.joinable()) receiver_thread_.join();
    }

    bool sendMessage(MessageType type,
                    const std::vector<uint8_t>& payload,
                    uint8_t receiver_id,
                    uint8_t priority)
    {
        // Create message header
        MessageHeader header;
        header.message_id = message_counter_++;
        header.type = type;
        header.payload_length = payload.size();
        header.timestamp_ms = getCurrentTimeMs();
        header.sender_id = node_id_;
        header.receiver_id = receiver_id;
        header.hop_count = 0;
        header.priority = priority;

        // Encrypt payload
        std::vector<uint8_t> iv = EncryptionEngine::generateRandom(16);
        std::vector<uint8_t> encrypted = EncryptionEngine::encrypt(
            payload,
            cipher_key_,
            iv
        );

        // Compute HMAC
        std::vector<uint8_t> hmac = EncryptionEngine::computeHMAC(
            encrypted,
            auth_key_
        );

        // Package into SecureMessage
        SecureMessage msg;
        msg.header = header;
        msg.encrypted_payload = encrypted;
        msg.auth_tag = hmac;
        msg.iv = iv;

        // Queue for transmission
        {
            std::lock_guard<std::mutex> lock(send_mutex_);
            send_queue_.push(msg);
        }
        send_cv_.notify_one();

        stats_.messages_sent++;
        stats_.bytes_transmitted += encrypted.size();

        return true;
    }

    void registerCallback(MessageType type, MessageCallback callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callbacks_[type] = callback;
    }

    CommsStats getStatistics() const {
        return stats_;
    }

private:
    uint8_t node_id_;
    LinkType link_type_;
    bool running_;
    uint32_t message_counter_;

    std::vector<uint8_t> cipher_key_;
    std::vector<uint8_t> auth_key_;
    KeyMaterial key_material_;

    std::thread sender_thread_;
    std::thread receiver_thread_;

    std::queue<SecureMessage> send_queue_;
    std::mutex send_mutex_;
    std::condition_variable send_cv_;

    std::queue<SecureMessage> receive_queue_;
    std::mutex receive_mutex_;
    std::condition_variable receive_cv_;

    std::map<MessageType, MessageCallback> callbacks_;
    std::mutex callback_mutex_;

    CommsStats stats_;

    void senderLoop() {
        while (running_) {
            SecureMessage msg;

            {
                std::unique_lock<std::mutex> lock(send_mutex_);
                send_cv_.wait(lock, [this] {
                    return !send_queue_.empty() || !running_;
                });

                if (!running_) break;

                if (!send_queue_.empty()) {
                    msg = send_queue_.front();
                    send_queue_.pop();
                }
            }

            // Actually send message over network
            // Placeholder - would use sockets/acoustic modem
            transmitMessage(msg);
        }
    }

    void receiverLoop() {
        while (running_) {
            // Receive messages from network
            // Placeholder - would use sockets/acoustic modem
            SecureMessage msg = receiveMessage();

            if (msg.encrypted_payload.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Verify HMAC
            if (!EncryptionEngine::verifyHMAC(
                msg.encrypted_payload,
                auth_key_,
                msg.auth_tag))
            {
                stats_.crypto_errors++;
                stats_.messages_dropped++;
                continue;
            }

            // Decrypt payload
            std::vector<uint8_t> plaintext = EncryptionEngine::decrypt(
                msg.encrypted_payload,
                cipher_key_,
                msg.iv
            );

            if (plaintext.empty()) {
                stats_.crypto_errors++;
                stats_.messages_dropped++;
                continue;
            }

            stats_.messages_received++;
            stats_.bytes_received += plaintext.size();

            // Invoke callback
            invokeCallback(msg.header.type, plaintext, msg.header);
        }
    }

    void transmitMessage(const SecureMessage& msg) {
        // Placeholder for actual transmission
        // Would serialize and send over socket/acoustic
    }

    SecureMessage receiveMessage() {
        // Placeholder for actual reception
        // Would receive and deserialize from socket/acoustic
        return SecureMessage{};
    }

    void invokeCallback(MessageType type,
                       const std::vector<uint8_t>& payload,
                       const MessageHeader& header)
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        auto it = callbacks_.find(type);
        if (it != callbacks_.end()) {
            it->second(type, payload, header);
        }
    }

    static uint64_t getCurrentTimeMs() {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
};

CryptoComms::CryptoComms(uint8_t node_id, LinkType link_type)
    : impl_(std::make_unique<Impl>(node_id, link_type)) {}

CryptoComms::~CryptoComms() = default;

bool CryptoComms::initialize(const std::vector<uint8_t>& cipher_key,
                             const std::vector<uint8_t>& auth_key) {
    return impl_->initialize(cipher_key, auth_key);
}

bool CryptoComms::start(const std::string& bind_address) {
    return impl_->start(bind_address);
}

void CryptoComms::stop() {
    impl_->stop();
}

bool CryptoComms::sendMessage(MessageType type,
                              const std::vector<uint8_t>& payload,
                              uint8_t receiver_id,
                              uint8_t priority) {
    return impl_->sendMessage(type, payload, receiver_id, priority);
}

void CryptoComms::registerCallback(MessageType type, MessageCallback callback) {
    impl_->registerCallback(type, callback);
}

CommsStats CryptoComms::getStatistics() const {
    return impl_->getStatistics();
}

// EncryptionEngine implementation
std::vector<uint8_t> EncryptionEngine::encrypt(
    const std::vector<uint8_t>& plaintext,
    const std::vector<uint8_t>& key,
    std::vector<uint8_t>& iv)
{
    if (iv.empty()) {
        iv = generateRandom(16);
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};

    std::vector<uint8_t> ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH);

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    int len;
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    int ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    ciphertext_len += len;
    ciphertext.resize(ciphertext_len);

    // Get GCM tag
    std::vector<uint8_t> tag(16);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    // Append tag
    ciphertext.insert(ciphertext.end(), tag.begin(), tag.end());

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext;
}

std::vector<uint8_t> EncryptionEngine::decrypt(
    const std::vector<uint8_t>& ciphertext,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv)
{
    if (ciphertext.size() < 16) return {};  // Need at least tag

    // Split ciphertext and tag
    std::vector<uint8_t> ct(ciphertext.begin(), ciphertext.end() - 16);
    std::vector<uint8_t> tag(ciphertext.end() - 16, ciphertext.end());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};

    std::vector<uint8_t> plaintext(ct.size());

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    int len;
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ct.data(), ct.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    int plaintext_len = len;

    // Set expected tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    // Finalize (verifies tag)
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};  // Authentication failed
    }

    plaintext_len += len;
    plaintext.resize(plaintext_len);

    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}

std::vector<uint8_t> EncryptionEngine::computeHMAC(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& key)
{
    unsigned int len;
    std::vector<uint8_t> hmac(EVP_MAX_MD_SIZE);

    HMAC(EVP_sha256(), key.data(), key.size(),
         data.data(), data.size(),
         hmac.data(), &len);

    hmac.resize(len);
    return hmac;
}

bool EncryptionEngine::verifyHMAC(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& tag)
{
    std::vector<uint8_t> computed = computeHMAC(data, key);
    return computed == tag;
}

std::vector<uint8_t> EncryptionEngine::generateRandom(size_t length) {
    std::vector<uint8_t> random(length);
    RAND_bytes(random.data(), length);
    return random;
}

// TimeSync implementation
uint64_t TimeSync::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

bool TimeSync::validateTimestamp(uint64_t timestamp, uint64_t window_ms) {
    uint64_t now = getCurrentTime();
    uint64_t diff = (now > timestamp) ? (now - timestamp) : (timestamp - now);
    return diff <= window_ms;
}

int64_t TimeSync::synchronize(uint64_t remote_timestamp, uint64_t round_trip_time) {
    uint64_t local_time = getCurrentTime();
    int64_t offset = remote_timestamp - local_time + (round_trip_time / 2);
    return offset;
}

} // namespace crypto
} // namespace rmh
