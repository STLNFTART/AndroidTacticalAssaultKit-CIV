# RMH Module - Build & Integration Guide

## Quick Start

### 1. Clone Repository
```bash
git clone https://github.com/STLNFTART/AndroidTacticalAssaultKit-CIV.git
cd AndroidTacticalAssaultKit-CIV/rmh-module
```

### 2. Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    python3 \
    python3-pip \
    git
```

**Python (for simulation):**
```bash
cd simulation
pip3 install -r requirements.txt
```

### 3. Build C++ Components

**Option A: Standalone Build** (for testing)
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

**Option B: Integration with ATAK Build System**
```bash
# From ATAK root directory
./gradlew :rmh-module:build
```

### 4. Run Tests

```bash
cd build
ctest --verbose
```

### 5. Run Simulation

```bash
cd simulation
python3 primal_logic_sim.py
```

Expected output:
- `primal_logic_results.png`: Visualization of stealth performance
- `metrics.json`: Raw data for analysis
- Console: Real-time progress and summary

---

## Component Details

### Primal Logic Kernel

**Files:**
- `signals/primal_logic_kernel.h`
- `signals/primal_logic_kernel.cpp`

**Build:**
```bash
g++ -std=c++17 -O3 -c signals/primal_logic_kernel.cpp -o primal_logic_kernel.o
```

**Dependencies:**
- C++17 compiler
- Standard library (math, vector, memory)

**API Example:**
```cpp
#include "signals/primal_logic_kernel.h"

// Configure kernel
rmh::stealth::PrimalLogicConfig config;
config.lambda_ = 0.1;
config.theta_base = 1.0;

// Create stealth management system
rmh::stealth::StealthManagementSystem stealth;
stealth.initialize(config);

// Update in control loop
double timestamp = get_current_time();
double rcs_control = stealth.update_rcs(measured_rcs, target_rcs, timestamp);
double thermal_control = stealth.update_thermal(measured_temp, target_temp, timestamp);
// ... etc for other domains

// Check stability
if (stealth.all_domains_stable()) {
    // Safe to operate
}
```

### Signal Processing Engine

**Files:**
- `signals/signal_processor.h`
- `signals/signal_processor.cpp`

**Dependencies:**
- Primal Logic kernel
- Standard library

**API Example:**
```cpp
#include "signals/signal_processor.h"

// Configure signal processor
rmh::signals::SignalProcessorConfig config;
config.min_snr_db = 10.0;
config.min_detections_for_confirmation = 3;

// Create processor
rmh::signals::SignalProcessor processor(config);
processor.initialize();

// Process sensor data
rmh::signals::SensorData data;
data.sensor_type = rmh::signals::SensorType::SIDE_SCAN_SONAR;
data.signal_strength = 0.85;
data.range_m = 45.0;
data.bearing_rad = 0.5;
data.quality = 0.9;

processor.process_sensor_data(data, timestamp);

// Get mine tracks
auto tracks = processor.get_mine_tracks();
for (const auto& track : tracks) {
    if (track.confirmed) {
        // Report confirmed mine contact
        report_mine(track.contact);
    }
}
```

### Crypto-Secure Communications

**Files:**
- `crypto/secure_comms.h`
- (Implementation stub - integrate with CommonCommo)

**Dependencies:**
- OpenSSL (libssl, libcrypto)
- ATAK CommonCommo (optional)

**Build with OpenSSL:**
```bash
g++ -std=c++17 -O3 \
    -I/usr/include/openssl \
    -c crypto/secure_comms.cpp \
    -o secure_comms.o
```

**Link:**
```bash
g++ -o rmh_module \
    primal_logic_kernel.o \
    signal_processor.o \
    secure_comms.o \
    -lssl -lcrypto -lpthread
```

**API Example:**
```cpp
#include "crypto/secure_comms.h"

// Configure crypto
rmh::crypto::CryptoConfig config;
config.encryption = rmh::crypto::EncryptionAlgorithm::AES_256_GCM;
config.key_exchange = rmh::crypto::KeyExchangeProtocol::ECDH_P256;

// Create secure channel
rmh::crypto::SecureChannel channel(config);
channel.initialize(true);  // true = initiator

// Key exchange (once at startup)
std::vector<uint8_t> peer_pubkey = receive_peer_key();
std::vector<uint8_t> our_pubkey = channel.key_exchange(peer_pubkey);
send_to_peer(our_pubkey);

// Send encrypted message
std::vector<uint8_t> payload = create_telemetry_data();
rmh::crypto::SecureMessage encrypted_msg;
if (channel.send_message(rmh::crypto::MessageType::TELEMETRY, payload, encrypted_msg)) {
    send_over_network(encrypted_msg);
}

// Receive encrypted message
rmh::crypto::SecureMessage received_msg = receive_from_network();
std::vector<uint8_t> decrypted_payload;
bool verified;
if (channel.receive_message(received_msg, decrypted_payload, verified)) {
    if (verified) {
        process_telemetry(decrypted_payload);
    }
}
```

---

## Integration with ATAK

### Plugin Structure

```
atak/ATAK/app/src/main/java/com/atakmap/android/rmh/
├── RMHMapComponent.java         # Main plugin entry
├── RMHDropDownReceiver.java     # UI handler
├── TowBodyTelemetryManager.java # TB telemetry processing
├── MineContactManager.java      # Mine track visualization
├── StealthStatusWidget.java     # Stealth status display
└── SecureCommsManager.java      # Crypto comms interface
```

### Building ATAK Plugin

```bash
cd atak/ATAK
./gradlew assembleDebug
```

### Installing Plugin

```bash
adb install -r app/build/outputs/apk/debug/atak-rmh-debug.apk
```

---

## Testing

### Unit Tests

**C++ Tests** (using Catch2 or Google Test):
```bash
cd build
./test_primal_logic_kernel
./test_signal_processor
./test_secure_comms
```

### Integration Tests

**Python Integration Test:**
```bash
cd simulation
python3 -m pytest test_integration.py
```

### Simulation Validation

```bash
cd simulation
python3 primal_logic_sim.py

# Check results
ls -lh primal_logic_results.png metrics.json
```

---

## Performance Tuning

### Primal Logic Kernel

**Tune λ (decay coefficient):**
- **Higher λ** (e.g., 0.5): Faster response, less smoothing
- **Lower λ** (e.g., 0.05): Slower response, more smoothing
- **Recommended**: 0.1 - 0.2 for most scenarios

**Tune Θ (adaptive weighting):**
- **Higher θ_gain**: More aggressive adaptation to errors
- **Lower θ_gain**: Smoother, more conservative
- **Recommended**: 0.5 - 1.0

**Example:**
```cpp
config.lambda = 0.15;           // Moderate decay
config.theta_base = 1.0;        // Neutral base
config.theta_adaptive_gain = 0.8;  // Moderately aggressive
```

### Signal Processing

**Detection Threshold:**
```cpp
config.min_snr_db = 10.0;  // Lower = more detections (+ more false alarms)
```

**Track Confirmation:**
```cpp
config.min_detections_for_confirmation = 3;  // Lower = faster tracks
config.max_coast_count = 5;  // Higher = more persistent tracks
```

---

## Deployment

### Production Build (Optimized)

```bash
mkdir build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
strip rmh_module  # Remove debug symbols
```

### Hardware Deployment

1. **Cross-compile** for target TB hardware (e.g., ARM):
```bash
export CC=arm-linux-gnueabihf-gcc
export CXX=arm-linux-gnueabihf-g++
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm.cmake ..
make -j$(nproc)
```

2. **Package** for deployment:
```bash
tar -czf rmh-module-v1.0.tar.gz \
    rmh_module \
    config/ \
    scripts/ \
    README.md
```

3. **Deploy** to Tow Body:
```bash
scp rmh-module-v1.0.tar.gz user@towbody:/opt/rmh/
ssh user@towbody
cd /opt/rmh
tar -xzf rmh-module-v1.0.tar.gz
```

---

## Troubleshooting

### OpenSSL Not Found
```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# macOS
brew install openssl
export OPENSSL_ROOT_DIR=/usr/local/opt/openssl
```

### Python Dependencies
```bash
pip3 install --upgrade pip
pip3 install numpy matplotlib scipy
```

### CMake Version Too Old
```bash
# Download latest CMake
wget https://github.com/Kitware/CMake/releases/download/v3.28.0/cmake-3.28.0-linux-x86_64.sh
chmod +x cmake-3.28.0-linux-x86_64.sh
sudo ./cmake-3.28.0-linux-x86_64.sh --prefix=/usr/local --skip-license
```

---

## Support

**Issues**: https://github.com/STLNFTART/AndroidTacticalAssaultKit-CIV/issues
**Documentation**: `rmh-module/docs/`
**Contact**: [Your Contact Info]

---

**Last Updated**: 2025-11-23
**Version**: 1.0
