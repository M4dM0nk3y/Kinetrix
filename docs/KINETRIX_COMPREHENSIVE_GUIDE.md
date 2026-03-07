# Kinetrix — Comprehensive Language Guide

> **Version 3.1** | A compiled robotics programming language
> Created by Soham Mulik

---

## Table of Contents

1. [What is Kinetrix?](#what-is-kinetrix)
2. [How the Compiler Works](#how-the-compiler-works)
3. [Compilation Pipeline](#compilation-pipeline)
4. [Language Syntax — Complete Reference](#language-syntax--complete-reference)
5. [Multi-Target Code Generation](#multi-target-code-generation)
6. [OTA Fleet Updates](#ota-fleet-updates)
7. [Standard Library](#standard-library)
8. [Error Handling & Diagnostics](#error-handling--diagnostics)
9. [Architecture Deep Dive](#architecture-deep-dive)

---

## What is Kinetrix?

Kinetrix is a **compiled programming language** designed specifically for robotics and embedded systems. You write human-readable code like `turn on pin 13` and the compiler translates it into native C/C++ or Python for your target board.

### Key Properties

| Property | Value |
|----------|-------|
| **Paradigm** | Imperative, concurrent (task-based) |
| **Typing** | Static (number, string, bool) |
| **Execution** | Compiled (source → native code) |
| **Targets** | Arduino, ESP32, Raspberry Pi, Pico, ROS2 |
| **File Extension** | `.kx` |
| **Compiler Binary** | `kcc` |

### What Makes It Different

```
Traditional (C++):                    Kinetrix:
──────────────────                    ─────────
digitalWrite(13, HIGH);               turn on pin 13
delay(1000);                          wait 1000
analogWrite(9, 128);                  set pin 9 to 128
if (digitalRead(2) == HIGH) {         if read pin 2 == 1 {
   Serial.println("pressed");            println "pressed"
}                                     }
```

---

## How the Compiler Works

The Kinetrix compiler (`kcc`) transforms your `.kx` source file into target-specific output through a **4-stage pipeline**:

```
┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   STAGE 1    │    │   STAGE 2    │    │   STAGE 3    │    │   STAGE 4    │
│              │    │              │    │              │    │              │
│    LEXER     │───▶│    PARSER    │───▶│  ANALYZER    │───▶│   CODEGEN    │
│  (Tokenize)  │    │  (Build AST) │    │  (Validate)  │    │  (Emit Code) │
│              │    │              │    │              │    │              │
│  Source text  │    │  Token stream│    │  Valid AST   │    │  Target code │
│  → Tokens    │    │  → AST tree  │    │  + warnings  │    │  (.cpp/.py)  │
└──────────────┘    └──────────────┘    └──────────────┘    └──────────────┘
```

### Stage 1: Lexer (Tokenizer)

**File:** `parser.c` (integrated lexer) | **File:** `parser.h` (token definitions)

The lexer reads raw source text character-by-character and breaks it into **tokens** — the smallest meaningful units of the language.

**Example input:**
```kinetrix
turn on pin 13
```

**Lexer output (token stream):**
```
TOK_TURN  →  "turn"
TOK_ON    →  "on"
TOK_PIN   →  "pin"
TOK_NUMBER → "13"
```

**Token categories defined in `parser.h`:**

| Category | Examples |
|----------|---------|
| **Keywords** | `program`, `make`, `if`, `else`, `while`, `for`, `loop`, `forever`, `turn`, `on`, `off`, `pin` |
| **Types** | `number`, `string`, `bool` |
| **Operators** | `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `and`, `or`, `not` |
| **Literals** | `42`, `3.14`, `"hello"`, `true`, `false` |
| **Identifiers** | Variable names, function names |
| **Punctuation** | `{`, `}`, `(`, `)`, `,` |

**Context-sensitive keywords:** Words like `count`, `size`, `value`, `ota`, `password` are treated as identifiers in normal contexts but recognized as keywords when they appear in specific syntactic positions. This allows you to use them as variable names without conflicts.

### Stage 2: Parser (AST Construction)

**File:** `parser.c` (~2200 lines) | **Structures:** `ast.h`, `ast.c`

The parser reads the token stream and builds an **Abstract Syntax Tree (AST)** — a tree representation of your program's structure.

**Example input:**
```kinetrix
program {
    make number x = 10
    if x > 5 {
        turn on pin 13
    }
}
```

**Parser output (AST):**
```
NODE_PROGRAM
└── NODE_BLOCK
    ├── NODE_VAR_DECL (name="x", type=number)
    │   └── NODE_NUMBER (value=10)
    └── NODE_IF
        ├── condition: NODE_BINARY_OP (op=GT)
        │   ├── left: NODE_IDENTIFIER (name="x")
        │   └── right: NODE_NUMBER (value=5)
        └── then_block: NODE_BLOCK
            └── NODE_GPIO_WRITE (pin=13, value=1)
```

**AST Node Types (defined in `ast.h`):**

The compiler defines ~70 AST node types covering every language construct:

| Category | Node Types |
|----------|-----------|
| **Program structure** | `NODE_PROGRAM`, `NODE_BLOCK` |
| **Variables** | `NODE_VAR_DECL`, `NODE_ASSIGNMENT`, `NODE_ARRAY_DECL`, `NODE_BUFFER_DECL`, `NODE_SHARED_DECL` |
| **Expressions** | `NODE_NUMBER`, `NODE_STRING`, `NODE_BOOL`, `NODE_IDENTIFIER`, `NODE_BINARY_OP`, `NODE_UNARY_OP`, `NODE_CALL` |
| **Control flow** | `NODE_IF`, `NODE_WHILE`, `NODE_FOR`, `NODE_REPEAT`, `NODE_FOREVER`, `NODE_BREAK`, `NODE_CONTINUE`, `NODE_RETURN` |
| **GPIO/Hardware** | `NODE_GPIO_WRITE`, `NODE_GPIO_READ`, `NODE_ANALOG_READ`, `NODE_ANALOG_WRITE`, `NODE_PULSE_READ` |
| **Communication** | `NODE_I2C_OPEN`, `NODE_I2C_READ`, `NODE_SPI_OPEN`, `NODE_SPI_TRANSFER`, `NODE_SERIAL_OPEN`, `NODE_SERIAL_SEND`, `NODE_SERIAL_RECV` |
| **Concurrency** | `NODE_TASK_DEF`, `NODE_TASK_START` |
| **Interrupts** | `NODE_INTERRUPT_PIN`, `NODE_INTERRUPT_TIMER`, `NODE_ENABLE_INTERRUPTS`, `NODE_DISABLE_INTERRUPTS` |
| **Safety** | `NODE_WATCHDOG_ENABLE`, `NODE_WATCHDOG_FEED`, `NODE_TRY`, `NODE_ASSERT` |
| **OTA** | `NODE_OTA_ENABLE` |
| **Wireless** | `NODE_RADIO_SEND`, `NODE_RADIO_READ`, `NODE_RADIO_AVAILABLE` |
| **Audio** | `NODE_TONE`, `NODE_NOTONE` |
| **Data types** | `NODE_STRUCT_DEF`, `NODE_STRUCT_INSTANCE`, `NODE_STRUCT_ACCESS`, `NODE_ARRAY_ACCESS`, `NODE_CAST`, `NODE_MATH_FUNC` |
| **Functions** | `NODE_FUNCTION_DEF` |

### Stage 3: Semantic Analysis

**Files:** `symbol_table.c`, `pin_tracker.c`, `diagnostics.c`

After parsing, the compiler performs validation:

- **Symbol table** tracks all declared variables, functions, and their types
- **Pin tracker** detects conflicts (e.g., same pin used as input and output)
- **Diagnostics** reports warnings for unused variables, unreachable code, etc.

### Stage 4: Code Generation

**Files:** `codegen.c`, `codegen_esp32.c`, `codegen_rpi.c`, `codegen_pico.c`, `codegen_ros2.c`

The code generator walks the AST and emits target-specific code. Each target has its own generator:

| File | Target | Output Language |
|------|--------|----------------|
| `codegen.c` | Arduino Uno/Nano/Mega | C++ (Arduino) |
| `codegen_esp32.c` | ESP32 | C++ (Arduino + ESP-IDF) |
| `codegen_rpi.c` | Raspberry Pi | Python 3 (RPi.GPIO) |
| `codegen_pico.c` | Raspberry Pi Pico | MicroPython |
| `codegen_ros2.c` | ROS2 | C++ (rclcpp) |

**Same Kinetrix code → different output per target:**

```kinetrix
turn on pin 13
```

| Target | Generated Code |
|--------|---------------|
| Arduino | `digitalWrite(13, HIGH);` |
| ESP32 | `digitalWrite(13, HIGH);` |
| RPi | `GPIO.output(13, 1)` |
| Pico | `Pin(13, Pin.OUT).value(1)` |
| ROS2 | `// GPIO via sysfs or pigpio` |

---

## Compilation Pipeline

### Command Line Interface

```bash
# Basic usage
./kcc source.kx --target <target> -o output_file

# Targets: arduino (default), esp32, rpi, pico, ros2
./kcc robot.kx                              # Arduino output
./kcc robot.kx --target esp32 -o robot.cpp  # ESP32 output
./kcc robot.kx --target rpi -o robot.py     # Raspberry Pi output
./kcc robot.kx --target pico -o robot.py    # Pico output
./kcc robot.kx --target ros2 -o robot.cpp   # ROS2 output

# Version
./kcc --version
```

### What the Compiler Does Internally

```
1. Read source file into memory
2. Initialize lexer with source text
3. Parse tokens → build AST
4. Run symbol table analysis
5. Run pin tracking & diagnostics
6. Select code generator based on --target
7. Walk AST → emit target code to output file
8. Report success or errors
```

### Build System

```bash
# Build the compiler from source
make              # Builds 'kcc' binary

# Clean build artifacts
make clean        # Removes .o files and kcc binary

# Run tests
./test_all.sh     # 48 automated tests
```

**Makefile targets:**
- `make` → compiles all `.c` files → links into `kcc`
- `make clean` → removes `.o` and `kcc`

---

## Language Syntax — Complete Reference

### Program Structure

Every Kinetrix program starts with the `program` block:

```kinetrix
program {
    # Your code here
}
```

### Comments

```kinetrix
# This is a single-line comment
```

### Variables

```kinetrix
# Declare with type
make number x = 10
make string name = "robot"
make bool active = true

# Constants
const number MAX_SPEED = 255

# Update
set x to 20
change x by 5       # x = x + 5
```

### Data Types

| Type | Description | Examples |
|------|------------|---------|
| `number` | Integer or floating-point | `42`, `3.14`, `-7` |
| `string` | Text | `"hello"`, `"robot1"` |
| `bool` | Boolean | `true`, `false` |

### Operators

| Category | Operators |
|----------|----------|
| **Arithmetic** | `+`, `-`, `*`, `/`, `%` |
| **Comparison** | `==`, `!=`, `<`, `>`, `<=`, `>=` |
| **Logical** | `and`, `or`, `not` |
| **String** | `+` (concatenation) |

### Control Flow

#### If / Else If / Else

```kinetrix
if temperature > 40 {
    println "TOO HOT"
} else if temperature < 10 {
    println "TOO COLD"
} else {
    println "JUST RIGHT"
}
```

#### While Loop

```kinetrix
while distance > 20 {
    set pin 9 to 200   # Drive forward
    wait 100
}
```

#### For Loop

```kinetrix
# Basic
for i from 0 to 180 {
    set pin 9 to i
    wait 20
}

# With step
for angle from 180 to 0 step -5 {
    set pin 9 to angle
    wait 10
}
```

#### Repeat Loop

```kinetrix
repeat 10 {
    turn on pin 13
    wait 100
    turn off pin 13
    wait 100
}
```

#### Forever Loop (Main Loop)

```kinetrix
loop forever {
    # This runs continuously — like Arduino's loop()
    turn on pin 13
    wait 1000
    turn off pin 13
    wait 1000
}
```

#### Break & Continue

```kinetrix
loop forever {
    make number val = read analog pin 34
    if val > 900 {
        break           # Exit loop
    }
    if val < 100 {
        continue        # Skip to next iteration
    }
    println val
}
```

### Functions

```kinetrix
# Function with parameters and return value
define function clamp(val number, lo number, hi number) returns number {
    if val < lo { return lo }
    if val > hi { return hi }
    return val
}

# Function without return
define function emergency_stop() {
    turn off pin 9
    turn off pin 10
    println "STOPPED"
}

# Calling functions
make number safe_speed = clamp(speed, 0, 255)
emergency_stop()
```

### GPIO (General Purpose Input/Output)

```kinetrix
# Digital output
turn on pin 13           # HIGH
turn off pin 13          # LOW

# Digital read
make number btn = read pin 2

# PWM / Analog output (0-255)
set pin 9 to 128         # 50% duty cycle

# Analog read (0-1023)
make number sensor = read analog pin 34
```

### Structs (Custom Types)

```kinetrix
# Define a struct
define type SensorData {
    number temperature
    number humidity
    number pressure
}

# Create an instance
make SensorData reading

# Access and set fields
reading.temperature = 25
reading.humidity = 60
println reading.temperature
```

### Arrays

```kinetrix
# Fixed-size array
make array readings size 10

# Access elements
readings[0] = 42
make number first = readings[0]

# Use in loops
for i from 0 to 9 {
    readings[i] = read analog pin 34
    wait 100
}
```

### Ring Buffers

```kinetrix
# Auto-evicting circular buffer
make buffer history size 5

# Push values (oldest auto-removed when full)
push sensor_value into history
```

### Communication Protocols

#### Serial / UART

```kinetrix
open serial at 115200

send serial "Hello World"
send serial sensor_value

make number received = receive serial
```

#### I2C

```kinetrix
open i2c

# Read a register from a device
make number temp = read i2c device 0x68 register 0x41

# Read multiple bytes into array
read i2c device 0x68 register 0x3B count 6 into data_array

# Write to a device
write i2c device 0x68 value 0x00
```

#### SPI

```kinetrix
open spi at 1000000     # 1 MHz clock

make number result = spi transfer 0xFF
```

### Library Wrappers (V3.1)

Kinetrix V3.1 introduces built-in cross-platform support for common hardware modules. The compiler automatically includes the correct underlying libraries (e.g. `Servo.h` or `RPi.GPIO`) based on the target platform.

#### Servo Motors
```kinetrix
attach servo pin 9
move servo to 90
wait 1000
detach servo pin 9
```

#### Ultrasonic Distance Sensors (HC-SR04)
```kinetrix
# Returns distance in cm
make float dist = read distance trigger 12 echo 11
```

#### DHT Temperature & Humidity Sensors
```kinetrix
attach dht11 pin 4    # Or use dht22
make float temp = read temperature
make float humid = read humidity
```

#### NeoPixel LED Strips (WS2812B)
```kinetrix
attach strip pin 6 count 30
set pixel 0 to 255 0 0    # Red
set pixel 1 to 0 255 0    # Green
set pixel 2 to 0 0 255    # Blue
show pixels
wait 1000
clear pixels
```

#### I2C LCD Displays
```kinetrix
attach lcd columns 16 rows 2
lcd print "Hello Robot" line 0
lcd print "Sensors OK" line 1
wait 2000
lcd clear
```

### Interrupts

```kinetrix
# Pin interrupt — fires instantly when pin state changes
on interrupt pin 2 rising {
    # This code runs immediately when pin 2 goes HIGH
    turn on pin 13
}

on interrupt pin 3 falling {
    # This code runs when pin 3 goes LOW
    turn off pin 13
}

# Timer interrupt — fires at regular intervals
on timer every 5000ms {
    # This runs every 5 seconds
    feed watchdog
}
```

### Concurrent Tasks

```kinetrix
# Shared variable (thread-safe access across tasks)
shared make number current_speed = 0

# Define tasks
task sensor_reader {
    current_speed = read analog pin 34
    wait 100
}

task motor_controller {
    set pin 9 to current_speed
    wait 50
}

# Start tasks (run concurrently)
start task sensor_reader
start task motor_controller
```

**How tasks compile per target:**

| Target | Implementation |
|--------|---------------|
| Arduino | Sequential in `loop()` (single-threaded) |
| ESP32 | FreeRTOS `xTaskCreate()` (true multi-core) |
| RPi | Python `threading.Thread()` |
| Pico | `_thread.start_new_thread()` |

### Safety Features

#### Watchdog Timer

```kinetrix
# Auto-restart if code freezes for >5 seconds
enable watchdog timeout 5000ms

loop forever {
    # Your code
    feed watchdog    # Reset the timer — "I'm still alive"
}
```

#### Error Handling

```kinetrix
try {
    make number val = read i2c device 0x68 register 0x3B
    println val
} on error {
    println "Sensor disconnected!"
}
```

#### Assertions

```kinetrix
# If condition is false, execute the action
assert motor_speed < 255 else emergency_stop()
assert battery_level > 10 else println "LOW BATTERY"
```

### Audio

```kinetrix
# Play tone on buzzer pin
tone pin 8 at 1000       # 1000 Hz tone
wait 500
notone pin 8              # Stop tone
```

### Math Functions

```kinetrix
make number s = sin(angle)
make number c = cos(angle)
make number t = tan(angle)
make number r = sqrt(value)
make number a = atan2(y, x)
make number b = asin(ratio)
make number d = acos(ratio)
```

### Wireless Communication (ESP-NOW)

```kinetrix
# Send data to peer robot
radio send 0 sensor_value

# Check if data available
if radio available {
    make number msg = radio read
    println msg
}
```

### Module System

```kinetrix
# Include another .kx file
include "libs/sonar.kx"
import "libs/drive.kx"    # 'import' is an alias for 'include'
```

### Type Casting

```kinetrix
make number x = 3.14
make number y = (int) x     # y = 3
make number z = (float) 42  # z = 42.0
```

---

## Multi-Target Code Generation

### How Code Generation Works

Each code generator walks the same AST but emits different output. Here's how a simple program transforms:

**Kinetrix Source:**
```kinetrix
program {
    open serial at 115200
    loop forever {
        make number val = read analog pin 34
        if val > 500 {
            turn on pin 13
        } else {
            turn off pin 13
        }
        wait 100
    }
}
```

**Arduino Output (`codegen.c`):**
```cpp
void setup() {
    Serial.begin(115200);
    pinMode(13, OUTPUT);
}
void loop() {
    int val = analogRead(34);
    if (val > 500) {
        digitalWrite(13, HIGH);
    } else {
        digitalWrite(13, LOW);
    }
    delay(100);
}
```

**ESP32 Output (`codegen_esp32.c`):**
```cpp
#include <WiFi.h>
void setup() {
    Serial.begin(115200);
    pinMode(13, OUTPUT);
}
void loop() {
    int val = analogRead(34);
    if ((val) > (500)) {
        digitalWrite(13, HIGH);
    } else {
        digitalWrite(13, LOW);
    }
    delay(100);
}
```

**Raspberry Pi Output (`codegen_rpi.c`):**
```python
#!/usr/bin/env python3
import RPi.GPIO as GPIO
import time
GPIO.setmode(GPIO.BCM)
try:
    while True:
        val = mcp.read_adc(34)
        if (val) > (500):
            GPIO.output(13, 1)
        else:
            GPIO.output(13, 0)
        time.sleep(100 / 1000.0)
except KeyboardInterrupt:
    print("\nStopped by user")
finally:
    GPIO.cleanup()
```

**Pico Output (`codegen_pico.c`):**
```python
# Generated by Kinetrix Compiler (Target: Raspberry Pi Pico)
from machine import Pin, ADC, PWM, I2C, UART
import utime, math
def _kinetrix_main():
    while True:
        val = _safe_adc(34)
        if (val) > (500):
            Pin(13, Pin.OUT).value(1)
        else:
            Pin(13, Pin.OUT).value(0)
        utime.sleep_ms(int(100))
if __name__ == '__main__':
    _kinetrix_main()
```

### Code Generation Architecture

Each code generator handles specific aspects:

| Responsibility | What Happens |
|----------------|-------------|
| **Hoisting** | Variables, functions, ISRs, and task definitions are extracted and placed before `main` |
| **Expression generation** | Recursive walk of expression nodes — handles operator precedence, function calls, type coercion |
| **Statement generation** | Translates each statement node to target-specific code |
| **Setup/loop split** | Arduino/ESP32 split code into `setup()` and `loop()`. Python targets use `try/finally` |
| **Pin initialization** | The pin tracker determines which pins are used and emits `pinMode()` calls automatically |

---

## OTA Fleet Updates

### Language Syntax

```kinetrix
enable ota "hostname" password "secret"
```

### What Gets Generated

**ESP32:** Full ArduinoOTA integration
```cpp
#include <ArduinoOTA.h>
// In setup():
WiFi.begin();
ArduinoOTA.setHostname("hostname");
ArduinoOTA.setPassword("secret");
ArduinoOTA.begin();
// In loop():
ArduinoOTA.handle();
```

**Raspberry Pi:** Avahi/mDNS network registration
```python
import subprocess
subprocess.Popen(['avahi-publish-service', 'hostname', '_kinetrix._tcp', '5050'])
```

**Pico W:** WebREPL wireless access
```python
import network, webrepl
webrepl.start()
```

**Arduino:** Not supported (no WiFi hardware) — emits a helpful comment

### Fleet Push Tool

```bash
./kcc_push.sh robot.kx --target esp32 --password secret

# What it does:
# 1. Compiles robot.kx for the target
# 2. Discovers OTA-enabled robots on the network (mDNS)
# 3. Pushes firmware to ALL discovered robots simultaneously
```

---

## Standard Library

Located in `libs/`:

| Module | File | Functions |
|--------|------|-----------|
| **Drive** | `libs/drive.kx` | Motor control helpers for differential drive robots |
| **Sonar** | `libs/sonar.kx` | HC-SR04 ultrasonic distance measurement |
| **Math** | `libs/math.kx` | Additional math utilities |
| **Music** | `libs/music.kx` | Musical note frequencies for buzzer |

**Usage:**
```kinetrix
include "libs/sonar.kx"

program {
    loop forever {
        # Use functions from the included library
    }
}
```

---

## Error Handling & Diagnostics

### Compiler Errors

The compiler reports errors with file location and description:

```
Error at line 5: Expected '{' after program
Error at line 12: Unknown variable 'speed'
Error at line 8: Type mismatch: expected number, got string
```

### Compiler Warnings (Diagnostics)

```
Warning: Variable 'temp' declared but never used
Warning: Code after 'return' is unreachable
Warning: Pin 13 used as both input and output
```

### Runtime Safety

The generated code includes safety measures:

- **Division by zero protection** — `(0 if divisor == 0 else a / b)` in Python targets
- **Array bounds** — checked at runtime where possible
- **Watchdog** — auto-restart on freeze
- **Try/on error** — catches hardware failures gracefully

---

## Architecture Deep Dive

### File Map

```
Kinetrix/
│
├── compiler_v3.c          ← ENTRY POINT: main(), CLI parsing, orchestration
│
├── parser.h               ← Token definitions, parser state structure
├── parser.c               ← Lexer (tokenizer) + Parser (AST builder)
│                             ~2200 lines, handles ALL syntax rules
│
├── ast.h                  ← AST node type enum, node data structures, constructors
├── ast.c                  ← AST node constructors, type system helpers
│                             ~70 node types, union-based node data
│
├── symbol_table.h/.c      ← Variable/function tracking during compilation
├── error.h/.c             ← Error reporting with line numbers
├── pin_tracker.h/.c       ← GPIO pin usage analysis and conflict detection
├── diagnostics.c          ← Unused variable, unreachable code warnings
│
├── codegen.h              ← Shared codegen interface (CodeGen struct)
├── codegen.c              ← Arduino code generator (~1000 lines)
├── codegen_esp32.c        ← ESP32 code generator (~900 lines)
├── codegen_rpi.c          ← Raspberry Pi code generator (~770 lines)
├── codegen_pico.c         ← Pico code generator (~560 lines)
├── codegen_ros2.c         ← ROS2 code generator (~400 lines)
│
├── Makefile               ← Build system
├── test_all.sh            ← 48 automated tests
├── kcc_push.sh            ← OTA fleet push tool
│
├── examples/              ← Advanced example programs
├── examples_working/      ← Beginner-friendly tutorials
├── libs/                  ← Standard library modules
└── docs/                  ← This documentation
```

### Data Flow

```
source.kx (text)
    │
    ▼
┌─────────────────────┐
│  Lexer               │  Reads characters → outputs tokens
│  (parser.c)          │  Handles: keywords, operators, literals, identifiers
└─────────┬───────────┘
          │ Token stream
          ▼
┌─────────────────────┐
│  Parser              │  Reads tokens → builds tree structure
│  (parser.c)          │  Recursive descent parser
│                      │  Handles: expressions, statements, blocks, functions
└─────────┬───────────┘
          │ ASTNode* (tree)
          ▼
┌─────────────────────┐
│  Analysis            │  Validates: types, scopes, pin conflicts
│  (symbol_table.c)    │  Reports: warnings, unused vars, unreachable code
│  (pin_tracker.c)     │
│  (diagnostics.c)     │
└─────────┬───────────┘
          │ Validated AST
          ▼
┌─────────────────────┐
│  Code Generator      │  Walks AST → emits target code
│  (codegen_*.c)       │  Handles: setup/loop split, pin init, hoisting
│                      │  Output: .cpp or .py file
└─────────────────────┘
```

### Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| **Integrated lexer + parser** | Keeps the codebase simple — single-pass tokenization |
| **Union-based AST nodes** | Memory efficient — each node stores only its relevant data |
| **Separate code generators per target** | Each target has unique idioms — separate files keep code clean |
| **Context-sensitive keywords** | Words like `count`, `size`, `value` work as both keywords and variable names |
| **Automatic pin initialization** | The pin tracker scans the AST to auto-emit `pinMode()` — less boilerplate |
| **Hoisting** | Functions, tasks, ISRs, and global variables are automatically moved to the correct position in generated code |

### Type System

Kinetrix uses a simple static type system defined in `ast.h`:

```
TYPE_INT       → int / long
TYPE_FLOAT     → float / double
TYPE_STRING    → char* / String / str
TYPE_BOOL      → bool
TYPE_VOID      → void
TYPE_ARRAY     → type[]
TYPE_STRUCT    → user-defined struct
```

Type checking happens during parsing and code generation. The `type_to_ctype()` function in `ast.c` maps Kinetrix types to C/Python types for each target.

---

## Testing

### Automated Test Suite

```bash
./test_all.sh
```

The test suite covers:

| Test Category | What's Tested |
|---------------|--------------|
| **Syntax parsing** | All statement types parse correctly |
| **Multi-target** | Same source compiles for all 5 targets |
| **Error handling** | Invalid code produces correct error messages |
| **Edge cases** | Negative numbers, string concat, nested structures |
| **Features** | Interrupts, tasks, I2C, OTA, radio, math, etc. |
| **Regression** | Previously fixed bugs stay fixed |

**Current status: 48/48 tests passing** ✅

---

## Quick Reference Card

```kinetrix
# ──── PROGRAM ────
program { }

# ──── VARIABLES ────
make number x = 10
make string s = "hi"
make bool b = true
const number PI = 3.14
set x to 20
change x by 5

# ──── GPIO ────
turn on pin 13
turn off pin 13
set pin 9 to 128
read pin 2
read analog pin 34

# ──── CONTROL FLOW ────
if x > 5 { } else { }
while x < 10 { }
for i from 0 to 9 { }
repeat 5 { }
loop forever { }
break
continue

# ──── FUNCTIONS ────
define function foo(a number) returns number { return a * 2 }

# ──── STRUCTS ────
define type Point { number x; number y }
make Point p
p.x = 10

# ──── ARRAYS / BUFFERS ────
make array data size 10
make buffer ring size 5
push val into ring

# ──── COMMUNICATION ────
open serial at 115200
send serial "msg"
open i2c
read i2c device 0x68 register 0x41
open spi at 1000000
spi transfer 0xFF

# ──── TASKS ────
shared make number val = 0
task reader { val = read analog pin 34; wait 100 }
start task reader

# ──── INTERRUPTS ────
on interrupt pin 2 rising { turn on pin 13 }
on timer every 5000ms { feed watchdog }

# ──── SAFETY ────
enable watchdog timeout 5000ms
feed watchdog
try { } on error { }
assert x < 100 else stop()

# ──── OTA ────
enable ota "fleet_name" password "pass"

# ──── WIRELESS ────
radio send 0 value
if radio available { make number m = radio read }

# ──── AUDIO ────
tone pin 8 at 1000
notone pin 8

# ──── MATH ────
sin(x)  cos(x)  tan(x)  sqrt(x)  atan2(y, x)

# ──── MODULES ────
include "libs/sonar.kx"

# ──── COMMENTS ────
# This is a comment
```

---

*Kinetrix v3.1 — Built for robotics. Built for the future.* 🤖
