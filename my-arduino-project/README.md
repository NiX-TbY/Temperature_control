# 🌡️ Temperature Control System

A comprehensive temperature control system built for ESP32-S3 with a 4.3" Waveshare display, featuring real-time monitoring, intelligent control algorithms, and a modern web interface.

## ✨ Features

- **Real-time Temperature Monitoring**: DHT22 sensor with humidity readings
- **Multiple Control Modes**: Manual Heat/Cool, Auto, Fan-only, Off
- **Touch Interface**: Intuitive button-based control system on 4.3" display
- **Safety Features**: Temperature limits, error detection, emergency stop
- **WiFi Connectivity**: Remote monitoring and control via web interface
- **Data Logging**: Historical temperature data with graphical display
- **PID Control**: Advanced temperature regulation algorithms
- **Responsive Web UI**: Modern, mobile-friendly control interface

## 🔧 Hardware Requirements

- **Waveshare ESP32-S3 4.3" Type B Display**
- **DHT22 Temperature/Humidity Sensor**
- **Relay modules** for heating/cooling control
- **PWM-controlled fan**
- **Buzzer** for alerts
- **Power supply** (5V recommended)

## 📋 Pin Configuration

| Component | ESP32-S3 Pin | Description |
|-----------|--------------|-------------|
| DHT22 Data | GPIO 15 | Temperature/humidity sensor |
| Heating Relay | GPIO 17 | Controls heating element |
| Cooling Relay | GPIO 18 | Controls cooling element |
| Fan PWM | GPIO 19 | PWM fan speed control |
| Buzzer | GPIO 20 | Audio alerts |
| Display Backlight | GPIO 2 | Screen brightness |
| Touch Reset | GPIO 5 | Touch controller reset |
| Touch SDA | GPIO 6 | I2C data line |
| Touch SCL | GPIO 7 | I2C clock line |

## 🚀 Building and Uploading

```bash
cd /workspaces/Temperature_control/my-arduino-project
pio pkg install
pio run
pio run --target upload
pio device monitor
```

## Setup Instructions
1. Clone the repository:
   ```
   git clone <repository-url>
   ```
2. Navigate to the project directory:
   ```
   cd my-arduino-project
   ```
3. Install the required libraries and dependencies using PlatformIO:
   ```
   pio lib install
   ```

## Usage
To build and upload the project to your Arduino board, use the following command:
```
pio run --target upload
```

## 🔧 Development & CI/CD

This project includes comprehensive GitHub Actions for automated building and testing:

- **Automated Build**: ESP32-S3 firmware compilation on every push
- **Dependency Management**: Dependabot keeps libraries updated
- **Security Scanning**: Automated vulnerability detection
- **Hardware Testing**: Support for hardware-in-the-loop testing (requires self-hosted runner)

### Development Workflow
1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Commit your changes: `git commit -m 'Add amazing feature'`
4. Push to the branch: `git push origin feature/amazing-feature`
5. Open a Pull Request

## 🛡️ Security

Please review our [Security Policy](.github/SECURITY.md) for:
- Vulnerability reporting procedures
- Security best practices
- Supported versions
- Hardware and network security guidelines

## 📋 Issues & Support

- **Bug Reports**: Use our [bug report template](.github/ISSUE_TEMPLATE/bug_report.yml)
- **Feature Requests**: Use our [feature request template](.github/ISSUE_TEMPLATE/feature_request.yml)
- **General Questions**: Open a GitHub Discussion

## Contributing
We welcome contributions! Please:
- Read our contributing guidelines
- Follow the pull request template
- Ensure all tests pass
- Update documentation as needed

Feel free to submit issues or pull requests for improvements and enhancements to the project.