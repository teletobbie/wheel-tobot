# wheel-tobot
The wheel tobot is a autonomous-driving two wheel robot

## Hardware
- Arduino Uno (ATmega328P)
- TB6612FNG Dual H-Bridge Motor Driver
- 2x DC Motors

| **TB6612FNG Pin** | **Connection** |
|-------------------|----------------|
| VM   | EXT Power (V) |
| GND  | EXT Power (GND) |
| GND  | Arduino (GND) |
| VCC  | Arduino (5V) |
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
  controller.cpp       - Main application
  motor_driver.h/cpp   - TB6612FNG motor driver
  arduino_hal.h/cpp    - Hardware abstraction layer (GPIO, PWM, ADC)
build/                 - Compiled binaries (gitignored)
toolchain/             - AVR toolchain (gitignored)
```

