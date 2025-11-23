/**
 * @file signal_processor.cpp
 * @brief Implementation of RMH Signal Processing Engine
 *
 * @author Primal Logic
 * @date 2025-11-23
 * @classification UNCLASSIFIED
 */

#include "signal_processor.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace rmh {
namespace signals {

// ============================================================================
// SensorData Implementation
// ============================================================================

double SensorData::compute_snr() const {
    if (noise_level < 1e-9) return 100.0;  // Very high SNR
    return 20.0 * std::log10(signal_strength / noise_level);
}

bool SensorData::is_valid() const {
    return signal_strength >= 0.0 &&
           noise_level >= 0.0 &&
           range_m >= 0.0 &&
           quality >= 0.0 && quality <= 1.0;
}

// ============================================================================
// MineContact Implementation
// ============================================================================

double MineContact::compute_confidence() const {
    // Weighted combination of detection metrics
    double snr_factor = std::tanh(snr_db / 20.0);  // Normalize to [0, 1]
    double quality_factor = signal_quality;
    double consistency_factor = std::min(1.0, classification_attempts / 5.0);

    return 0.4 * snr_factor + 0.4 * quality_factor + 0.2 * consistency_factor;
}

bool MineContact::is_high_confidence() const {
    return compute_confidence() > 0.75 && classification_attempts >= 3;
}

// ============================================================================
// FeatureExtractor Implementation
// ============================================================================

FeatureExtractor::FeatureExtractor(const SignalProcessorConfig& config)
    : config_(config)
{
}

MineFeatures FeatureExtractor::extract(const SensorData& data) {
    MineFeatures features;

    if (!data.is_valid()) {
        return features;  // Return empty features
    }

    // Basic signal features
    features.acoustic_return = data.signal_strength;
    features.magnetic_anomaly = extract_magnetic_anomaly(data);
    features.aspect_ratio = extract_aspect_ratio(data);
    features.surface_roughness = extract_surface_roughness(data);
    features.material_density = extract_material_density(data);
    features.size_estimate_m = estimate_size(data);
    features.depth_m = data.range_m;

    // Shape descriptors
    features.shape_descriptor = extract_shape_descriptor(data);

    // Shadow characteristics
    features.shadow_length_m = estimate_shadow_length(data);
    features.shadow_contrast = estimate_shadow_contrast(data);

    return features;
}

double FeatureExtractor::extract_magnetic_anomaly(const SensorData& data) {
    // Simplified magnetic anomaly detection
    // In real system, would process magnetometer data
    if (data.sensor_type != SensorType::MAGNETOMETER) {
        return 0.0;
    }

    return data.signal_strength * 0.8;  // Placeholder
}

double FeatureExtractor::extract_aspect_ratio(const SensorData& data) {
    // Extract aspect ratio from sonar imagery
    // Simplified: assume cylindrical mine shape
    double length = std::sqrt(data.signal_strength) * 2.0;
    double width = std::sqrt(data.signal_strength);

    return length / (width + 1e-6);
}

double FeatureExtractor::extract_surface_roughness(const SensorData& data) {
    // Surface roughness from acoustic return variation
    return data.noise_level / (data.signal_strength + 1e-6);
}

double FeatureExtractor::extract_material_density(const SensorData& data) {
    // Estimate material density from acoustic return strength
    // Metal = high density, composite = lower
    return data.signal_strength * 0.6;
}

double FeatureExtractor::estimate_size(const SensorData& data) {
    // Estimate size from signal strength and range
    // Using inverse square law approximation
    double size = std::sqrt(data.signal_strength * data.range_m * data.range_m);
    return std::clamp(size, 0.1, 5.0);  // Typical mine sizes: 0.1m - 5m
}

std::vector<double> FeatureExtractor::extract_shape_descriptor(const SensorData& data) {
    // Simplified shape descriptor (in real system would be FFT of boundary)
    std::vector<double> descriptor(8);
    for (size_t i = 0; i < descriptor.size(); i++) {
        descriptor[i] = data.signal_strength * std::sin(i * M_PI / 4.0);
    }
    return descriptor;
}

double FeatureExtractor::estimate_shadow_length(const SensorData& data) {
    // Shadow length depends on mine height and illumination angle
    double height_estimate = estimate_size(data) * 0.5;
    double angle_rad = 45.0 * M_PI / 180.0;  // Typical sonar angle
    return height_estimate / std::tan(angle_rad);
}

double FeatureExtractor::estimate_shadow_contrast(const SensorData& data) {
    // Shadow contrast in sonar imagery
    return std::tanh(data.signal_strength / 2.0);
}

// ============================================================================
// MineClassifier Implementation
// ============================================================================

MineClassifier::MineClassifier(const SignalProcessorConfig& config)
    : config_(config)
{
    initialize_templates();
}

MineType MineClassifier::classify(const MineFeatures& features, double& confidence) {
    // Multi-stage classification

    // Stage 1: Rule-based pre-classification
    MineType preliminary_type = rule_based_classification(features);

    // Stage 2: Feature matching against templates
    MineType template_type = template_matching(features, confidence);

    // If both agree and confidence is high, return result
    if (preliminary_type == template_type && confidence > 0.7) {
        return template_type;
    }

    // Stage 3: Advanced ML-based classification (placeholder)
    // In production, would use trained neural network
    if (confidence < 0.6) {
        confidence = 0.5;
        return MineType::UNKNOWN;
    }

    return template_type;
}

MineType MineClassifier::rule_based_classification(const MineFeatures& features) {
    // Simple rule-based classification

    // Bottom mines: typically larger, cylindrical
    if (features.size_estimate_m > 1.5 && features.aspect_ratio > 1.5) {
        return MineType::BOTTOM_MINE;
    }

    // Moored mines: spherical or cylindrical, elevated off bottom
    if (features.size_estimate_m > 0.8 && features.depth_m > 20.0 &&
        features.aspect_ratio < 1.5) {
        return MineType::MOORED_MINE;
    }

    // Drifting mines: smaller, mobile
    if (features.size_estimate_m < 1.0) {
        return MineType::DRIFTING_MINE;
    }

    return MineType::UNKNOWN;
}

MineType MineClassifier::template_matching(const MineFeatures& features, double& confidence) {
    double best_match_score = 0.0;
    MineType best_match_type = MineType::UNKNOWN;

    // Match against each template
    for (const auto& templ : templates_) {
        double score = compute_match_score(features, templ);
        if (score > best_match_score) {
            best_match_score = score;
            best_match_type = templ.mine_type;
        }
    }

    confidence = best_match_score;
    return best_match_type;
}

double MineClassifier::compute_match_score(const MineFeatures& features,
                                           const MineTemplate& templ) {
    // Weighted feature matching
    double score = 0.0;
    double total_weight = 0.0;

    // Size matching
    double size_diff = std::abs(features.size_estimate_m - templ.typical_size_m);
    double size_score = std::exp(-size_diff * 2.0);
    score += size_score * 0.3;
    total_weight += 0.3;

    // Aspect ratio matching
    double aspect_diff = std::abs(features.aspect_ratio - templ.aspect_ratio);
    double aspect_score = std::exp(-aspect_diff);
    score += aspect_score * 0.2;
    total_weight += 0.2;

    // Material/acoustic return matching
    double acoustic_diff = std::abs(features.acoustic_return - templ.acoustic_signature);
    double acoustic_score = std::exp(-acoustic_diff);
    score += acoustic_score * 0.25;
    total_weight += 0.25;

    // Magnetic anomaly matching
    double mag_diff = std::abs(features.magnetic_anomaly - templ.magnetic_signature);
    double mag_score = std::exp(-mag_diff * 2.0);
    score += mag_score * 0.25;
    total_weight += 0.25;

    return score / total_weight;
}

void MineClassifier::initialize_templates() {
    // Initialize mine type templates (simplified)

    // Bottom mine template
    MineTemplate bottom_template;
    bottom_template.mine_type = MineType::BOTTOM_MINE;
    bottom_template.typical_size_m = 2.0;
    bottom_template.aspect_ratio = 2.0;
    bottom_template.acoustic_signature = 0.8;
    bottom_template.magnetic_signature = 0.7;
    templates_.push_back(bottom_template);

    // Moored mine template
    MineTemplate moored_template;
    moored_template.mine_type = MineType::MOORED_MINE;
    moored_template.typical_size_m = 1.2;
    moored_template.aspect_ratio = 1.1;
    moored_template.acoustic_signature = 0.7;
    moored_template.magnetic_signature = 0.5;
    templates_.push_back(moored_template);

    // Drifting mine template
    MineTemplate drifting_template;
    drifting_template.mine_type = MineType::DRIFTING_MINE;
    drifting_template.typical_size_m = 0.6;
    drifting_template.aspect_ratio = 1.0;
    drifting_template.acoustic_signature = 0.5;
    drifting_template.magnetic_signature = 0.3;
    templates_.push_back(drifting_template);
}

// ============================================================================
// TrackingFilter Implementation
// ============================================================================

TrackingFilter::TrackingFilter(const SignalProcessorConfig& config)
    : config_(config)
    , next_track_id_(1)
{
}

void TrackingFilter::update(const std::vector<MineContact>& detections, double timestamp) {
    // Data association: match detections to existing tracks
    std::vector<bool> detection_associated(detections.size(), false);

    // Update existing tracks
    for (auto& track : tracks_) {
        bool updated = false;

        // Find nearest detection
        double min_distance = config_.max_association_distance_m;
        size_t best_detection_idx = 0;

        for (size_t i = 0; i < detections.size(); i++) {
            if (detection_associated[i]) continue;

            double distance = compute_distance(track, detections[i]);
            if (distance < min_distance) {
                min_distance = distance;
                best_detection_idx = i;
                updated = true;
            }
        }

        if (updated) {
            // Update track with detection
            update_track(track, detections[best_detection_idx], timestamp);
            detection_associated[best_detection_idx] = true;
        } else {
            // No detection - increment coast count
            track.coast_count++;
        }
    }

    // Create new tracks for unassociated detections
    for (size_t i = 0; i < detections.size(); i++) {
        if (!detection_associated[i]) {
            MineTrack new_track;
            new_track.track_id = next_track_id_++;
            new_track.contact = detections[i];
            new_track.first_detection_time = timestamp;
            new_track.last_update_time = timestamp;
            new_track.detection_count = 1;
            new_track.coast_count = 0;
            new_track.confirmed = false;
            tracks_.push_back(new_track);
        }
    }

    // Remove stale tracks
    tracks_.erase(
        std::remove_if(tracks_.begin(), tracks_.end(),
            [this](const MineTrack& track) {
                return track.coast_count > config_.max_coast_count;
            }),
        tracks_.end()
    );
}

std::vector<MineTrack> TrackingFilter::get_confirmed_tracks() const {
    std::vector<MineTrack> confirmed;
    for (const auto& track : tracks_) {
        if (track.confirmed) {
            confirmed.push_back(track);
        }
    }
    return confirmed;
}

std::vector<MineTrack> TrackingFilter::get_all_tracks() const {
    return tracks_;
}

void TrackingFilter::reset() {
    tracks_.clear();
    next_track_id_ = 1;
}

double TrackingFilter::compute_distance(const MineTrack& track, const MineContact& detection) {
    // Euclidean distance in 3D space
    double dx = track.contact.position[0] - detection.position[0];
    double dy = track.contact.position[1] - detection.position[1];
    double dz = track.contact.position[2] - detection.position[2];
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

void TrackingFilter::update_track(MineTrack& track, const MineContact& detection, double timestamp) {
    // Update track with new detection
    track.contact = detection;
    track.last_update_time = timestamp;
    track.detection_count++;
    track.coast_count = 0;

    // Confirm track after multiple consistent detections
    if (track.detection_count >= config_.min_detections_for_confirmation &&
        !track.confirmed) {
        track.confirmed = true;
    }
}

// ============================================================================
// SignalProcessor Implementation
// ============================================================================

SignalProcessor::SignalProcessor(const SignalProcessorConfig& config)
    : config_(config)
    , feature_extractor_(config)
    , classifier_(config)
    , tracker_(config)
    , initialized_(false)
{
}

void SignalProcessor::initialize() {
    tracker_.reset();
    initialized_ = true;
}

void SignalProcessor::process_sensor_data(const SensorData& data, double timestamp) {
    if (!initialized_) {
        throw std::runtime_error("SignalProcessor not initialized");
    }

    if (!data.is_valid()) {
        return;  // Skip invalid data
    }

    // Stage 1: Feature extraction
    MineFeatures features = feature_extractor_.extract(data);

    // Stage 2: Detection decision
    if (!is_potential_mine(features, data)) {
        return;  // Not a mine, skip
    }

    // Stage 3: Classification
    double classification_confidence = 0.0;
    MineType mine_type = classifier_.classify(features, classification_confidence);

    // Stage 4: Create mine contact
    MineContact contact;
    contact.contact_id = static_cast<uint32_t>(timestamp * 1000.0);  // Unique ID
    contact.position[0] = data.range_m * std::cos(data.bearing_rad);
    contact.position[1] = data.range_m * std::sin(data.bearing_rad);
    contact.position[2] = -data.range_m;  // Depth (negative Z)
    contact.mine_type = mine_type;
    contact.classification_confidence = classification_confidence;
    contact.snr_db = data.compute_snr();
    contact.signal_quality = data.quality;
    contact.detection_timestamp = timestamp;
    contact.classification_attempts = 1;
    contact.false_alarm_probability = estimate_false_alarm_prob(features, data);

    // Stage 5: Update tracker
    std::vector<MineContact> detections = {contact};
    tracker_.update(detections, timestamp);
}

std::vector<MineTrack> SignalProcessor::get_mine_tracks() const {
    return tracker_.get_confirmed_tracks();
}

std::vector<MineTrack> SignalProcessor::get_all_tracks() const {
    return tracker_.get_all_tracks();
}

void SignalProcessor::reset() {
    tracker_.reset();
}

bool SignalProcessor::is_potential_mine(const MineFeatures& features, const SensorData& data) {
    // Multi-criteria detection decision

    // Criterion 1: SNR threshold
    if (data.compute_snr() < config_.min_snr_db) {
        return false;
    }

    // Criterion 2: Size constraints
    if (features.size_estimate_m < 0.1 || features.size_estimate_m > 5.0) {
        return false;
    }

    // Criterion 3: Signal quality
    if (data.quality < 0.3) {
        return false;
    }

    // Criterion 4: Acoustic return strength
    if (features.acoustic_return < 0.2) {
        return false;
    }

    return true;
}

double SignalProcessor::estimate_false_alarm_prob(const MineFeatures& features, const SensorData& data) {
    // Estimate false alarm probability using detection statistics

    // Higher SNR → lower false alarm
    double snr_factor = 1.0 / (1.0 + data.compute_snr() / 10.0);

    // Higher quality → lower false alarm
    double quality_factor = 1.0 - data.quality;

    // Typical mine features → lower false alarm
    double feature_factor = 0.5;
    if (features.size_estimate_m > 0.5 && features.size_estimate_m < 3.0) {
        feature_factor = 0.2;  // Size in typical mine range
    }

    double pfa = 0.3 * snr_factor + 0.3 * quality_factor + 0.4 * feature_factor;
    return std::clamp(pfa, 0.0, 1.0);
}

} // namespace signals
} // namespace rmh
