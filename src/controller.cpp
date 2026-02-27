/**
 * @file controller.cpp
 * @brief Main controller for wheel-tobot autonomous robot
 *
 * Receives commands from Raspberry Pi via GPIO pins (3-bit binary encoding)
 * Commands: 0=Stop, 1=Forward, 2=Backward, 3=Left, 4=Right
 *
 * @author wheel-tobot project
 * @date 2026
 */

#include "arduino_hal.h"
#include "blink.h"
#include "motor_driver.h"
#include <util/delay.h>

#define MOTOR_SPEED 50 // 0-255

// GPIO pins for command input from Pi
#define CMD_BIT0 PIN_4 // LSB
#define CMD_BIT1 PIN_6
#define CMD_BIT2 PIN_7 // MSB

// Command definitions
#define CMD_STOP 0     // 000
#define CMD_FORWARD 1  // 001
#define CMD_BACKWARD 2 // 010
#define CMD_LEFT 3     // 011
#define CMD_RIGHT 4    // 100

/**
 * @brief Initialize command input pins
 */
void initCommandPins()
{
  PinMode(CMD_BIT0, false); // Input
  PinMode(CMD_BIT1, false); // Input
  PinMode(CMD_BIT2, false); // Input
}

/**
 * @brief Read 3-bit command from GPIO pins
 * @return Command value (0-7)
 */
uint8_t readCommand()
{
  uint8_t cmd = 0;
  if (DigitalRead(CMD_BIT0))
    cmd |= 0b001;
  if (DigitalRead(CMD_BIT1))
    cmd |= 0b010;
  if (DigitalRead(CMD_BIT2))
    cmd |= 0b100;
  return cmd;
}

/**
 * @brief Execute motor command
 * @param cmd Command code
 */
void executeCommand(uint8_t cmd)
{
  switch (cmd)
  {
  case CMD_FORWARD: // Forward
    MotorA_Drive(MOTOR_SPEED);
    MotorB_Drive(MOTOR_SPEED);
    break;

  case CMD_BACKWARD: // Backward
    MotorA_Drive(-MOTOR_SPEED);
    MotorB_Drive(-MOTOR_SPEED);
    break;

  case CMD_LEFT: // Turn left
    MotorA_Drive(-MOTOR_SPEED);
    MotorB_Drive(MOTOR_SPEED);
    break;

  case CMD_RIGHT: // Turn right
    MotorA_Drive(MOTOR_SPEED);
    MotorB_Drive(-MOTOR_SPEED);
    break;

  case CMD_STOP: // Stop
  default:
    StopAllMotors();
    break;
  }
}

/**
 * @brief Main program entry point
 */
int main()
{
  SetupMotors();
  initCommandPins();

  // Blink LED to show Arduino is ready
  TestArduino();

  uint8_t last_cmd = CMD_STOP;

  while (1)
  {
    // Read command from GPIO pins
    uint8_t cmd = readCommand();

    // Only execute if command changed (reduces jitter)
    if (cmd != last_cmd)
    {
      executeCommand(cmd);
      last_cmd = cmd;
    }

    // Small delay to debounce
    _delay_ms(10);
  }

  return 0;
}
