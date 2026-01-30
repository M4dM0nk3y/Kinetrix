# Kinetrix V1.0

> A high-level, compiled robotics programming language for Raspberry Pi and Arduino platforms

[![Language](https://img.shields.io/badge/language-Kinetrix-blue.svg)](https://github.com/yourusername/kinetrix)
[![Platform](https://img.shields.io/badge/platform-Arduino%20%7C%20Raspberry%20Pi-orange.svg)](https://github.com/yourusername/kinetrix)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## 📖 Overview

Kinetrix is a robotics programming language designed specifically for makers and educators. It abstracts hardware complexity while maintaining direct control over GPIO pins, sensors, motors, and communication protocols. With its natural English-like syntax and minimal boilerplate, Kinetrix makes robotics programming accessible to beginners while remaining powerful for advanced users.

### Key Features

- **🎯 Readability**: Natural English-like syntax
- **⚡ Simplicity**: Minimal boilerplate code
- **🤖 Hardware-focused**: Built-in support for robotics peripherals
- **🔧 Compiled**: Translates to optimized C/Arduino code
- **🎓 Educational**: Perfect for learning robotics programming

## 🚀 Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/yourusername/kinetrix.git
cd kinetrix

# Build the compiler
make

# Add to PATH (optional)
export PATH=$PATH:$(pwd)
```

### Your First Program

Create a file named `blink.kx`:

```kinetrix
program blink_led {
    forever {
        turn on 13
        wait 1000
        turn off 13
        wait 1000
    }
}
```

### Compile and Deploy

```bash
# Compile your program
kinetrix blink.kx

# Upload the generated Kinetrix_Arduino.ino to your Arduino board
# using Arduino IDE or arduino-cli
```

## 📚 Language Guide

### Basic Syntax

#### Variables

```kinetrix
make var speed = 100
make var sensor_value = 0
make var distance = 45.5

# Modify variables
change speed by 10
speed = 200
```

#### Control Structures

**If Statements:**
```kinetrix
if temperature > 30 {
    turn on 13
} else {
    turn off 13
}

# Logical operators
if speed > 50 and distance < 100 {
    print "Approaching target"
}
```

**Loops:**
```kinetrix
# Forever loop
forever {
    print "Running..."
    wait 1000
}

# Repeat loop
repeat 5 {
    turn on 13
    wait 200
    turn off 13
    wait 200
}

# While loop
make var counter = 0
while counter < 10 {
    print counter
    change counter by 1
}
```

#### Functions

```kinetrix
# Simple function
def blink_led {
    turn on 13
    wait 500
    turn off 13
    wait 500
}

# Function with parameters
def set_motor_speed(speed) {
    set pin 3 to speed
    set pin 5 to speed
}

# Function with return value
def calculate_distance(duration) {
    make var distance = duration / 58
    return distance
}
```

### Hardware Operations

#### GPIO Control

```kinetrix
# Digital output
turn on 13
turn off 13

# PWM output (0-255)
set pin 3 to 128

# Servo control (0-180 degrees)
servo pin 9 set 90
```

#### Sensor Input

```kinetrix
# Digital input
if pin 2 is high {
    print "Button pressed"
}

# Analog input (0-1023)
make var light_level = read analog 0

# Ultrasonic sensor
make var duration = read pulse pin 7
make var distance = duration / 58
```

#### Motor Control

```kinetrix
def forward {
    set pin 3 to 200
    set pin 5 to 200
}

def stop_motors {
    set pin 3 to 0
    set pin 5 to 0
}
```

#### Audio Output

```kinetrix
# Play tone
tone pin 8 freq 440  # A note (440 Hz)
wait 1000
notone pin 8

# Musical notes
tone pin 8 freq 261  # C (261 Hz)
tone pin 8 freq 294  # D (294 Hz)
tone pin 8 freq 329  # E (329 Hz)
```

### Advanced Features

#### Arrays

```kinetrix
# Declare array
make array temperatures size 5

# Set values
set array temperatures at 0 to 23.5
set array temperatures at 1 to 24.1

# Read values
make var temp = temperatures[0]
```

#### I2C Communication

```kinetrix
def init_mpu6050 {
    i2c begin
    i2c start 0x68
    i2c send 0x6B
    i2c send 0
    i2c stop
}

def read_gyro_data {
    i2c start 0x68
    i2c send 0x43
    i2c stop
    make var gyro_x = read i2c 0x68
    return gyro_x
}
```

#### Mathematical Operations

```kinetrix
# Basic arithmetic
make var sum = 10 + 5
make var product = 10 * 5

# Trigonometric functions
make var sine_val = sin(angle)
make var cosine_val = cos(angle)
make var tangent_val = tan(angle)

# Square root
make var root = sqrt(value)
```

## 🤖 Complete Examples

### Obstacle Avoidance Robot

```kinetrix
def measure_distance {
    make var duration = read pulse pin 7
    make var distance = duration / 58
    return distance
}

def forward {
    set pin 3 to 200
    set pin 5 to 200
}

def backward {
    set pin 3 to 150
    set pin 5 to 150
    wait 500
    set pin 3 to 0
    set pin 5 to 0
}

def turn_right {
    set pin 3 to 200
    set pin 5 to 0
    wait 400
}

program obstacle_avoidance {
    forever {
        make var dist = measure_distance
        
        if dist < 20 {
            backward
            turn_right
        } else {
            forward
        }
        
        wait 100
    }
}
```

### Line Following Robot

```kinetrix
program line_follower {
    forever {
        make var left_sensor = read analog 0
        make var right_sensor = read analog 1
        
        # Both sensors on white - go straight
        if left_sensor < 500 and right_sensor < 500 {
            set pin 3 to 200
            set pin 5 to 200
        }
        
        # Left sensor on black - turn left
        if left_sensor > 500 {
            set pin 3 to 100
            set pin 5 to 200
        }
        
        # Right sensor on black - turn right
        if right_sensor > 500 {
            set pin 3 to 200
            set pin 5 to 100
        }
        
        wait 50
    }
}
```

### Temperature Monitor with Alert

```kinetrix
def play_alert {
    tone pin 8 freq 1000
    wait 200
    notone pin 8
    wait 100
    tone pin 8 freq 1000
    wait 200
    notone pin 8
}

program temp_monitor {
    make var threshold = 500
    
    forever {
        make var temp = read analog 0
        print "Temperature:"
        print temp
        
        if temp > threshold {
            print "WARNING: High temperature!"
            play_alert
            turn on 13
        } else {
            turn off 13
        }
        
        wait 1000
    }
}
```

## 🔌 Pin Reference

### Arduino Pin Assignments

| Pin Type | Pins | Usage |
|----------|------|-------|
| Digital I/O | 2-13 | General purpose digital input/output |
| Analog Input | A0-A5 | Sensor inputs (0-1023 range) |
| PWM Output | 3, 5, 6, 9, 10, 11 | Motor control, LED brightness (0-255) |

### Common Pin Usage

- **Pin 13**: Built-in LED
- **Pins 3, 5**: Motor control
- **Pin 7**: Ultrasonic sensor trigger/echo
- **Pin 8**: Buzzer/speaker
- **Pin 9**: Servo motor
- **Analog 0-1**: Sensor inputs

## 📝 Best Practices

1. **Use descriptive variable names**
   ```kinetrix
   make var motor_speed = 200  # Good
   make var s = 200            # Bad
   ```

2. **Comment your code**
   ```kinetrix
   # Initialize sensors before main loop
   make var sensor_threshold = 500
   ```

3. **Create reusable functions**
   ```kinetrix
   def blink_times(count) {
       repeat count {
           turn on 13
           wait 200
           turn off 13
           wait 200
       }
   }
   ```

4. **Use appropriate delays**
   ```kinetrix
   wait 50     # Responsive
   wait 5000   # May cause lag
   ```

## 🛠️ Troubleshooting

### Common Issues

**Program doesn't start:**
- Ensure `program` block exists
- Check for syntax errors (missing braces)

**Unexpected behavior:**
- Verify pin numbers match hardware
- Check sensor wiring
- Add debug `print` statements

**Compilation errors:**
- Ensure keywords are lowercase
- Check for unclosed braces `{}`
- Verify function names match calls

## 📖 Language Reference

### Reserved Keywords

```
program, def, make, var, set, change, by, to
turn, on, off, wait, forever, if, else
repeat, while, break, return
read, analog, serial, pulse
servo, tone, notone, freq, print
pin, is, high, low
array, size, index, of
i2c, begin, start, send, stop
and, or
sin, cos, tan, sqrt, asin, acos, atan, atan2
```

### Data Types

Kinetrix uses dynamic typing with a unified numeric type (float). All variables are declared using:

```kinetrix
make var variable_name = value
```

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Built for makers and educators
- Inspired by the need for accessible robotics programming
- Designed to work seamlessly with Arduino and Raspberry Pi platforms

## 📧 Contact

Project Link: [https://github.com/yourusername/kinetrix](https://github.com/yourusername/kinetrix)

---

**Made with ❤️ for the robotics community**
