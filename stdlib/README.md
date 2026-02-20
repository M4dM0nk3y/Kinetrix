# Kinetrix Standard Library - README

## Overview

The Kinetrix Standard Library provides production-ready components for robotics and automation projects.

## Categories

### 🎯 Advanced (Proprietary)
**License**: Proprietary (Kinetrix Pro subscription required)

- **advanced_autonomous_system.kx** - Multi-sensor fusion, adaptive PID, Kalman filtering
- **swarm_intelligence.kx** - Distributed consensus, swarm behaviors, task allocation

### 🎮 Control
**License**: MIT

- **pid.kx** - Standard PID controller with anti-windup

### 📡 Sensors
**License**: MIT

- **imu.kx** - MPU6050 IMU driver (accelerometer + gyroscope)

## Installation

```bash
# Install package manager
npm install -g kinetrix-pm

# Install a library
kpm install control/pid
kpm install sensors/imu
```

## Usage

```javascript
// Include library in your code
include "control/pid.kx"
include "sensors/imu.kx"

program {
    # Initialize
    pid_init(1.0, 0.1, 0.05)
    imu_init()
    
    # Use in your code
    loop forever {
        imu_read_all()
        make var output = pid_compute(target, measured, 0.01)
        # ...
    }
}
```

## Examples

See `examples/` directory for complete projects:
- `line_follower.kx` - PID-based line following
- `self_balancing_robot.kx` - IMU + PID balancing

## Contributing

Want to contribute a library? See CONTRIBUTING.md

## License

- **Advanced libraries**: Proprietary
- **Standard libraries**: MIT
- **Examples**: MIT
