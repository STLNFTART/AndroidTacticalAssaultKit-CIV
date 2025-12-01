# RMH Module Simulation Environment

This directory contains simulation and testing resources for the RMH Module.

## Simulation Tools

### 1. OMNeT++ Network Simulation

**Purpose**: Test crypto communications and network protocols

**Setup**:
```bash
# Install OMNeT++
wget https://github.com/omnetpp/omnetpp/releases/download/omnetpp-6.0.1/omnetpp-6.0.1-src.tgz
tar xzf omnetpp-6.0.1-src.tgz
cd omnetpp-6.0.1
. setenv
./configure && make
```

**Run**:
```bash
cd simulation/omnetpp
./run_comms_test.sh
```

### 2. Gazebo + ROS Robotics Simulation

**Purpose**: Test autonomy controller with realistic vehicle dynamics

**Setup**:
```bash
# Install ROS Noetic
sudo apt install ros-noetic-desktop-full
sudo apt install ros-noetic-gazebo-ros-pkgs

# Source ROS
source /opt/ros/noetic/setup.bash
```

**Run**:
```bash
cd simulation/gazebo
roslaunch rmh_simulation tb_autonomy.launch
```

### 3. NetLogo Agent-Based Simulation

**Purpose**: High-level mission planning and multi-vehicle coordination

**Setup**:
```bash
# Install NetLogo
wget https://ccl.northwestern.edu/netlogo/6.4.0/NetLogo-6.4.0-64.tgz
tar xzf NetLogo-6.4.0-64.tgz
cd NetLogo-6.4.0
./NetLogo
```

**Run**:
```bash
# Open simulation/netlogo/rmh_mission.nlogo in NetLogo GUI
```

## Test Scenarios

### Scenario 1: Basic Mine Detection
- **Location**: Panama City test range (simulated)
- **Objective**: Detect 5 mines in 1km² area
- **Success**: >90% detection rate, <10% false positives
- **Duration**: 2 hours simulated time

### Scenario 2: Crypto Communications Under Jamming
- **Location**: Network simulator
- **Objective**: Maintain comms with 50% channel jamming
- **Success**: <5% packet loss, crypto overhead <10%
- **Duration**: 30 minutes

### Scenario 3: Autonomous Search Pattern
- **Location**: Gazebo ocean environment
- **Objective**: Complete lawn-mower pattern with obstacles
- **Success**: 100% area coverage, no collisions
- **Duration**: 1 hour simulated time

### Scenario 4: Lost Comms Recovery
- **Location**: Gazebo
- **Objective**: Recover and surface when comms lost
- **Success**: Surface within 60 seconds
- **Duration**: 5 minutes

## Synthetic Data Generation

Generate synthetic sonar/mag data for testing:

```bash
cd simulation/data_gen
python3 generate_synthetic_data.py \
    --output test_dataset.bin \
    --duration 3600 \
    --mines 10 \
    --clutter 50
```

## Validation Metrics

Track these metrics during simulation:

- **Detection Performance**:
  - True positive rate (TP / (TP + FN))
  - False positive rate (FP / (FP + TN))
  - Precision (TP / (TP + FP))
  - F1 score

- **Crypto Performance**:
  - Encryption/decryption throughput (Mbps)
  - Key exchange latency (ms)
  - Authentication failure rate

- **Autonomy Performance**:
  - Waypoint arrival accuracy (meters)
  - Search area coverage (%)
  - Collision avoidance success rate
  - Energy efficiency (J/km)

## Continuous Integration

Automated simulation tests run on every commit:

```bash
# Run full test suite
cd simulation
./run_all_tests.sh
```

This generates a test report in `simulation/results/`.

## Visualization

View simulation results:

```bash
# Start visualization server
cd simulation/viz
python3 -m http.server 8000

# Open browser to http://localhost:8000
```

Displays:
- Tow Body track
- Detected contacts
- Search coverage heatmap
- Performance metrics

---

**Classification**: UNCLASSIFIED
**Last Updated**: 2025-11-23
