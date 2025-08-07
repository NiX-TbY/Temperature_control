# Unit Tests

This directory contains unit tests for the Temperature Control System.

## Running Tests

```bash
pio test
```

## Test Categories

- **Hardware Tests**: Sensor and display initialization
- **Logic Tests**: Temperature control algorithms
- **Safety Tests**: Error handling and limits
- **Interface Tests**: Touch and button interactions

## Test Environment

Tests run on the target hardware (ESP32-S3) to ensure real-world compatibility.

## Available Tests

- `test_temperature_sensor_init()` - Verifies sensor initialization
- `test_temperature_controller_init()` - Verifies controller setup
- `test_display_init()` - Tests display functionality
- `test_temperature_range_validation()` - Tests temperature limits
- `test_mode_switching()` - Tests operation mode changes
- `test_sensor_data_validity()` - Validates sensor readings
- `test_safety_limits()` - Tests safety mechanisms
