/**
 * @file controller.cpp
 * @brief Main controller for wheel-tobot autonomous robot
 *
 * Demonstration program for TB6612FNG motor driver control.
 * Executes a simple movement pattern: forward, backward, and turn.
 *
 * @author wheel-tobot project
 * @date 2026
 */

#include "blink.h"
#include "motor_driver.h"
#include <util/delay.h>

/**
 * @brief Main program entry point
 *
 * Initializes motors and runs a continuous demo loop with the following pattern:
 * 1. Move forward at 50% speed for 2 seconds
 * 2. Stop for 1 second
 * 3. Move backward at 75% speed for 2 seconds
 * 4. Stop for 1 second
 * 5. Turn right for 1 second
 * 6. Stop for 2 seconds
 *
 * @return Never returns (infinite loop)
 */
int main()
{
  setupMotors();

  while (1)
  {
    // Test that Arduino is working (blink LED 5 times)
    testArduino();

    // Move forward at 50% speed
    motorA_drive(128);
    motorB_drive(128);
    _delay_ms(2000);

    // Stop
    stopAllMotors();
    _delay_ms(1000);

    // Move backward at 75% speed
    motorA_drive(-192);
    motorB_drive(-192);
    _delay_ms(2000);

    // Stop
    stopAllMotors();
    _delay_ms(1000);

    // Turn right (left motor forward, right motor reverse)
    motorA_drive(128);
    motorB_drive(-128);
    _delay_ms(1000);

    stopAllMotors();
    _delay_ms(2000);
  }

  return 0;
}
