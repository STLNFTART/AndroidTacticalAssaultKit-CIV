# Statement of Work (SOW)
## Remote Minehunting (RMH) Module Development and Demonstration

**Solicitation**: N00024-25-R-6304
**Contractor**: Primal Logic
**Period of Performance**: 90 days from contract award
**Classification**: UNCLASSIFIED

---

## 1. Scope of Work

Primal Logic shall design, develop, integrate, and demonstrate a Remote Minehunting (RMH) Module providing:

- Signal processing for mine detection and classification
- Crypto-secure communications between Tow Body, MCM USV, and ATAK C2
- Autonomous control algorithms for Tow Body navigation
- ATAK plugin for operator interface and mission visualization

---

## 2. Technical Objectives

| Objective | Performance Requirement |
|-----------|-------------------------|
| **Detection Range** | ≥200 meters (side-scan sonar) |
| **Classification Accuracy** | ≥90% true positive rate, ≤10% false positive rate |
| **Sensor Latency** | ≤2 seconds (sensor data → ATAK display) |
| **Crypto Overhead** | ≤10% throughput reduction vs. cleartext |
| **Comms Security** | AES-256-GCM encryption + HMAC-SHA256 authentication |
| **Autonomy** | Waypoint navigation with ±5m accuracy |
| **Endurance** | No more than 5% power increase over baseline |

---

## 3. Deliverables

### 3.1 Software Deliverables

| Item | Description | Format | Due Date |
|------|-------------|--------|----------|
| **Signal Processing Module** | C++ library with sonar/mag processing, ML classifier | Source + .so | Day 45 |
| **Crypto Comms Module** | C++ library with AES encryption, ECDH key exchange | Source + .so | Day 30 |
| **Autonomy Controller** | C++ library with path planning, PID control | Source + .so | Day 45 |
| **ATAK Plugin** | Android plugin (APK) for RMH C2 | Source + APK | Day 60 |
| **Integration Package** | Full RMH module integrated with test harness | Source + binary | Day 75 |

### 3.2 Documentation Deliverables

| Item | Description | Due Date |
|------|-------------|----------|
| **Architecture Document** | System design, interfaces, data flows | Day 15 |
| **API Specification** | Module APIs and integration guide | Day 30 |
| **User Manual** | ATAK plugin operator guide | Day 60 |
| **Test Report** | Lab validation results, performance metrics | Day 85 |
| **Final Technical Report** | Complete system description, demo results | Day 90 |

### 3.3 Demonstration

**Demo Event**: Live demonstration at NSWC Panama City Division (or contractor facility if approved)

**Date**: Day 80-85

**Demo Scenario**:
1. Ingest recorded sonar/mag sensor data (government-furnished or surrogate)
2. Display real-time detection and classification in ATAK
3. Show crypto-secure message exchange (encrypted telemetry)
4. Demonstrate autonomous waypoint navigation (simulation or live if platform available)
5. Measure and report performance metrics

---

## 4. Work Breakdown Structure (WBS)

### Phase 1: Core Development (Days 1-45)

**Task 1.1**: Signal Processing Engine Development
- Implement FFT-based sonar processing
- Integrate TensorFlow Lite ML classifier
- Develop multi-sensor fusion logic
- Unit testing with synthetic data

**Task 1.2**: Crypto Communications Module
- Integrate ATAK MeshNetCrypto APIs
- Implement ECDH key exchange
- Add FHSS anti-jamming (optional stretch goal)
- Crypto penetration testing

**Task 1.3**: Autonomy Controller
- Implement PID controllers (heading, depth, speed)
- Develop path planning (lawn-mower, spiral patterns)
- Obstacle avoidance using potential fields
- Lost-comms recovery behavior

**Milestone 1**: Core modules functional (Day 45)

---

### Phase 2: Integration (Days 46-75)

**Task 2.1**: ATAK Plugin Development
- Build RMH map overlay (TB track, contacts, search area)
- Create operator control UI
- Integrate with crypto comms module (JNI bridge)
- User acceptance testing

**Task 2.2**: System Integration
- Integrate signal processing + crypto + autonomy
- Build test harness for end-to-end testing
- Performance benchmarking
- Bug fixes and optimization

**Task 2.3**: Lab Validation
- Test with recorded sensor datasets
- Validate crypto security (penetration test)
- Measure latency, throughput, classification accuracy

**Milestone 2**: Integrated system ready for demo (Day 75)

---

### Phase 3: Demonstration and Reporting (Days 76-90)

**Task 3.1**: Demonstration Preparation
- Coordinate with NAVSEA TPOC for demo logistics
- Prepare demo scenario and test data
- Rehearse demo execution

**Task 3.2**: Live Demonstration
- Execute demo at NSWC PCD or approved location
- Capture performance data
- Government witness and evaluation

**Task 3.3**: Final Reporting
- Compile test results and demo outcomes
- Draft final technical report
- Lessons learned and recommendations for Phase II

**Milestone 3**: Demo complete, final report delivered (Day 90)

---

## 5. Government-Furnished Items (GFI)

Contractor requests the following government-furnished items to support demonstration:

1. **Sensor Data**: Unclassified or releasable sonar and magnetometer datasets from RMH or similar MCM missions
2. **Test Facility Access**: Lab space at NSWC PCD for integration testing (Days 70-85)
3. **ATAK Licenses**: ATAK development licenses and API documentation
4. **Technical POC**: Designated TPOC for technical coordination and requirement clarification

If GFI items are not available, contractor shall provide surrogate data/resources at no additional cost.

---

## 6. Contractor-Furnished Items (CFI)

1. Development hardware (laptops, test servers)
2. Software licenses (TensorFlow, OpenCV, Android Studio)
3. Travel to demo site (2 personnel, 3 days)
4. Surrogate sensor data (if GFI not available)

---

## 7. Meetings and Reviews

| Review | Date | Purpose |
|--------|------|---------|
| **Kickoff Meeting** | Day 5 | Finalize requirements, schedule, and GFI coordination |
| **Architecture Review** | Day 15 | Review system design and interfaces |
| **Progress Review 1** | Day 30 | Review crypto/comms module completion |
| **Progress Review 2** | Day 60 | Review ATAK plugin and integration status |
| **Demo Rehearsal** | Day 75 | Dry-run demonstration with TPOC |
| **Final Demo** | Day 80-85 | Live demonstration and evaluation |
| **Closeout** | Day 90 | Final report review and contract closeout |

All meetings conducted via secure VTC or in-person at government facility.

---

## 8. Security and Data Rights

### 8.1 Security Requirements
- All work performed on NIST 800-171 compliant systems
- No classified data handling (UNCLASSIFIED program)
- ITAR-controlled crypto source code restricted to US persons
- NDA with NAVSEA for any proprietary government-furnished data

### 8.2 Data Rights
- **Government Purpose Rights**: All software and documentation developed under this SOW
- **Contractor Proprietary**: Pre-existing Primal Logic IP (clearly marked)
- **ATAK Integration**: ATAK APIs and MeshNetCrypto remain government-owned
- **Open Source**: Third-party open-source libraries (TensorFlow, OpenCV) retain original licenses

---

## 9. Acceptance Criteria

The government shall accept deliverables based on:

1. **Functional Testing**: All modules pass unit and integration tests
2. **Performance Metrics**: Meet or exceed Technical Objectives (Section 2)
3. **Documentation**: Complete and accurate per SOW requirements
4. **Demonstration**: Successful live demo with government witness
5. **Code Quality**: Clean compilation, no critical security vulnerabilities

---

## 10. Period of Performance and Schedule

**Contract Award**: T+0
**Program Start**: T+5 days
**Milestone 1 (Core Dev Complete)**: T+45 days
**Milestone 2 (Integration Complete)**: T+75 days
**Milestone 3 (Demo & Final Report)**: T+90 days

Contractor shall provide bi-weekly status reports to government TPOC.

---

## 11. Cost and Payment

**Total Contract Value**: [To Be Negotiated]

**Payment Structure**:
- 20% upon Milestone 1 completion (Day 45)
- 30% upon Milestone 2 completion (Day 75)
- 50% upon Milestone 3 acceptance (Day 90)

**Cost Type**: Firm Fixed Price (FFP) preferred; Cost-Plus-Fixed-Fee (CPFF) acceptable

---

## 12. Points of Contact

**Government TPOC**: [To Be Assigned - PMS-495]
**Contracting Officer**: [To Be Assigned - NAVSEA]

**Contractor**:
Primal Logic
[Your Name], Program Manager
[Your Email]
[Your Phone]

---

**This Statement of Work is UNCLASSIFIED and approved for NAVSEA distribution.**
