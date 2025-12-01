# RMH Module - Build Instructions

**Classification**: UNCLASSIFIED
**Version**: 1.0.0
**Last Updated**: 2025-11-23

---

## Prerequisites

### System Requirements
- **OS**: Ubuntu 20.04+ or Debian 11+
- **CPU**: x86_64 (Intel/AMD) with AVX2 support
- **RAM**: Minimum 8GB (16GB recommended)
- **Disk**: 10GB free space

### Required Software

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Build tools
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    ninja-build

# C++ compiler (GCC 9+ or Clang 10+)
sudo apt install -y \
    gcc-10 g++-10

# OpenSSL
sudo apt install -y \
    libssl-dev

# Optional: OpenCV
sudo apt install -y \
    libopencv-dev

# Optional: TensorFlow Lite
# Download from https://www.tensorflow.org/lite
```

---

## Building C++ Modules

### Quick Build

```bash
cd rmh-module
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### Build Options

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build without tests
cmake -DBUILD_TESTS=OFF ..

# Build without examples
cmake -DBUILD_EXAMPLES=OFF ..

# Enable GPU acceleration (requires CUDA)
cmake -DENABLE_GPU=ON ..

# Custom install prefix
cmake -DCMAKE_INSTALL_PREFIX=/opt/rmh ..
```

### Build Targets

```bash
# Build all libraries
make rmh_signals rmh_crypto rmh_autonomy rmh_module

# Build specific library
make rmh_signals

# Build examples
make example_signal_processing
make example_crypto_comms
make example_autonomy

# Build and run tests
make test

# Install
sudo make install
```

---

## Building ATAK Plugin

### Prerequisites

```bash
# Java Development Kit 8
sudo apt install -y openjdk-8-jdk

# Android SDK (download from https://developer.android.com/studio)
export ANDROID_HOME=$HOME/Android/Sdk
export PATH=$PATH:$ANDROID_HOME/tools:$ANDROID_HOME/platform-tools

# ATAK SDK (obtain from TAK.gov)
# Place ATAK.jar in rmh-module/atak-plugin/libs/
```

### Build Plugin

```bash
cd rmh-module/atak-plugin

# Debug build
./gradlew assembleDebug

# Release build (requires signing key)
./gradlew assembleRelease

# Output: build/outputs/apk/debug/rmh-plugin-debug.apk
```

### Install on Device

```bash
# Install via ADB
adb install -r build/outputs/apk/debug/rmh-plugin-debug.apk

# Or copy to device and install via ATAK plugin manager
```

---

## Running Examples

### Signal Processing Example

```bash
./build/examples/example_signal_processing
```

Expected output:
```
=== RMH Signal Processing Example ===
Initializing signal processor...
  Detection threshold: 5 dB
  Classification threshold: 0.7

Test 1: Processing signal WITHOUT mine...
  Contacts detected: 0

Test 2: Processing signal WITH mine...
  Contacts detected: 1
    Contact ID: 1
    Classification: Mine-Like Object
    Confidence: 95%
    Location: 37.85, -122.45
```

### Crypto Communications Example

```bash
./build/examples/example_crypto_comms
```

### Autonomy Controller Example

```bash
./build/examples/example_autonomy
```

---

## Running Tests

### Unit Tests

```bash
cd build
ctest --output-on-failure

# Run specific test
ctest -R signal_processor_test

# Verbose output
ctest -V
```

### Integration Tests

```bash
# Run full integration test suite
cd build
./tests/integration_test_all
```

---

## Configuration

### System Configuration

Edit `/etc/rmh/rmh_config.yaml`:

```yaml
system:
  node_id: 1
  classification: "UNCLASSIFIED"

signal_processing:
  detection_threshold_db: 5.0
  classification_threshold: 0.70

crypto:
  bind_address: "0.0.0.0:5555"
  key_rotation_interval_min: 15

autonomy:
  control_mode: "SEMI_AUTO"
  waypoint_tolerance_m: 5.0
```

Or use the provided template:

```bash
sudo mkdir -p /etc/rmh
sudo cp config/rmh_config.yaml /etc/rmh/
sudo chmod 644 /etc/rmh/rmh_config.yaml
```

---

## Troubleshooting

### OpenSSL Not Found

```bash
# Install OpenSSL development files
sudo apt install libssl-dev

# Specify OpenSSL path
cmake -DOPENSSL_ROOT_DIR=/usr/local/ssl ..
```

### TensorFlow Lite Not Found

```bash
# Download TensorFlow Lite
wget https://github.com/tensorflow/tensorflow/releases/download/v2.x/tensorflow-lite-2.x-linux-x86_64.tar.gz
tar xzf tensorflow-lite-2.x-linux-x86_64.tar.gz
sudo cp -r include/tensorflow /usr/local/include/
sudo cp lib/libtensorflowlite.so /usr/local/lib/

# Update library cache
sudo ldconfig
```

### Build Errors

```bash
# Clean build directory
rm -rf build/*
cd build
cmake ..
make clean
make -j$(nproc)
```

### Runtime Errors

```bash
# Check library path
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Verify installation
ldd build/examples/example_signal_processing
```

---

## Cross-Compilation (for Embedded Systems)

### For ARM64 (e.g., NVIDIA Jetson)

```bash
# Install cross-compiler
sudo apt install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Create toolchain file
cat > arm64-toolchain.cmake <<EOF
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
EOF

# Configure for ARM64
cmake -DCMAKE_TOOLCHAIN_FILE=arm64-toolchain.cmake ..
make -j$(nproc)
```

---

## Performance Optimization

### Enable Compiler Optimizations

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native" \
      ..
```

### Link-Time Optimization (LTO)

```bash
cmake -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ..
```

### Profile-Guided Optimization (PGO)

```bash
# Step 1: Build with profiling
cmake -DCMAKE_CXX_FLAGS="-fprofile-generate" ..
make

# Step 2: Run representative workload
./build/examples/example_signal_processing

# Step 3: Rebuild with profile data
cmake -DCMAKE_CXX_FLAGS="-fprofile-use" ..
make
```

---

## Packaging

### Create Debian Package

```bash
cd rmh-module
mkdir -p build/package
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make package
```

Output: `rmh-module-1.0.0-Linux.deb`

### Install Package

```bash
sudo dpkg -i rmh-module-1.0.0-Linux.deb
```

---

## Docker Build (Alternative)

```bash
# Build Docker image
docker build -t rmh-module:1.0.0 .

# Run example in container
docker run --rm rmh-module:1.0.0 /usr/local/bin/example_signal_processing
```

---

## Continuous Integration

GitHub Actions workflow (`.github/workflows/build.yml`):

```yaml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install Dependencies
        run: |
          sudo apt update
          sudo apt install -y build-essential cmake libssl-dev
      - name: Build
        run: |
          mkdir build && cd build
          cmake ..
          make -j$(nproc)
      - name: Test
        run: |
          cd build
          ctest --output-on-failure
```

---

## Support

For build issues, contact:
- **Technical Lead**: [Your Name]
- **Email**: [Your Email]
- **GitHub Issues**: https://github.com/STLNFTART/AndroidTacticalAssaultKit-CIV/issues

---

**Classification**: UNCLASSIFIED
**Distribution**: Approved for NAVSEA and authorized personnel
