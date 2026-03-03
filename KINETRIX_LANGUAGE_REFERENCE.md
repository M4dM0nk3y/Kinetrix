# 🤖 Kinetrix Language Reference
### The Robotics Programming Language for Everyone

> **Version:** 3.0 | **Compiler:** `kcc` | **Target:** Arduino / Embedded Systems

---

## Table of Contents

1. [What is Kinetrix?](#what-is-kinetrix)
2. [Why Kinetrix? The Problem It Solves](#why-kinetrix)
3. [How It Compares to Other Languages](#comparison)
4. [Program Structure](#program-structure)
5. [Variables & Constants](#variables--constants)
6. [Data Types](#data-types)
7. [Operators](#operators)
8. [Control Flow](#control-flow)
9. [Loops](#loops)
10. [Functions](#functions)
11. [Hardware I/O](#hardware-io)
12. [Built-in Functions](#built-in-functions)
13. [Math Library](#math-library)
14. [Timing](#timing)
15. [Output & Debugging](#output--debugging)
16. [Complete Example Programs](#complete-example-programs)
17. [Error Reference](#error-reference)
18. [Compiler Usage](#compiler-usage)

---

## What is Kinetrix?

**Kinetrix** is a purpose-built programming language designed specifically for **robotics and embedded systems**. It compiles to Arduino-compatible C++ code, giving you the simplicity of a beginner-friendly language with the full power of Arduino hardware.

```
Kinetrix Source (.kx)  →  kcc compiler  →  Arduino C++ (.ino)  →  Upload to Hardware
```

### Core Philosophy
- **Human-readable syntax** — reads almost like English
- **Hardware-first design** — pins, sensors, motors are first-class citizens
- **Zero boilerplate** — no `setup()`, `loop()`, `#include`, or `void main()`
- **Safe by default** — prevents common hardware mistakes at compile time
- **Beginner-friendly** — learn robotics without learning C++ first

---

## Why Kinetrix?

### The Problem It Solves

| Problem | Without Kinetrix | With Kinetrix |
|---------|-----------------|---------------|
| Blink an LED | 12 lines of C++ | 4 lines of Kinetrix |
| Read a sensor | `analogRead(A0)` (confusing) | `read analog pin 0` (clear) |
| Motor control | Manual PWM math | `set pin 9 to 200` |
| Beginner barrier | Must learn C++ first | Start in 5 minutes |
| Hardware errors | Runtime crashes | Compile-time checks |

### What Value It Brings

1. **🎓 Education** — Perfect for teaching robotics to students aged 10+
2. **⚡ Speed** — Write robotics code 5x faster than raw Arduino C++
3. **🔒 Safety** — Compiler catches hardware errors before they damage components
4. **📖 Readability** — Code reads like instructions, not cryptic symbols
5. **🔧 Power** — Full access to Arduino hardware capabilities underneath
6. **🌍 Accessibility** — No prior programming experience required

---

## Comparison

### Kinetrix vs Python

| Feature | Python | Kinetrix |
|---------|--------|----------|
| Runs on Arduino | ❌ No | ✅ Yes |
| Hardware control | Needs libraries | Built-in |
| Speed | Slow (interpreted) | Fast (compiled C++) |
| Memory usage | High | Ultra-low |
| Robotics syntax | Generic | Purpose-built |
| Real-time control | ❌ No | ✅ Yes |

```python
# Python (does NOT run on Arduino)
import time
import RPi.GPIO as GPIO
GPIO.setup(13, GPIO.OUT)
GPIO.output(13, GPIO.HIGH)
time.sleep(1)
GPIO.output(13, GPIO.LOW)
```

```kinetrix
# Kinetrix (runs on Arduino directly!)
program {
    turn on pin 13
    wait 1000
    turn off pin 13
}
```

---

### Kinetrix vs C++ (Arduino)

| Feature | Arduino C++ | Kinetrix |
|---------|-------------|----------|
| Lines to blink LED | ~12 | ~4 |
| Syntax complexity | High | Low |
| Beginner-friendly | ❌ No | ✅ Yes |
| Performance | Native | Native (same output) |
| Hardware access | Full | Full |
| Boilerplate | Required | None |

```cpp
// Arduino C++ — verbose and confusing for beginners
#include <Arduino.h>
void setup() {
    pinMode(13, OUTPUT);
    Serial.begin(9600);
}
void loop() {
    int sensor = analogRead(A0);
    int speed = map(sensor, 0, 1023, 0, 255);
    analogWrite(9, speed);
    delay(10);
}
```

```kinetrix
# Kinetrix — clean and readable
program {
    loop forever {
        make var sensor = read analog pin 0
        make var speed = map(sensor, 0, 1023, 0, 255)
        set pin 9 to speed
        wait 10
    }
}
```

---

### Kinetrix vs Scratch/Block-based

| Feature | Scratch | Kinetrix |
|---------|---------|----------|
| Real hardware | ❌ No | ✅ Yes |
| Text-based | ❌ No | ✅ Yes |
| Professional skills | ❌ No | ✅ Yes |
| Beginner-friendly | ✅ Yes | ✅ Yes |
| Scalability | Limited | Full |

---

## Program Structure

Every Kinetrix program is wrapped in a `program { }` block:

```kinetrix
program {
    # Your code goes here
    # Comments start with #
}
```

The compiler automatically generates:
- `#include <Wire.h>` (hardware libraries)
- `void setup()` (initialization)
- `void loop()` (main loop)

You never write these yourself.

---

## Variables & Constants

### Variables

```kinetrix
make var name = value
```

**Examples:**
```kinetrix
make var speed = 150
make var temperature = 36.5
make var message = "hello"
make var running = true
make var sensor = read analog pin 0
```

### Constants

```kinetrix
const NAME = value
```

**Examples:**
```kinetrix
const LED_PIN = 13
const MAX_SPEED = 255
const MIN_SPEED = 0
const THRESHOLD = 512
```

Constants are values that don't change. Use them to make code readable.

### Changing Variables

```kinetrix
# Increment/decrement
change speed by 10       # speed = speed + 10
change speed by -5       # speed = speed - 5

# Direct assignment (inside functions or loops)
set speed to 200
```

---

## Data Types

Kinetrix handles types automatically — you don't need to declare them:

| Type | Example | Notes |
|------|---------|-------|
| Integer | `make var x = 10` | Whole numbers |
| Float | `make var x = 3.14` | Decimal numbers |
| Boolean | `make var x = true` | `true` or `false` |
| String | `make var x = "hello"` | Text in quotes |

---

## Operators

### Arithmetic Operators

| Operator | Symbol | Example | Result |
|----------|--------|---------|--------|
| Add | `+` | `5 + 3` | `8` |
| Subtract | `-` | `10 - 4` | `6` |
| Multiply | `*` | `6 * 7` | `42` |
| Divide | `/` | `10 / 2` | `5` |
| Modulo | `%` | `10 % 3` | `1` |

```kinetrix
make var result = (speed * 2) + offset
make var remainder = count % 4
make var average = (a + b) / 2
```

### Comparison Operators

| Operator | Meaning | Example |
|----------|---------|---------|
| `==` | Equal to | `if x == 10` |
| `!=` | Not equal to | `if x != 0` |
| `<` | Less than | `if speed < 100` |
| `>` | Greater than | `if sensor > 500` |
| `<=` | Less than or equal | `if count <= 10` |
| `>=` | Greater than or equal | `if temp >= 37` |

### Logical Operators

| Operator | Meaning | Example |
|----------|---------|---------|
| `and` | Both must be true | `if x > 0 and x < 100` |
| `or` | At least one true | `if button == 1 or sensor > 500` |
| `not` | Invert condition | `if not (x == 0)` |

```kinetrix
if speed > 50 and speed < 200 {
    print "normal speed"
}

if sensor < 100 or sensor > 900 {
    print "out of range"
}

if not (button == 0) {
    turn on pin 13
}
```

---

## Control Flow

### If / Else

```kinetrix
if condition {
    # runs if condition is true
}
```

```kinetrix
if condition {
    # runs if true
} else {
    # runs if false
}
```

### Else-If Chains

```kinetrix
if speed < 30 {
    print "slow"
} else if speed < 70 {
    print "medium"
} else if speed < 150 {
    print "fast"
} else {
    print "very fast"
}
```

You can chain as many `else if` blocks as you need.

---

## Loops

### Loop Forever

Runs continuously — perfect for the main robot loop:

```kinetrix
loop forever {
    # This runs forever
    make var sensor = read analog pin 0
    set pin 9 to sensor / 4
    wait 10
}
```

### Repeat N Times

```kinetrix
repeat 5 {
    turn on pin 13
    wait 500
    turn off pin 13
    wait 500
}
```

### While Loop

Runs while a condition is true:

```kinetrix
make var count = 0
while count < 10 {
    print count
    change count by 1
    wait 100
}
```

### For Loop

Counts from a start value to an end value:

```kinetrix
for i from 0 to 9 {
    set pin 9 to i * 28
    wait 100
}
```

```kinetrix
# Count backwards using while
make var i = 10
while i >= 0 {
    print i
    change i by -1
}
```

### Break

Exit a loop early:

```kinetrix
make var count = 0
loop forever {
    change count by 1
    if count >= 10 {
        break
    }
}
```

---

## Functions

Define reusable blocks of code with `def`:

```kinetrix
def functionName(param1, param2) {
    # function body
}
```

### Defining Functions

```kinetrix
def blink(pin, times) {
    repeat times {
        turn on pin pin
        wait 500
        turn off pin pin
        wait 500
    }
}

def flash_sos() {
    repeat 3 { blink(13, 1) }
    repeat 3 {
        turn on pin 13
        wait 300
        turn off pin 13
        wait 300
    }
    repeat 3 { blink(13, 1) }
}
```

### Calling Functions

```kinetrix
program {
    def blink(pin, times) {
        repeat times {
            turn on pin pin
            wait 500
            turn off pin pin
            wait 500
        }
    }

    loop forever {
        blink(13, 3)
        blink(12, 5)
        wait 1000
    }
}
```

### Return Values

```kinetrix
def calculate_speed(sensor_val) {
    make var speed = map(sensor_val, 0, 1023, 0, 255)
    return speed
}
```

---

## Hardware I/O

### Digital Output (LEDs, Relays, Buzzers)

```kinetrix
turn on pin 13        # Set pin HIGH (5V)
turn off pin 13       # Set pin LOW (0V)
```

### PWM Output (Motors, Servos, LED brightness)

```kinetrix
set pin 9 to 128      # 50% duty cycle (0-255)
set pin 10 to 255     # Full speed
set pin 11 to 0       # Off
```

PWM pins on Arduino: **3, 5, 6, 9, 10, 11**

### Digital Input (Buttons, Switches, Digital Sensors)

```kinetrix
make var button = read pin 2      # Returns 0 or 1
make var sensor = read pin 7      # Returns 0 or 1

if button == 1 {
    turn on pin 13
}
```

### Analog Input (Potentiometers, LDRs, IR Sensors, Temperature)

```kinetrix
make var value = read analog pin 0    # Returns 0-1023 (A0)
make var light = read analog pin 1    # Returns 0-1023 (A1)
make var temp  = read analog pin 2    # Returns 0-1023 (A2)
```

Analog pins: **A0 (pin 0), A1 (pin 1), A2 (pin 2), A3 (pin 3), A4 (pin 4), A5 (pin 5)**

### Servo Control

```kinetrix
set servo pin 9 to 90     # Set servo to 90 degrees (0-180)
set servo pin 10 to 0     # Full left
set servo pin 11 to 180   # Full right
```

### Tone / Buzzer

```kinetrix
tone pin 8 to 440     # Play 440Hz (A note) on pin 8
notone pin 8          # Stop tone
```

### I2C Communication

```kinetrix
i2c begin
i2c start 0x3C        # Start transmission to address 0x3C
i2c send 0x00         # Send byte
i2c stop              # End transmission
```

---

## Built-in Functions

These functions are available anywhere in your code:

### map(value, fromLow, fromHigh, toLow, toHigh)

Re-maps a number from one range to another. **Essential for robotics.**

```kinetrix
# Convert sensor reading (0-1023) to motor speed (0-255)
make var speed = map(sensor, 0, 1023, 0, 255)

# Convert sensor to angle (0-180 degrees)
make var angle = map(sensor, 0, 1023, 0, 180)

# Invert a value
make var inverted = map(value, 0, 255, 255, 0)
```

### constrain(value, min, max)

Clamps a value between a minimum and maximum. Prevents hardware damage.

```kinetrix
# Never exceed safe motor speed
make var safe_speed = constrain(speed, 0, 200)

# Keep servo in valid range
make var angle = constrain(target, 0, 180)
```

### abs(value)

Returns the absolute value (removes negative sign):

```kinetrix
make var distance = abs(target - current)
make var error = abs(sensor - setpoint)
```

### min(a, b) and max(a, b)

Returns the smaller or larger of two values:

```kinetrix
make var slower = min(speed1, speed2)
make var faster = max(speed1, speed2)
make var capped = min(speed, 200)      # Cap at 200
```

### random(min, max)

Returns a random number between min and max:

```kinetrix
make var rand_speed = random(50, 200)
make var rand_pin = random(2, 13)
```

---

## Math Library

All standard math functions are available:

| Function | Description | Example |
|----------|-------------|---------|
| `sin(x)` | Sine | `make var s = sin(angle)` |
| `cos(x)` | Cosine | `make var c = cos(angle)` |
| `tan(x)` | Tangent | `make var t = tan(angle)` |
| `sqrt(x)` | Square root | `make var r = sqrt(x*x + y*y)` |
| `asin(x)` | Arc sine | `make var a = asin(0.5)` |
| `acos(x)` | Arc cosine | `make var a = acos(0.5)` |
| `atan(x)` | Arc tangent | `make var a = atan(y / x)` |
| `abs(x)` | Absolute value | `make var d = abs(error)` |

```kinetrix
# Calculate distance using Pythagorean theorem
make var dx = x2 - x1
make var dy = y2 - y1
make var distance = sqrt(dx * dx + dy * dy)

# Sine wave for smooth LED fading
make var brightness = sin(t) * 127 + 128
set pin 9 to brightness
```

---

## Timing

### Wait (Milliseconds)

Pause execution for N milliseconds:

```kinetrix
wait 1000     # Wait 1 second
wait 500      # Wait 0.5 seconds
wait 100      # Wait 0.1 seconds
wait 10       # Wait 10 milliseconds
```

### Wait Microseconds

For precise, high-speed timing:

```kinetrix
wait_us 500   # Wait 500 microseconds (0.5ms)
wait_us 100   # Wait 100 microseconds
```

Use `wait_us` for:
- Ultrasonic sensor pulse timing
- High-frequency PWM
- Precise stepper motor control

---

## Output & Debugging

### Print to Serial Monitor

```kinetrix
print "Hello, Robot!"       # Print string
print speed                 # Print variable
print sensor                # Print sensor value
```

Open Arduino IDE → Tools → Serial Monitor (9600 baud) to see output.

---

## Complete Example Programs

### 1. LED Blink

```kinetrix
program {
    loop forever {
        turn on pin 13
        wait 1000
        turn off pin 13
        wait 1000
    }
}
```

### 2. Button-Controlled LED

```kinetrix
program {
    loop forever {
        make var button = read pin 2
        if button == 1 {
            turn on pin 13
        } else {
            turn off pin 13
        }
    }
}
```

### 3. Potentiometer-Controlled LED Brightness

```kinetrix
program {
    loop forever {
        make var sensor = read analog pin 0
        make var brightness = map(sensor, 0, 1023, 0, 255)
        set pin 9 to brightness
        wait 10
    }
}
```

### 4. Traffic Light

```kinetrix
program {
    loop forever {
        turn on pin 13      # Red
        wait 3000
        turn off pin 13
        turn on pin 12      # Yellow
        wait 1000
        turn off pin 12
        turn on pin 11      # Green
        wait 3000
        turn off pin 11
    }
}
```

### 5. Line Follower Robot

```kinetrix
program {
    loop forever {
        make var left  = read analog pin 0
        make var right = read analog pin 1

        if left > 500 and right > 500 {
            # Both on line - go forward
            set pin 9 to 200
            set pin 10 to 200
        } else if left < 500 and right > 500 {
            # Drift left - turn right
            set pin 9 to 200
            set pin 10 to 50
        } else if left > 500 and right < 500 {
            # Drift right - turn left
            set pin 9 to 50
            set pin 10 to 200
        } else {
            # Lost line - stop
            set pin 9 to 0
            set pin 10 to 0
        }
        wait 10
    }
}
```

### 6. Sensor-Controlled Motor with Safety

```kinetrix
program {
    const MAX_SPEED = 200
    const MIN_SPEED = 0

    def safe_drive(raw_speed) {
        make var speed = constrain(raw_speed, MIN_SPEED, MAX_SPEED)
        set pin 9 to speed
    }

    loop forever {
        make var sensor = read analog pin 0
        make var target = map(sensor, 0, 1023, 0, 255)
        safe_drive(target)
        wait 20
    }
}
```

### 7. PWM Fade (Breathing LED)

```kinetrix
program {
    loop forever {
        for i from 0 to 255 {
            set pin 9 to i
            wait 5
        }
        for i from 255 to 0 {
            set pin 9 to i
            wait 5
        }
    }
}
```

### 8. Ultrasonic Distance Sensor

```kinetrix
program {
    loop forever {
        # Trigger pulse
        turn on pin 9
        wait_us 10
        turn off pin 9

        # Read echo (simplified)
        make var echo = read pin 10
        if echo == 1 {
            print "Object detected!"
            set pin 11 to 200    # Slow down motor
        } else {
            set pin 11 to 255    # Full speed
        }
        wait 50
    }
}
```

### 9. Function-Based Robot

```kinetrix
program {
    def forward(speed) {
        set pin 9 to speed
        set pin 10 to speed
    }

    def turn_left(speed) {
        set pin 9 to 0
        set pin 10 to speed
    }

    def turn_right(speed) {
        set pin 9 to speed
        set pin 10 to 0
    }

    def stop_robot() {
        set pin 9 to 0
        set pin 10 to 0
    }

    loop forever {
        forward(200)
        wait 2000
        turn_left(150)
        wait 500
        forward(200)
        wait 2000
        turn_right(150)
        wait 500
        stop_robot()
        wait 1000
    }
}
```

### 10. 4-Bit Binary Counter

```kinetrix
program {
    make var count = 0
    loop forever {
        # Display count in binary on pins 10-13
        if count % 2 == 1 { turn on pin 10 } else { turn off pin 10 }
        if (count / 2) % 2 == 1 { turn on pin 11 } else { turn off pin 11 }
        if (count / 4) % 2 == 1 { turn on pin 12 } else { turn off pin 12 }
        if (count / 8) % 2 == 1 { turn on pin 13 } else { turn off pin 13 }

        change count by 1
        if count >= 16 { change count by -16 }
        wait 500
    }
}
```

---

## Error Reference

### Common Errors and Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| `Unexpected token` | Typo in keyword | Check spelling |
| `Expected '{'` | Missing opening brace | Add `{` after condition/loop |
| `Expected '}'` | Missing closing brace | Add `}` at end of block |
| `Compilation failed` | Syntax error | Check the line number reported |

### Common Mistakes

```kinetrix
# ❌ WRONG - missing 'var'
make x = 10

# ✅ CORRECT
make var x = 10

# ❌ WRONG - missing 'pin' keyword
turn on 13

# ✅ CORRECT
turn on pin 13

# ❌ WRONG - no braces
if x > 10
    print "big"

# ✅ CORRECT
if x > 10 {
    print "big"
}
```

---

## Compiler Usage

### Build the Compiler

```bash
cd ~/Downloads/Kinetrix_Release
gcc -o kcc compiler_v3.c parser.c codegen.c ast.c error.c pin_tracker.c symbol_table.c diagnostics.c -lm
```

### Compile a Program

```bash
./kcc myprogram.kx -o myprogram.ino
```

### Upload to Arduino

1. Open `myprogram.ino` in Arduino IDE
2. Select your board: **Tools → Board → Arduino Uno**
3. Select your port: **Tools → Port → /dev/cu.usbmodem...**
4. Click **Upload** (→ button)

### Run All Tests

```bash
./test_all_features.sh
```

---

## Quick Syntax Cheatsheet

```kinetrix
# Variables
make var x = 10
const MAX = 255

# Math
make var result = (a + b) * c / d

# Conditions
if x > 10 and y < 5 {
} else if x == 0 {
} else {
}

# Loops
loop forever { }
repeat 10 { }
while x < 100 { }
for i from 0 to 9 { }

# Hardware
turn on pin 13
turn off pin 13
set pin 9 to 200
make var s = read analog pin 0
make var b = read pin 2

# Functions
def myFunc(a, b) {
    return a + b
}
myFunc(10, 20)

# Built-ins
map(val, 0, 1023, 0, 255)
constrain(val, 0, 200)
abs(error)
min(a, b)
max(a, b)
random(0, 100)

# Math
sqrt(x)  sin(x)  cos(x)  tan(x)

# Timing
wait 1000
wait_us 500

# Output
print "hello"
print variable
```

---

## Language Summary

| Category | Count | Features |
|----------|-------|---------|
| Operators | 11 | `+` `-` `*` `/` `%` `==` `!=` `<` `>` `<=` `>=` |
| Logic | 3 | `and` `or` `not` |
| Loops | 4 | `loop forever` `repeat` `while` `for` |
| Control | 3 | `if` `else if` `else` |
| Hardware | 6 | GPIO, PWM, analog read, digital read, servo, tone |
| Built-ins | 6 | `map` `constrain` `abs` `min` `max` `random` |
| Math | 7 | `sin` `cos` `tan` `sqrt` `asin` `acos` `atan` |
| Functions | ✅ | `def`, parameters, return values |
| Types | 4 | int, float, bool, string |
| Timing | 2 | `wait` (ms), `wait_us` (µs) |

---

*Kinetrix — Making Robotics Accessible to Everyone* 🤖

*Compiler: kcc v3.0 | Language: Kinetrix | Target: Arduino/AVR*
