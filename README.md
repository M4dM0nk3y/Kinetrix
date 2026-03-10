# KINETRIX

**The first programming language designed specifically for robotics — write once, compile for any board.**

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
make number speed = 100
make string name = "robot1"
make bool active = true
const number MAX_SPEED = 255
```

### GPIO Control

```kinetrix
turn on pin 13
turn off pin 13
set pin 9 to 128              # PWM
make number val = read pin 2   # Digital read
make number a = read analog pin 34  # Analog read
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
define function clamp(val number, lo number, hi number) returns number {
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
shared make number sensor_val = 0

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
make number temp = read i2c device 0x68 register 0x41
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
connect websocket "ws://echo.websocket.org"
make var w_msg = ws receive
ws send "hello ws"
ws close
```

### Safety Features

```kinetrix
enable watchdog timeout 5000ms
feed watchdog

try {
    make number val = read i2c device 0x68 register 0x3B
} on error {
    send serial "Sensor failed!"
}

assert motor_speed < 255 else emergency_stop()
```

### Arrays & Buffers

```kinetrix
make array readings size 10
make buffer history size 5

push sensor_val into history
readings[0] = 42
```

### Wireless Communication (ESP-NOW)

```kinetrix
radio send 0 sensor_value    # Send to peer
if radio available {
    make number msg = radio read
}
```

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
├── examples/              # Example programs
└── libs/                  # Standard library modules
```

---

## Testing

The CI automatically compiles all example `.kx` files across all 5 targets on every push.

```bash
# Run locally:
make
for f in examples/*.kx; do ./kcc --target=arduino "$f"; done
```

---

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
