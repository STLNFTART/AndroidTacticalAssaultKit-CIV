/**
 * @file example_autonomy.cpp
 * @brief Example demonstrating RMH autonomy controller
 */

#include "autonomy_controller.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace rmh::autonomy;

void printState(const VehicleState& state) {
    std::cout << "Vehicle State:" << std::endl;
    std::cout << "  Position: " << state.latitude << ", " << state.longitude << std::endl;
    std::cout << "  Depth: " << state.depth << " m" << std::endl;
    std::cout << "  Heading: " << state.heading << " deg" << std::endl;
    std::cout << "  Speed: " << state.speed << " knots" << std::endl;
}

void printControl(const std::vector<double>& control) {
    std::cout << "Control Output:" << std::endl;
    std::cout << "  Rudder: " << (control[0] * 100.0) << "%" << std::endl;
    std::cout << "  Throttle: " << (control[1] * 100.0) << "%" << std::endl;
    std::cout << "  Dive Plane: " << (control[2] * 100.0) << "%" << std::endl;
}

int main() {
    std::cout << "=== RMH Autonomy Controller Example ===" << std::endl;
    std::cout << std::endl;

    // Configure autonomy controller
    AutonomyConfig config;
    config.initial_mode = ControlMode::SEMI_AUTO;
    config.search_pattern = SearchPattern::LAWN_MOWER;
    config.obstacle_avoidance_enabled = true;
    config.waypoint_tolerance = 5.0;  // 5 meters

    config.safety_limits.max_depth = 100.0;
    config.safety_limits.min_altitude = 5.0;
    config.safety_limits.max_speed = 8.0;
    config.safety_limits.geofence_radius = 500.0;
    config.safety_limits.comms_timeout_ms = 60000;  // 60 seconds

    std::cout << "Initializing autonomy controller..." << std::endl;
    std::cout << "  Mode: Semi-Autonomous" << std::endl;
    std::cout << "  Search pattern: Lawn Mower" << std::endl;
    std::cout << "  Obstacle avoidance: Enabled" << std::endl;
    std::cout << std::endl;

    AutonomyController controller(config);

    // Set initial vehicle state
    VehicleState initial_state;
    initial_state.latitude = 37.8;
    initial_state.longitude = -122.4;
    initial_state.depth = 10.0;
    initial_state.altitude = 8.0;
    initial_state.heading = 0.0;  // North
    initial_state.speed = 0.0;
    initial_state.pitch = 0.0;
    initial_state.roll = 0.0;
    initial_state.timestamp_ms = 1000;

    controller.initialize(initial_state);

    std::cout << "Initial State:" << std::endl;
    printState(initial_state);
    std::cout << std::endl;

    // Define search area
    std::cout << "Defining search area..." << std::endl;
    SearchArea search_area;
    search_area.boundary = {
        {37.8, -122.4, 0, 0, 0, 0, "SW"},
        {37.81, -122.4, 0, 0, 0, 0, "NW"},
        {37.81, -122.39, 0, 0, 0, 0, "NE"},
        {37.8, -122.39, 0, 0, 0, 0, "SE"}
    };
    search_area.min_depth = 5.0;
    search_area.max_depth = 15.0;
    search_area.pattern = SearchPattern::LAWN_MOWER;
    search_area.track_spacing = 50.0;  // 50 meters

    controller.setSearchArea(search_area);
    std::cout << "  Boundary: 4 waypoints" << std::endl;
    std::cout << "  Track spacing: " << search_area.track_spacing << " m" << std::endl;
    std::cout << std::endl;

    // Add obstacle
    std::cout << "Adding obstacle..." << std::endl;
    Obstacle obs;
    obs.latitude = 37.805;
    obs.longitude = -122.395;
    obs.radius = 20.0;  // 20 meter avoidance radius
    obs.depth_min = 0.0;
    obs.depth_max = 50.0;
    obs.type = "rock_formation";

    controller.addObstacle(obs);
    std::cout << "  Location: " << obs.latitude << ", " << obs.longitude << std::endl;
    std::cout << "  Avoidance radius: " << obs.radius << " m" << std::endl;
    std::cout << std::endl;

    // Simulate mission
    std::cout << "Starting mission simulation..." << std::endl;
    std::cout << "Running 10 control loops..." << std::endl;
    std::cout << std::endl;

    VehicleState current_state = initial_state;

    for (int i = 0; i < 10; ++i) {
        std::cout << "--- Control Loop " << (i+1) << " ---" << std::endl;

        // Update state
        controller.updateState(current_state);

        // Compute control
        auto control = controller.computeControl();

        printControl(control);
        std::cout << std::endl;

        // Get current target
        Waypoint target = controller.getCurrentTarget();
        std::cout << "Current Target:" << std::endl;
        std::cout << "  Position: " << target.latitude << ", " << target.longitude << std::endl;
        std::cout << "  Depth: " << target.depth << " m" << std::endl;
        std::cout << "  Speed: " << target.speed << " knots" << std::endl;
        std::cout << std::endl;

        double time_to_wp = controller.getTimeToWaypoint();
        std::cout << "  Time to waypoint: " << time_to_wp << " seconds" << std::endl;
        std::cout << std::endl;

        // Simulate vehicle dynamics (very simplified)
        current_state.heading += control[0] * 5.0;  // Rudder affects heading
        current_state.depth += control[2] * 0.5;    // Dive plane affects depth
        current_state.speed = control[1] * config.safety_limits.max_speed;

        // Move vehicle (simplified kinematics)
        double heading_rad = current_state.heading * M_PI / 180.0;
        double distance = current_state.speed * 0.5144 * 0.1;  // knots to m/s, 0.1s timestep
        current_state.latitude += (distance / 111000.0) * std::cos(heading_rad);
        current_state.longitude += (distance / (111000.0 * std::cos(current_state.latitude * M_PI / 180.0))) * std::sin(heading_rad);

        current_state.timestamp_ms += 100;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Final State:" << std::endl;
    printState(current_state);
    std::cout << std::endl;

    // Test emergency stop
    std::cout << "Testing emergency stop..." << std::endl;
    controller.emergencyStop();
    std::cout << "  Mode: " << static_cast<int>(controller.getControlMode()) << " (Emergency Surface)" << std::endl;

    auto emergency_control = controller.computeControl();
    printControl(emergency_control);
    std::cout << std::endl;

    std::cout << "=== Example Complete ===" << std::endl;

    return 0;
}
