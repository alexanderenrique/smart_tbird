# Smart Thunderbird CAN Network - Project Organization

## Overview

This document describes the organized structure for the Smart Thunderbird CAN BUS sensor network project. The project is designed to support multiple sensor nodes with different hardware configurations while maintaining clean, maintainable code.

## Directory Structure

```
smart_tbird/
├── nodes/                          # Individual CAN nodes
│   ├── display_node/              # Main display unit (ESP32 + TFT)
│   │   ├── main.cpp              # Display node main code
│   │   └── lv_conf.h             # LVGL configuration
│   ├── central_node/              # Central sensor hub (ESP32 + SHT31 + MPU6050)
│   │   ├── main.cpp              # Central node main code
│   │   ├── mpu6050_sensor.h      # MPU6050 sensor class header
│   │   ├── mpu6050_sensor.cpp    # MPU6050 sensor implementation
│   │   ├── mpu6050_can_client.h  # MPU6050 CAN client header
│   │   └── mpu6050_can_client.cpp # MPU6050 CAN client implementation
│   ├── sht31_node/               # SHT31 temperature/humidity sensor (legacy)
│   │   ├── main.cpp              # SHT31 node main code
│   │   ├── sht31_sensor.cpp      # SHT31 sensor library
│   │   ├── sht31_sensor.h        # SHT31 sensor header
│   │   ├── sht31_can_client.cpp  # SHT31 CAN client
│   │   └── sht31_wifi_client.cpp # Legacy WiFi client
│   ├── tmp36_node/               # TMP36 temperature sensor
│   │   ├── main.cpp              # TMP36 node main code
│   │   ├── tmp36_sensor.cpp      # TMP36 sensor library
│   │   ├── tmp36_sensor.h        # TMP36 sensor header
│   │   ├── tmp36_can_client.cpp  # TMP36 CAN client
│   │   └── tmp36_wifi_client.cpp # Legacy WiFi client
│   └── shared/                   # Shared libraries and utilities
│       ├── can_lib/              # CAN communication library
│       │   ├── can_messages.h    # CAN message definitions
│       │   ├── can_manager.h     # CAN manager class header
│       │   └── can_manager.cpp   # CAN manager implementation
│       ├── sensor_lib/           # Sensor-specific libraries
│       └── utils/                # Utility functions
│           └── node_config.h     # Hardware configuration
├── docs/                         # Documentation
│   ├── hardware/                 # Hardware documentation
│   ├── software/                 # Software documentation
│   └── deployment/               # Deployment guides
├── include/                      # Legacy include files
├── lib/                          # External libraries
├── cpp_hold/                     # Legacy code (for reference)
├── platformio.ini               # PlatformIO configuration
└── README.md                     # Project overview
```

## Node Types and Configurations

### 1. Display Node (ESP32 + TFT Display + LDR)
- **Hardware**: ESP32, TFT touchscreen, LDR sensor, MCP2515 CAN controller
- **Function**: Main display unit showing sensor data from all nodes with auto-dimming
- **Features**: LVGL GUI, touch interface, real-time data visualization, LDR auto-dimming
- **Build Command**: `pio run -e display_node`

### 2. Central Node (ESP32 + SHT31 + MPU6050) - **PRIMARY SENSOR HUB**
- **Hardware**: ESP32, SHT31 temperature/humidity sensor, MPU6050 6-axis IMU, MCP2515
- **Function**: Comprehensive sensor hub providing environmental and motion data
- **Features**: 
  - SHT31 temperature/humidity monitoring
  - MPU6050 6-axis IMU with 100Hz sampling
  - Digital Low-Pass Filtering (DLPF) for noise reduction
  - Moving average smoothing for real-time display
  - Absolute maximum G-force tracking since power-on
  - Automotive G-force monitoring (acceleration, braking, cornering)
- **Data Transmission**:
  - Smoothed G-force data at 2Hz (every 500ms) for real-time display
  - Absolute maximum G-forces at 2Hz (every 500ms) for peak monitoring
  - Temperature/humidity data at 0.5Hz (every 2 seconds)
- **Build Command**: `pio run -e central_node`



## MPU6050 IMU Features (Central Node)

### High-Performance Motion Sensing
The central node includes a comprehensive MPU6050 6-axis IMU implementation optimized for automotive applications:

#### Sampling and Filtering
- **100Hz raw sampling rate** for maximum data capture
- **Digital Low-Pass Filtering (DLPF)** with ~44Hz cutoff frequency
- **50-sample moving average smoothing** (0.5-second window)
- **2Hz smoothed data transmission** for real-time display

#### G-Force Monitoring
- **X-axis**: Forward/backward acceleration (acceleration/braking)
- **Y-axis**: Left/right acceleration (cornering forces)
- **Z-axis**: Up/down acceleration (bumps, hills, impacts)
- **Absolute maximum tracking** since power-on (no automatic resets)

#### Data Transmission
- **Smoothed G-force data**: 2Hz (every 500ms) for smooth dashboard display
- **Absolute maximum values**: 2Hz (every 500ms) for peak G-force monitoring
- **Optimized CAN bus usage**: No raw data transmission to reduce bus load

#### Automotive Applications
- **Performance monitoring**: Track acceleration, braking, and cornering
- **Safety analysis**: Monitor maximum forces experienced
- **Driving behavior**: Analyze G-force patterns
- **Impact detection**: Identify sudden changes in motion

## Shared Libraries

### CAN Manager (`shared/can_lib/`)
- **Purpose**: Unified CAN communication interface
- **Features**:
  - Automatic CAN controller initialization
  - Message queuing and reliable transmission
  - Error handling and recovery
  - Heartbeat management
  - Network diagnostics

### Node Configuration (`shared/utils/node_config.h`)
- **Purpose**: Hardware-specific pin definitions and settings
- **Features**:
  - Conditional compilation based on node type
  - Pin assignments for different hardware
  - Timing configurations
  - Utility functions

### CAN Messages (`shared/can_lib/can_messages.h`)
- **Purpose**: Standardized CAN message definitions
- **Features**:
  - Message ID assignments
  - Data structures for all sensor types
  - Utility functions for data conversion
  - Error codes and status definitions
- **Current Message Types**:
  - `CAN_MSG_SHT31_TEMP_HUMIDITY` (0x101): SHT31 temperature/humidity data (0.5Hz)
  - `CAN_MSG_TMP36_TEMPERATURE` (0x102): TMP36 temperature data
  - `CAN_MSG_MPU6050_SMOOTHED` (0x103): MPU6050 smoothed G-force data (2Hz)
  - `CAN_MSG_MPU6050_MAX` (0x104): MPU6050 absolute maximum G-forces (2Hz)
  - `CAN_MSG_SENSOR_STATUS` (0x105): Sensor health and status information

## Building and Deployment

### Prerequisites
- PlatformIO installed
- ESP32/ESP32-C3 development boards
- MCP2515 CAN controller modules
- TJA1050 CAN transceivers
- Sensors (SHT31, TMP36, MPU6050, LDR)

### Build Commands
```bash
# Build all nodes
pio run

# Build specific nodes
pio run -e display_node
pio run -e central_node      # Primary sensor hub (SHT31 + MPU6050)
pio run -e sht31_node        # Legacy standalone SHT31
pio run -e tmp36_node

# Upload specific node
pio run -e display_node -t upload
pio run -e central_node -t upload
pio run -e sht31_node -t upload
pio run -e tmp36_node -t upload
```

### Hardware Setup
1. Connect MCP2515 modules to all devices
2. Wire CAN bus (CAN-H and CAN-L)
3. Add 120Ω termination resistors at both ends
4. Power up all devices
5. Monitor serial output for initialization status

## Adding New Nodes

### 1. Create Node Directory
```bash
mkdir nodes/new_sensor_node
```

### 2. Create Main Code
- Copy template from existing node
- Modify sensor-specific code
- Update pin assignments in `node_config.h`

### 3. Update PlatformIO Configuration
- Add new environment in `platformio.ini`
- Define node-specific build flags
- Set appropriate board and libraries

### 4. Update CAN Messages
- Add new message IDs if needed
- Define data structures
- Update message handling code

### 5. Test and Deploy
- Build and upload to hardware
- Test CAN communication
- Verify sensor readings

## Best Practices

### Code Organization
- Keep node-specific code in respective directories
- Use shared libraries for common functionality
- Maintain consistent naming conventions
- Document hardware pin assignments

### CAN Communication
- Use standardized message IDs
- Implement proper error handling
- Include checksums for data validation
- Send regular heartbeats

### Hardware Configuration
- Define all pins in `node_config.h`
- Use conditional compilation for different nodes
- Document pin assignments clearly
- Test hardware connections thoroughly

### Error Handling
- Implement comprehensive error checking
- Log errors to serial output
- Send error reports via CAN
- Include recovery mechanisms

## Troubleshooting

### Common Issues
1. **CAN Controller Not Initializing**
   - Check SPI connections
   - Verify power supply
   - Check MCP2515 module

2. **No Data Received**
   - Verify CAN bus wiring
   - Check termination resistors
   - Ensure all devices powered

3. **Build Errors**
   - Check library dependencies
   - Verify include paths
   - Check board configuration

### Debug Information
- All nodes provide serial output
- CAN manager includes diagnostic functions
- Status LEDs indicate communication state
- Error counters track system health

## Current Capabilities

### Implemented Features
- ✅ Multi-node CAN network with standardized communication
- ✅ High-performance MPU6050 IMU with 100Hz sampling
- ✅ Digital filtering and smoothing for automotive applications
- ✅ Real-time G-force monitoring and absolute maximum tracking
- ✅ Environmental monitoring (temperature/humidity)
- ✅ Auto-dimming display with LDR sensor
- ✅ Comprehensive error handling and status reporting
- ✅ Modular, maintainable code architecture

## Future Enhancements

### Planned Features
- Automatic sensor discovery
- Configurable update intervals
- Power management modes
- Network diagnostics dashboard
- Data logging capabilities
- GPS integration for location-based G-force analysis
- Advanced filtering algorithms (Kalman filters)
- Real-time data visualization improvements

### Expansion Possibilities
- Additional sensor types (pressure, light, proximity)
- Actuator control (fans, pumps, valves)
- Remote monitoring via WiFi/Bluetooth
- Integration with other CAN systems
- Wireless gateway support
- Machine learning for driving behavior analysis
- Integration with vehicle OBD-II systems

---

This organization structure provides a scalable, maintainable foundation for your CAN BUS sensor network while keeping each node's code organized and easily manageable.
