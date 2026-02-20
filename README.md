<div align="center">

# ⚡ Kinetrix

### The Robotics Programming Language

**Write robots in plain English. Compile to Arduino C++ instantly.**

[![Version](https://img.shields.io/badge/version-v2.0-00d4ff?style=flat-square)](https://github.com/M4dM0nk3y/Kinetrix/releases/tag/v2.0)
[![License](https://img.shields.io/badge/license-MIT-7c3aed?style=flat-square)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-10b981?style=flat-square)](#install)
[![Website](https://img.shields.io/badge/website-live-00d4ff?style=flat-square)](https://m4dm0nk3y.github.io/Kinetrix/)

<br/>

[🌐 Website](https://m4dm0nk3y.github.io/Kinetrix/) · [📄 Language Guide](https://m4dm0nk3y.github.io/Kinetrix/KINETRIX_COMPREHENSIVE_GUIDE.pdf) · [📦 Releases](https://github.com/M4dM0nk3y/Kinetrix/releases)

</div>

---

## 🚀 Install

**One line — Mac / Linux:**

```bash
curl -fsSL https://raw.githubusercontent.com/M4dM0nk3y/Kinetrix/main/install.sh | bash
```

Or [download the binary manually](https://github.com/M4dM0nk3y/Kinetrix/releases/tag/v2.0).

---

## ✨ What is Kinetrix?

Kinetrix is a beginner-friendly programming language built for robotics and embedded systems. Write clean, readable code — Kinetrix compiles it to production-ready Arduino C++.

**No boilerplate. No complexity. Just robotics.**

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

**↓ Compiles to:**

```cpp
#include <Arduino.h>
void setup() { pinMode(13, OUTPUT); }
void loop() {
    digitalWrite(13, HIGH);
    delay(1000);
    digitalWrite(13, LOW);
    delay(1000);
}
```

---

## ⚡ Quick Start

```bash
# 1. Install Kinetrix
curl -fsSL https://raw.githubusercontent.com/M4dM0nk3y/Kinetrix/main/install.sh | bash

# 2. Write a program
echo 'program { loop forever { turn on pin 13  wait 1000  turn off pin 13  wait 1000 } }' > blink.kx

# 3. Compile it
kcc blink.kx -o blink.ino

# 4. Upload blink.ino to your Arduino via Arduino IDE — done! 🎉
```

---

## 🎯 Language Features

| Feature | Syntax |
|---------|--------|
| Variables | `make var speed = 150` |
| Constants | `const MAX = 255` |
| GPIO | `turn on pin 13` · `set pin 9 to 200` |
| Loops | `loop forever {}` · `repeat 10 {}` · `while x < 100 {}` |
| Control flow | `if x > 10 and x < 200 { } else if { } else { }` |
| Functions | `def blink(pin, times) { }` |
| Servo | `set servo pin 9 to 90` |
| I2C | `i2c begin` · `i2c start 0x3C` · `i2c send 0x00` |
| Timing | `wait 1000` · `wait_us 500` |
| Math | `sin(x)` · `cos(x)` · `sqrt(x)` · `random(0, 100)` |
| Safety | `constrain(val, 0, 200)` · `map(x, 0,1023, 0,255)` |
| Debug | `print "hello"` · `print variable` |

---

## 🧩 Target Platforms

```bash
kcc program.kx              # Arduino Uno / Mega / Nano (default)
kcc program.kx --target esp32   # ESP32
kcc program.kx --target rpi     # Raspberry Pi
kcc program.kx --target ros2    # ROS2
kcc program.kx --target pico    # Raspberry Pi Pico
```

---

## 📄 Documentation

- **[Language Guide (PDF)](https://m4dm0nk3y.github.io/Kinetrix/KINETRIX_COMPREHENSIVE_GUIDE.pdf)** — complete reference
- **[Website](https://m4dm0nk3y.github.io/Kinetrix/)** — examples, syntax reference, comparison

---

## 🗑️ Uninstall

```bash
curl -fsSL https://raw.githubusercontent.com/M4dM0nk3y/Kinetrix/main/install.sh | bash -s uninstall
```

---

## 📄 License

MIT — use freely.

---

<div align="center">
Built with ❤️ by <a href="https://github.com/M4dM0nk3y">Soham Mulik</a>
</div>
