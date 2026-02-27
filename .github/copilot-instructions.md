# GitHub Copilot Instructions for Wheel-Tobot Project

## Project Overview
Autonomous wheeled robot with vision capabilities combining Raspberry Pi Zero 2W and Arduino Uno.

### Hardware Architecture
- **Raspberry Pi Zero 2W**: Vision processing (camera + line following), ultrasonic sensors, high-level decision making
- **Arduino Uno (ATmega328P)**: Motor control via TB6612FNG dual H-bridge driver
- **Communication**: UART serial (115200 baud) - Pi sends commands to Arduino
- **Motors**: 2x DC motors controlled via TB6612FNG
- **Sensors**: Camera (PiCamera), HC-SR04 ultrasonic sensor
- **Power**: 7.4V (2x 18650 Li-ion batteries)

### Project Structure
```
src/
├── camera/
│   └── follow-line.py      # Pi: Computer vision & sensor processing
├── arduino_hal.h/cpp        # Arduino: Hardware Abstraction Layer
├── motor_driver.h/cpp       # Arduino: TB6612FNG motor control
├── blink.h/cpp             # Arduino: LED diagnostic functions  
└── controller.cpp          # Arduino: Main serial command handler
```

---

## C++ Coding Style (Arduino)

### Function Naming Convention
- **Public functions** (declared in `.h` files): Start with **Capital letter**
- **Static/private functions** (internal to `.cpp` files): Start with **lowercase letter**

### Doxygen Documentation Style
- **Header files (`.h`)**:
  - Include `@file` and `@brief` header comment at top
  - **NO** individual function documentation
  - Keep declarations clean and minimal
  - Example:
    ```cpp
    /**
     * @file motor_driver.h
     * @brief TB6612FNG Dual H-Bridge Motor Driver Interface
     */
    
    void SetupMotors();
    void MotorA_Drive(int16_t speed);
    ```

- **Implementation files (`.cpp`)**:
  - Full Doxygen comments for all functions (public and static)
  - Include `@brief`, `@param`, `@return`, `@note` as needed
  - Example:
    ```cpp
    /**
     * @brief Control Motor A speed and direction
     * @param speed Motor speed and direction
     *              - Positive values (1-255): Forward motion
     *              - Negative values (-255 to -1): Reverse motion
     *              - Zero (0): Brake
     */
    void MotorA_Drive(int16_t speed)
    {
      setMotor(motorA, speed);
    }
    ```
