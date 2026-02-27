# wheel-tobot
The wheel tobot is a autonomous-driving two wheel robot with vision capabilities

## Hardware
- **Raspberry Pi Zero 2W** - Vision processing
- **Arduino Uno (ATmega328P)** - Motor control execution
- **PiCamera** (v1, v2, or HQ) - Line following vision
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

## Communication: Pi → Arduino via GPIO

Communication is **one-way** using 3-bit binary encoding via GPIO pins.

### GPIO Command Interface

**Raspberry Pi → Arduino:**
| Pi GPIO (BCM) | Pi Physical Pin | Arduino Pin | Signal |
|---------------|-----------------|-------------|--------|
| GPIO 17 | 11 | Pin 4 | Command Bit 0 (LSB) |
| GPIO 27 | 13 | Pin 6 | Command Bit 1 |
| GPIO 22 | 15 | Pin 7 | Command Bit 2 (MSB) |
| GND | 6, 9, 14, etc. | GND | Common Ground |

### Command Encoding (3-bit)

| Binary | Decimal | Command |
|--------|---------|---------|
| 000 | 0 | Stop |
| 001 | 1 | Forward |
| 010 | 2 | Backward |
| 011 | 3 | Turn Left |
| 100 | 4 | Turn Right |

**Voltage Levels:**
- Pi outputs 3.3V (safe for Arduino 5V input - reads as HIGH)
- Arduino only reads (input mode, high impedance)
- No level shifters needed for one-way communication

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
    follow-line.py      - Pi: Vision processing and line following
  controller.cpp        - Arduino: GPIO command receiver and motor control
  motor_driver.h/cpp    - TB6612FNG motor driver interface
  arduino_hal.h/cpp     - Hardware abstraction layer (GPIO, PWM, ADC)
  blink.h/cpp          - LED diagnostic functions
build/                  - Compiled binaries (gitignored)
toolchain/              - AVR toolchain (gitignored)
.github/
  copilot-instructions.md - Project coding standards and conventions
```

## Raspberry Pi Setup

### Software Installation
```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Python dependencies
sudo apt install -y python3-pip python3-opencv python3-numpy
sudo apt install -y python3-picamera2 python3-rpi.gpio

# Enable camera
sudo raspi-config
# Navigate to: Interface Options → Camera → Enable
# Reboot when prompted
```

### Running Vision System
```bash
# Copy Python script to Pi
scp src/camera/follow-line.py pi@raspberrypi.local:~/

# Run on Pi
python3 ~/follow-line.py

# Press Ctrl+C to quit
```

### GPIO Setup Notes
- Script uses BCM numbering internally
- Ensure /boot/config.txt has `enable_uart=1` if using serial (not needed for GPIO mode)
- Run with `sudo` if GPIO access denied

