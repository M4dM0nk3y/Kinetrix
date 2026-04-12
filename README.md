# KINETRIX

**The first programming language designed specifically for robotics — write once, compile for any board.**

![Build](https://img.shields.io/badge/build-passing-brightgreen) ![Tests](https://img.shields.io/badge/tests-115%2F115-brightgreen) ![Security](https://img.shields.io/badge/security-hardened-blue) ![Targets](https://img.shields.io/badge/targets-5%20platforms-orange)

```kinetrix
program {
    enable ota "my_fleet" password "secret"

    loop forever {
        turn on pin 13
        wait 1000
        turn off pin 13
        wait 1000
    }
}
```

One file. Five targets. Fleet OTA updates. Zero boilerplate.

---

## Why Kinetrix?

| Feature | C/C++ | MicroPython | Kinetrix |
|---------|-------|-------------|----------|
| Syntax | `digitalWrite(13, HIGH);` | `Pin(13).value(1)` | `turn on pin 13` |
| Multi-target | ❌ One board | ⚠️ 2 boards | ✅ **5 targets** |
| OTA Fleet Updates | ❌ Manual | ❌ No | ✅ **Built-in** |
| Concurrent Tasks | ❌ Manual threads | ❌ GIL-limited | ✅ **Native tasks** |
| Performance | ✅ Native | ❌ Interpreted | ✅ **Compiles to native C/C++** |
| Learning Curve | Steep | Medium | **Beginner-friendly** |

---

## Supported Targets

| Target | Board | Output |
|--------|-------|--------|
| `arduino` | Arduino Uno/Nano/Mega | `.ino` (C++) |
| `esp32` | ESP32 Dev Module | `.cpp` (Arduino C++ with ESP-IDF) |
| `rpi` | Raspberry Pi | `.py` (Python 3 + RPi.GPIO) |
| `pico` | Raspberry Pi Pico/Pico W | `.py` (MicroPython) |
| `ros2` | ROS2 Nodes | `.cpp` (ROS2 C++) |

---

## Quick Start

### Build the Compiler

```bash
git clone https://github.com/M4dM0nk3y/Kinetrix.git
cd Kinetrix
make
```

### Write Your First Program

```kinetrix
# blink.kx
program {
    loop forever {
        turn on pin 13
        wait 500
        turn off pin 13
        wait 500
    }
}
```

### Compile

```bash
# For Arduino
./kcc blink.kx --target arduino -o blink.ino

# For ESP32
./kcc blink.kx --target esp32 -o blink.cpp

# For Raspberry Pi
./kcc blink.kx --target rpi -o blink.py

# For Pico
./kcc blink.kx --target pico -o blink.py
```

### Upload

Upload the generated file to your board using Arduino IDE, `arduino-cli`, Thonny, or `scp`.

---

## Language Features

### Variables & Types

```kinetrix
make int speed = 100
make var name = "robot1"
make bool active = true
const int MAX_SPEED = 255
```

### GPIO Control

```kinetrix
turn on pin 13
turn off pin 13
set pin 9 to 128              # PWM
make int val = read pin 2   # Digital read
make int a = read analog pin 34  # Analog read
```

### Control Flow

```kinetrix
if speed > 100 {
    print "Too fast!"
} else if speed < 10 {
    print "Too slow"
} else {
    print "Just right"
}

repeat 10 { turn on pin 13; wait 100; turn off pin 13; wait 100 }

for i from 0 to 180 { set pin 9 to i; wait 20 }

while active { feed watchdog; wait 100 }

loop forever { /* main loop */ }
```

### Functions

```kinetrix
def clamp(val int, lo int, hi int) int {
    if val < lo { return lo }
    if val > hi { return hi }
    return val
}
```

### Structs

```kinetrix
define type SensorData {
    number temp
    number humidity
    number pressure
}
make SensorData reading
```

### Concurrent Tasks

```kinetrix
shared make int sensor_val = 0

task sensor_loop {
    sensor_val = read analog pin 34
    wait 1000
}

task motor_loop {
    set pin 9 to sensor_val
    wait 100
}

start task sensor_loop
start task motor_loop
```

### I2C / SPI / Serial

```kinetrix
open serial at 115200
open i2c
open spi at 1000000

send serial "Hello"
make int temp = read i2c device 0x68 register 0x41
```

### Interrupts

```kinetrix
on interrupt pin 2 rising {
    turn on pin 13
}

on timer every 5000ms {
    feed watchdog
}
```

### OTA Fleet Updates

```kinetrix
enable ota "my_fleet" password "secret123"
```

Then push updates to all robots on the network:

```bash
./kcc_push.sh robot.kx --target esp32 --password secret123
```

### Library Wrappers — Wave 1: Sensors & Output (V3.1)

Native cross-platform syntax for common components. The compiler automatically includes `Servo.h`, `RPi.GPIO`, etc. based on the target.

```kinetrix
# Servo Motors
attach servo pin 9
move servo to 90
detach servo pin 9

# Sensors
make float dist = read distance trigger 12 echo 11
attach dht11 pin 4
make float temp = read temperature

# NeoPixel LEDs
attach strip pin 6 count 30
set pixel 0 to 255 0 0
show pixels

# LCD Displays
attach lcd columns 16 rows 2
lcd print "Hello" line 0
```

### Library Wrappers — Wave 2: Motion & Motor Control (V3.1) 🆕

Full motor control and closed-loop feedback — compile for any board.

```kinetrix
# Stepper Motors (A4988 / DRV8825)
attach stepper step 2 dir 3
set stepper speed 500
move stepper 200

# DC Motor Drivers (L298N / TB6612)
attach motor enable 9 forward 8 reverse 7
move motor forward at 255
move motor reverse at 128
stop motor

# Quadrature Encoders
attach encoder pin_a 2 pin_b 3
make float pos = read encoder
reset encoder

# Brushless ESCs
attach esc pin 10
set esc throttle 1500

# PID Controllers
attach pid kp 2.0 ki 0.5 kd 1.0
set pid target 100
make float out = compute pid pos
```

### Library Wrappers — Wave 3: Communication & Networking (V3.1) 🆕

Full stack networking features across all target platforms.

```kinetrix
# BLE
enable ble "KinetrixRobot"
ble advertise "sensor_data"
make var b_msg = ble receive
ble send "hello ble"

# WiFi
connect wifi "MyNetwork" password "12345678"
make var ip = wifi ip

# MQTT
connect mqtt "broker.hivemq.com" port 1883
mqtt subscribe "kinetrix/test"
make var m_msg = mqtt read
mqtt publish "kinetrix/test" "online"

# HTTP/REST
make var res = http get "http://api.kinetrix.com/status"
http post "http://api.kinetrix.com/data" body "{'status': 'ok'}"

# WebSockets
connect websocket "ws://my-server.local:8080/feed"
make var w_msg = ws receive
ws send "hello ws"
ws close
```

### Library Wrappers — Wave 4: Advanced Robotics & Storage 🆕

Native bindings for IMUs, GPS receivers, LIDAR, and local file storage.

```kinetrix
# 9-DOF IMU (BNO080 / BNO055)
attach imu
make float heading = read orientation
make float accel_x = read accel x
make float gyro_z = read gyro z

# GPS Receivers
attach gps baud 9600
make float latitude = read location lat
make float longitude = read location lon
make float speed = read speed
make float altitude = read altitude

# LIDAR / Time-of-Flight Sensors (VL53L0X)
attach lidar
make float dist = read distance precise

# SD Card File Systems
mount sd chip_select 5
open file "log.csv"
write file "1024, 25.4"
close file
```

### Library Wrappers — Wave 5: Output Systems & Edge AI Vision 🆕

Native support for rich output (OLED displays, audio synthesis) and smart AI vision sensors (HuskyLens).

```kinetrix
# OLED Displays (SSD1306)
attach oled width 128 height 64
oled print "Kinetrix V3!" at x 10 y 20
oled draw circle x 64 y 32 radius 15
oled draw rect x 0 y 0 width 128 height 64
oled show
oled clear

# Audio & Sound Generation
attach audio pin 25
set volume 80
play frequency 440 duration 1000
play sound "beep.wav"

# Smart AI Vision Cameras (HuskyLens)
attach camera protocol i2c
make bool found = read camera detect "person"
make float cx = read camera object x
make float cy = read camera object y
```

### Library Wrappers — Wave 6: Advanced Locomotion, Kinematics & Edge AI 🆕

Mecanum drive kinematics, sensor fusion data processing, and simple Edge AI (TFLite Micro) inference seamlessly compiling to all platforms!

```kinetrix
# Mecanum / Omni-Directional Drive
attach mecanum fl 2 fr 3 bl 4 br 5
move mecanum x 50 y 0 turn 50
stop mecanum

# Sensor Fusion (Kalman Filters)
attach kalman
make float filtered = compute kalman raw 10.5

# Edge AI Inference (TensorFlow Lite Micro)
load ai model "model.tflite"
make float prediction = compute ai filtered
```

### Library Wrappers — Wave 7: The Master Automaton 🆕

Robotic Arm inverse kinematics, 2D grid pathfinding, and quadcopter flight stabilization — the most advanced robotics operations, natively compiled!

```kinetrix
# Robotic Arm (Inverse Kinematics)
attach arm dof 3 length1 10.5 length2 8.2 length3 5.0
move arm to x 15 y 5 z 10

# Autonomous Pathfinding (A*)
make grid myMap size 10 10
set grid myMap obstacle at x 5 y 5
make var route = compute path from x 0 y 0 to x 9 y 9

# Quadcopter Flight Stabilization
attach quadcopter fl 2 fr 3 bl 4 br 5
set drone target pitch 10 roll 0 yaw 0 throttle 1500
```

### Safety Features

```kinetrix
enable watchdog timeout 5000ms
feed watchdog

try {
    make int val = read i2c device 0x68 register 0x3B
} on error {
    send serial "Sensor failed!"
}

assert motor_speed < 255 else emergency_stop()
```

### Arrays & Buffers

```kinetrix
make array readings size 10
make buffer history size 5

push history sensor_val
readings[0] = 42
```

### Wireless Communication (ESP-NOW)

```kinetrix
radio_send_peer(0, sensor_value)  # Send to peer
if radio_available {
    make int msg = radio_read
}
```

---

## Package Manager & Fleet Management

### Package Manager (kpm)

Install community and standard library packages:

```bash
python3 kpm.py install drive
python3 kpm.py install sonar
python3 kpm.py search "motor"
python3 kpm.py list
```

### Fleet Management

Kinetrix includes a complete fleet management stack:

```bash
# Start the package registry (local or cloud)
export KPM_API_KEY="your-secret-key"
python3 registry_server.py

# Deploy to robot fleet via OTA
./kcc_push.sh robot.kx --target esp32 --password secret123

# On the robot — auto-update agent
export ROBOT_ID="robot-01"
python3 robot_agent.py
```

The robot agent supports:
- Automatic polling for OTA updates
- Semantic version checking (rejects downgrades)
- Secure tar extraction (Zip Slip protection)

---

## Project Structure

```
Kinetrix/
├── compiler_v3.c          # Main compiler driver
├── parser.c / parser.h    # Lexer + Parser (source → AST)
├── ast.c / ast.h          # Abstract Syntax Tree definitions
├── codegen.c / codegen.h  # Arduino code generator
├── codegen_esp32.c        # ESP32 code generator
├── codegen_rpi.c          # Raspberry Pi code generator
├── codegen_pico.c         # Pico code generator
├── codegen_ros2.c         # ROS2 code generator
├── pin_tracker.c/.h       # Pin usage analysis
├── symbol_table.c/.h      # Symbol table for variables
├── error.c/.h             # Error reporting
├── diagnostics.c          # Compiler diagnostics
├── Makefile               # Build system
├── kcc_push.sh            # OTA fleet push tool
├── kpm.py                 # Package manager (CLI)
├── registry_server.py     # Package registry server
├── robot_agent.py         # OTA update agent for robots
├── test_all.sh            # CI test suite (115 tests)
├── examples/              # Example programs
└── libs/                  # Standard library modules
    ├── drive.kx           # Motor drive abstractions
    ├── sonar.kx           # Ultrasonic distance sensor
    ├── math.kx            # Math utilities
    ├── music.kx           # Tone/melody helpers
    └── test_lib.kx        # Linker test module
```

---

## Testing

The CI suite compiles all example `.kx` files across all 5 targets:

```bash
# Run the full test suite (115 tests)
bash test_all.sh

# Or compile a single file
./kcc examples/wave7_test.kx --target arduino
./kcc examples/wave7_test.kx --target esp32
./kcc examples/wave7_test.kx --target rpi
```

### Security Hardening

The compiler has undergone a comprehensive 6-pass security audit covering:
- **Memory safety**: All AST nodes properly freed, NULL guards on malloc
- **Injection prevention**: String literals escaped across all 5 backends
- **Platform correctness**: No C++ exceptions on AVR/ESP32, AVR-safe BFS
- **Fleet security**: Path traversal protection, API key auth, Zip Slip guards
- **Zero warnings**: Compiles clean with `-Wall -Wextra`

## Version

```bash
./kcc --version
# Kinetrix Compiler v3.1.0
```

---

## Examples

See [`examples/`](examples/) for complete programs:

| Example | Features Demonstrated |
|---------|----------------------|
| `v3_esp32_all.kx` | Full ESP32 feature demo |
| `v3_interrupt_control.kx` | Interrupts, timers |
| `v3_task_codegen_test.kx` | Concurrent tasks |
| `v3_radio_test.kx` | ESP-NOW wireless |
| `v3_ota_demo.kx` | OTA fleet updates |
| `v3_library_demo.kx` | Servo, sensors, NeoPixel, LCD |

---

## License

MIT License — see [LICENSE](LICENSE)

---

## Author

**Soham Mulik** — [@M4dM0nk3y](https://github.com/M4dM0nk3y)

---

**Built for robotics. Built for the future.** 🤖
