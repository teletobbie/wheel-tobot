/**
 * @file controller.cpp
 * @brief Main controller for wheel-tobot autonomous robot
 *
 * Receives commands from Raspberry Pi via UART serial communication
 * Commands: Line following error values or motor control commands
 *
 * @author wheel-tobot project
 * @date 2026
 */

#include "arduino_hal.h"
#include "blink.h"
#include "motor_driver.h"
#include <stdlib.h>
#include <util/delay.h>

#define MOTOR_SPEED 120
#define MIN_MOTOR_SPEED 80

#define MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @brief Initialize UART serial communication
 */
void initSerial()
{
  // Initialize UART at 115200 baud
  Uart_Init(UART_BAUD_SELECT(115200, F_CPU));
}

/**
 * @brief Process received line following error value with proportional control
 * @param error Line position error in pixels (-320 to +320)
 *              - Negative: line is left, turn left
 *              - Positive: line is right, turn right
 *              - Zero: line centered, go straight
 *
 * Uses differential drive: adjusts left/right motor speeds proportionally to error
 */
void processLineError(int16_t error)
{

  // TODO: add Stall detection using a MPU-6050

  // Proportional gain - tune this value (0.2 = gentle, 0.5 = moderate, 0.8 = aggressive)
  const float Kp = 0.5f;

  // Calculate proportional adjustment (-127 to +127 range)
  float adjustment = Kp * error;

  // Clamp adjustment to prevent excessive speed difference
  if (adjustment > 100.0f)
    adjustment = 100.0f;
  if (adjustment < -100.0f)
    adjustment = -100.0f;

  // Apply differential drive: subtract from left, add to right
  // Positive error (line right) → reduce left speed, increase right speed → turn right
  // Negative error (line left) → increase left speed, reduce right speed → turn left
  int16_t left_speed = MOTOR_SPEED - (int16_t)adjustment;
  int16_t right_speed = MOTOR_SPEED + (int16_t)adjustment;

  // Clamp motor speeds to valid range
  if (left_speed > 255)
    left_speed = 255;
  if (left_speed < MIN_MOTOR_SPEED)
    left_speed = MIN_MOTOR_SPEED;
  if (right_speed > 255)
    right_speed = 255;
  if (right_speed < MIN_MOTOR_SPEED)
    right_speed = MIN_MOTOR_SPEED;

  // Special case: if error is very small, go straight
  if (error >= -5 && error <= 5)
  {
    left_speed = MOTOR_SPEED;
    right_speed = MOTOR_SPEED;
  }

  MotorA_Drive(left_speed);
  MotorB_Drive(right_speed);
}

/**
 * @brief Main program entry point
 */
int main()
{
  SetupMotors();
  initSerial();

  // Blink LED to show Arduino is ready
  TestArduino();

  Uart_Puts("Arduino Ready - Waiting for error values\r\n");

  // Serial packet state machine
  enum PacketState
  {
    WAIT_SYNC,
    READ_HIGH,
    READ_LOW,
    READ_CHECKSUM
  };
  PacketState state = WAIT_SYNC;
  uint8_t error_high = 0;
  uint8_t error_low = 0;
  uint8_t checksum = 0;
  static uint16_t no_data_counter = 0;

  while (1)
  {
    // Check if data available
    uint16_t received = Uart_Getc();

    if (!(received & UART_NO_DATA))
    {
      uint8_t data = (uint8_t)received;

      // State machine for packet parsing
      switch (state)
      {
      case WAIT_SYNC:
        if (data == 0xFF)
        {
          state = READ_HIGH;
        }
        break;

      case READ_HIGH:
        error_high = data;
        state = READ_LOW;
        break;

      case READ_LOW:
        error_low = data;
        state = READ_CHECKSUM;
        break;

      case READ_CHECKSUM:
        checksum = data;

        // Verify checksum
        uint8_t calculated = (0xFF + error_high + error_low) & 0xFF;
        if (checksum == calculated)
        {
          // Reconstruct signed 16-bit error value
          int16_t error = (int16_t)((error_high << 8) | error_low);

          // Process the error with proportional control
          processLineError(error);
        }

        // Reset state machine
        state = WAIT_SYNC;
        break;
      }
    }
    else
    {
      // No data received for 1000ms+ means Pi stopped sending - stop motors for safety
      no_data_counter++;
      if (no_data_counter > 1000) // ~1000ms without data
      {
        StopAllMotors();
        no_data_counter = 1000;
      }
    }

    _delay_ms(1); // Small delay to prevent CPU overload
  }

  return 0;
}