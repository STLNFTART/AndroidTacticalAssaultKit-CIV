#!/usr/bin/env python3
"""
Primal Logic Kernel Simulation for RMH Testing

Demonstrates Primal Logic stealth modules operating against designed threats
in a Remote Minehunting scenario.

Features:
- Five-domain stealth signature management (RCS, IR, EMCON, Acoustic, Maneuver)
- Mine detection and classification with stealth-enabled Tow Body
- Virtual threat environment with enemy sensors
- Real-time visualization and metrics

Author: Primal Logic
Date: 2025-11-23
Classification: UNCLASSIFIED
"""

import numpy as np
import matplotlib.pyplot as plt
from dataclasses import dataclass
from typing import List, Tuple, Optional
from enum import Enum
import json


class StealthDomain(Enum):
    """Stealth domain types"""
    RADAR_RCS = "radar_rcs"
    THERMAL_IR = "thermal_ir"
    EMCON = "emcon"
    ACOUSTIC = "acoustic"
    MANEUVER = "maneuver"


@dataclass
class PrimalLogicConfig:
    """Configuration for Primal Logic kernel"""
    lambda_: float = 0.1          # Exponential decay coefficient
    theta_base: float = 1.0        # Base adaptive weighting
    theta_adaptive_gain: float = 0.5  # Adaptive gain
    sampling_rate_hz: float = 100.0   # Sampling rate
    stability_bound: float = 10.0     # Maximum correction magnitude
    enable_adaptive: bool = True      # Enable adaptive weighting


class PrimalLogicKernel:
    """
    Primal Logic Integral Kernel

    Implements: I_PL(t) = ∫₀ᵗ Θ(τ) f(τ) e^(-λ(t-τ)) dτ

    Using discrete-time exponential smoothing:
    I_PL[n] = I_PL[n-1] * e^(-λ*dt) + Θ[n] * f[n] * dt
    """

    def __init__(self, config: PrimalLogicConfig):
        self.config = config
        self.output = 0.0
        self.last_timestamp = 0.0
        self.history_error = []
        self.history_time = []

    def update(self, error_signal: float, timestamp_sec: float) -> float:
        """Update kernel with new error signal"""
        if not self.history_time:
            # First update
            self.output = 0.0
            self.last_timestamp = timestamp_sec
            self.history_error.append(error_signal)
            self.history_time.append(timestamp_sec)
            return self.output

        # Compute time delta
        dt = timestamp_sec - self.last_timestamp
        if dt < 1e-9:
            return self.output

        # Compute adaptive weighting Θ(τ)
        theta = self._compute_theta(error_signal, dt)

        # Compute exponential decay
        decay = np.exp(-self.config.lambda_ * dt)

        # Update integral using discrete exponential smoothing
        self.output = self.output * decay + theta * error_signal * dt

        # Apply stability bound
        if abs(self.output) > self.config.stability_bound:
            self.output = np.sign(self.output) * self.config.stability_bound

        # Update history
        self.history_error.append(error_signal)
        self.history_time.append(timestamp_sec)

        # Trim history (keep last 10 seconds)
        max_history = int(self.config.sampling_rate_hz * 10.0)
        if len(self.history_error) > max_history:
            self.history_error.pop(0)
            self.history_time.pop(0)

        self.last_timestamp = timestamp_sec
        return self.output

    def _compute_theta(self, error: float, dt: float) -> float:
        """Compute adaptive weighting Θ(τ)"""
        if not self.config.enable_adaptive:
            return self.config.theta_base

        # Adaptive: Θ(τ) = θ_base + θ_gain * |error|
        theta = self.config.theta_base + self.config.theta_adaptive_gain * abs(error)
        return np.clip(theta, 0.1, 10.0)

    def reset(self):
        """Reset kernel state"""
        self.output = 0.0
        self.last_timestamp = 0.0
        self.history_error = []
        self.history_time = []

    def is_stable(self) -> bool:
        """Check stability"""
        return abs(self.output) <= self.config.stability_bound


class StealthDomainController:
    """Domain-specific Primal Logic controller"""

    def __init__(self, domain: StealthDomain, config: PrimalLogicConfig):
        self.domain = domain
        self.kernel = PrimalLogicKernel(config)
        self.current_error = 0.0
        self.output_scale = self._get_output_scale()

    def _get_output_scale(self) -> float:
        """Get domain-specific output scaling"""
        scales = {
            StealthDomain.RADAR_RCS: 1.0,
            StealthDomain.THERMAL_IR: 10.0,
            StealthDomain.EMCON: 1.0,
            StealthDomain.ACOUSTIC: 1.0,
            StealthDomain.MANEUVER: 0.1,
        }
        return scales.get(self.domain, 1.0)

    def update(self, measured: float, target: float, timestamp: float) -> float:
        """Update controller"""
        # Compute error
        self.current_error = measured - target

        # Update kernel
        kernel_output = self.kernel.update(self.current_error, timestamp)

        # Transform for domain
        return self._transform_output(kernel_output)

    def _transform_output(self, kernel_output: float) -> float:
        """Transform output for domain-specific actuators"""
        output = kernel_output * self.output_scale

        # Most domains use negative feedback
        if self.domain != StealthDomain.MANEUVER:
            output = -output

        return output


class TowBody:
    """Simulated Tow Body with stealth capabilities"""

    def __init__(self, config: PrimalLogicConfig):
        self.position = np.array([0.0, 0.0, -50.0])  # x, y, z (depth)
        self.velocity = np.array([2.0, 0.0, 0.0])     # m/s

        # Stealth controllers
        self.rcs_controller = StealthDomainController(StealthDomain.RADAR_RCS, config)
        self.thermal_controller = StealthDomainController(StealthDomain.THERMAL_IR, config)
        self.emcon_controller = StealthDomainController(StealthDomain.EMCON, config)
        self.acoustic_controller = StealthDomainController(StealthDomain.ACOUSTIC, config)
        self.maneuver_controller = StealthDomainController(StealthDomain.MANEUVER, config)

        # Signature baselines (what enemy sensors would detect without stealth)
        self.base_rcs = -5.0      # dBsm (relatively small target)
        self.base_thermal = 290.0  # K
        self.base_emcon = -30.0    # dBm
        self.base_acoustic = 120.0 # dB

        # Target signatures (stealth goals)
        self.target_rcs = -20.0     # dBsm (very low)
        self.target_thermal = 285.0  # K (blend with water temp)
        self.target_emcon = -60.0    # dBm (very low emissions)
        self.target_acoustic = 100.0 # dB (quiet)

        # Current signatures (with environmental noise)
        self.current_rcs = self.base_rcs
        self.current_thermal = self.base_thermal
        self.current_emcon = self.base_emcon
        self.current_acoustic = self.base_acoustic

    def update(self, dt: float, timestamp: float, threat_detected: bool = False):
        """Update Tow Body state and stealth systems"""

        # Add environmental noise to signatures
        self.current_rcs = self.base_rcs + np.random.normal(0, 0.5)
        self.current_thermal = self.base_thermal + np.random.normal(0, 0.2)
        self.current_emcon = self.base_emcon + np.random.normal(0, 1.0)
        self.current_acoustic = self.base_acoustic + np.random.normal(0, 0.3)

        # If threat detected, enhance stealth mode
        if threat_detected:
            self.target_rcs = -25.0
            self.target_emcon = -70.0

        # Update all stealth controllers
        rcs_control = self.rcs_controller.update(self.current_rcs, self.target_rcs, timestamp)
        thermal_control = self.thermal_controller.update(self.current_thermal, self.target_thermal, timestamp)
        emcon_control = self.emcon_controller.update(self.current_emcon, self.target_emcon, timestamp)
        acoustic_control = self.acoustic_controller.update(self.current_acoustic, self.target_acoustic, timestamp)

        # Apply stealth controls to signatures
        self.base_rcs += rcs_control * dt * 0.1
        self.base_thermal += thermal_control * dt * 0.01
        self.base_emcon += emcon_control * dt * 0.1
        self.base_acoustic += acoustic_control * dt * 0.05

        # Clamp to physical limits
        self.base_rcs = np.clip(self.base_rcs, -30.0, 10.0)
        self.base_thermal = np.clip(self.base_thermal, 280.0, 300.0)
        self.base_emcon = np.clip(self.base_emcon, -80.0, -20.0)
        self.base_acoustic = np.clip(self.base_acoustic, 90.0, 130.0)

        # Update position
        self.position += self.velocity * dt

    def get_detectability(self) -> float:
        """Compute overall detectability score [0, 1]"""
        # Normalize each signature to [0, 1] where 1 = easily detected
        rcs_detect = np.clip((self.current_rcs + 30.0) / 40.0, 0, 1)
        thermal_detect = np.clip((self.current_thermal - 280.0) / 20.0, 0, 1)
        emcon_detect = np.clip((self.current_emcon + 80.0) / 60.0, 0, 1)
        acoustic_detect = np.clip((self.current_acoustic - 90.0) / 40.0, 0, 1)

        # Weighted average (EMCON and RCS most important for detection)
        return 0.3 * rcs_detect + 0.15 * thermal_detect + 0.35 * emcon_detect + 0.2 * acoustic_detect


class Mine:
    """Simulated underwater mine"""

    def __init__(self, position: np.ndarray, mine_type: str):
        self.position = position
        self.mine_type = mine_type  # "moored", "bottom", "drifting"
        self.signature_strength = np.random.uniform(0.5, 1.0)
        self.detected = False
        self.classified = False

    def get_sensor_return(self, tb_position: np.ndarray, noise_level: float = 0.1) -> float:
        """Get sensor return strength based on distance"""
        distance = np.linalg.norm(self.position - tb_position)

        # Inverse square law with noise
        if distance < 0.1:
            distance = 0.1

        return_strength = self.signature_strength / (distance ** 2)
        return_strength += np.random.normal(0, noise_level)

        return max(0, return_strength)


class ThreatSensor:
    """Enemy sensor system trying to detect Tow Body"""

    def __init__(self, position: np.ndarray, sensor_type: str):
        self.position = position
        self.sensor_type = sensor_type  # "radar", "acoustic", "magnetic"
        self.detection_threshold = 0.6  # Detectability threshold
        self.detection_count = 0

    def detect(self, tow_body: TowBody) -> bool:
        """Attempt to detect Tow Body"""
        detectability = tow_body.get_detectability()

        # Distance attenuation
        distance = np.linalg.norm(self.position - tow_body.position)
        distance_factor = 1.0 / (1.0 + distance / 100.0)

        # Detection probability
        detection_prob = detectability * distance_factor

        detected = detection_prob > self.detection_threshold
        if detected:
            self.detection_count += 1

        return detected


class RMHSimulation:
    """Complete RMH simulation with Primal Logic stealth"""

    def __init__(self, config: PrimalLogicConfig):
        self.config = config
        self.tow_body = TowBody(config)
        self.mines: List[Mine] = []
        self.threats: List[ThreatSensor] = []
        self.time = 0.0
        self.dt = 1.0 / config.sampling_rate_hz

        # Metrics
        self.metrics = {
            'time': [],
            'rcs': [],
            'thermal': [],
            'emcon': [],
            'acoustic': [],
            'detectability': [],
            'detections': [],
            'mines_detected': [],
            'position_x': [],
            'position_y': [],
        }

    def add_mine(self, position: np.ndarray, mine_type: str):
        """Add mine to environment"""
        self.mines.append(Mine(position, mine_type))

    def add_threat(self, position: np.ndarray, sensor_type: str):
        """Add threat sensor to environment"""
        self.threats.append(ThreatSensor(position, sensor_type))

    def step(self):
        """Single simulation step"""
        # Check for threat detection
        threat_detected = any(threat.detect(self.tow_body) for threat in self.threats)

        # Update Tow Body with stealth systems
        self.tow_body.update(self.dt, self.time, threat_detected)

        # Check for mine detection
        for mine in self.mines:
            if not mine.detected:
                sensor_return = mine.get_sensor_return(self.tow_body.position)
                if sensor_return > 0.05:  # Detection threshold
                    mine.detected = True

        # Record metrics
        self.metrics['time'].append(self.time)
        self.metrics['rcs'].append(self.tow_body.current_rcs)
        self.metrics['thermal'].append(self.tow_body.current_thermal)
        self.metrics['emcon'].append(self.tow_body.current_emcon)
        self.metrics['acoustic'].append(self.tow_body.current_acoustic)
        self.metrics['detectability'].append(self.tow_body.get_detectability())
        self.metrics['detections'].append(sum(t.detection_count for t in self.threats))
        self.metrics['mines_detected'].append(sum(1 for m in self.mines if m.detected))
        self.metrics['position_x'].append(self.tow_body.position[0])
        self.metrics['position_y'].append(self.tow_body.position[1])

        self.time += self.dt

    def run(self, duration_sec: float):
        """Run simulation for specified duration"""
        num_steps = int(duration_sec / self.dt)
        print(f"Running RMH simulation for {duration_sec}s ({num_steps} steps)...")

        for i in range(num_steps):
            self.step()

            if (i + 1) % 1000 == 0:
                progress = (i + 1) / num_steps * 100
                mines_found = sum(1 for m in self.mines if m.detected)
                detections = sum(t.detection_count for t in self.threats)
                print(f"  {progress:5.1f}% | Mines: {mines_found}/{len(self.mines)} | "
                      f"Detected by threats: {detections} times")

        print(f"Simulation complete!")
        print(f"  Total mines detected: {self.metrics['mines_detected'][-1]}/{len(self.mines)}")
        print(f"  Total threat detections: {self.metrics['detections'][-1]}")
        print(f"  Average detectability: {np.mean(self.metrics['detectability']):.3f}")
        print(f"  Final position: ({self.tow_body.position[0]:.1f}, "
              f"{self.tow_body.position[1]:.1f}, {self.tow_body.position[2]:.1f})")

    def plot_results(self, save_path: Optional[str] = None):
        """Plot simulation results"""
        fig, axes = plt.subplots(3, 2, figsize=(14, 10))
        fig.suptitle('Primal Logic RMH Simulation Results', fontsize=16)

        # Stealth signatures
        axes[0, 0].plot(self.metrics['time'], self.metrics['rcs'], label='Current RCS')
        axes[0, 0].axhline(y=self.tow_body.target_rcs, color='r', linestyle='--', label='Target RCS')
        axes[0, 0].set_xlabel('Time (s)')
        axes[0, 0].set_ylabel('RCS (dBsm)')
        axes[0, 0].set_title('Radar Cross Section')
        axes[0, 0].legend()
        axes[0, 0].grid(True)

        axes[0, 1].plot(self.metrics['time'], self.metrics['thermal'], label='Current Temp')
        axes[0, 1].axhline(y=self.tow_body.target_thermal, color='r', linestyle='--', label='Target Temp')
        axes[0, 1].set_xlabel('Time (s)')
        axes[0, 1].set_ylabel('Temperature (K)')
        axes[0, 1].set_title('Thermal Signature')
        axes[0, 1].legend()
        axes[0, 1].grid(True)

        axes[1, 0].plot(self.metrics['time'], self.metrics['emcon'], label='Current Emissions')
        axes[1, 0].axhline(y=self.tow_body.target_emcon, color='r', linestyle='--', label='Target Emissions')
        axes[1, 0].set_xlabel('Time (s)')
        axes[1, 0].set_ylabel('Emission Power (dBm)')
        axes[1, 0].set_title('EMCON')
        axes[1, 0].legend()
        axes[1, 0].grid(True)

        axes[1, 1].plot(self.metrics['time'], self.metrics['acoustic'], label='Current Noise')
        axes[1, 1].axhline(y=self.tow_body.target_acoustic, color='r', linestyle='--', label='Target Noise')
        axes[1, 1].set_xlabel('Time (s)')
        axes[1, 1].set_ylabel('Acoustic Level (dB)')
        axes[1, 1].set_title('Acoustic Signature')
        axes[1, 1].legend()
        axes[1, 1].grid(True)

        # Detectability and mission metrics
        axes[2, 0].plot(self.metrics['time'], self.metrics['detectability'], 'r-', linewidth=2)
        axes[2, 0].axhline(y=0.6, color='k', linestyle='--', label='Detection Threshold')
        axes[2, 0].set_xlabel('Time (s)')
        axes[2, 0].set_ylabel('Detectability [0-1]')
        axes[2, 0].set_title('Overall Detectability')
        axes[2, 0].legend()
        axes[2, 0].grid(True)
        axes[2, 0].set_ylim([0, 1])

        # Mission progress
        ax2 = axes[2, 1]
        ax2.plot(self.metrics['time'], self.metrics['mines_detected'], 'g-', linewidth=2, label='Mines Detected')
        ax2.set_xlabel('Time (s)')
        ax2.set_ylabel('Mines Detected', color='g')
        ax2.tick_params(axis='y', labelcolor='g')
        ax2.grid(True)

        ax2_twin = ax2.twinx()
        ax2_twin.plot(self.metrics['time'], self.metrics['detections'], 'r-', linewidth=2, label='Threat Detections')
        ax2_twin.set_ylabel('Threat Detections', color='r')
        ax2_twin.tick_params(axis='y', labelcolor='r')

        axes[2, 1].set_title('Mission Progress')

        plt.tight_layout()

        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"Plot saved to {save_path}")
        else:
            plt.show()

    def save_metrics(self, filepath: str):
        """Save metrics to JSON"""
        with open(filepath, 'w') as f:
            json.dump(self.metrics, f, indent=2)
        print(f"Metrics saved to {filepath}")


def main():
    """Run demonstration simulation"""
    print("=" * 70)
    print("PRIMAL LOGIC RMH SIMULATION")
    print("Five-Domain Stealth Module Integration Test")
    print("=" * 70)
    print()

    # Configure Primal Logic kernel
    config = PrimalLogicConfig(
        lambda_=0.15,           # Moderate decay
        theta_base=1.0,         # Base weighting
        theta_adaptive_gain=0.8,  # Strong adaptive response
        sampling_rate_hz=10.0,  # 10 Hz update rate
        stability_bound=5.0,    # Moderate bound
        enable_adaptive=True
    )

    # Create simulation
    sim = RMHSimulation(config)

    # Add mines at various positions
    print("Setting up environment...")
    sim.add_mine(np.array([100.0, 20.0, -45.0]), "bottom")
    sim.add_mine(np.array([200.0, -30.0, -50.0]), "moored")
    sim.add_mine(np.array([350.0, 10.0, -48.0]), "bottom")
    sim.add_mine(np.array([450.0, -15.0, -52.0]), "bottom")
    sim.add_mine(np.array([600.0, 5.0, -47.0]), "moored")
    print(f"  Added {len(sim.mines)} mines")

    # Add threat sensors
    sim.add_threat(np.array([250.0, 100.0, -30.0]), "acoustic")
    sim.add_threat(np.array([400.0, -120.0, -20.0]), "radar")
    print(f"  Added {len(sim.threats)} threat sensors")
    print()

    # Run simulation
    duration = 300.0  # 5 minutes
    sim.run(duration)
    print()

    # Plot results
    print("Generating visualization...")
    sim.plot_results(save_path='/home/user/AndroidTacticalAssaultKit-CIV/rmh-module/simulation/primal_logic_results.png')

    # Save metrics
    sim.save_metrics('/home/user/AndroidTacticalAssaultKit-CIV/rmh-module/simulation/metrics.json')

    print()
    print("=" * 70)
    print("SIMULATION COMPLETE")
    print("=" * 70)


if __name__ == "__main__":
    main()
