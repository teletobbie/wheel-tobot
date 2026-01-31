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

**Linux/Mac:**
- Install via package manager: `sudo apt install gcc-avr avr-libc avrdude` (Ubuntu/Debian)
- Or download from Microchip website

### Building
1. Open project in VS Code
2. Press `Ctrl+Shift+B` to build
3. Output files will be in `build/` folder:
   - `controller.elf` - ELF executable
   - `controller.hex` - Intel HEX for uploading to Arduino

### Project Structure
```
controller.cpp       - Main application
motor_driver.h/cpp   - TB6612FNG motor driver
arduino_hal.h/cpp    - Hardware abstraction layer (GPIO, PWM)
build/              - Compiled binaries (gitignored)
toolchain/          - AVR toolchain (gitignored)
```

## Upload to Arduino
Use avrdude or Arduino IDE to upload `build/controller.hex` to your Arduino Uno.

