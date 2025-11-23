/**
 * @file primal_logic_kernel.cpp
 * @brief Implementation of Primal Logic Integral Kernel
 *
 * @author Primal Logic
 * @date 2025-11-23
 * @classification UNCLASSIFIED
 */

#include "primal_logic_kernel.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace rmh {
namespace stealth {

// ============================================================================
// PrimalLogicKernel Implementation
// ============================================================================

PrimalLogicKernel::PrimalLogicKernel(const PrimalLogicConfig& config)
    : config_(config)
    , output_(0.0)
    , last_timestamp_(0.0)
    , custom_theta_(nullptr)
{
    if (config_.lambda <= 0.0) {
        throw std::invalid_argument("Lambda must be positive for stability");
    }
    if (config_.sampling_rate_hz <= 0.0) {
        throw std::invalid_argument("Sampling rate must be positive");
    }

    // Reserve history buffers (keep last ~10 seconds of history)
    size_t history_size = static_cast<size_t>(config_.sampling_rate_hz * 10.0);
    history_error_.reserve(history_size);
    history_time_.reserve(history_size);
}

double PrimalLogicKernel::update(double error_signal, double timestamp_sec) {
    if (history_time_.empty()) {
        // First update - initialize
        output_ = 0.0;
        last_timestamp_ = timestamp_sec;
        history_error_.push_back(error_signal);
        history_time_.push_back(timestamp_sec);
        return output_;
    }

    // Compute time delta
    double dt = timestamp_sec - last_timestamp_;
    if (dt < 0.0) {
        throw std::runtime_error("Timestamp must be monotonically increasing");
    }
    if (dt < 1e-9) {
        // Too small dt, skip update
        return output_;
    }

    // Compute adaptive weighting Θ(τ)
    double theta = compute_theta(error_signal, dt);

    // Compute exponential decay factor
    double decay = compute_decay(dt);

    // Update integral using discrete exponential smoothing:
    // I_PL[n] = I_PL[n-1] * e^(-λ*dt) + Θ[n] * f[n] * dt
    //
    // This is the discrete-time approximation of:
    // I_PL(t) = ∫₀ᵗ Θ(τ) f(τ) e^(-λ(t-τ)) dτ
    output_ = output_ * decay + theta * error_signal * dt;

    // Apply stability bound
    if (std::abs(output_) > config_.stability_bound) {
        output_ = std::copysign(config_.stability_bound, output_);
    }

    // Update history
    history_error_.push_back(error_signal);
    history_time_.push_back(timestamp_sec);

    // Trim history if too large (keep last 10 seconds)
    size_t max_history = static_cast<size_t>(config_.sampling_rate_hz * 10.0);
    if (history_error_.size() > max_history) {
        history_error_.erase(history_error_.begin());
        history_time_.erase(history_time_.begin());
    }

    last_timestamp_ = timestamp_sec;
    return output_;
}

void PrimalLogicKernel::reset() {
    output_ = 0.0;
    last_timestamp_ = 0.0;
    history_error_.clear();
    history_time_.clear();
}

bool PrimalLogicKernel::is_stable() const {
    return std::abs(output_) <= config_.stability_bound;
}

void PrimalLogicKernel::set_adaptive_weighting(std::function<double(double, double)> theta_func) {
    custom_theta_ = theta_func;
}

double PrimalLogicKernel::compute_theta(double error, double dt) {
    // If custom Θ function provided, use it
    if (custom_theta_) {
        return custom_theta_(error, dt);
    }

    if (!config_.enable_adaptive_weighting) {
        return config_.theta_base;
    }

    // Adaptive weighting based on error magnitude:
    // Θ(τ) = θ_base + θ_gain * |error|
    //
    // This increases weighting when error is large (faster correction)
    // and decreases when error is small (smoother behavior)
    double theta = config_.theta_base + config_.theta_adaptive_gain * std::abs(error);

    // Clamp to reasonable range [0.1, 10.0]
    theta = std::max(0.1, std::min(10.0, theta));

    return theta;
}

double PrimalLogicKernel::compute_decay(double dt) const {
    // e^(-λ * dt)
    return std::exp(-config_.lambda * dt);
}

// ============================================================================
// StealthDomainController Implementation
// ============================================================================

StealthDomainController::StealthDomainController(StealthDomain domain, const PrimalLogicConfig& config)
    : domain_(domain)
    , kernel_(config)
    , current_error_(0.0)
    , output_scale_(1.0)
{
    // Set domain-specific output scaling
    switch (domain_) {
        case StealthDomain::RADAR_RCS:
            output_scale_ = 1.0;  // dBsm units
            break;
        case StealthDomain::THERMAL_IR:
            output_scale_ = 10.0;  // K units, scaled for actuator control
            break;
        case StealthDomain::EMCON:
            output_scale_ = 1.0;  // dBm units
            break;
        case StealthDomain::ACOUSTIC:
            output_scale_ = 1.0;  // dB units
            break;
        case StealthDomain::MANEUVER:
            output_scale_ = 0.1;  // m/s units, scaled for smooth trajectory
            break;
    }
}

double StealthDomainController::update(double measured_signature, double target_signature, double timestamp_sec) {
    // Compute error signal
    current_error_ = compute_error(measured_signature, target_signature);

    // Update kernel with error
    double kernel_output = kernel_.update(current_error_, timestamp_sec);

    // Transform output for domain-specific actuators
    return transform_output(kernel_output);
}

void StealthDomainController::reset() {
    kernel_.reset();
    current_error_ = 0.0;
}

double StealthDomainController::compute_error(double measured, double target) {
    // Error = measured - target
    // Positive error means signature too high, need to reduce
    // Negative error means signature too low, need to increase
    return measured - target;
}

double StealthDomainController::transform_output(double kernel_output) {
    // Apply domain-specific scaling
    double output = kernel_output * output_scale_;

    // Domain-specific transformations
    switch (domain_) {
        case StealthDomain::RADAR_RCS:
            // RCS control: negative output reduces RCS (deploy absorbers, adjust geometry)
            output = -output;
            break;

        case StealthDomain::THERMAL_IR:
            // Thermal control: negative output increases cooling
            output = -output;
            break;

        case StealthDomain::EMCON:
            // EMCON: negative output reduces emissions
            output = -output;
            break;

        case StealthDomain::ACOUSTIC:
            // Acoustic: negative output activates noise cancellation
            output = -output;
            break;

        case StealthDomain::MANEUVER:
            // Maneuver: output directly adjusts trajectory smoothly
            // No sign flip needed
            break;
    }

    return output;
}

// ============================================================================
// StealthManagementSystem Implementation
// ============================================================================

StealthManagementSystem::StealthManagementSystem()
    : initialized_(false)
{
}

void StealthManagementSystem::initialize(const PrimalLogicConfig& config) {
    rcs_controller_ = std::make_unique<StealthDomainController>(StealthDomain::RADAR_RCS, config);
    thermal_controller_ = std::make_unique<StealthDomainController>(StealthDomain::THERMAL_IR, config);
    emcon_controller_ = std::make_unique<StealthDomainController>(StealthDomain::EMCON, config);
    acoustic_controller_ = std::make_unique<StealthDomainController>(StealthDomain::ACOUSTIC, config);
    maneuver_controller_ = std::make_unique<StealthDomainController>(StealthDomain::MANEUVER, config);

    initialized_ = true;
}

double StealthManagementSystem::update_rcs(double measured_rcs, double target_rcs, double timestamp_sec) {
    if (!initialized_) {
        throw std::runtime_error("StealthManagementSystem not initialized");
    }
    return rcs_controller_->update(measured_rcs, target_rcs, timestamp_sec);
}

double StealthManagementSystem::update_thermal(double measured_temp, double target_temp, double timestamp_sec) {
    if (!initialized_) {
        throw std::runtime_error("StealthManagementSystem not initialized");
    }
    return thermal_controller_->update(measured_temp, target_temp, timestamp_sec);
}

double StealthManagementSystem::update_emcon(double measured_power, double target_power, double timestamp_sec) {
    if (!initialized_) {
        throw std::runtime_error("StealthManagementSystem not initialized");
    }
    return emcon_controller_->update(measured_power, target_power, timestamp_sec);
}

double StealthManagementSystem::update_acoustic(double measured_noise, double target_noise, double timestamp_sec) {
    if (!initialized_) {
        throw std::runtime_error("StealthManagementSystem not initialized");
    }
    return acoustic_controller_->update(measured_noise, target_noise, timestamp_sec);
}

double StealthManagementSystem::update_maneuver(double measured_velocity, double target_velocity, double timestamp_sec) {
    if (!initialized_) {
        throw std::runtime_error("StealthManagementSystem not initialized");
    }
    return maneuver_controller_->update(measured_velocity, target_velocity, timestamp_sec);
}

bool StealthManagementSystem::all_domains_stable() const {
    if (!initialized_) return false;

    return rcs_controller_->get_kernel().is_stable() &&
           thermal_controller_->get_kernel().is_stable() &&
           emcon_controller_->get_kernel().is_stable() &&
           acoustic_controller_->get_kernel().is_stable() &&
           maneuver_controller_->get_kernel().is_stable();
}

const StealthDomainController* StealthManagementSystem::get_controller(StealthDomain domain) const {
    if (!initialized_) return nullptr;

    switch (domain) {
        case StealthDomain::RADAR_RCS: return rcs_controller_.get();
        case StealthDomain::THERMAL_IR: return thermal_controller_.get();
        case StealthDomain::EMCON: return emcon_controller_.get();
        case StealthDomain::ACOUSTIC: return acoustic_controller_.get();
        case StealthDomain::MANEUVER: return maneuver_controller_.get();
        default: return nullptr;
    }
}

void StealthManagementSystem::reset_all() {
    if (!initialized_) return;

    rcs_controller_->reset();
    thermal_controller_->reset();
    emcon_controller_->reset();
    acoustic_controller_->reset();
    maneuver_controller_->reset();
}

StealthManagementSystem::SystemStatus StealthManagementSystem::get_status() const {
    SystemStatus status;

    if (!initialized_) {
        status.all_stable = false;
        status.rcs_output = 0.0;
        status.thermal_output = 0.0;
        status.emcon_output = 0.0;
        status.acoustic_output = 0.0;
        status.maneuver_output = 0.0;
        status.max_error = 0.0;
        return status;
    }

    status.all_stable = all_domains_stable();
    status.rcs_output = rcs_controller_->get_kernel().get_output();
    status.thermal_output = thermal_controller_->get_kernel().get_output();
    status.emcon_output = emcon_controller_->get_kernel().get_output();
    status.acoustic_output = acoustic_controller_->get_kernel().get_output();
    status.maneuver_output = maneuver_controller_->get_kernel().get_output();

    // Compute max error across all domains
    status.max_error = std::max({
        std::abs(rcs_controller_->get_error()),
        std::abs(thermal_controller_->get_error()),
        std::abs(emcon_controller_->get_error()),
        std::abs(acoustic_controller_->get_error()),
        std::abs(maneuver_controller_->get_error())
    });

    return status;
}

} // namespace stealth
} // namespace rmh
