 README.md 
# Kinetrix V1.0 🤖
### The Compiled Robotics Language for the Modern Engineer.

**Kinetrix** is a high-level, statically compiled programming language designed specifically for robotics, embedded systems, and autonomous drones. It bridges the gap between the simplicity of Python and the raw performance of C++, compiling directly to optimized Arduino/AVR code.

---

## ⚡ Key Features
* **Native Performance:** Compiles to optimized C++ for real-time control.
* **Advanced Math:** Full support for Floating Point, Trigonometry (`sin`, `cos`, `tan`), and Inverse Kinematics (`acos`, `atan2`).
* **Drone Ready:** Native I2C protocol support for Gyroscopes, Accelerometers, and OLED displays.
* **Memory Management:** Simple array syntax for data logging and signal processing.
* **Hardware Control:** Direct keywords for GPIO, PWM, Servos, and Serial communication.

---

## 🛠 Installation
 Clone the repository and run the installer script on your Raspberry Pi or Linux machine:
#git clone [https://github.com/M4dM0nk3y/Kinetrix.git](https://github.com/M4dM0nk3y/Kinetrix.git)
#cd Kinetrix
#./install.sh

🚀 Usage
Create a file ending in .kx and compile it to your board:
kinetrix my_robot.kx

📖 Syntax Reference
1. Program Structure
Every program entry point is main.

program main {
    print "System Online"
    # Your code here
}

2. Variables & Math
Kinetrix uses floating-point math by default for precision.

make var speed = 100.5
change speed by 50       # speed is now 150.5
make var result = 10 * 5

3. Inverse Kinematics & Trigonometry
Essential for Robotic Arms and Hexapods.

make var x = cos(90)
make var y = sin(45)
make var angle = acos(0.5)      # Inverse Cosine
make var heading = atan2(10, 10) # 2-Argument ArcTangent

4. Logic & Flow Control

wait 1000               # Delay in ms

if sensor > 500 {
    turn on 13
} else {
    turn off 13
}

while val < 100 {
    print "Waiting..."
    break
}

5. Hardware I/O

turn on 13              # Digital Write High
turn off 13             # Digital Write Low
servo 9 set 90          # Move Servo on Pin 9 to 90 degrees
make var val = read analog pin 0

6. Memory (Arrays)
Store sensor history or mapping data.

make array readings size 10
set array readings at 0 to 12.5
make var last = readings[0]

7. Serial Connectivity
Read commands from a laptop or Bluetooth module.

print "Waiting for command..."
make var cmd = read serial  # Parses float from serial buffer

8. Custom Functions

def calculate_pid(error) {
    return error * 0.5
}

9. Drone & Sensor Protocols (I2C)
Talk to gyroscopes, accelerometers, and OLED screens.

i2c begin               # Start I2C bus
i2c start 0x68          # Begin talking to device at address 0x68
i2c send 0x6B           # Send a byte of data
i2c stop                # End transmission
make var data = read i2c 0x68  # Read 1 byte from device


