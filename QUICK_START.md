# KINETRIX QUICK START
## Get Running in 5 Minutes!

---

## 🚀 3 SIMPLE STEPS

### STEP 1: Write Code
Open any text editor and create `blink.kx`:

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

### STEP 2: Compile
Open Terminal and run:

```bash
cd ~/Downloads/Kinetrix_Release
./kcc blink.kx -o blink.ino
```

### STEP 3: Upload
1. Open `blink.ino` in Arduino IDE
2. Select your board (ESP32/Arduino)
3. Click Upload button
4. Done! LED blinks! 🎉

---

## 📝 THE WORKFLOW

```
Text Editor → Write .kx file
    ↓
Terminal → Compile with ./kcc
    ↓
Arduino IDE → Upload .ino file
    ↓
Hardware → Watch it run!
```

---

## 💡 EXAMPLE COMMANDS

**Compile any file:**
```bash
./kcc mycode.kx -o output.ino
```

**Compile an example:**
```bash
./kcc examples_working/02_traffic_light.kx -o traffic.ino
```

**Test all features:**
```bash
./test_all_features.sh
```

---

## 🎯 WHAT YOU NEED

- ✅ Text editor (VS Code, TextEdit, Notepad, etc.)
- ✅ Terminal (built into Mac/Windows)
- ✅ Arduino IDE (free download)
- ✅ ESP32 or Arduino board
- ✅ USB cable

---

## 📚 LEARN MORE

- **Full Guide:** `HOW_TO_USE_KINETRIX.md`
- **Language Reference:** `LANGUAGE_REFERENCE.md`
- **Examples:** `examples_working/` folder
- **Feature List:** `COMPLETE_FEATURE_LIST.md`

---

**That's it! Now start coding!** 💪
