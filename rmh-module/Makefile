# RMH Module - Convenience Makefile
# Wraps CMake build system for common tasks

.PHONY: all build clean install test examples docs help

# Default target
all: build

# Build everything
build:
	@echo "Building RMH Module..."
	@mkdir -p build
	@cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$$(nproc)
	@echo "Build complete!"

# Debug build
debug:
	@echo "Building RMH Module (Debug)..."
	@mkdir -p build
	@cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j$$(nproc)

# Clean build directory
clean:
	@echo "Cleaning build directory..."
	@rm -rf build
	@echo "Clean complete!"

# Install to system
install: build
	@echo "Installing RMH Module..."
	@cd build && sudo make install
	@echo "Installation complete!"

# Uninstall from system
uninstall:
	@echo "Uninstalling RMH Module..."
	@cd build && sudo make uninstall
	@echo "Uninstall complete!"

# Run tests
test: build
	@echo "Running tests..."
	@cd build && ctest --output-on-failure
	@echo "Tests complete!"

# Build examples
examples: build
	@echo "Building examples..."
	@cd build && make example_signal_processing example_crypto_comms example_autonomy
	@echo "Examples built!"

# Run examples
run-examples: examples
	@echo "=== Running Signal Processing Example ==="
	@./build/examples/example_signal_processing
	@echo ""
	@echo "=== Running Crypto Comms Example ==="
	@./build/examples/example_crypto_comms
	@echo ""
	@echo "=== Running Autonomy Example ==="
	@./build/examples/example_autonomy

# Generate documentation
docs:
	@echo "Generating documentation..."
	@doxygen Doxyfile
	@echo "Documentation generated in docs/html/"

# Build ATAK plugin
atak-plugin:
	@echo "Building ATAK plugin..."
	@cd atak-plugin && ./gradlew assembleDebug
	@echo "ATAK plugin built: atak-plugin/build/outputs/apk/debug/rmh-plugin-debug.apk"

# Install ATAK plugin to connected device
atak-install: atak-plugin
	@echo "Installing ATAK plugin to device..."
	@adb install -r atak-plugin/build/outputs/apk/debug/rmh-plugin-debug.apk
	@echo "ATAK plugin installed!"

# Package for distribution
package: build
	@echo "Creating distribution package..."
	@cd build && make package
	@echo "Package created!"

# Format code
format:
	@echo "Formatting code..."
	@find signals crypto autonomy -name "*.cpp" -o -name "*.h" | xargs clang-format -i
	@echo "Code formatted!"

# Static analysis
analyze:
	@echo "Running static analysis..."
	@mkdir -p build
	@cd build && scan-build cmake .. && scan-build make
	@echo "Analysis complete!"

# Check for security vulnerabilities
security-check:
	@echo "Checking for security vulnerabilities..."
	@cppcheck --enable=all --inconclusive signals/ crypto/ autonomy/
	@echo "Security check complete!"

# Help target
help:
	@echo "RMH Module - Available targets:"
	@echo ""
	@echo "  make build          - Build all C++ modules (default)"
	@echo "  make debug          - Build with debug symbols"
	@echo "  make clean          - Remove build directory"
	@echo "  make install        - Install to system (/usr/local)"
	@echo "  make uninstall      - Remove from system"
	@echo "  make test           - Run unit tests"
	@echo "  make examples       - Build example programs"
	@echo "  make run-examples   - Build and run all examples"
	@echo "  make docs           - Generate API documentation"
	@echo "  make atak-plugin    - Build ATAK Android plugin"
	@echo "  make atak-install   - Install ATAK plugin to device"
	@echo "  make package        - Create distribution package"
	@echo "  make format         - Format source code"
	@echo "  make analyze        - Run static analysis"
	@echo "  make security-check - Check for vulnerabilities"
	@echo "  make help           - Show this help message"
	@echo ""
	@echo "For detailed build instructions, see BUILD_INSTRUCTIONS.md"
