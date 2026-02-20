<div align="center">

# ⚡ Kinetrix

### The Robotics Programming Language

**Write robots in plain English. Compile to Arduino C++ instantly.**

[![Version](https://img.shields.io/badge/version-v2.0-00d4ff?style=flat-square)](https://github.com/M4dM0nk3y/Kinetrix/releases/tag/v2.0)
[![License](https://img.shields.io/badge/license-MIT-7c3aed?style=flat-square)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-10b981?style=flat-square)](#-install)
[![Website](https://img.shields.io/badge/website-live-00d4ff?style=flat-square)](https://m4dm0nk3y.github.io/Kinetrix/)

<br/>

[🌐 Website](https://m4dm0nk3y.github.io/Kinetrix/) · [📄 Language Guide](https://m4dm0nk3y.github.io/Kinetrix/KINETRIX_COMPREHENSIVE_GUIDE.pdf) · [📦 Releases](https://github.com/M4dM0nk3y/Kinetrix/releases)

</div>

---

## 📦 Install

**One line — Mac / Linux:**

```bash
curl -fsSL https://raw.githubusercontent.com/M4dM0nk3y/Kinetrix/main/install.sh | bash
```

Or [download the binary manually →](https://github.com/M4dM0nk3y/Kinetrix/releases/tag/v2.0)

**To uninstall:**
```bash
curl -fsSL https://raw.githubusercontent.com/M4dM0nk3y/Kinetrix/main/install.sh | bash -s uninstall
```

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

## 🚀 How to Use Kinetrix

### Step 1 — Write your program

Create a file with a `.kx` extension in any text editor (VS Code, nano, Notepad):

```bash
nano my_robot.kx
```

Write your Kinetrix code:

```kinetrix
program {
    make var speed = 150

    loop forever {
        set pin 9 to speed
        wait 500
        turn off pin 9
        wait 500
    }
}
```

Save the file.

---

### Step 2 — Compile it

Open your Terminal and run:

```bash
kcc my_robot.kx -o my_robot.ino
```

This produces a `my_robot.ino` file — standard Arduino C++ that any board understands.

**Compile for different targets:**

```bash
kcc program.kx                       # Arduino Uno / Mega / Nano (default)
kcc program.kx -o out.ino --target esp32   # ESP32
kcc program.kx -o out.c   --target rpi     # Raspberry Pi
kcc program.kx -o out.cpp --target ros2    # ROS2
kcc program.kx -o out.ino --target pico    # Raspberry Pi Pico
```

---

### Step 3 — Upload to your board

1. Open **Arduino IDE**
2. Go to **File → Open** and select your `.ino` file
3. Go to **Tools → Board** and select your board (e.g. Arduino Uno)
4. Go to **Tools → Port** and select the USB port your board is on
5. Click the **Upload →** button

✅ Your robot is now running!

---

## 🎯 Language Reference

### Variables

```kinetrix
make var speed = 150        # integer
make var temp  = 36.5       # float
make var label = "ready"    # string
make var flag  = true       # boolean
const MAX = 255             # constant
change speed by 10          # increment
set speed to 200            # reassign
```

### Control Flow

```kinetrix
if x > 50 and x < 200 {
    print "normal range"
} else if x == 0 {
    print "stopped"
} else {
    print "out of range"
}
```

### Loops

```kinetrix
loop forever { ... }         # infinite loop
repeat 10 { ... }            # fixed count
while x < 100 { ... }        # condition-based
for i from 0 to 9 { ... }    # range-based
```

### GPIO (Hardware Control)

```kinetrix
turn on pin 13               # digital HIGH
turn off pin 13              # digital LOW
set pin 9 to 200             # PWM / analog write
read analog pin 0            # read analog value
```

### Functions

```kinetrix
def blink(pin, times) {
    repeat times {
        turn on pin pin
        wait 500
        turn off pin pin
        wait 500
    }
}

blink(13, 3)
```

### Servo & Tone

```kinetrix
set servo pin 9 to 90        # rotate servo to 90°
set servo pin 10 to 180      # rotate servo to 180°
tone pin 8 to 440            # play 440Hz tone
notone pin 8                 # stop tone
```

### I2C Communication

```kinetrix
i2c begin
i2c start 0x3C
i2c send 0x00
i2c stop
```

### Timing & Debug

```kinetrix
wait 1000                    # delay 1000ms
wait_us 500                  # delay 500 microseconds
print "hello"                # serial print string
print variable               # serial print variable
```

### Math Library

```kinetrix
sin(angle)   cos(angle)   tan(angle)
sqrt(x*x + y*y)
atan(y / x)
random(0, 100)
constrain(speed, 0, 200)
map(sensor, 0, 1023, 0, 255)
abs(error)
min(a, b)    max(a, b)
```

---

## 🧩 Target Platforms

| Board | Command |
|-------|---------|
| Arduino Uno / Mega / Nano | `kcc program.kx` (default) |
| ESP32 | `kcc program.kx --target esp32` |
| Raspberry Pi | `kcc program.kx --target rpi` |
| ROS2 | `kcc program.kx --target ros2` |
| Raspberry Pi Pico | `kcc program.kx --target pico` |

---

## 💡 Example Programs

### Blink LED
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

### Traffic Light
```kinetrix
program {
    loop forever {
        turn on pin 2   wait 3000   turn off pin 2
        turn on pin 3   wait 1000   turn off pin 3
        turn on pin 4   wait 3000   turn off pin 4
    }
}
```

### Motor Speed Control
```kinetrix
program {
    make var speed = 0
    loop forever {
        for speed from 0 to 255 {
            set pin 9 to speed
            wait 10
        }
        for speed from 255 to 0 {
            set pin 9 to speed
            wait 10
        }
    }
}
```

### Servo Sweep
```kinetrix
program {
    loop forever {
        set servo pin 9 to 0
        wait 1000
        set servo pin 9 to 90
        wait 1000
        set servo pin 9 to 180
        wait 1000
    }
}
```

---

## 📄 Documentation

- **[Language Guide (PDF)](https://m4dm0nk3y.github.io/Kinetrix/KINETRIX_COMPREHENSIVE_GUIDE.pdf)** — complete reference
- **[Website](https://m4dm0nk3y.github.io/Kinetrix/)** — interactive examples, syntax reference, comparison table

---

## 📄 License

MIT — use freely.

---

<div align="center">
Built with ❤️ by <a href="https://github.com/M4dM0nk3y">Soham Mulik</a>
</div>
