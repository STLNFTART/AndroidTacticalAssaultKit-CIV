/**
 * @file example_signal_processing.cpp
 * @brief Example demonstrating RMH signal processing capabilities
 */

#include "signal_processor.h"
#include <iostream>
#include <vector>
#include <cmath>

using namespace rmh::signals;

// Generate synthetic sonar data with embedded mine signature
std::vector<uint8_t> generateSyntheticSonar(bool include_mine) {
    std::vector<float> signal(1024);

    // Background noise
    for (size_t i = 0; i < signal.size(); ++i) {
        signal[i] = 0.1f * (std::rand() / (float)RAND_MAX - 0.5f);
    }

    // Add mine signature if requested
    if (include_mine) {
        size_t mine_pos = 512;
        for (size_t i = 0; i < 64; ++i) {
            if (mine_pos + i < signal.size()) {
                signal[mine_pos + i] += 0.8f * std::sin(2.0 * M_PI * i / 16.0);
            }
        }
    }

    // Convert to bytes
    std::vector<uint8_t> bytes(signal.size() * sizeof(float));
    std::memcpy(bytes.data(), signal.data(), bytes.size());

    return bytes;
}

int main() {
    std::cout << "=== RMH Signal Processing Example ===" << std::endl;
    std::cout << std::endl;

    // Configure signal processor
    ProcessorConfig config;
    config.detection_threshold = 5.0f;  // 5 dB SNR
    config.classification_threshold = 0.7f;  // 70% confidence
    config.adaptive_filtering = true;
    config.enable_sensor_fusion = false;
    config.active_sensors = {SensorType::SONAR_SIDE_SCAN};

    std::cout << "Initializing signal processor..." << std::endl;
    SignalProcessor processor(config);
    std::cout << "  Detection threshold: " << config.detection_threshold << " dB" << std::endl;
    std::cout << "  Classification threshold: " << config.classification_threshold << std::endl;
    std::cout << std::endl;

    // Test 1: Process clean signal (no mine)
    std::cout << "Test 1: Processing signal WITHOUT mine..." << std::endl;
    SensorData clean_data;
    clean_data.type = SensorType::SONAR_SIDE_SCAN;
    clean_data.timestamp_ms = 1000;
    clean_data.raw_data = generateSyntheticSonar(false);
    clean_data.location = {37.8, -122.4, 10.0, 5.0};

    auto contacts1 = processor.processSensorData(clean_data);
    std::cout << "  Contacts detected: " << contacts1.size() << std::endl;
    std::cout << std::endl;

    // Test 2: Process signal with mine
    std::cout << "Test 2: Processing signal WITH mine..." << std::endl;
    SensorData mine_data;
    mine_data.type = SensorType::SONAR_SIDE_SCAN;
    mine_data.timestamp_ms = 2000;
    mine_data.raw_data = generateSyntheticSonar(true);
    mine_data.location = {37.85, -122.45, 12.0, 5.0};

    auto contacts2 = processor.processSensorData(mine_data);
    std::cout << "  Contacts detected: " << contacts2.size() << std::endl;

    for (const auto& contact : contacts2) {
        std::cout << "    Contact ID: " << contact.contact_id << std::endl;
        std::cout << "    Classification: " << contact.description << std::endl;
        std::cout << "    Confidence: " << (contact.confidence * 100.0f) << "%" << std::endl;
        std::cout << "    Location: " << contact.location.latitude << ", "
                  << contact.location.longitude << std::endl;
    }
    std::cout << std::endl;

    // Display statistics
    std::cout << "Processing Statistics:" << std::endl;
    auto stats = processor.getStatistics();
    for (const auto& [key, value] : stats) {
        std::cout << "  " << key << ": " << value << std::endl;
    }
    std::cout << std::endl;

    std::cout << "=== Example Complete ===" << std::endl;

    return 0;
}
