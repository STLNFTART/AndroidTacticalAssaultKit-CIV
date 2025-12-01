/**
 * @file signal_processor.h
 * @brief RMH Signal Processing Engine for Mine Detection and Classification
 *
 * Processes sonar, magnetometer, and imaging sensor data to detect and
 * classify underwater mines with high accuracy and low false positive rate.
 *
 * @author Primal Logic
 * @date 2025-11-23
 * @classification UNCLASSIFIED
 */

#ifndef RMH_SIGNAL_PROCESSOR_H
#define RMH_SIGNAL_PROCESSOR_H

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <map>

namespace rmh {
namespace signals {

/**
 * @brief Sensor types supported by the RMH module
 */
enum class SensorType {
    SONAR_SIDE_SCAN,      // Side-scan sonar
    SONAR_FORWARD_LOOK,   // Forward-looking sonar
    MAGNETOMETER,         // Magnetic anomaly detector
    OPTICAL_CAMERA,       // Optical/imaging
    SYNTHETIC_APERTURE    // Synthetic aperture sonar (SAS)
};

/**
 * @brief Contact classification results
 */
enum class ContactClass {
    MINE_LIKE_OBJECT,     // High confidence mine
    MINE_LIKE_SUSPECT,    // Possible mine, needs re-investigation
    NON_MINE_CLUTTER,     // Identified as clutter (rock, debris, etc.)
    UNKNOWN               // Insufficient data
};

/**
 * @brief Geographic location with depth
 */
struct GeoLocation {
    double latitude;      // Decimal degrees
    double longitude;     // Decimal degrees
    double depth;         // Meters below surface
    double altitude;      // Meters above seafloor
};

/**
 * @brief Raw sensor data packet
 */
struct SensorData {
    SensorType type;
    uint64_t timestamp_ms;   // Unix timestamp in milliseconds
    std::vector<uint8_t> raw_data;
    GeoLocation location;
    std::map<std::string, double> metadata;  // Sensor-specific metadata
};

/**
 * @brief Detected contact report
 */
struct ContactReport {
    uint64_t contact_id;
    ContactClass classification;
    float confidence;        // 0.0 to 1.0
    GeoLocation location;
    std::vector<SensorType> contributing_sensors;  // Multi-sensor fusion
    std::string description; // Human-readable details
    std::vector<uint8_t> imagery;  // Optional imagery data
    uint64_t detection_time_ms;
};

/**
 * @brief Signal processing configuration
 */
struct ProcessorConfig {
    // Detection thresholds
    float detection_threshold;       // SNR threshold (dB)
    float classification_threshold;  // ML confidence threshold (0.0-1.0)

    // Filtering parameters
    float noise_floor_db;            // Expected noise floor
    bool adaptive_filtering;         // Enable adaptive noise cancellation

    // ML model paths
    std::string ml_model_path;       // Path to TensorFlow Lite model

    // Multi-sensor fusion
    bool enable_sensor_fusion;       // Combine data from multiple sensors
    std::vector<SensorType> active_sensors;

    // Performance tuning
    int max_processing_threads;      // Parallel processing threads
    bool enable_gpu_acceleration;    // Use GPU if available
};

/**
 * @brief Main signal processing engine
 */
class SignalProcessor {
public:
    /**
     * @brief Constructor
     * @param config Processing configuration
     */
    explicit SignalProcessor(const ProcessorConfig& config);

    /**
     * @brief Destructor
     */
    ~SignalProcessor();

    /**
     * @brief Process incoming sensor data
     * @param data Raw sensor data
     * @return Vector of detected contacts (may be empty)
     */
    std::vector<ContactReport> processSensorData(const SensorData& data);

    /**
     * @brief Update processor configuration
     * @param config New configuration
     */
    void updateConfig(const ProcessorConfig& config);

    /**
     * @brief Get current processing statistics
     * @return Map of statistic name to value
     */
    std::map<std::string, double> getStatistics() const;

    /**
     * @brief Reset internal state (clear detections, reset filters)
     */
    void reset();

private:
    class Impl;  // PIMPL pattern for implementation hiding
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Sonar signal processing utilities
 */
class SonarProcessor {
public:
    /**
     * @brief Apply matched filter to sonar data
     * @param data Input signal
     * @param template_signal Expected target signature
     * @return Correlation output
     */
    static std::vector<float> matchedFilter(
        const std::vector<float>& data,
        const std::vector<float>& template_signal);

    /**
     * @brief Detect peaks in processed signal
     * @param data Input signal
     * @param threshold Detection threshold
     * @return Indices of detected peaks
     */
    static std::vector<size_t> detectPeaks(
        const std::vector<float>& data,
        float threshold);

    /**
     * @brief Estimate range and bearing from sonar ping
     * @param data Time-domain sonar return
     * @param sample_rate Sampling frequency (Hz)
     * @param sound_speed Speed of sound in water (m/s)
     * @return Pair of (range_meters, bearing_degrees)
     */
    static std::pair<double, double> estimateRangeBearing(
        const std::vector<float>& data,
        double sample_rate,
        double sound_speed = 1500.0);
};

/**
 * @brief Magnetometer signal processing
 */
class MagnetometerProcessor {
public:
    /**
     * @brief Detect magnetic anomalies
     * @param field_strength Magnetic field measurements (nT)
     * @param baseline Expected baseline field
     * @param threshold Anomaly threshold (standard deviations)
     * @return Indices of anomalies
     */
    static std::vector<size_t> detectAnomalies(
        const std::vector<float>& field_strength,
        float baseline,
        float threshold);

    /**
     * @brief Estimate dipole parameters from anomaly
     * @param anomaly Magnetic anomaly data
     * @return Estimated dipole moment and orientation
     */
    static std::pair<double, double> estimateDipole(
        const std::vector<float>& anomaly);
};

/**
 * @brief ML-based classification engine
 */
class MLClassifier {
public:
    /**
     * @brief Load ML model from file
     * @param model_path Path to TensorFlow Lite model
     * @return true if successful
     */
    bool loadModel(const std::string& model_path);

    /**
     * @brief Classify contact from features
     * @param features Extracted feature vector
     * @return Pair of (classification, confidence)
     */
    std::pair<ContactClass, float> classify(const std::vector<float>& features);

    /**
     * @brief Extract features from sensor data
     * @param data Processed sensor data
     * @return Feature vector for ML input
     */
    static std::vector<float> extractFeatures(const std::vector<float>& data);

private:
    void* model_;  // Opaque pointer to TFLite model
};

/**
 * @brief Multi-sensor fusion engine
 */
class SensorFusion {
public:
    /**
     * @brief Fuse contacts from multiple sensors
     * @param contacts Vector of contacts from different sensors
     * @param correlation_threshold Spatial correlation threshold (meters)
     * @return Fused contact reports
     */
    static std::vector<ContactReport> fuseContacts(
        const std::vector<ContactReport>& contacts,
        double correlation_threshold = 10.0);

    /**
     * @brief Weight sensor contributions based on reliability
     * @param contacts Input contacts
     * @param sensor_weights Map of sensor type to weight
     * @return Weighted, fused contacts
     */
    static std::vector<ContactReport> weightedFusion(
        const std::vector<ContactReport>& contacts,
        const std::map<SensorType, float>& sensor_weights);
};

} // namespace signals
} // namespace rmh

#endif // RMH_SIGNAL_PROCESSOR_H
