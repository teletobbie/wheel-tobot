/**
 * @file blink.cpp
 * @brief LED blink functions to verify Arduino is working
 */

#include "blink.h"
#include "arduino_hal.h"
#include <util/delay.h>

/**
 * @brief Test Arduino by blinking built-in LED
 *
 * Blinks the LED on pin 13 five times to indicate Arduino is working.
 * Useful for debugging hardware issues.
 */
void TestArduino()
{
  /* Configure pin 13 (built-in LED) as output */
  PinMode(PIN_13, true);

  /* Blink 5 times */
  for (int i = 0; i < 5; i++)
  {
    DigitalWrite(PIN_13, true); // LED ON
    _delay_ms(200);
    DigitalWrite(PIN_13, false); // LED OFF
    _delay_ms(200);
  }
}

/**
 * @brief Blink built-in LED rapidly to indicate a warning
 *
 * Useful for signaling errors or critical conditions.
 */
void BlinkWarning()
{
  /* Configure pin 13 (built-in LED) as output */
  PinMode(PIN_13, true);

  /* Blink rapidly to indicate warning */
  for (int i = 0; i < 10; i++)
  {
    DigitalWrite(PIN_13, true); // LED ON
    _delay_ms(100);
    DigitalWrite(PIN_13, false); // LED OFF
    _delay_ms(100);
  }
}
