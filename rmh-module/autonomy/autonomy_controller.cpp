/**
 * @file autonomy_controller.cpp
 * @brief Implementation of RMH Autonomy Controller
 */

#include "autonomy_controller.h"
#include <algorithm>
#include <cmath>
#include <queue>

namespace rmh {
namespace autonomy {

// AutonomyController Implementation
class AutonomyController::Impl {
public:
    explicit Impl(const AutonomyConfig& config)
        : config_(config)
        , current_mode_(config.initial_mode)
        , current_phase_(MissionPhase::IDLE)
        , heading_pid_(1.0, 0.1, 0.5)
        , depth_pid_(0.8, 0.05, 0.3)
        , speed_pid_(0.5, 0.1, 0.2)
    {
        heading_pid_.setLimits(-1.0, 1.0);
        depth_pid_.setLimits(-1.0, 1.0);
        speed_pid_.setLimits(0.0, 1.0);
    }

    bool initialize(const VehicleState& initial_state) {
        current_state_ = initial_state;
        last_update_time_ = getCurrentTimeMs();
        return true;
    }

    void updateState(const VehicleState& state) {
        current_state_ = state;
        last_update_time_ = state.timestamp_ms;

        // Check for lost comms
        uint64_t time_since_update = getCurrentTimeMs() - last_update_time_;
        if (LostCommsRecovery::shouldInitiateRecovery(
            time_since_update,
            config_.safety_limits.comms_timeout_ms))
        {
            setControlMode(ControlMode::EMERGENCY_SURFACE);
        }
    }

    bool setControlMode(ControlMode mode) {
        if (mode_callback_) {
            mode_callback_(current_mode_, mode);
        }
        current_mode_ = mode;
        return true;
    }

    ControlMode getControlMode() const {
        return current_mode_;
    }

    bool setSearchArea(const SearchArea& area) {
        search_area_ = area;

        // Generate waypoints based on search pattern
        waypoints_ = PathPlanner::generateLawnMower(area, area.track_spacing);

        return true;
    }

    void addWaypoint(const Waypoint& waypoint) {
        waypoints_.push_back(waypoint);
    }

    void clearWaypoints() {
        waypoints_.clear();
    }

    std::vector<double> computeControl() {
        std::vector<double> control = {0.0, 0.0, 0.0};  // [rudder, throttle, dive_plane]

        if (current_mode_ == ControlMode::MANUAL) {
            // No autonomous control in manual mode
            return control;
        }

        if (current_mode_ == ControlMode::EMERGENCY_SURFACE) {
            // Emergency surface behavior
            control[1] = 0.5;   // Half throttle
            control[2] = -1.0;  // Full up on dive planes
            return control;
        }

        if (waypoints_.empty()) {
            return control;
        }

        // Get current target waypoint
        Waypoint target = waypoints_.front();

        // Check if waypoint reached
        if (NavigationController::isWaypointReached(
            current_state_,
            target,
            config_.waypoint_tolerance))
        {
            waypoints_.erase(waypoints_.begin());
            if (!waypoints_.empty()) {
                target = waypoints_.front();
            }
        }

        // Compute desired heading
        double desired_heading = NavigationController::computeBearing(
            current_state_,
            target
        );

        // Obstacle avoidance
        if (config_.obstacle_avoidance_enabled && !obstacles_.empty()) {
            desired_heading = ObstacleAvoidance::computeAvoidanceHeading(
                current_state_,
                target,
                obstacles_
            );
        }

        // Compute control outputs using PID
        double dt = 0.1;  // 10 Hz control loop

        // Heading control
        double heading_error = normalizeAngle(desired_heading - current_state_.heading);
        control[0] = heading_pid_.compute(0.0, heading_error, dt);

        // Depth control
        control[2] = depth_pid_.compute(target.depth, current_state_.depth, dt);

        // Speed control
        double speed_error = target.speed - current_state_.speed;
        control[1] = speed_pid_.compute(target.speed, current_state_.speed, dt);

        // Apply safety limits
        applySafetyLimits(control);

        return control;
    }

    void emergencyStop() {
        setControlMode(ControlMode::EMERGENCY_SURFACE);
        clearWaypoints();
    }

    MissionPhase getMissionPhase() const {
        return current_phase_;
    }

    void addObstacle(const Obstacle& obstacle) {
        obstacles_.push_back(obstacle);
    }

    void clearObstacles() {
        obstacles_.clear();
    }

    Waypoint getCurrentTarget() const {
        if (waypoints_.empty()) {
            Waypoint wp;
            wp.latitude = current_state_.latitude;
            wp.longitude = current_state_.longitude;
            wp.depth = current_state_.depth;
            return wp;
        }
        return waypoints_.front();
    }

    double getTimeToWaypoint() const {
        if (waypoints_.empty()) return 0.0;

        Waypoint target = waypoints_.front();
        double distance = NavigationController::computeDistance(current_state_, target);

        if (current_state_.speed > 0.1) {
            return distance / (current_state_.speed * 0.5144);  // knots to m/s
        }

        return 0.0;
    }

    void registerStateCallback(StateCallback callback) {
        state_callback_ = callback;
    }

    void registerModeCallback(ModeCallback callback) {
        mode_callback_ = callback;
    }

private:
    AutonomyConfig config_;
    ControlMode current_mode_;
    MissionPhase current_phase_;
    VehicleState current_state_;
    uint64_t last_update_time_;

    SearchArea search_area_;
    std::vector<Waypoint> waypoints_;
    std::vector<Obstacle> obstacles_;

    PIDController heading_pid_;
    PIDController depth_pid_;
    PIDController speed_pid_;

    StateCallback state_callback_;
    ModeCallback mode_callback_;

    double normalizeAngle(double angle) {
        while (angle > 180.0) angle -= 360.0;
        while (angle < -180.0) angle += 360.0;
        return angle;
    }

    void applySafetyLimits(std::vector<double>& control) {
        // Clamp control outputs
        for (auto& val : control) {
            val = std::clamp(val, -1.0, 1.0);
        }

        // Check depth limits
        if (current_state_.depth > config_.safety_limits.max_depth) {
            control[2] = -1.0;  // Emergency surface
        }

        // Check altitude
        if (current_state_.altitude < config_.safety_limits.min_altitude) {
            control[2] = 1.0;  // Dive
        }
    }

    uint64_t getCurrentTimeMs() {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
    }
};

AutonomyController::AutonomyController(const AutonomyConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

AutonomyController::~AutonomyController() = default;

bool AutonomyController::initialize(const VehicleState& initial_state) {
    return impl_->initialize(initial_state);
}

void AutonomyController::updateState(const VehicleState& state) {
    impl_->updateState(state);
}

bool AutonomyController::setControlMode(ControlMode mode) {
    return impl_->setControlMode(mode);
}

ControlMode AutonomyController::getControlMode() const {
    return impl_->getControlMode();
}

bool AutonomyController::setSearchArea(const SearchArea& area) {
    return impl_->setSearchArea(area);
}

void AutonomyController::addWaypoint(const Waypoint& waypoint) {
    impl_->addWaypoint(waypoint);
}

void AutonomyController::clearWaypoints() {
    impl_->clearWaypoints();
}

std::vector<double> AutonomyController::computeControl() {
    return impl_->computeControl();
}

void AutonomyController::emergencyStop() {
    impl_->emergencyStop();
}

MissionPhase AutonomyController::getMissionPhase() const {
    return impl_->getMissionPhase();
}

void AutonomyController::addObstacle(const Obstacle& obstacle) {
    impl_->addObstacle(obstacle);
}

void AutonomyController::clearObstacles() {
    impl_->clearObstacles();
}

Waypoint AutonomyController::getCurrentTarget() const {
    return impl_->getCurrentTarget();
}

double AutonomyController::getTimeToWaypoint() const {
    return impl_->getTimeToWaypoint();
}

void AutonomyController::registerStateCallback(StateCallback callback) {
    impl_->registerStateCallback(callback);
}

void AutonomyController::registerModeCallback(ModeCallback callback) {
    impl_->registerModeCallback(callback);
}

// PIDController implementation
PIDController::PIDController(double kp, double ki, double kd)
    : kp_(kp), ki_(ki), kd_(kd)
    , integral_(0.0), prev_error_(0.0)
    , out_min_(-1.0), out_max_(1.0) {}

double PIDController::compute(double setpoint, double measured, double dt) {
    double error = setpoint - measured;

    integral_ += error * dt;
    double derivative = (error - prev_error_) / dt;

    double output = kp_ * error + ki_ * integral_ + kd_ * derivative;

    output = std::clamp(output, out_min_, out_max_);

    prev_error_ = error;

    return output;
}

void PIDController::reset() {
    integral_ = 0.0;
    prev_error_ = 0.0;
}

void PIDController::setLimits(double min, double max) {
    out_min_ = min;
    out_max_ = max;
}

// NavigationController implementation
double NavigationController::computeBearing(
    const VehicleState& current,
    const Waypoint& target)
{
    double lat1 = current.latitude * M_PI / 180.0;
    double lon1 = current.longitude * M_PI / 180.0;
    double lat2 = target.latitude * M_PI / 180.0;
    double lon2 = target.longitude * M_PI / 180.0;

    double dlon = lon2 - lon1;

    double y = std::sin(dlon) * std::cos(lat2);
    double x = std::cos(lat1) * std::sin(lat2) -
               std::sin(lat1) * std::cos(lat2) * std::cos(dlon);

    double bearing = std::atan2(y, x) * 180.0 / M_PI;

    // Normalize to 0-360
    if (bearing < 0) bearing += 360.0;

    return bearing;
}

double NavigationController::computeDistance(
    const VehicleState& current,
    const Waypoint& target)
{
    const double R = 6371000.0;  // Earth radius in meters

    double lat1 = current.latitude * M_PI / 180.0;
    double lat2 = target.latitude * M_PI / 180.0;
    double dlat = (target.latitude - current.latitude) * M_PI / 180.0;
    double dlon = (target.longitude - current.longitude) * M_PI / 180.0;

    double a = std::sin(dlat/2) * std::sin(dlat/2) +
               std::cos(lat1) * std::cos(lat2) *
               std::sin(dlon/2) * std::sin(dlon/2);

    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));

    return R * c;
}

bool NavigationController::isWaypointReached(
    const VehicleState& current,
    const Waypoint& target,
    double tolerance)
{
    double distance = computeDistance(current, target);
    return distance <= tolerance;
}

// PathPlanner implementation
std::vector<Waypoint> PathPlanner::generateLawnMower(
    const SearchArea& area,
    double track_spacing)
{
    std::vector<Waypoint> waypoints;

    if (area.boundary.size() < 3) return waypoints;

    // Simple lawn-mower pattern generation
    // Production would use more sophisticated algorithm

    double min_lat = area.boundary[0].latitude;
    double max_lat = area.boundary[0].latitude;
    double min_lon = area.boundary[0].longitude;
    double max_lon = area.boundary[0].longitude;

    for (const auto& pt : area.boundary) {
        min_lat = std::min(min_lat, pt.latitude);
        max_lat = std::max(max_lat, pt.latitude);
        min_lon = std::min(min_lon, pt.longitude);
        max_lon = std::max(max_lon, pt.longitude);
    }

    // Generate parallel tracks
    double lat_spacing = track_spacing / 111000.0;  // Approx degrees
    bool left_to_right = true;

    for (double lat = min_lat; lat <= max_lat; lat += lat_spacing) {
        Waypoint wp1, wp2;
        wp1.latitude = lat;
        wp1.longitude = left_to_right ? min_lon : max_lon;
        wp1.depth = (area.min_depth + area.max_depth) / 2.0;
        wp1.speed = 5.0;  // 5 knots

        wp2.latitude = lat;
        wp2.longitude = left_to_right ? max_lon : min_lon;
        wp2.depth = wp1.depth;
        wp2.speed = 5.0;

        waypoints.push_back(wp1);
        waypoints.push_back(wp2);

        left_to_right = !left_to_right;
    }

    return waypoints;
}

// ObstacleAvoidance implementation
double ObstacleAvoidance::computeAvoidanceHeading(
    const VehicleState& current,
    const Waypoint& target,
    const std::vector<Obstacle>& obstacles)
{
    // Potential field method
    double desired_heading = NavigationController::computeBearing(
        {current.latitude, current.longitude, 0, 0, 0, 0, 0},
        target
    );

    // Check for nearby obstacles
    for (const auto& obs : obstacles) {
        VehicleState obs_state;
        obs_state.latitude = obs.latitude;
        obs_state.longitude = obs.longitude;

        double dist = NavigationController::computeDistance(current, {obs.latitude, obs.longitude, 0, 0, 0, 0, ""});

        if (dist < obs.radius * 2.0) {
            // Obstacle too close - steer away
            double obs_bearing = NavigationController::computeBearing(current, {obs.latitude, obs.longitude, 0, 0, 0, 0, ""});

            // Add repulsive heading adjustment
            double avoidance_angle = 45.0;  // Steer 45 degrees away
            if (std::abs(desired_heading - obs_bearing) < 90.0) {
                desired_heading += avoidance_angle;
            }
        }
    }

    // Normalize
    while (desired_heading >= 360.0) desired_heading -= 360.0;
    while (desired_heading < 0.0) desired_heading += 360.0;

    return desired_heading;
}

bool ObstacleAvoidance::isCollision(
    const VehicleState& position,
    const std::vector<Obstacle>& obstacles)
{
    for (const auto& obs : obstacles) {
        double dist = NavigationController::computeDistance(
            position,
            {obs.latitude, obs.longitude, 0, 0, 0, 0, ""}
        );

        if (dist < obs.radius) {
            return true;
        }
    }

    return false;
}

// LostCommsRecovery implementation
bool LostCommsRecovery::shouldInitiateRecovery(
    uint64_t time_without_comms,
    uint64_t timeout_threshold)
{
    return time_without_comms >= timeout_threshold;
}

Waypoint LostCommsRecovery::executeRecovery(
    const VehicleState& current,
    const Waypoint& home_position,
    uint64_t time_without_comms)
{
    // Emergency surface at current location
    Waypoint recovery;
    recovery.latitude = current.latitude;
    recovery.longitude = current.longitude;
    recovery.depth = 0.0;  // Surface
    recovery.speed = 2.0;  // Slow speed
    recovery.id = "RECOVERY_SURFACE";

    return recovery;
}

} // namespace autonomy
} // namespace rmh
