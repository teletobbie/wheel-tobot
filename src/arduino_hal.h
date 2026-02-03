/**
 * @file arduino_hal.h
 * @brief Arduino Hardware Abstraction Layer for ATmega328P
 *
 * Provides Arduino-style API for GPIO, PWM, and ADC operations on ATmega328P.
 * Supports all digital pins (0-13), analog pins (A0-A5), PWM pins (3,5,6,9,10,11),
 * and analog input channels.
 */

#ifndef ARDUINO_HAL_H
#define ARDUINO_HAL_H

#include <avr/io.h>
#include <stdint.h>

/**
 * @brief Arduino Uno pin definitions (ATmega328P)
 *
 * Maps Arduino pin numbers to ATmega328P port/pin combinations.
 */
enum Pin
{
  // Digital pins - using unique non-overlapping values
  PIN_0 = 0,   ///< Arduino digital pin 0 (RX, Port D0)
  PIN_1 = 1,   ///< Arduino digital pin 1 (TX, Port D1)
  PIN_2 = 2,   ///< Arduino digital pin 2 (Port D2)
  PIN_3 = 3,   ///< Arduino digital pin 3 (PWM, Port D3)
  PIN_4 = 4,   ///< Arduino digital pin 4 (Port D4)
  PIN_5 = 5,   ///< Arduino digital pin 5 (PWM, Port D5)
  PIN_6 = 6,   ///< Arduino digital pin 6 (PWM, Port D6)
  PIN_7 = 7,   ///< Arduino digital pin 7 (Port D7)
  PIN_8 = 8,   ///< Arduino digital pin 8 (Port B0)
  PIN_9 = 9,   ///< Arduino digital pin 9 (PWM, Port B1)
  PIN_10 = 10, ///< Arduino digital pin 10 (PWM, Port B2)
  PIN_11 = 11, ///< Arduino digital pin 11 (PWM, Port B3)
  PIN_12 = 12, ///< Arduino digital pin 12 (Port B4)
  PIN_13 = 13, ///< Arduino digital pin 13 (LED_BUILTIN, Port B5)

  // Analog pins - can also be used as digital
  PIN_A0 = 14, ///< Arduino analog pin A0 (digital 14, Port C0)
  PIN_A1 = 15, ///< Arduino analog pin A1 (digital 15, Port C1)
  PIN_A2 = 16, ///< Arduino analog pin A2 (digital 16, Port C2)
  PIN_A3 = 17, ///< Arduino analog pin A3 (digital 17, Port C3)
  PIN_A4 = 18, ///< Arduino analog pin A4 (digital 18, SDA, Port C4)
  PIN_A5 = 19  ///< Arduino analog pin A5 (digital 19, SCL, Port C5)
};

/**
 * @brief Analog input channel mapping for ADC
 */
enum AnalogChannel
{
  ANALOG_0 = 0, ///< Analog channel 0 (pin A0)
  ANALOG_1 = 1, ///< Analog channel 1 (pin A1)
  ANALOG_2 = 2, ///< Analog channel 2 (pin A2)
  ANALOG_3 = 3, ///< Analog channel 3 (pin A3)
  ANALOG_4 = 4, ///< Analog channel 4 (pin A4)
  ANALOG_5 = 5  ///< Analog channel 5 (pin A5)
};

/**
 * @brief Configure a pin as input or output
 * @param pin The pin to configure
 * @param output true for output, false for input
 */
void pinMode(Pin pin, bool output);

/**
 * @brief Write a digital value to a pin
 * @param pin The pin to write to
 * @param high true for HIGH (5V), false for LOW (0V)
 */
void digitalWrite(Pin pin, bool high);

/**
 * @brief Read the digital state of a pin
 * @param pin The pin to read from
 * @return true if pin is HIGH, false if LOW
 */
bool digitalRead(Pin pin);

/**
 * @brief Set PWM duty cycle on a PWM-capable pin
 * @param pin PWM pin (3, 5, 6, 9, 10, or 11)
 * @param value Duty cycle (0-255, where 0=0% and 255=100%)
 * @note Requires initPWM() to be called first
 */
void analogWrite(Pin pin, uint8_t value);

/**
 * @brief Read analog voltage from an ADC channel
 * @param channel Analog channel (ANALOG_0 to ANALOG_5)
 * @return 10-bit ADC value (0-1023, where 0=0V and 1023=5V)
 * @note Requires initADC() to be called first
 */
uint16_t analogRead(AnalogChannel channel);

/**
 * @brief Initialize hardware PWM timers
 *
 * Configures Timer0, Timer1, and Timer2 for Fast PWM mode.
 * Must be called before using analogWrite().
 */
void initPWM();

/**
 * @brief Initialize Analog-to-Digital Converter
 *
 * Configures ADC with AVCC reference and 125kHz clock.
 * Must be called before using analogRead().
 */
void initADC();

#endif // ARDUINO_HAL_H
