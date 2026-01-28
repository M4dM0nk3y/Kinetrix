# Kinetrix V1.0 🤖

### The Compiled Robotics Language for the Modern Engineer.

[**View on GitHub**](https://github.com/M4dM0nk3y/Kinetrix)

**Kinetrix** is a high-level language designed for robotics. It compiles directly to C++ for real-time control on Raspberry Pi and Arduino.

---

## ⚡ Installation

Run this on your Raspberry Pi:

```bash
git clone [https://github.com/M4dM0nk3y/Kinetrix.git](https://github.com/M4dM0nk3y/Kinetrix.git)
cd Kinetrix
./install.sh
```

---

## 📖 Syntax Reference

### Variables & Math
Kinetrix uses floating-point math by default.

```
make var speed = 100.5
change speed by 50       # speed is now 150.5
make var result = 10 * 5
```

### Inverse Kinematics
Essential for Robotic Arms.

```
make var x = cos(90)
make var y = sin(45)
make var angle = acos(0.5)      # Inverse Cosine
```

### Logic & Flow Control

```
wait 1000               # Delay in ms

if sensor > 500 {
    turn on 13
} else {
    turn off 13
}
```

### Hardware I/O

```
turn on 13              # Digital Write High
turn off 13             # Digital Write Low
servo 9 set 90          # Move Servo on Pin 9
make var val = read analog pin 0
```

### Arrays (Memory)

```
make array readings size 10
set array readings at 0 to 12.5
make var last = readings[0]
```

### Drone Protocols (I2C)
Talk to gyroscopes and sensors.

```
i2c begin
i2c start 0x68          # Talk to MPU6050
i2c send 0x6B           # Wake up sensor
i2c stop
make var data = read i2c 0x68
```

<br>
<center>Maintained by M4dM0nk3y</center>
