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
│   ├── sht31_node/               # SHT31 temperature/humidity sensor
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

### 2. SHT31 Node (ESP32-C3 + SHT31 Sensor)
- **Hardware**: ESP32-C3, SHT31 temperature/humidity sensor, MCP2515
- **Function**: Reads temperature and humidity, transmits via CAN
- **Features**: I2C communication, error handling, status reporting
- **Build Command**: `pio run -e sht31_node`

### 3. TMP36 Node (ESP32 + TMP36 Sensor)
- **Hardware**: ESP32, TMP36 analog temperature sensor, MCP2515
- **Function**: Reads temperature from analog sensor, transmits via CAN
- **Features**: Analog reading, voltage conversion, temperature calculation
- **Build Command**: `pio run -e tmp36_node`



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

## Building and Deployment

### Prerequisites
- PlatformIO installed
- ESP32/ESP32-C3 development boards
- MCP2515 CAN controller modules
- TJA1050 CAN transceivers
- Sensors (SHT31, TMP36, LDR)

### Build Commands
```bash
# Build all nodes
pio run

# Build specific nodes
pio run -e display_node
pio run -e sht31_node
pio run -e tmp36_node

# Upload specific node
pio run -e display_node -t upload
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

## Future Enhancements

### Planned Features
- Automatic sensor discovery
- Configurable update intervals
- Power management modes
- Network diagnostics dashboard
- Data logging capabilities

### Expansion Possibilities
- Additional sensor types
- Actuator control
- Remote monitoring
- Integration with other CAN systems
- Wireless gateway support

---

This organization structure provides a scalable, maintainable foundation for your CAN BUS sensor network while keeping each node's code organized and easily manageable.
