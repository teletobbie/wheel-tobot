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
  // Digital pins (Port D)
  PIN_0 = PD0, ///< Arduino digital pin 0 (RX)
  PIN_1 = PD1, ///< Arduino digital pin 1 (TX)
  PIN_2 = PD2, ///< Arduino digital pin 2
  PIN_3 = PD3, ///< Arduino digital pin 3 (PWM)
  PIN_4 = PD4, ///< Arduino digital pin 4
  PIN_5 = PD5, ///< Arduino digital pin 5 (PWM)
  PIN_6 = PD6, ///< Arduino digital pin 6 (PWM)
  PIN_7 = PD7, ///< Arduino digital pin 7

  // Digital pins (Port B)
  PIN_8 = PB0,  ///< Arduino digital pin 8
  PIN_9 = PB1,  ///< Arduino digital pin 9 (PWM)
  PIN_10 = PB2, ///< Arduino digital pin 10 (PWM)
  PIN_11 = PB3, ///< Arduino digital pin 11 (PWM)
  PIN_12 = PB4, ///< Arduino digital pin 12
  PIN_13 = PB5, ///< Arduino digital pin 13 (LED_BUILTIN)

  // Analog pins (Port C) - can also be used as digital
  PIN_A0 = PC0, ///< Arduino analog pin A0 (digital 14)
  PIN_A1 = PC1, ///< Arduino analog pin A1 (digital 15)
  PIN_A2 = PC2, ///< Arduino analog pin A2 (digital 16)
  PIN_A3 = PC3, ///< Arduino analog pin A3 (digital 17)
  PIN_A4 = PC4, ///< Arduino analog pin A4 (digital 18, SDA)
  PIN_A5 = PC5  ///< Arduino analog pin A5 (digital 19, SCL)
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
