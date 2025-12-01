/**
 * @file autonomy_controller.h
 * @brief RMH Autonomy and Control System for Tow Body Operations
 *
 * Provides autonomous navigation, mine search patterns, obstacle avoidance,
 * and safety behaviors for the Tow Body during remote minehunting missions.
 *
 * @author Primal Logic
 * @date 2025-11-23
 * @classification UNCLASSIFIED
 */

#ifndef RMH_AUTONOMY_CONTROLLER_H
#define RMH_AUTONOMY_CONTROLLER_H

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace rmh {
namespace autonomy {

/**
 * @brief Control modes for Tow Body
 */
enum class ControlMode {
    MANUAL,           // Direct operator control
    SEMI_AUTO,        // Waypoint navigation with operator oversight
    FULL_AUTO,        // AI-driven autonomous operation
    EMERGENCY_SURFACE // Emergency surface and stop
};

/**
 * @brief Mission phase
 */
enum class MissionPhase {
    IDLE,             // Waiting for mission start
    DEPLOYING,        // Deploying from MCM USV
    TRANSITING,       // Transiting to search area
    SEARCHING,        // Active mine hunting
    INVESTIGATING,    // Re-investigating contact
    RECOVERING,       // Returning to MCM USV
    SURFACED          // Emergency surfaced
};

/**
 * @brief Search pattern types
 */
enum class SearchPattern {
    LAWN_MOWER,       // Standard back-and-forth pattern
    SPIRAL,           // Expanding spiral search
    SECTOR_SCAN,      // Sector-by-sector coverage
    RANDOM_WALK,      // Random walk with full coverage
    CUSTOM            // User-defined waypoints
};

/**
 * @brief Geographic waypoint
 */
struct Waypoint {
    double latitude;
    double longitude;
    double depth;              // Target depth (meters)
    double speed;              // Target speed (knots)
    double heading;            // Target heading (degrees, 0=North)
    uint64_t arrival_time_ms;  // Expected arrival (0 = ASAP)
    std::string id;            // Waypoint identifier
};

/**
 * @brief Search area definition
 */
struct SearchArea {
    std::vector<Waypoint> boundary;  // Polygon boundary
    double min_depth;                // Minimum safe depth
    double max_depth;                // Maximum operating depth
    SearchPattern pattern;
    double track_spacing;            // Spacing between search tracks (m)
};

/**
 * @brief Tow Body state estimate
 */
struct VehicleState {
    double latitude;
    double longitude;
    double depth;
    double altitude;           // Height above seafloor
    double heading;            // 0-360 degrees
    double speed;              // Knots
    double pitch;              // Degrees
    double roll;               // Degrees
    uint64_t timestamp_ms;
};

/**
 * @brief Obstacle information
 */
struct Obstacle {
    double latitude;
    double longitude;
    double radius;             // Avoidance radius (meters)
    double depth_min;          // Obstacle depth range
    double depth_max;
    std::string type;          // e.g., "rock", "wreck", "shipping_lane"
};

/**
 * @brief Safety limits and constraints
 */
struct SafetyLimits {
    double max_depth;          // Maximum depth (meters)
    double min_altitude;       // Minimum altitude above seafloor (meters)
    double max_speed;          // Maximum speed (knots)
    double max_pitch;          // Maximum pitch angle (degrees)
    double max_roll;           // Maximum roll angle (degrees)
    double geofence_radius;    // Maximum distance from MCM USV (meters)
    uint32_t comms_timeout_ms; // Max time without comms before emergency surface
};

/**
 * @brief Autonomy configuration
 */
struct AutonomyConfig {
    ControlMode initial_mode;
    SearchPattern search_pattern;
    SafetyLimits safety_limits;
    bool obstacle_avoidance_enabled;
    bool adaptive_search_enabled;      // Adjust search based on detections
    double waypoint_tolerance;         // Waypoint arrival tolerance (meters)
};

/**
 * @brief Callback for state updates
 */
using StateCallback = std::function<void(const VehicleState& state)>;

/**
 * @brief Callback for mode changes
 */
using ModeCallback = std::function<void(ControlMode old_mode, ControlMode new_mode)>;

/**
 * @brief Main autonomy controller
 */
class AutonomyController {
public:
    /**
     * @brief Constructor
     * @param config Autonomy configuration
     */
    explicit AutonomyController(const AutonomyConfig& config);

    /**
     * @brief Destructor
     */
    ~AutonomyController();

    /**
     * @brief Initialize controller with vehicle state
     * @param initial_state Starting vehicle state
     * @return true if successful
     */
    bool initialize(const VehicleState& initial_state);

    /**
     * @brief Update vehicle state (call at high rate, e.g., 10 Hz)
     * @param state Current vehicle state from sensors
     */
    void updateState(const VehicleState& state);

    /**
     * @brief Set control mode
     * @param mode New control mode
     * @return true if mode change accepted
     */
    bool setControlMode(ControlMode mode);

    /**
     * @brief Get current control mode
     */
    ControlMode getControlMode() const;

    /**
     * @brief Set mission search area
     * @param area Search area definition
     * @return true if area is valid
     */
    bool setSearchArea(const SearchArea& area);

    /**
     * @brief Add waypoint to navigation queue
     * @param waypoint Waypoint to add
     */
    void addWaypoint(const Waypoint& waypoint);

    /**
     * @brief Clear all waypoints
     */
    void clearWaypoints();

    /**
     * @brief Get current mission phase
     */
    MissionPhase getMissionPhase() const;

    /**
     * @brief Add obstacle to avoidance list
     * @param obstacle Obstacle information
     */
    void addObstacle(const Obstacle& obstacle);

    /**
     * @brief Remove all obstacles
     */
    void clearObstacles();

    /**
     * @brief Compute control outputs (rudder, throttle, dive plane)
     * @return Control vector [rudder, throttle, dive_plane] in range [-1, 1]
     */
    std::vector<double> computeControl();

    /**
     * @brief Emergency stop - surface immediately
     */
    void emergencyStop();

    /**
     * @brief Register callback for state updates
     * @param callback Function to call on state update
     */
    void registerStateCallback(StateCallback callback);

    /**
     * @brief Register callback for mode changes
     * @param callback Function to call on mode change
     */
    void registerModeCallback(ModeCallback callback);

    /**
     * @brief Get current navigation target
     * @return Next waypoint or current target position
     */
    Waypoint getCurrentTarget() const;

    /**
     * @brief Get estimated time to next waypoint
     * @return Seconds to waypoint arrival
     */
    double getTimeToWaypoint() const;

    /**
     * @brief Get search coverage percentage
     * @return Percentage of search area covered (0-100)
     */
    double getSearchCoverage() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Path planning utilities
 */
class PathPlanner {
public:
    /**
     * @brief Generate lawn mower search pattern
     * @param area Search area
     * @param track_spacing Spacing between tracks (meters)
     * @return Ordered list of waypoints
     */
    static std::vector<Waypoint> generateLawnMower(
        const SearchArea& area,
        double track_spacing);

    /**
     * @brief Generate expanding spiral search pattern
     * @param center Center point
     * @param max_radius Maximum radius (meters)
     * @param track_spacing Spacing between spirals
     * @return Ordered list of waypoints
     */
    static std::vector<Waypoint> generateSpiral(
        const Waypoint& center,
        double max_radius,
        double track_spacing);

    /**
     * @brief Compute shortest collision-free path
     * @param start Start position
     * @param goal Goal position
     * @param obstacles List of obstacles
     * @return Path as ordered waypoints
     */
    static std::vector<Waypoint> planPath(
        const Waypoint& start,
        const Waypoint& goal,
        const std::vector<Obstacle>& obstacles);

    /**
     * @brief Check if path is collision-free
     * @param path Path to check
     * @param obstacles List of obstacles
     * @return true if path is safe
     */
    static bool isPathSafe(
        const std::vector<Waypoint>& path,
        const std::vector<Obstacle>& obstacles);
};

/**
 * @brief Navigation controller (low-level guidance)
 */
class NavigationController {
public:
    /**
     * @brief Compute heading to waypoint
     * @param current Current position
     * @param target Target waypoint
     * @return Bearing in degrees (0-360)
     */
    static double computeBearing(
        const VehicleState& current,
        const Waypoint& target);

    /**
     * @brief Compute distance to waypoint
     * @param current Current position
     * @param target Target waypoint
     * @return Distance in meters
     */
    static double computeDistance(
        const VehicleState& current,
        const Waypoint& target);

    /**
     * @brief Check if waypoint reached
     * @param current Current position
     * @param target Target waypoint
     * @param tolerance Arrival tolerance (meters)
     * @return true if within tolerance
     */
    static bool isWaypointReached(
        const VehicleState& current,
        const Waypoint& target,
        double tolerance);

    /**
     * @brief Compute cross-track error
     * @param current Current position
     * @param path_start Path segment start
     * @param path_end Path segment end
     * @return Cross-track error in meters (+ = right, - = left)
     */
    static double computeCrossTrackError(
        const VehicleState& current,
        const Waypoint& path_start,
        const Waypoint& path_end);
};

/**
 * @brief PID controller for heading/depth control
 */
class PIDController {
public:
    /**
     * @brief Constructor
     * @param kp Proportional gain
     * @param ki Integral gain
     * @param kd Derivative gain
     */
    PIDController(double kp, double ki, double kd);

    /**
     * @brief Compute control output
     * @param setpoint Desired value
     * @param measured Current value
     * @param dt Time step (seconds)
     * @return Control output
     */
    double compute(double setpoint, double measured, double dt);

    /**
     * @brief Reset integral and derivative terms
     */
    void reset();

    /**
     * @brief Set output limits
     * @param min Minimum output
     * @param max Maximum output
     */
    void setLimits(double min, double max);

private:
    double kp_, ki_, kd_;
    double integral_;
    double prev_error_;
    double out_min_;
    double out_max_;
};

/**
 * @brief Obstacle avoidance using potential fields
 */
class ObstacleAvoidance {
public:
    /**
     * @brief Compute avoidance vector
     * @param current Current position
     * @param target Target position
     * @param obstacles List of obstacles
     * @return Adjusted heading to avoid obstacles
     */
    static double computeAvoidanceHeading(
        const VehicleState& current,
        const Waypoint& target,
        const std::vector<Obstacle>& obstacles);

    /**
     * @brief Check if position is in collision
     * @param position Position to check
     * @param obstacles List of obstacles
     * @return true if collision detected
     */
    static bool isCollision(
        const VehicleState& position,
        const std::vector<Obstacle>& obstacles);

    /**
     * @brief Compute safe speed based on obstacle proximity
     * @param current Current position
     * @param obstacles List of obstacles
     * @param max_speed Maximum speed
     * @return Safe speed (knots)
     */
    static double computeSafeSpeed(
        const VehicleState& current,
        const std::vector<Obstacle>& obstacles,
        double max_speed);
};

/**
 * @brief Lost communications recovery behavior
 */
class LostCommsRecovery {
public:
    /**
     * @brief Execute lost-comms recovery procedure
     * @param current Current vehicle state
     * @param home_position MCM USV position
     * @param time_without_comms Time since last valid message (ms)
     * @return Recovery waypoint (usually surface or return home)
     */
    static Waypoint executeRecovery(
        const VehicleState& current,
        const Waypoint& home_position,
        uint64_t time_without_comms);

    /**
     * @brief Check if recovery procedure should be initiated
     * @param time_without_comms Time since last message (ms)
     * @param timeout_threshold Timeout threshold (ms)
     * @return true if recovery should start
     */
    static bool shouldInitiateRecovery(
        uint64_t time_without_comms,
        uint64_t timeout_threshold);
};

} // namespace autonomy
} // namespace rmh

#endif // RMH_AUTONOMY_CONTROLLER_H
