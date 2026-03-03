# 🤖 Kinetrix V3.0 Comprehensive Guide
### The Ultimate Robotics Programming Language

> **Version:** 3.0 | **Compiler:** `kcc` | **Targets:** Arduino, ESP32, Raspberry Pi, Pico, ROS2

Kinetrix is a deterministic, statically-typed, event-driven language designed to make embedded firmware development safe, fast, and human-readable. Version 3.0 completes the language roadmap by adding enterprise features like Structs, Concurrency, Hardware Interrupts, and robust typed FFI.

This manual serves as the **All-In-One Guide**, covering everything from blinking an LED to writing autonomous multi-tasking robotics logic.

---

## 📑 Table of Contents
1. [Program Structure](#1-program-structure)
2. [Data Types & Variables](#2-data-types--variables)
3. [Arrays & Buffers (V3.0)](#3-arrays--buffers-v30)
4. [Structs / Custom Types (V3.0)](#4-structs--custom-types-v30)
5. [Hardware I/O](#5-hardware-io)
6. [Communication Protocols (V3.0)](#6-communication-protocols-v30)
7. [Control Flow & Loops](#7-control-flow--loops)
8. [Concurrency & Tasks (V3.0)](#8-concurrency--tasks-v30)
9. [Hardware Interrupts (V3.0)](#9-hardware-interrupts-v30)
10. [Error Handling & Watchdogs (V3.0)](#10-error-handling--watchdogs-v30)
11. [Functions & FFI (V3.0)](#11-functions--ffi-v30)

---

## 1. Program Structure

Every Kinetrix program must be wrapped inside a `program` block. There is no need for `setup()` or `loop()` functions. Kinetrix sets up the hardware automatically.

```kinetrix
program my_robot {
    // Initialization code runs once here
    turn on pin 13
    
    // The main loop runs continuously
    loop forever {
        turn on pin 13
        wait 500
        turn off pin 13
        wait 500
    }
}
```

---

## 2. Data Types & Variables

V3.0 introduces a strict explicit-typing system with 4 primitive types:
- `int` (Whole numbers: -32768 to 32767)
- `float` (Decimals: 3.14)
- `bool` (True/False: 1 or 0)
- `byte` (8-bit numbers: `0xFF`, `0b1010`)

### Declaration and Assignment
```kinetrix
// Explicit type declarations
make int speed = 200
make float temp = 36.5
make bool is_active = 1

// Updating variables
set speed to 150
change speed by 10     // Adds 10 (speed is now 160)
change speed by -20    // Subtracts 20 (speed is now 140)

// Casting types
make float precise_speed = cast float speed / 255.0
```

*Note: The legacy `make var x = 5` duck-typing is still supported but explicit types are recommended for V3.0.*

---

## 3. Arrays & Buffers (V3.0)

Kinetrix handles collections natively, bypassing complex C++ pointer math.

### Arrays
Fixed-size contiguous memory blocks.
```kinetrix
make array int sensor_history[100]

set sensor_history[0] to 42
make int last_val = sensor_history[0]
```

### Buffers
Rolling/cyclical memory blocks designed for moving averages. When a buffer fills up, it automatically overwrites the oldest data.
```kinetrix
make buffer window[10] of float

push window 3.14
push window 6.28
// ... pushed 10 times ...
// 11th push automatically overwrites the 1st element!
```

---

## 4. Structs / Custom Types (V3.0)

Group related data together using the `define type` keyword. Note: Field names cannot overlap with Kinetrix keywords (like `pin` or `value`).

```kinetrix
define type Telemetry {
    float temp_val
    int lux_level
    bool is_valid
}

program {
    make Telemetry current_data
    
    // Dot-notation access
    set current_data.temp_val to 26.5
    set current_data.lux_level to 1024
    
    // Deep assignment via math
    change current_data.temp_val by 1.5
}
```

---

## 5. Hardware I/O

Kinetrix focuses heavily on embedded I/O with English-like syntax.

### Digital I/O
```kinetrix
turn on pin 13                // Sets pin HIGH
turn off pin 13               // Sets pin LOW
make int state = read pin 2   // Reads digital state (0 or 1)
```

### Analog / PWM
```kinetrix
// Read ADC
make int raw = read analog pin 0  // 0-1023

// Write PWM (0-255)
set pin 9 to 128
```

### Advanced Hardware
```kinetrix
// Read ultrasonic distance (cm)
make int dist = read pulse pin 5

// Set Servo angle
set servo pin 9 to 180

// Piezo buzzers
tone on pin 8 freq 440
notone on pin 8
```

---

## 6. Communication Protocols (V3.0)

V3.0 introduces high-level native abstractions for UART (Serial), I2C, and SPI.

### Initialization
```kinetrix
open serial at 115200 baud
open i2c
open spi at 1000000 hz
```

### Reading and Writing
```kinetrix
// Serial
send serial "Initializing Phase 1"
make int cmd = receive serial

// I2C 
// Read register 0x3B from device 0x68
make int accel_x = read i2c device 104 register 59
// Write 0x00 to device 0x68
write i2c device 104 value 0

// SPI (Bidirectional transfer)
make int spi_resp = transfer spi 0xFF
```

### Named Devices (Optional)
```kinetrix
define device bme280 as i2c at 118
make int raw_temp = read bme280 register 250

// UART Named Device
define device gps as uart at 9600
make int gps_data = read gps
```

---

## 7. Control Flow & Loops

### If / Else
```kinetrix
if dist < 10 {
    set pin 9 to 0
} else if dist < 20 {
    set pin 9 to 128
} else {
    set pin 9 to 255
}
```

### Conditional Loops
```kinetrix
// Repeat exact N times
repeat 5 times {
    turn on pin 13
    wait 100
    turn off pin 13
    wait 100
}

// While loops
while dist > 10 {
    set dist to read pulse pin 5
    wait 50
}
```

---

## 8. Concurrency & Tasks (V3.0)

Kinetrix allows multi-tasking without blocking the main event loop. The compiler automatically translates this into cooperative multithreading (Arduino) or FreeRTOS/pthreads (ESP32/Pi).

```kinetrix
// Shared variables communicate between tasks
shared make bool system_active = 0

// Define a background task
task blinker {
    loop forever {
        if system_active > 0 {
            turn on pin 13
            wait 250
            turn off pin 13
        }
        wait 250
    }
}

program {
    // Kick off background tasks
    start task blinker
    
    // Main loop remains responsive
    loop forever {
        make int btn = read pin 2
        set system_active to btn
        wait 50
    }
}
```

---

## 9. Hardware Interrupts (V3.0)

For time-critical events, bypass the main loop entirely using Interrupt Service Routines (ISRs). 

### Pin Interrupts
Supported edge triggers: `rising`, `falling`, `changing`.
```kinetrix
on pin 2 rising {
    // Triggers immediately when Pin 2 goes from LOW to HIGH
    change trigger_count by 1
}
```

### Timer Interrupts
```kinetrix
// Milliseconds (ms)
on timer every 500 ms {
    set background_flag to 1
}

// Microseconds (us) - High precision
on timer every 1000 us {
    set precise_flag to 1
}
```
*Warning: Do not use `wait` or `print` inside interrupt blocks!*

### Interrupt Priority and Critical Sections

When sharing data between Interrupt Service Routines (ISRs) and the main loop, you must protect the variables to avoid data corruption. Interrupts can pause the main loop at any exact clock cycle. Use `disable interrupts` and `enable interrupts` to wrap these "critical sections". *(Note: The `shared` keyword handles concurrent tasks, but not hardware interrupts).*

```kinetrix
shared make int volatile_pulse_count = 0

on pin 2 rising {
    change volatile_pulse_count by 1
}

program {
    loop forever {
        // Critical Section: Safely copy and reset the volatile counter
        disable interrupts
        make int safe_copy = volatile_pulse_count
        set volatile_pulse_count to 0
        enable interrupts
        
        // safe_copy is now safely isolated from ISR race conditions
        wait 1000
    }
}
```

---

## 10. Error Handling & Watchdogs (V3.0)

Build resilient robots that recover from hardware faults.

### Watchdog Timer
Automatically reboots the microcontroller if the main loop freezes.
```kinetrix
program {
    enable watchdog timeout 2000ms
    
    loop forever {
        feed watchdog // Must call every loop!
        
        // ... complicated logic ...
    }
}
```

### Recoverable Errors (Try-Catch)
Use `try` and `on error` to handle hardware faults without crashing the system:
```kinetrix
try {
    make int val = read i2c device 104 register 59
    // ... process data ...
} on error {
    turn off pin 9 // Failsafe
    send serial "I2C Sensor Fault!"
}
```

### Unrecoverable Errors (Assert)
Use `assert` for conditions that represent fatal logic flaws. If an assert fails, the default behavior is to immediately halt execution to prevent damage. *(Note: If an assert fails inside a try block, the system routes directly to the `on error` block).*
```kinetrix
make int current_temp = read analog pin 0
// If temperature goes negative, halt the robot!
assert current_temp >= 0
```

---

## 11. Functions & FFI (V3.0)

### Native Kinetrix Functions
```kinetrix
def compute_speed(int distance, int time) {
    make int spd = distance / time
    return spd
}
```

### Enhanced Foreign Function Interface (FFI)
In V3.0, Kinetrix can securely call external C++ libraries passing strongly-typed parameters and Arrays/Buffers.
```kinetrix
// Define the external signature
extern "C++" def process_signal(data: array float, len: int) -> float

program {
    make array float audio[512]
    // ... fill audio ...

    // Call native C++ algorithm!
    make float pitch = process_signal(audio, 512)
}
```

---
**Happy Engineering!** With Kinetrix V3.0, you are equipped to build the most sophisticated embedded robotic systems with ease and safety.
