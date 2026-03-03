# HOW TO USE KINETRIX - COMPLETE GUIDE
## From Writing Code to Running on Hardware

---

## 📝 STEP 1: WRITE YOUR KINETRIX CODE

### Where to Write:
**Use any text editor:**
- VS Code (recommended)
- TextEdit (Mac)
- Notepad (Windows)
- Sublime Text
- Any editor you like!

### How to Write:

1. **Open your text editor**
2. **Create a new file**
3. **Save it with `.kx` extension**
   - Example: `my_robot.kx`
   - Must end in `.kx`!

### Example Code:

```kinetrix
# Blink LED on pin 13
program {
    loop forever {
        turn on pin 13
        wait 1000
        turn off pin 13
        wait 1000
    }
}
```

**Save this as:** `blink.kx`

---

## ⚙️ STEP 2: COMPILE YOUR CODE

### What is Compiling?
Converting your Kinetrix code (`.kx`) into Arduino code (`.ino`)

### How to Compile:

**Open Terminal** (Mac) or Command Prompt (Windows)

**Navigate to Kinetrix folder:**
```bash
cd ~/Downloads/Kinetrix_Release
```

**Compile your code:**
```bash
./kcc blink.kx -o blink.ino
```

**What this does:**
- `./kcc` - Runs the Kinetrix compiler
- `blink.kx` - Your source file
- `-o blink.ino` - Output Arduino file

**You should see:**
```
✓ Compilation successful!
Generated: blink.ino
```

---

## 📤 STEP 3: UPLOAD TO HARDWARE

### Option A: Using Arduino IDE (Easiest!)

1. **Open Arduino IDE**
2. **File → Open → Select `blink.ino`**
3. **Tools → Board → Select your board** (ESP32/Arduino)
4. **Tools → Port → Select your USB port**
5. **Click Upload button** (→ arrow icon)
6. **Wait for "Done uploading"**
7. **Watch your LED blink!** 🎉

### Option B: Using Command Line

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 blink.ino
arduino-cli upload -p /dev/cu.usbserial-0001 --fqbn esp32:esp32:esp32 blink.ino
```

---

## 🔄 COMPLETE WORKFLOW EXAMPLE

### Example: Traffic Light Project

**1. Write the code** (in VS Code or any editor):

```kinetrix
# Traffic Light
program {
    loop forever {
        # Red light
        turn on pin 2
        wait 3000
        turn off pin 2
        
        # Yellow light
        turn on pin 3
        wait 1000
        turn off pin 3
        
        # Green light
        turn on pin 4
        wait 3000
        turn off pin 4
    }
}
```

**Save as:** `traffic_light.kx`

**2. Compile it:**

```bash
cd ~/Downloads/Kinetrix_Release
./kcc traffic_light.kx -o traffic_light.ino
```

**3. Upload it:**

- Open `traffic_light.ino` in Arduino IDE
- Select board and port
- Click Upload
- Done!

---

## 📂 FILE ORGANIZATION

### Your Project Structure:

```
Kinetrix_Release/
├── kcc                          # The compiler (executable)
├── my_project.kx               # Your Kinetrix code
├── my_project.ino              # Generated Arduino code
├── examples_working/           # Example programs
│   ├── 01_led_blink.kx
│   ├── 02_traffic_light.kx
│   └── ...
└── test_*.kx                   # Test files
```

### Recommended Workflow:

1. **Keep all `.kx` files in Kinetrix_Release folder**
2. **Compile creates `.ino` files in same folder**
3. **Upload `.ino` files to Arduino**

---

## 🎯 QUICK REFERENCE

### Write Code:
```
1. Open text editor
2. Write Kinetrix code
3. Save as filename.kx
```

### Compile:
```bash
cd ~/Downloads/Kinetrix_Release
./kcc filename.kx -o output.ino
```

### Upload:
```
1. Open output.ino in Arduino IDE
2. Select board and port
3. Click Upload
```

---

## 💡 COMMON WORKFLOWS

### Workflow 1: Quick Test
```bash
# Write code in editor, save as test.kx
./kcc test.kx -o test.ino
# Open test.ino in Arduino IDE and upload
```

### Workflow 2: Using Examples
```bash
# Compile an example
./kcc examples_working/01_led_blink.kx -o my_blink.ino
# Open my_blink.ino in Arduino IDE and upload
```

### Workflow 3: Iterative Development
```bash
# Edit your code in editor
./kcc my_robot.kx -o my_robot.ino
# Upload in Arduino IDE
# See results, edit code again
./kcc my_robot.kx -o my_robot.ino
# Upload again
```

---

## 🛠️ TOOLS YOU NEED

### Required:
1. **Text Editor** - To write `.kx` files
   - VS Code (free, recommended)
   - Any text editor works!

2. **Terminal** - To run compiler
   - Mac: Terminal app (built-in)
   - Windows: Command Prompt (built-in)

3. **Arduino IDE** - To upload to hardware
   - Download: arduino.cc
   - Free!

4. **Kinetrix Compiler** - Already have it!
   - The `kcc` file in your folder

### Optional:
- **ESP32/Arduino board** - For hardware testing
- **USB cable** - To connect board to computer
- **LEDs, wires, breadboard** - For projects

---

## 📋 STEP-BY-STEP CHECKLIST

**First Time Setup:**
- [ ] Install Arduino IDE
- [ ] Install board drivers (ESP32/Arduino)
- [ ] Test Arduino IDE works (upload example sketch)
- [ ] Locate Kinetrix_Release folder

**Every Time You Code:**
- [ ] Write code in text editor
- [ ] Save as `.kx` file
- [ ] Open Terminal
- [ ] Navigate to Kinetrix_Release folder
- [ ] Run: `./kcc yourfile.kx -o output.ino`
- [ ] Open `output.ino` in Arduino IDE
- [ ] Select board and port
- [ ] Click Upload
- [ ] Test on hardware!

---

## 🎓 FOR YOUR COLLEGE DEMO

### What to Show:

**1. Write Code (2 min)**
- Open VS Code
- Show simple LED blink code
- Save as `demo.kx`

**2. Compile (30 sec)**
- Open Terminal
- Run: `./kcc demo.kx -o demo.ino`
- Show success message

**3. Upload (1 min)**
- Open `demo.ino` in Arduino IDE
- Show board/port selection
- Click Upload
- Show "Done uploading"

**4. Hardware Demo (1 min)**
- Point to ESP32
- Show LED blinking
- Explain: "This is running my compiled Kinetrix code!"

**Total: 5 minutes, very impressive!** 🚀

---

## ❓ TROUBLESHOOTING

### "Command not found: ./kcc"
**Solution:** Make sure you're in the right folder
```bash
cd ~/Downloads/Kinetrix_Release
ls kcc  # Should show the file
```

### "Permission denied"
**Solution:** Make compiler executable
```bash
chmod +x kcc
```

### "Compilation failed"
**Solution:** Check your syntax
- Make sure you have `program { }`
- Check for typos
- Look at examples in `examples_working/`

### "Upload failed"
**Solution:** Check Arduino IDE
- Is board selected correctly?
- Is port selected correctly?
- Is USB cable connected?
- Try pressing reset button on board

---

## 🚀 YOU'RE READY!

**The complete process is:**

```
Write .kx → Compile to .ino → Upload to hardware → Run!
```

**That's it!** Simple, right? 💪

**Now go build something awesome!** 🎉
