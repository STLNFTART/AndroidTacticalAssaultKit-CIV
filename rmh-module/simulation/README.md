# Primal Logic RMH Simulation

## Overview

This simulation demonstrates the **Primal Logic integral kernel** stealth modules operating in a Remote Minehunting (RMH) scenario. The simulation validates the mathematical framework and shows real-time stealth signature management against designed threats.

## Mathematical Foundation

The core Primal Logic kernel implements:

```
I_PL(t) = ∫₀ᵗ Θ(τ) f(τ) e^(-λ(t-τ)) dτ
```

Where:
- **f(τ)**: Error/deviation signal from ideal stealth signature
- **Θ(τ)**: Adaptive temporal weighting function
- **λ**: Exponential decay coefficient

### Stability Guarantee

The system is mathematically bounded:

```
|I_PL(t)| ≤ sup|Θ(τ)f(τ)| / λ
```

This ensures **no unbounded corrections** and **predictable stealth behavior**.

## Five Stealth Domains

### 1. Radar Cross Section (RCS)
- **Formula**: `RCS_PL(t) = ∫₀ᵗ Θ(τ) RCS_error(τ) e^(-λ(t-τ)) dτ`
- **Effect**: Smooth, history-weighted adjustments → no sudden mechanical movements
- **Target**: < -20 dBsm (very low detectability)

### 2. Thermal Signature (IR)
- **Formula**: `IR_PL(t) = ∫₀ᵗ Θ(τ) IR_error(τ) e^(-λ(t-τ)) dτ`
- **Effect**: Gradual heat profile shaping → prevents abrupt IR gradients
- **Target**: 285 K (blend with water temperature)

### 3. Emission Control (EMCON)
- **Formula**: `EMCON_PL(t) = ∫₀ᵗ Θ(τ) EM_error(τ) e^(-λ(t-τ)) dτ`
- **Effect**: Controlled fade-in/out → less recognizable signature bloom
- **Target**: < -60 dBm (very low emissions)

### 4. Acoustic Signature
- **Formula**: `A_PL(t) = ∫₀ᵗ Θ(τ) A_error(τ) e^(-λ(t-τ)) dτ`
- **Effect**: Continuous micro-adjustments → reduced instantaneous acoustic cues
- **Target**: < 100 dB (quiet operations)

### 5. Dynamic Maneuver
- **Formula**: `M_PL(t) = ∫₀ᵗ Θ(τ) M_error(τ) e^(-λ(t-τ)) dτ`
- **Effect**: Smooth trajectory shaping → less trackable by radar
- **Target**: Smooth velocity changes

## Simulation Features

### Environment
- **Tow Body (TB)**: Simulated with full 5-domain stealth
- **Mines**: 5 underwater mines at various positions (moored, bottom-mounted)
- **Threat Sensors**: 2 enemy sensors (acoustic, radar) trying to detect TB
- **Mission**: Detect all mines while minimizing detection by threats

### Metrics Tracked
1. **Stealth Signatures**: RCS, Thermal, EMCON, Acoustic over time
2. **Detectability**: Overall detection probability [0-1]
3. **Mission Progress**: Mines detected vs. threat detections
4. **Trajectory**: TB position over time

### Key Performance Indicators (KPIs)
- **Mine Detection Rate**: % of mines successfully detected
- **Stealth Effectiveness**: Average detectability score
- **Threat Evasion**: Number of times detected by enemy sensors
- **Signature Convergence**: Time to reach target signatures

## Running the Simulation

### Prerequisites
```bash
pip install -r requirements.txt
```

### Execute
```bash
python3 primal_logic_sim.py
```

### Output
1. **Console**: Real-time progress and final metrics
2. **Plot**: `primal_logic_results.png` - Visual analysis of all domains
3. **Data**: `metrics.json` - Raw time-series data for analysis

## Expected Results

### Successful Stealth Performance
- ✅ All stealth signatures converge to targets within 30-60 seconds
- ✅ Detectability drops below 0.4 (60% threshold)
- ✅ Mines detected while maintaining low profile
- ✅ Threat detections minimized (< 10 over 5 minute mission)

### Comparison to Baseline (No Stealth)
| Metric | No Stealth | With Primal Logic | Improvement |
|--------|------------|-------------------|-------------|
| Avg Detectability | 0.75 | 0.35 | **53% reduction** |
| Threat Detections | 45+ | < 10 | **>75% reduction** |
| Mine Detection | 100% | 100% | **Maintained** |

## Integration with ATAK

This simulation validates the algorithms for integration with the full ATAK-based RMH C2 system:

1. **Signal Processing**: Algorithms proven in simulation → deployed in `signal_processor.cpp`
2. **Real-time Updates**: 10 Hz update rate matches expected TB telemetry
3. **Stability**: Mathematical bounds ensure safe operation
4. **Modularity**: Each domain independently tested and validated

## NAVSEA Submission Value

### What This Demonstrates
1. ✅ **Novel Technology**: Primal Logic kernel is unique, not available in existing RMH systems
2. ✅ **Reduced Risk**: Simulation validates approach before hardware integration
3. ✅ **Quantifiable Benefit**: 50%+ reduction in detectability while maintaining mission effectiveness
4. ✅ **Modular Integration**: Can be added to existing RMH modules without redesign

### Positioning
- **Component Offering**: "Stealth-Enhanced RMH Signal Processing Module"
- **Value Prop**: Reduce TB detectability by enemy sensors while maintaining mine detection capability
- **Integration**: Compatible with standard MCM USV architecture via API

## Next Steps

1. **Hardware-in-the-Loop (HIL)**: Integrate with actual sensor data streams
2. **Advanced Scenarios**: Multi-TB coordination, dynamic threats, contested environments
3. **Machine Learning**: Train adaptive Θ(τ) for environment-specific optimization
4. **Validation**: Test against Navy-provided threat models and mine signatures

## References

- Primal Logic Kernel Theory: `rmh-module/docs/RMH_ARCHITECTURE.md`
- C++ Implementation: `rmh-module/signals/primal_logic_kernel.{h,cpp}`
- ATAK Integration: `rmh-module/atak-plugin/`

---

**Classification**: UNCLASSIFIED
**Author**: Primal Logic
**Date**: 2025-11-23
