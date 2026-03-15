/**
 * @file ultra_sonic.cpp
 * @brief implementation of ultrasonic sensor interface to measure distance to obstacles
 * - uses a trigger pin to send a pulse and an echo pin to measure the time until the pulse returns
 * - distance is calculated based on the time of flight of the pulse (speed of sound)
 *
 *
 *
 */

#include "ultra_sonic.h"
#include "arduino_hal.h"
#include <util/delay.h>

/**
 * @brief Initializes the ultrasonic sensor
 *
 * - trigger pin is pin 7 (output)
 *
 * - echo pin is pin 6 (input)
 */
void SetupUltraSonic()
{
  /* Configure trigger pin (pin 7) as output */
  PinMode(PIN_7, true);

  /* Configure echo pin (pin 6) as input */
  PinMode(PIN_6, false);
}

/**
 * @brief Measures the distance up to 5 meters to an obstacle using the ultrasonic sensor
 *
 * @return float Distance in centimeters, or -1.0f if out of range
 */
float MeasureDistance()
{
  /* Send a 10 microsecond pulse on the trigger pin */
  DigitalWrite(PIN_7, true);
  _delay_us(10);
  DigitalWrite(PIN_7, false);

  /* Wait for echo pin to go HIGH and measure the duration */
  uint32_t start_time = Micros();
  while (!DigitalRead(PIN_6))
  {
    if (Micros() - start_time > 30000) /* Timeout after 30ms (max range ~5m) */
      return -1.0f;                    /* Indicate out of range */
  }

  uint32_t echo_start = Micros();
  while (DigitalRead(PIN_6))
  {
    if (Micros() - echo_start > 30000) /* Timeout after 30ms */
      return -1.0f;                    /* Indicate out of range */
  }
  uint32_t echo_end = Micros();

  /* Calculate distance based on time of flight */
  uint32_t duration = echo_end - echo_start;       // Duration in microseconds
  float distance_cm = (duration / 2.0f) * 0.0343f; // Speed of sound ~343 m/s

  return distance_cm;
}

/**
 * @brief Send formatted distance reading over UART for debugging
 * @param distance Distance value from sensor
 */
void printDistance(float distance)
{
  char buffer[32];

  if (distance < 0)
  {
    Uart_Puts("Distance: ERROR (out of range)\r\n");
  }
  else
  {
    /* Convert float to string manually (no sprintf in bare metal) */
    int16_t cm = (int16_t)distance;
    int16_t mm = (int16_t)((distance - cm) * 10);

    buffer[0] = 'D';
    buffer[1] = ':';
    buffer[2] = ' ';

    /* Simple integer to string conversion */
    if (cm >= 100)
    {
      buffer[3] = '0' + (cm / 100);
      buffer[4] = '0' + ((cm / 10) % 10);
      buffer[5] = '0' + (cm % 10);
      buffer[6] = '.';
      buffer[7] = '0' + mm;
      buffer[8] = ' ';
      buffer[9] = 'c';
      buffer[10] = 'm';
      buffer[11] = '\r';
      buffer[12] = '\n';
      buffer[13] = '\0';
    }
    else if (cm >= 10)
    {
      buffer[3] = '0' + (cm / 10);
      buffer[4] = '0' + (cm % 10);
      buffer[5] = '.';
      buffer[6] = '0' + mm;
      buffer[7] = ' ';
      buffer[8] = 'c';
      buffer[9] = 'm';
      buffer[10] = '\r';
      buffer[11] = '\n';
      buffer[12] = '\0';
    }
    else
    {
      buffer[3] = '0' + cm;
      buffer[4] = '.';
      buffer[5] = '0' + mm;
      buffer[6] = ' ';
      buffer[7] = 'c';
      buffer[8] = 'm';
      buffer[9] = '\r';
      buffer[10] = '\n';
      buffer[11] = '\0';
    }

    Uart_Puts(buffer);
  }
}

/**
 * @brief Test mode: continuously read and display ultrasonic sensor values
 *
 * Call this from main() instead of the normal loop to test the sensor.
 * Outputs distance readings over UART every 500ms.
 * @note then use putty -serial COM6 -sercfg 57600,8,n,1,N command to view the output on Windows (adjust COM port as needed)
 */
void TestUltraSonicSensor()
{
  Uart_Puts("=== Ultrasonic Sensor Test Mode ===\r\n");
  Uart_Puts("Reading distance every 500ms...\r\n\r\n");

  while (1)
  {
    float distance = MeasureDistance();
    printDistance(distance);
    _delay_ms(500);
  }
}