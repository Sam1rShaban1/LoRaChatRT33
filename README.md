# LoRaChatRT - LoRa Mesh Network Chat with MQTT Integration 🌐

## 📡 Overview
LoRaChatRT is a sophisticated mesh networking solution that enables long-range communication using LoRa technology, with the added capability of transmitting routing tables via MQTT. This project is based on the [LoraChat](https://github.com/Jaimi5/LoRaChat) project, enhanced with real-time routing table synchronization.

## 🛠️ Hardware Requirements
- TTGO T-BEAM v1.1 or v1.2
- Arduino Nano 33 BLE Sense (with additional LoRa module)
- ESP32 with LoRa capability
- Display module (SSD1306)
- GPS module (built into T-BEAM)
- Battery management via AXP192/AXP2101

## ⭐ Features
- **Mesh Networking**: Implements BMX6 routing protocol for efficient mesh communication
- **MQTT Integration**: Real-time transmission of routing tables via MQTT
- **GPS Functionality**: Built-in GPS tracking and location sharing
- **Power Management**: Efficient battery management system
- **OLED Display**: Real-time status and message display
- **Multiple Communication Modes**:
  - LoRa mesh networking 📡
  - WiFi connectivity 📶
  - MQTT messaging 🔄
  - Bluetooth capabilities 📱
- **Sensor Integration** (with Arduino Nano 33 BLE Sense):
  - Temperature & Humidity 🌡️
  - Gesture recognition 👋
  - Proximity sensing 📏
  - Color detection 🎨
  - Sound detection 🔊

## 📁 Project Structure
```
├── src/
│   ├── battery/      # Battery management
│   ├── bluetooth/    # Bluetooth functionality
│   ├── commands/     # Command processing
│   ├── configuration/# System configuration
│   ├── gps/         # GPS functionality
│   ├── led/         # LED control
│   ├── loramesh/    # LoRa mesh networking
│   ├── message/     # Message handling
│   ├── monitor/     # System monitoring
│   ├── mqtt/        # MQTT integration
│   ├── sensor/      # Sensor data handling
│   ├── simulator/   # Testing simulations
│   ├── time/        # Time synchronization
│   └── wifi/        # WiFi connectivity
├── include/         # Header files
├── lib/            # External libraries
├── test/           # Test files
└── MonitoringService/ # Monitoring service component
```

## 📚 Dependencies
- ESP32 Arduino Core (espressif32@5.2.0)
- Arduino Nano 33 BLE Sense board support
- LoRaMesher (BMX6NewRoutingProtocol branch)
- TinyGPSPlus
- AXP202X_Library
- Adafruit SSD1306
- Adafruit GFX Library
- ArduinoJSON
- Other utilities (OneWire, SPI, Wire)

## 🚀 Setup and Configuration
1. Install PlatformIO IDE
2. Clone this repository
3. Open the project in PlatformIO
4. Configure your MQTT settings in the configuration files
5. Select your target board (T-BEAM or Nano 33 BLE Sense)
6. Upload the firmware to your device

## 💻 Building and Flashing
```bash
# Build the project
pio run

# Upload to your device
pio run --target upload

# Monitor serial output
pio device monitor
```

## ⚙️ Configuration Options
The system can be configured through `config.h`, including:
- LoRa parameters (frequency, bandwidth, etc.)
- MQTT broker settings
- WiFi credentials
- Display settings
- GPS configurations
- Power management settings
- Sensor configurations (for Nano 33 BLE Sense)

## 🔍 Monitoring and Debugging
- Built-in ESP32 exception decoder
- Serial monitoring at 115200 baud
- OLED display status information
- MQTT-based remote monitoring
- Sensor data visualization

## 🔋 Power Management
- Intelligent battery management via AXP192/AXP2101
- Power-saving modes
- Battery level monitoring and reporting
- Sleep mode optimization

## 🧪 Testing
- Comprehensive test suite in the `test/` directory
- Simulator available for testing mesh networking
- Hardware-in-the-loop testing capabilities
- Sensor calibration tools

## 🤝 Contributing
We love contributions! Whether it's:
- 🐛 Bug Reports
- 💡 Feature Suggestions
- 📝 Documentation Improvements
- 🔧 Code Contributions

Please feel free to submit pull requests or open issues.

## 📄 License
See the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments
- Based on the original [LoRaChat](https://github.com/Jaimi5/LoRaChat) project
- Thanks to the LoRaMesher team for the mesh networking protocol
- TTGO T-BEAM hardware team
- Arduino team for the Nano 33 BLE Sense support

## ❗ Troubleshooting
Common issues and their solutions:
1. 📡 GPS not acquiring fix: Use resetGPS functionality
2. 🔌 MQTT connection issues: Check WiFi and broker settings
3. 🖥️ Display problems: Verify I2C connections and display configuration
4. 🎯 Sensor calibration: Follow the calibration procedure in documentation
5. 🔄 Mesh network issues: Check LoRa parameters and antenna connection

## 📬 Contact
For support or queries:
- 📮 Open an issue in the GitHub repository
- 💬 Join our community chat
- 📧 Check our documentation wiki

## 🔜 Roadmap
- Enhanced sensor integration
- Mobile app development
- Web dashboard improvements
- Advanced power management features
- Extended mesh networking capabilities
