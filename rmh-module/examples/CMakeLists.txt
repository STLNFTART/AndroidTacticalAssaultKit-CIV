cmake_minimum_required(VERSION 3.15)

# Signal Processing Example
add_executable(example_signal_processing
    example_signal_processing.cpp
)
target_link_libraries(example_signal_processing
    PRIVATE rmh_signals
)

# Crypto Communications Example
add_executable(example_crypto_comms
    example_crypto_comms.cpp
)
target_link_libraries(example_crypto_comms
    PRIVATE rmh_crypto
)

# Autonomy Controller Example
add_executable(example_autonomy
    example_autonomy.cpp
)
target_link_libraries(example_autonomy
    PRIVATE rmh_autonomy
)

# Full System Integration Example
add_executable(example_full_system
    example_full_system.cpp
)
target_link_libraries(example_full_system
    PRIVATE rmh_module
)

install(TARGETS
    example_signal_processing
    example_crypto_comms
    example_autonomy
    example_full_system
    RUNTIME DESTINATION bin/examples
)
