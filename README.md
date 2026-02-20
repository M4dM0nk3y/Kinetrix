# KINETRIX
## A Simple Programming Language for Robotics

---

## 🚀 Quick Start

**1. Write code** (in any text editor):
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

**2. Compile:**
```bash
./kcc blink.kx -o blink.ino
```

**3. Upload** `blink.ino` to Arduino/ESP32 using Arduino IDE

**Done!** 🎉

---

## ✨ Features

- ✅ Simple, readable syntax
- ✅ Print statements for debugging
- ✅ If/else conditionals
- ✅ Repeat loops
- ✅ Variables and math
- ✅ GPIO control
- ✅ PWM/Analog output
- ✅ Compiles to Arduino C++

---

## 📚 Documentation

- **[Quick Start](QUICK_START.md)** - Get running in 5 minutes
- **[How to Use](HOW_TO_USE_KINETRIX.md)** - Complete guide
- **[Language Reference](LANGUAGE_REFERENCE.md)** - All features
- **[Examples](examples_working/)** - 8 working examples

---

## 🎯 Example Code

### LED Blink
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
        turn on pin 2
        wait 3000
        turn off pin 2
        
        turn on pin 3
        wait 1000
        turn off pin 3
        
        turn on pin 4
        wait 3000
        turn off pin 4
    }
}
```

### With Conditionals
```kinetrix
program {
    make var x = 15
    
    if x > 10 {
        print "X is greater than 10"
        turn on pin 13
    } else {
        print "X is 10 or less"
        turn off pin 13
    }
}
```

---

## 🛠️ Requirements

- Text editor (VS Code, TextEdit, etc.)
- Terminal
- Arduino IDE
- ESP32 or Arduino board

---

## 📖 More Examples

See `examples_working/` folder:
- `01_led_blink.kx` - Simple LED blink
- `02_traffic_light.kx` - Traffic light simulation
- `03_pwm_fade.kx` - PWM fade effect
- `04_knight_rider.kx` - LED chaser
- `05_motor_speed.kx` - Motor control
- `06_heartbeat.kx` - Heartbeat pattern
- `07_binary_counter.kx` - Binary counter
- `08_sos_signal.kx` - Morse code SOS

---

## 🧪 Testing

Run comprehensive tests:
```bash
./test_all_features.sh
```

Expected result: **8/8 tests pass** ✅

---

## 📝 Syntax Cheat Sheet

```kinetrix
# Variables
make var x = 10
set x to 20
change x by 5

# GPIO
turn on pin 13
turn off pin 13
set pin 9 to 128

# Control Flow
if x > 10 { }
repeat 10 { }
loop forever { }

# I/O
print "Hello"
print x
wait 1000
```

---

## 🎓 For Students

Perfect for:
- Learning robotics
- Arduino projects
- College assignments
- Hackathons
- Quick prototyping

---

## 📄 License

Open source - use freely!

---

## 🤝 Contributing

Found a bug? Have a feature request? Let me know!

---

**Built with ❤️ for robotics enthusiasts**
