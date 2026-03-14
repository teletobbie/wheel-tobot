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

#define UART_BAUD_SELECT(baudRate, xtalCpu) ((xtalCpu) / ((baudRate) * 16UL) - 1)
#define UART_NO_DATA 0x0100

enum Pin
{
  /* Digital pins - using unique non-overlapping values */
  PIN_0 = 0,
  PIN_1 = 1,
  PIN_2 = 2,
  PIN_3 = 3,
  PIN_4 = 4,
  PIN_5 = 5,
  PIN_6 = 6,
  PIN_7 = 7,
  PIN_8 = 8,
  PIN_9 = 9,
  PIN_10 = 10,
  PIN_11 = 11,
  PIN_12 = 12,
  PIN_13 = 13,
  PIN_A0 = 14,
  PIN_A1 = 15,
  PIN_A2 = 16,
  PIN_A3 = 17,
  PIN_A4 = 18,
  PIN_A5 = 19
};

enum AnalogChannel
{
  ANALOG_0 = 0,
  ANALOG_1 = 1,
  ANALOG_2 = 2,
  ANALOG_3 = 3,
  ANALOG_4 = 4,
  ANALOG_5 = 5
};

void PinMode(Pin pin, bool output);
void DigitalWrite(Pin pin, bool high);
bool DigitalRead(Pin pin);
void AnalogWrite(Pin pin, uint8_t value);
uint16_t AnalogRead(AnalogChannel channel);
void InitPWM();
void InitADC();

void Uart_Init(uint16_t ubrr);
uint16_t Uart_Getc();
void Uart_Putc(uint8_t data);
void Uart_Puts(const char *str);

#endif // ARDUINO_HAL_H
