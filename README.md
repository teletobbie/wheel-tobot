# wheel-tobot
The wheel tobot is a autonomous-driving two wheel robot with vision capabilities

## Hardware
- **Raspberry Pi Zero 2W** - Vision processing
- **Arduino Uno (ATmega328P)** - Motor control execution
- **Freenove Camera Module IMX219** - Line following vision
- **TB6612FNG Dual H-Bridge Motor Driver** - Motor control
- **2x DC Motors** - Drivetrain
- **2x Samsung INR18650-25R Li-ion batteries** - Power (7.4V, 2500mAh, 20A)
- **LM2596 Buck Converter** - 7.4V → 5V regulation

## Power Distribution

### Battery Specifications
- **Model:** Samsung INR18650-25R (2x in series)
- **Voltage:** 7.4V nominal (6.0V empty → 8.4V fully charged)
- **Capacity:** 2500mAh
- **Max discharge:** 20A continuous

### Power Architecture

```
18650 Battery Pack (7.4V, GND)
    │
    ├─→ LM2596 Buck Converter (7.4V → 5V, 3A)
    │    │
    │    └─→ Breadboard 5V Rail
    │         ├─→ Raspberry Pi GPIO Pin 2 or 4 (5V power)
    │         ├─→ Arduino 5V pin (NOT Vin)
    │         └─→ Motor Driver VCC (logic power)
    │
    └─→ Motor Driver VM (7.4V direct, motor power)

Common GND: Battery(-) = Breadboard GND = Pi GND = Arduino GND = Motor Driver GND
```

### Wiring Checklist

**Power Distribution:**
- [ ] Battery+ → LM2596 IN+
- [ ] Battery- → LM2596 IN- (GND)
- [ ] LM2596 OUT+ → Breadboard + rail (5V)
- [ ] LM2596 OUT- → Breadboard - rail (GND)
- [ ] Breadboard 5V → Raspberry Pi Pin 2 or 4
- [ ] Breadboard GND → Raspberry Pi Pin 6 (or any GND pin)
- [ ] Breadboard 5V → Arduino 5V pin
- [ ] Breadboard GND → Arduino GND
- [ ] Breadboard 5V → Motor Driver VCC
- [ ] Breadboard GND → Motor Driver GND
- [ ] Battery+ (7.4V) → Motor Driver VM
- [ ] Battery- → Motor Driver GND (common ground)

**UART Communication:**
- [ ] Raspberry Pi GPIO 14 (Pin 8) → Arduino Pin 0 (RX)
- [ ] (Optional) Arduino Pin 1 (TX) → Raspberry Pi GPIO 15 (Pin 10) via voltage divider
- [ ] Verify common ground connection between Pi and Arduino

**Estimated Runtime:**
- Light use (slow driving): ~1.5 hours
- Heavy use (full speed): ~45 minutes

### Important Safety Notes

⚠️ **Critical:**
1. **Adjust LM2596 output to 5.0-5.1V BEFORE connecting** - Measure with multimeter!
2. **Common ground is mandatory** - All GND must connect together
3. **Pi GPIO is 3.3V** - Never connect Arduino 5V outputs to Pi GPIO
4. **Use proper wire gauge:**
   - Battery to buck: 18-20 AWG
   - Buck to breadboard: 20-22 AWG
   - Breadboard to devices: 22-24 AWG
5. **Don't discharge batteries below 6.0V** (3.0V per cell)

## Communication: Pi → Arduino via UART Serial

Communication uses **UART serial** at 115200 baud for real-time line following control.

### UART Connection

**Raspberry Pi → Arduino:**
| Raspberry Pi | Pi Physical Pin | Arduino Pin | Signal |
|--------------|-----------------|-------------|--------|
| GPIO 14 (TXD) | 8 | Pin 0 (RX) | Transmit (Pi → Arduino) |
| GPIO 15 (RXD) | 10 | Pin 1 (TX) | Receive (Arduino → Pi, optional) |
| GND | 6, 9, 14, etc. | GND | Common Ground |

**Enable Hardware UART on Raspberry Pi:**
```bash
sudo raspi-config
# Interface Options → Serial Port
# Login shell over serial: NO
# Serial hardware enabled: YES
# Reboot when prompted
```

### Serial Protocol

**Packet Format:** 4 bytes per command
```
[SYNC] [ERROR_HIGH] [ERROR_LOW] [CHECKSUM]
0xFF   (error>>8)   (error&0xFF) (0xFF+high+low)&0xFF
```

**Error Value Range:** -320 to +320 pixels
- **Negative values**: Line detected left of center → turn left
- **Positive values**: Line detected right of center → turn right  
- **Zero**: Line centered → drive straight

**Control System:**
- Proportional (P) control with differential drive
- Adjusts left/right motor speeds based on error magnitude
- Safety timeout: Motors stop if no data received for 1 second

**Voltage Levels:**
- Pi UART outputs 3.3V (safe for Arduino 5V RX input)
- Arduino UART outputs 5V (use voltage divider if Pi RX is used)

## TB6612FNG Motor Driver Connections

| **TB6612FNG Pin** | **Connection** |
|-------------------|----------------|
| VM   | Battery+ (7.4V) |
| GND  | Common Ground |
| VCC  | 5V (from LM2596) |
| A01  | MotorA (+) |
| A02  | MotorA (–) |
| B01  | MotorB (+) |
| B02  | MotorB (–) |
| PWMA | Arduino (Pin 3) |
| AI1  | Arduino (Pin 9) |
| AI2  | Arduino (Pin 8) |
| STBY | Arduino (Pin 10) |
| BI1  | Arduino (Pin 11) |
| BI2  | Arduino (Pin 12) |
| PWMB | Arduino (Pin 5) |

*note based on https://youtu.be/3LBiyBTnt7g?si=8lGq11LnlwiDD2ZM

## Development Setup

### Required Toolchain
Download and extract the AVR 8-Bit Toolchain to `toolchain/` folder:

**Windows:**
- Download: [AVR 8-Bit Toolchain (Windows)](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers)
- Extract to: `toolchain/avr8-gnu-toolchain-win32_x86_64/`
- Install AVRDUDE for uploading: `scoop install avrdude` or download from https://github.com/avrdudes/avrdude/releases

**Linux/Mac:**
- Install via package manager: `sudo apt install gcc-avr avr-libc avrdude` (Ubuntu/Debian)
- Or download from Microchip website

### Building
1. Open project in VS Code
2. Press `Ctrl+Shift+B` 
3. Select "Build Complete Arduino Uno Project"
4. Output files will be in `build/` folder:
   - `controller.elf` - ELF executable
   - `controller.hex` - Intel HEX for uploading to Arduino

### Uploading to Arduino
1. Connect Arduino Uno via USB
2. Press `Ctrl+Shift+B`
3. Select "Upload to Arduino Uno (COM6)" (or COM5 depending on your port)
4. The task will automatically:
   - Build the project
   - Convert to HEX format
   - Upload to Arduino using avrdude

**Find your Arduino COM port:**
```powershell
Get-WmiObject Win32_PnPEntity | Where-Object { $_.Caption -match 'Arduino' } | Select-Object Caption
```

Or use Device Manager → Ports (COM & LPT)

### Project Structure
```
src/
  camera/
    follow-line.py      - Pi: Vision processing & UART serial communication
    requirements.txt    - Python dependencies
  controller.cpp        - Arduino: UART serial receiver & proportional motor control
  motor_driver.h/cpp    - TB6612FNG motor driver interface
  arduino_hal.h/cpp     - Hardware abstraction layer (GPIO, PWM, UART)
  blink.h/cpp          - LED diagnostic functions
setup-static-ip-nmcli.sh - Pi network setup script (NetworkManager)
build/                   - Compiled binaries (gitignored)
toolchain/               - AVR toolchain (gitignored)
.github/
  copilot-instructions.md - Project coding standards and conventions
```

## Raspberry Pi Setup

### Camera Module Setup (Freenove IMX219)

**Physical Installation:**
1. **Power off Raspberry Pi** before connecting camera
2. Locate the Camera Module port (between HDMI and USB on Pi Zero 2W)
3. Gently pull up the plastic clip on the camera port
4. Insert the ribbon cable with **contacts facing the contacts in the port**
5. Push the plastic clip back down to secure

**Software Configuration:**

```bash
# 1. Edit config file
sudo nano /boot/firmware/config.txt

# 2. Disable automatic camera detection (find this line and change to 0)
camera_auto_detect=0

# 3. Add camera overlay at the END of the file:
dtoverlay=imx219

# Note: For Raspberry Pi 5, use: dtoverlay=imx219,cam0
# Note: For OV5647 camera, use: dtoverlay=ov5647 instead

# 4. Save (Ctrl+O, Enter) and exit (Ctrl+X)

# 5. Reboot
sudo reboot

# 6. Verify camera is detected
ls /dev/video*
# Should show: /dev/video0

# 7. Test camera
rpicam-hello
# Should show 5-second preview

# 8. Capture test image
rpicam-jpeg -o test.jpg -t 2000 --width 800 --height 600
```

**Troubleshooting:**
- If camera not detected, check ribbon cable is fully inserted with correct orientation
- Ensure ribbon cable contacts face the right direction on BOTH ends
- Try reconnecting while Pi is powered off
- Verify `dtoverlay=imx219` is at the END of config.txt

### Static IP Configuration (Recommended for Robot)

For robots that reboot frequently, configure a static IP to avoid DHCP issues:

```bash
# Copy script from your PC to Pi
scp setup-static-ip-nmcli.sh tobias@192.168.1.178:~/

# SSH into Pi and run
chmod +x setup-static-ip-nmcli.sh
./setup-static-ip-nmcli.sh
```

This ensures the Pi always uses `192.168.1.178` and connects faster on boot without DHCP negotiation delays.

**Note:** Also configure a static DHCP reservation in your router (MAC → 192.168.1.178) as a backup to prevent IP conflicts.

### Software Installation
```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Python dependencies (recommended method for Raspberry Pi)
sudo apt install -y python3-pip python3-opencv python3-numpy
sudo apt install -y python3-picamera2 python3-serial python3-flask

# Alternative: Install from requirements.txt (if system packages not available)
# cd ~/camera
# pip3 install -r requirements.txt

# Enable camera and UART
sudo raspi-config
# Navigate to: Interface Options → Camera → Enable
# Navigate to: Interface Options → Serial Port
#   - Login shell over serial: NO
#   - Serial hardware enabled: YES
# Reboot when prompted
```

### Running Vision System
```bash
# Copy Python scripts to Pi
scp -r src/camera tobias@192.168.1.178:~/

# Run on Pi (headless mode)
cd ~/camera
python3 follow-line.py

# Run with web streaming (access at http://pi-tobias.local:5000)
python3 follow-line.py --stream

# Press Ctrl+C to quit
```

### UART Setup Notes
- Hardware UART uses GPIO 14 (TX) and GPIO 15 (RX)
- Must disable serial console login in `raspi-config`
- Serial device: `/dev/ttyS0` (hardware UART) or `/dev/ttyAMA0` (depending on Pi model)
- Baud rate: 115200
- Connection: Pi TX → Arduino RX, common ground required
- No level shifters needed for Pi TX → Arduino RX (3.3V is valid HIGH for 5V Arduino)

**Verify UART is enabled:**
```bash
ls -l /dev/serial*
# Should show: /dev/serial0 -> ttyS0 or ttyAMA0
``` 

