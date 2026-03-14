/**
 * @file arduino_hal.cpp
 * @brief Implementation of Arduino Hardware Abstraction Layer
 *
 * Provides low-level GPIO, PWM, and ADC functionality for ATmega328P.
 * Directly manipulates AVR registers for efficient hardware control.
 */

#include "arduino_hal.h"

void PinMode(Pin pin, bool output)
{
  uint8_t bit;

  if (output)
  {
    /* Port D (pins 0-7) */
    if (pin >= PIN_0 && pin <= PIN_7)
    {
      bit = pin - PIN_0;
      DDRD |= (1 << bit);
    }
    /* Port B (pins 8-13) */
    else if (pin >= PIN_8 && pin <= PIN_13)
    {
      bit = pin - PIN_8;
      DDRB |= (1 << bit);
    }
    /* Port C (analog pins A0-A5) */
    else if (pin >= PIN_A0 && pin <= PIN_A5)
    {
      bit = pin - PIN_A0;
      DDRC |= (1 << bit);
    }
  }
  else
  {
    /* Set as input */
    if (pin >= PIN_0 && pin <= PIN_7)
    {
      bit = pin - PIN_0;
      DDRD &= ~(1 << bit);
    }
    else if (pin >= PIN_8 && pin <= PIN_13)
    {
      bit = pin - PIN_8;
      DDRB &= ~(1 << bit);
    }
    else if (pin >= PIN_A0 && pin <= PIN_A5)
    {
      bit = pin - PIN_A0;
      DDRC &= ~(1 << bit);
    }
  }
}

void DigitalWrite(Pin pin, bool high)
{
  uint8_t bit;

  /* Port D (pins 0-7) */
  if (pin >= PIN_0 && pin <= PIN_7)
  {
    bit = pin - PIN_0;
    if (high)
      PORTD |= (1 << bit);
    else
      PORTD &= ~(1 << bit);
  }
  /* Port B (pins 8-13) */
  else if (pin >= PIN_8 && pin <= PIN_13)
  {
    bit = pin - PIN_8;
    if (high)
      PORTB |= (1 << bit);
    else
      PORTB &= ~(1 << bit);
  }
  /* Port C (analog pins A0-A5) */
  else if (pin >= PIN_A0 && pin <= PIN_A5)
  {
    bit = pin - PIN_A0;
    if (high)
      PORTC |= (1 << bit);
    else
      PORTC &= ~(1 << bit);
  }
}

bool DigitalRead(Pin pin)
{
  uint8_t bit;

  /* Port D (pins 0-7) */
  if (pin >= PIN_0 && pin <= PIN_7)
  {
    bit = pin - PIN_0;
    return (PIND & (1 << bit)) != 0;
  }
  /* Port B (pins 8-13) */
  else if (pin >= PIN_8 && pin <= PIN_13)
  {
    bit = pin - PIN_8;
    return (PINB & (1 << bit)) != 0;
  }
  /* Port C (analog pins A0-A5) */
  else if (pin >= PIN_A0 && pin <= PIN_A5)
  {
    bit = pin - PIN_A0;
    return (PINC & (1 << bit)) != 0;
  }
  return false;
}

void AnalogWrite(Pin pin, uint8_t value)
{
  /* Timer0 (pins 5 and 6) */
  if (pin == PIN_6)
  {
    OCR0A = value; /* Timer0 channel A (pin 6) */
  }
  else if (pin == PIN_5)
  {
    OCR0B = value; /* Timer0 channel B (pin 5) */
  }
  /* Timer1 (pins 9 and 10) */
  else if (pin == PIN_9)
  {
    OCR1A = value; /* Timer1 channel A (pin 9) */
  }
  else if (pin == PIN_10)
  {
    OCR1B = value; /* Timer1 channel B (pin 10) */
  }
  /* Timer2 (pins 3 and 11) */
  else if (pin == PIN_3)
  {
    OCR2B = value; /* Timer2 channel B (pin 3) */
  }
  else if (pin == PIN_11)
  {
    OCR2A = value; /* Timer2 channel A (pin 11) */
  }
}

uint16_t AnalogRead(AnalogChannel channel)
{
  /* Select ADC channel (0-5) */
  ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

  /* Start conversion */
  ADCSRA |= (1 << ADSC);

  /* Wait for conversion to complete */
  while (ADCSRA & (1 << ADSC))
    ;

  /* Return ADC value (10-bit: 0-1023) */
  return ADC;
}

void InitPWM()
{
  /* Configure Timer0 for Fast PWM mode (pins 5, 6)
   * COM0A1, COM0B1: Clear OC0A/OC0B on compare match, set at BOTTOM
   * WGM01, WGM00: Fast PWM mode */
  TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);
  /* CS01: Prescaler = 8 (PWM frequency ~7.8kHz) */
  TCCR0B = (1 << CS01);

  /* Configure Timer1 for Fast PWM 8-bit mode (pins 9, 10) */
  TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10);
  TCCR1B = (1 << WGM12) | (1 << CS11); /* Prescaler 8 */

  /* Configure Timer2 for Fast PWM mode (pins 3, 11) */
  TCCR2A = (1 << COM2A1) | (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
  TCCR2B = (1 << CS21); /* Prescaler 8 */
}

void InitADC()
{
  /* Set reference voltage to AVCC (5V) */
  ADMUX = (1 << REFS0);

  /* Enable ADC and set prescaler to 128 (125kHz @ 16MHz)
   * ADC needs 50-200kHz for optimal accuracy */
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

/**
 * @brief Initialize UART serial communication
 * @param ubrr Baud rate register value (use UART_BAUD_SELECT macro)
 */
void Uart_Init(uint16_t ubrr)
{
  /* Set baud rate */
  UBRR0H = (uint8_t)(ubrr >> 8);
  UBRR0L = (uint8_t)ubrr;

  /* Enable receiver and transmitter */
  UCSR0B = (1 << RXEN0) | (1 << TXEN0);

  /* Set frame format: 8 data bits, 1 stop bit, no parity */
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

/**
 * @brief Receive a byte from UART (non-blocking)
 * @return Received byte or UART_NO_DATA if no data available
 * @note Check return value with: if (!(received & UART_NO_DATA))
 */
uint16_t Uart_Getc()
{
  /* Check if data is available */
  if (UCSR0A & (1 << RXC0))
  {
    return UDR0;
  }
  return UART_NO_DATA;
}

/**
 * @brief Transmit a byte via UART (blocking)
 * @param data Byte to transmit
 */
void Uart_Putc(uint8_t data)
{
  /* Wait for empty transmit buffer */
  while (!(UCSR0A & (1 << UDRE0)))
    ;

  /* Put data into buffer, sends the data */
  UDR0 = data;
}

/**
 * @brief Transmit a null-terminated string via UART
 * @param str Pointer to string to transmit
 */
void Uart_Puts(const char *str)
{
  while (*str)
  {
    Uart_Putc(*str++);
  }
}
