/**
 * @file primal_logic_kernel.h
 * @brief Primal Logic Integral Kernel for Stealth Signature Management
 *
 * Implements temporal smoothing and adaptive control for five stealth domains:
 * - Radar Cross Section (RCS)
 * - Infrared/Thermal (IR)
 * - Electromagnetic Emission Control (EMCON)
 * - Acoustic Signature
 * - Dynamic Maneuver
 *
 * Core Algorithm:
 *   I_PL(t) = ∫₀ᵗ Θ(τ) f(τ) e^(-λ(t-τ)) dτ
 *
 * Where:
 *   f(τ) = error/deviation signal from ideal stealth signature
 *   Θ(τ) = adaptive temporal weighting function
 *   λ = exponential decay coefficient
 *
 * Stability Guarantee:
 *   |I_PL(t)| ≤ sup|Θ(τ)f(τ)| / λ
 *
 * @author Primal Logic
 * @date 2025-11-23
 * @classification UNCLASSIFIED
 */

#ifndef PRIMAL_LOGIC_KERNEL_H
#define PRIMAL_LOGIC_KERNEL_H

#include <cstdint>
#include <vector>
#include <cmath>
#include <functional>
#include <memory>

namespace rmh {
namespace stealth {

/**
 * @brief Stealth domain types
 */
enum class StealthDomain {
    RADAR_RCS,      ///< Radar Cross Section
    THERMAL_IR,     ///< Infrared/Thermal signature
    EMCON,          ///< Electromagnetic emission control
    ACOUSTIC,       ///< Acoustic signature
    MANEUVER        ///< Dynamic maneuver profile
};

/**
 * @brief Configuration parameters for Primal Logic kernel
 */
struct PrimalLogicConfig {
    double lambda;                  ///< Exponential decay coefficient (λ)
    double theta_base;              ///< Base adaptive weighting (Θ base)
    double theta_adaptive_gain;     ///< Adaptive gain for Θ(τ)
    double sampling_rate_hz;        ///< Sampling rate in Hz
    double stability_bound;         ///< Maximum allowed correction magnitude
    bool enable_adaptive_weighting; ///< Enable/disable adaptive Θ(τ)

    PrimalLogicConfig()
        : lambda(0.1)
        , theta_base(1.0)
        , theta_adaptive_gain(0.5)
        , sampling_rate_hz(100.0)
        , stability_bound(10.0)
        , enable_adaptive_weighting(true)
    {}
};

/**
 * @brief Primal Logic Integral Kernel
 *
 * Computes temporally-smoothed stealth signature corrections using
 * exponentially-weighted history integration.
 */
class PrimalLogicKernel {
public:
    explicit PrimalLogicKernel(const PrimalLogicConfig& config);
    ~PrimalLogicKernel() = default;

    /**
     * @brief Update kernel with new error signal
     * @param error_signal Current deviation from ideal signature
     * @param timestamp_sec Current time in seconds
     * @return Smoothed correction output I_PL(t)
     */
    double update(double error_signal, double timestamp_sec);

    /**
     * @brief Reset kernel state
     */
    void reset();

    /**
     * @brief Get current kernel output
     * @return Current I_PL(t) value
     */
    double get_output() const { return output_; }

    /**
     * @brief Get stability bound check
     * @return True if output is within stability bounds
     */
    bool is_stable() const;

    /**
     * @brief Set adaptive weighting function
     * @param theta_func Custom Θ(τ) function
     */
    void set_adaptive_weighting(std::function<double(double, double)> theta_func);

    /**
     * @brief Get kernel configuration
     */
    const PrimalLogicConfig& get_config() const { return config_; }

private:
    /**
     * @brief Compute adaptive weighting Θ(τ)
     * @param error Current error signal
     * @param dt Time delta
     * @return Θ(τ) value
     */
    double compute_theta(double error, double dt);

    /**
     * @brief Compute exponential decay weight
     * @param dt Time delta since last update
     * @return e^(-λ * dt)
     */
    double compute_decay(double dt) const;

    PrimalLogicConfig config_;
    double output_;                     ///< Current kernel output I_PL(t)
    double last_timestamp_;             ///< Last update timestamp
    std::vector<double> history_error_; ///< Error signal history buffer
    std::vector<double> history_time_;  ///< Time history buffer
    std::function<double(double, double)> custom_theta_; ///< Custom Θ function
};

/**
 * @brief Domain-Specific Primal Logic Controller
 *
 * Applies Primal Logic kernel to specific stealth domain with
 * domain-appropriate error signal processing.
 */
class StealthDomainController {
public:
    StealthDomainController(StealthDomain domain, const PrimalLogicConfig& config);
    ~StealthDomainController() = default;

    /**
     * @brief Update controller with measured and target signatures
     * @param measured_signature Current measured signature value
     * @param target_signature Desired signature value
     * @param timestamp_sec Current time
     * @return Control output for actuators/systems
     */
    double update(double measured_signature, double target_signature, double timestamp_sec);

    /**
     * @brief Get domain type
     */
    StealthDomain get_domain() const { return domain_; }

    /**
     * @brief Get kernel instance
     */
    const PrimalLogicKernel& get_kernel() const { return kernel_; }

    /**
     * @brief Get current error signal
     */
    double get_error() const { return current_error_; }

    /**
     * @brief Reset controller state
     */
    void reset();

private:
    /**
     * @brief Compute domain-specific error signal
     */
    double compute_error(double measured, double target);

    /**
     * @brief Apply domain-specific output scaling/transformation
     */
    double transform_output(double kernel_output);

    StealthDomain domain_;
    PrimalLogicKernel kernel_;
    double current_error_;
    double output_scale_;  ///< Domain-specific output scaling
};

/**
 * @brief Multi-Domain Stealth Management System
 *
 * Coordinates Primal Logic controllers across all five stealth domains.
 */
class StealthManagementSystem {
public:
    StealthManagementSystem();
    ~StealthManagementSystem() = default;

    /**
     * @brief Initialize all domain controllers
     * @param config Configuration for all kernels
     */
    void initialize(const PrimalLogicConfig& config);

    /**
     * @brief Update RCS domain
     * @param measured_rcs Current RCS in dBsm
     * @param target_rcs Target RCS in dBsm
     * @param timestamp_sec Current time
     * @return RCS actuator control output
     */
    double update_rcs(double measured_rcs, double target_rcs, double timestamp_sec);

    /**
     * @brief Update thermal/IR domain
     * @param measured_temp Current temperature signature (K)
     * @param target_temp Target temperature (K)
     * @param timestamp_sec Current time
     * @return Thermal management control output
     */
    double update_thermal(double measured_temp, double target_temp, double timestamp_sec);

    /**
     * @brief Update EMCON domain
     * @param measured_power Current emission power (dBm)
     * @param target_power Target emission power (dBm)
     * @param timestamp_sec Current time
     * @return Emission control output
     */
    double update_emcon(double measured_power, double target_power, double timestamp_sec);

    /**
     * @brief Update acoustic domain
     * @param measured_noise Current acoustic signature (dB)
     * @param target_noise Target acoustic signature (dB)
     * @param timestamp_sec Current time
     * @return Acoustic control output
     */
    double update_acoustic(double measured_noise, double target_noise, double timestamp_sec);

    /**
     * @brief Update maneuver domain
     * @param measured_velocity Current velocity vector magnitude (m/s)
     * @param target_velocity Target velocity (m/s)
     * @param timestamp_sec Current time
     * @return Maneuver control output
     */
    double update_maneuver(double measured_velocity, double target_velocity, double timestamp_sec);

    /**
     * @brief Check if all domains are stable
     */
    bool all_domains_stable() const;

    /**
     * @brief Get controller for specific domain
     */
    const StealthDomainController* get_controller(StealthDomain domain) const;

    /**
     * @brief Reset all domain controllers
     */
    void reset_all();

    /**
     * @brief Get system status report
     */
    struct SystemStatus {
        bool all_stable;
        double rcs_output;
        double thermal_output;
        double emcon_output;
        double acoustic_output;
        double maneuver_output;
        double max_error;
    };
    SystemStatus get_status() const;

private:
    std::unique_ptr<StealthDomainController> rcs_controller_;
    std::unique_ptr<StealthDomainController> thermal_controller_;
    std::unique_ptr<StealthDomainController> emcon_controller_;
    std::unique_ptr<StealthDomainController> acoustic_controller_;
    std::unique_ptr<StealthDomainController> maneuver_controller_;

    bool initialized_;
};

} // namespace stealth
} // namespace rmh

#endif // PRIMAL_LOGIC_KERNEL_H
