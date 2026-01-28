# Kinetrix V1.0
A compiled robotics language for Raspberry Pi.
Usage: `kinetrix robot.kx`

### 9. Drone & Sensor Protocols (I2C)
Talk to gyroscopes, accelerometers, and OLED screens.
```javascript
i2c begin               # Start I2C bus
i2c start 0x68          # Begin talking to device at address 0x68
i2c send 0x6B           # Send a byte of data
i2c stop                # End transmission
make var data = read i2c 0x68  # Read 1 byte from device
