/**
 * @file controller.cpp
 * @brief Main controller for wheel-tobot autonomous robot
 *
 * Receives commands from Raspberry Pi via UART serial communication.
 * Message format (4 bytes): [0xFF][ERROR_HIGH][ERROR_LOW][DIRECTION]
 * - ERROR: 16-bit signed line following error (-32768 to +32767)
 * - DIRECTION: Command byte (0=forward, 1=backward, 2=left, 3=right, etc.)
 *
 * @author wheel-tobot project
 * @date 2026
 */

#include "arduino_hal.h"
#include "blink.h"
#include "line_follower.h"
#include "motor_driver.h"
#include "serial_logger.h"
#include "ultra_sonic.h"
#include <stdlib.h>
#include <util/delay.h>

#define COLLISION_THRESHOLD_CM 20.0f
#define BAUD_RATE 57600

/**
 * @brief Check for obstacles and stop motors if too close
 * @param threshold_cm Distance threshold in centimeters
 * @return true if obstacle detected (should skip further processing), false otherwise
 *
 * Measures distance with ultrasonic sensor. If obstacle is closer than threshold,
 * stops all motors and blinks warning LED.
 */
static bool checkObstacle(float threshold_cm)
{
  float distanceFront = MeasureDistance();
  bool tooCloseToObstacle = distanceFront > 0 && distanceFront < threshold_cm;

  if (tooCloseToObstacle)
  {
    /* Safety override: stop immediately regardless of UART commands */
    StopAllMotors();
    BlinkWarning();
    return true;
  }

  return false;
}

/**
 * @brief Main program entry point
 */
int main()
{
  SetupMotors();
  Uart_Init(UART_BAUD_SELECT(BAUD_RATE, F_CPU));
  InitTimer();
  SetupUltraSonic();

  /* Blink LED to show Arduino is ready */
  TestArduino();

  /* Message buffer for UART reception */
  uint8_t buffer[4]; /* Fixed 4-byte message: [SYNC][ERROR_HIGH][ERROR_LOW][DIRECTION] */
  static uint16_t no_data_counter = 0;

  while (1)
  {
    /* Check obstacle sensor - independent of UART */
    if (checkObstacle(COLLISION_THRESHOLD_CM))
    {
      _delay_ms(1);
      continue; /* Skip UART processing while obstacle detected */
    }

    /* Check if UART data available */
    if (Uart_Available())
    {
      /* Read 4-byte packet with 2ms timeout (4 bytes @ 57600 baud ≈ 700us) */
      uint8_t bytesRead = Uart_ReadBytesTimeout(buffer, 4, 2000);

      /* Process complete packet */
      if (bytesRead == 4 && buffer[0] == 0xFF)
      {
        /* Reconstruct signed 16-bit error value */
        int16_t error = (int16_t)((buffer[1] << 8) | buffer[2]);

        /* Get direction byte */
        Direction direction = (Direction)buffer[3];

        /* Process line following command with state-aware control */
        ProcessLineCommand(error, direction);

        /* Reset no-data counter on valid packet */
        no_data_counter = 0;
      }
    }
    else
    {
      /* No UART data received - increment timeout counter */
      no_data_counter++;
      if (no_data_counter > 1000)
      {
        /* No data for ~1000ms - Pi stopped sending, stop motors for safety */
        StopAllMotors();
        no_data_counter = 0; /* Reset to avoid overflow */
      }
    }

    _delay_ms(1); /* Small delay to prevent CPU overload */
  }

  return 0;
}