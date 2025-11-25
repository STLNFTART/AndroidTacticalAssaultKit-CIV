/**
 * @file signal_processor.cpp
 * @brief Implementation of RMH Signal Processing Engine
 */

#include "signal_processor.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <fstream>

namespace rmh {
namespace signals {

// Implementation class (PIMPL pattern)
class SignalProcessor::Impl {
public:
    explicit Impl(const ProcessorConfig& config)
        : config_(config)
        , ml_classifier_(std::make_unique<MLClassifier>())
    {
        if (!config_.ml_model_path.empty()) {
            if (!ml_classifier_->loadModel(config_.ml_model_path)) {
                throw std::runtime_error("Failed to load ML model: " + config_.ml_model_path);
            }
        }

        // Initialize statistics
        stats_["contacts_detected"] = 0.0;
        stats_["total_pings_processed"] = 0.0;
        stats_["average_processing_time_ms"] = 0.0;
        stats_["false_alarm_rate"] = 0.0;
    }

    std::vector<ContactReport> processSensorData(const SensorData& data) {
        std::vector<ContactReport> contacts;

        // Update statistics
        stats_["total_pings_processed"] += 1.0;

        // Process based on sensor type
        switch (data.type) {
            case SensorType::SONAR_SIDE_SCAN:
            case SensorType::SONAR_FORWARD_LOOK:
            case SensorType::SYNTHETIC_APERTURE:
                contacts = processSonarData(data);
                break;

            case SensorType::MAGNETOMETER:
                contacts = processMagData(data);
                break;

            case SensorType::OPTICAL_CAMERA:
                contacts = processOpticalData(data);
                break;

            default:
                break;
        }

        // Update contact statistics
        stats_["contacts_detected"] += contacts.size();

        return contacts;
    }

    void updateConfig(const ProcessorConfig& config) {
        config_ = config;
    }

    std::map<std::string, double> getStatistics() const {
        return stats_;
    }

    void reset() {
        stats_.clear();
        stats_["contacts_detected"] = 0.0;
        stats_["total_pings_processed"] = 0.0;
        stats_["average_processing_time_ms"] = 0.0;
        stats_["false_alarm_rate"] = 0.0;
    }

private:
    ProcessorConfig config_;
    std::unique_ptr<MLClassifier> ml_classifier_;
    mutable std::map<std::string, double> stats_;

    std::vector<ContactReport> processSonarData(const SensorData& data) {
        std::vector<ContactReport> contacts;

        // Convert raw bytes to float array
        std::vector<float> signal = bytesToFloat(data.raw_data);

        // Apply matched filter
        std::vector<float> filtered = SonarProcessor::matchedFilter(
            signal,
            getMinSignature()
        );

        // Detect peaks above threshold
        std::vector<size_t> peaks = SonarProcessor::detectPeaks(
            filtered,
            config_.detection_threshold
        );

        // For each detection, classify and create contact report
        for (size_t peak : peaks) {
            // Extract features around peak
            std::vector<float> features = extractSonarFeatures(signal, peak);

            // ML classification
            auto [classification, confidence] = ml_classifier_->classify(features);

            // Only report if confidence exceeds threshold
            if (confidence >= config_.classification_threshold) {
                ContactReport contact;
                contact.contact_id = generateContactId();
                contact.classification = classification;
                contact.confidence = confidence;
                contact.location = data.location;
                contact.contributing_sensors.push_back(data.type);
                contact.detection_time_ms = data.timestamp_ms;
                contact.description = classificationToString(classification);

                contacts.push_back(contact);
            }
        }

        return contacts;
    }

    std::vector<ContactReport> processMagData(const SensorData& data) {
        std::vector<ContactReport> contacts;

        std::vector<float> field = bytesToFloat(data.raw_data);

        // Detect anomalies
        float baseline = computeBaseline(field);
        std::vector<size_t> anomalies = MagnetometerProcessor::detectAnomalies(
            field,
            baseline,
            config_.detection_threshold
        );

        for (size_t idx : anomalies) {
            ContactReport contact;
            contact.contact_id = generateContactId();
            contact.classification = ContactClass::MINE_LIKE_SUSPECT;
            contact.confidence = 0.7f;  // Mag alone = medium confidence
            contact.location = data.location;
            contact.contributing_sensors.push_back(data.type);
            contact.detection_time_ms = data.timestamp_ms;
            contact.description = "Magnetic anomaly detected";

            contacts.push_back(contact);
        }

        return contacts;
    }

    std::vector<ContactReport> processOpticalData(const SensorData& data) {
        std::vector<ContactReport> contacts;
        // Optical processing placeholder
        // Would implement image processing + CNN classification here
        return contacts;
    }

    std::vector<float> bytesToFloat(const std::vector<uint8_t>& bytes) {
        std::vector<float> result;
        result.reserve(bytes.size() / sizeof(float));

        for (size_t i = 0; i + sizeof(float) <= bytes.size(); i += sizeof(float)) {
            float val;
            std::memcpy(&val, &bytes[i], sizeof(float));
            result.push_back(val);
        }

        return result;
    }

    std::vector<float> getMinSignature() {
        // Return typical mine signature for matched filter
        // This would be loaded from a database in production
        std::vector<float> signature(64);
        for (size_t i = 0; i < signature.size(); ++i) {
            signature[i] = std::sin(2.0 * M_PI * i / 16.0);
        }
        return signature;
    }

    std::vector<float> extractSonarFeatures(const std::vector<float>& signal, size_t center) {
        // Extract features around detection for ML classifier
        std::vector<float> features;
        size_t window = 128;
        size_t start = (center > window/2) ? center - window/2 : 0;
        size_t end = std::min(center + window/2, signal.size());

        for (size_t i = start; i < end; ++i) {
            features.push_back(signal[i]);
        }

        // Pad if needed
        while (features.size() < window) {
            features.push_back(0.0f);
        }

        return features;
    }

    float computeBaseline(const std::vector<float>& field) {
        if (field.empty()) return 0.0f;
        return std::accumulate(field.begin(), field.end(), 0.0f) / field.size();
    }

    uint64_t generateContactId() {
        static uint64_t counter = 0;
        return ++counter;
    }

    std::string classificationToString(ContactClass cls) {
        switch (cls) {
            case ContactClass::MINE_LIKE_OBJECT: return "Mine-Like Object";
            case ContactClass::MINE_LIKE_SUSPECT: return "Suspect";
            case ContactClass::NON_MINE_CLUTTER: return "Clutter";
            default: return "Unknown";
        }
    }
};

// SignalProcessor public interface
SignalProcessor::SignalProcessor(const ProcessorConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

SignalProcessor::~SignalProcessor() = default;

std::vector<ContactReport> SignalProcessor::processSensorData(const SensorData& data) {
    return impl_->processSensorData(data);
}

void SignalProcessor::updateConfig(const ProcessorConfig& config) {
    impl_->updateConfig(config);
}

std::map<std::string, double> SignalProcessor::getStatistics() const {
    return impl_->getStatistics();
}

void SignalProcessor::reset() {
    impl_->reset();
}

// SonarProcessor implementation
std::vector<float> SonarProcessor::matchedFilter(
    const std::vector<float>& data,
    const std::vector<float>& template_signal)
{
    std::vector<float> output(data.size(), 0.0f);

    for (size_t n = 0; n < data.size(); ++n) {
        float sum = 0.0f;
        for (size_t k = 0; k < template_signal.size() && (n >= k); ++k) {
            sum += data[n - k] * template_signal[k];
        }
        output[n] = sum;
    }

    return output;
}

std::vector<size_t> SonarProcessor::detectPeaks(
    const std::vector<float>& data,
    float threshold)
{
    std::vector<size_t> peaks;

    for (size_t i = 1; i < data.size() - 1; ++i) {
        if (data[i] > threshold &&
            data[i] > data[i-1] &&
            data[i] > data[i+1])
        {
            peaks.push_back(i);
        }
    }

    return peaks;
}

std::pair<double, double> SonarProcessor::estimateRangeBearing(
    const std::vector<float>& data,
    double sample_rate,
    double sound_speed)
{
    // Find time of peak return
    auto max_it = std::max_element(data.begin(), data.end());
    size_t peak_idx = std::distance(data.begin(), max_it);

    // Convert to time
    double time = peak_idx / sample_rate;

    // Calculate range (two-way travel)
    double range = (time * sound_speed) / 2.0;

    // Bearing estimation (simplified - would use beam-forming in production)
    double bearing = 0.0;  // Placeholder

    return {range, bearing};
}

// MagnetometerProcessor implementation
std::vector<size_t> MagnetometerProcessor::detectAnomalies(
    const std::vector<float>& field_strength,
    float baseline,
    float threshold)
{
    std::vector<size_t> anomalies;

    // Compute standard deviation
    float variance = 0.0f;
    for (float val : field_strength) {
        variance += (val - baseline) * (val - baseline);
    }
    float std_dev = std::sqrt(variance / field_strength.size());

    // Find values beyond threshold * std_dev
    for (size_t i = 0; i < field_strength.size(); ++i) {
        if (std::abs(field_strength[i] - baseline) > threshold * std_dev) {
            anomalies.push_back(i);
        }
    }

    return anomalies;
}

std::pair<double, double> MagnetometerProcessor::estimateDipole(
    const std::vector<float>& anomaly)
{
    // Simplified dipole estimation
    // Production would use least-squares fitting
    double moment = 0.0;
    double orientation = 0.0;

    if (!anomaly.empty()) {
        moment = *std::max_element(anomaly.begin(), anomaly.end());
    }

    return {moment, orientation};
}

// MLClassifier implementation
bool MLClassifier::loadModel(const std::string& model_path) {
    // TensorFlow Lite model loading
    // Placeholder - would use actual TFLite API
    return true;
}

std::pair<ContactClass, float> MLClassifier::classify(const std::vector<float>& features) {
    // Placeholder ML inference
    // Production would run TFLite interpreter

    // For now, simple threshold-based classification
    float mean = std::accumulate(features.begin(), features.end(), 0.0f) / features.size();

    if (mean > 0.7f) {
        return {ContactClass::MINE_LIKE_OBJECT, 0.95f};
    } else if (mean > 0.3f) {
        return {ContactClass::MINE_LIKE_SUSPECT, 0.65f};
    } else {
        return {ContactClass::NON_MINE_CLUTTER, 0.85f};
    }
}

std::vector<float> MLClassifier::extractFeatures(const std::vector<float>& data) {
    std::vector<float> features;

    // Extract statistical features
    float mean = std::accumulate(data.begin(), data.end(), 0.0f) / data.size();
    features.push_back(mean);

    float variance = 0.0f;
    for (float val : data) {
        variance += (val - mean) * (val - mean);
    }
    features.push_back(std::sqrt(variance / data.size()));

    float max_val = *std::max_element(data.begin(), data.end());
    features.push_back(max_val);

    float min_val = *std::min_element(data.begin(), data.end());
    features.push_back(min_val);

    return features;
}

// SensorFusion implementation
std::vector<ContactReport> SensorFusion::fuseContacts(
    const std::vector<ContactReport>& contacts,
    double correlation_threshold)
{
    std::vector<ContactReport> fused;

    // Simple spatial clustering
    std::vector<bool> used(contacts.size(), false);

    for (size_t i = 0; i < contacts.size(); ++i) {
        if (used[i]) continue;

        ContactReport merged = contacts[i];
        used[i] = true;

        // Find nearby contacts
        for (size_t j = i + 1; j < contacts.size(); ++j) {
            if (used[j]) continue;

            double dist = computeDistance(
                contacts[i].location,
                contacts[j].location
            );

            if (dist < correlation_threshold) {
                // Merge contacts
                merged.confidence = std::max(merged.confidence, contacts[j].confidence);
                merged.contributing_sensors.insert(
                    merged.contributing_sensors.end(),
                    contacts[j].contributing_sensors.begin(),
                    contacts[j].contributing_sensors.end()
                );
                used[j] = true;
            }
        }

        fused.push_back(merged);
    }

    return fused;
}

std::vector<ContactReport> SensorFusion::weightedFusion(
    const std::vector<ContactReport>& contacts,
    const std::map<SensorType, float>& sensor_weights)
{
    // Weighted fusion based on sensor reliability
    std::vector<ContactReport> weighted = contacts;

    for (auto& contact : weighted) {
        float weight_sum = 0.0f;
        for (auto sensor : contact.contributing_sensors) {
            auto it = sensor_weights.find(sensor);
            if (it != sensor_weights.end()) {
                weight_sum += it->second;
            }
        }

        if (weight_sum > 0.0f) {
            contact.confidence *= weight_sum / contact.contributing_sensors.size();
        }
    }

    return weighted;
}

double SensorFusion::computeDistance(const GeoLocation& a, const GeoLocation& b) {
    // Haversine formula for distance on sphere
    const double R = 6371000.0;  // Earth radius in meters

    double lat1 = a.latitude * M_PI / 180.0;
    double lat2 = b.latitude * M_PI / 180.0;
    double dlat = (b.latitude - a.latitude) * M_PI / 180.0;
    double dlon = (b.longitude - a.longitude) * M_PI / 180.0;

    double a_val = std::sin(dlat/2) * std::sin(dlat/2) +
                   std::cos(lat1) * std::cos(lat2) *
                   std::sin(dlon/2) * std::sin(dlon/2);

    double c = 2 * std::atan2(std::sqrt(a_val), std::sqrt(1-a_val));

    return R * c;
}

} // namespace signals
} // namespace rmh
