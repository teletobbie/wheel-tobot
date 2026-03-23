/**
 * @file serial_logger.cpp
 * @brief Implementation of serial logging utilities for PID control data
 */

#include "serial_logger.h"
#include "arduino_hal.h"

/**
 * @brief Convert integer to string
 * @param value Integer value to convert
 * @param buffer Buffer to store the result (must be at least 7 bytes for int16_t)
 * @note Handles negative values
 */
static void intToStr(int16_t value, char *buffer)
{
  char temp[7];
  int i = 0;
  bool negative = false;

  if (value < 0)
  {
    negative = true;
    value = -value;
  }

  /* Handle zero case */
  if (value == 0)
  {
    buffer[0] = '0';
    buffer[1] = '\0';
    return;
  }

  /* Convert digits in reverse order */
  while (value > 0)
  {
    temp[i++] = '0' + (value % 10);
    value /= 10;
  }

  /* Add negative sign if needed */
  int j = 0;
  if (negative)
  {
    buffer[j++] = '-';
  }

  /* Reverse the string */
  while (i > 0)
  {
    buffer[j++] = temp[--i];
  }
  buffer[j] = '\0';
}

/**
 * @brief Convert float to string with 2 decimal places
 * @param value Float value to convert
 * @param buffer Buffer to store the result (must be at least 10 bytes)
 */
static void floatToStr(float value, char *buffer)
{
  int16_t intPart = (int16_t)value;
  int16_t decPart = (int16_t)((value - (float)intPart) * 100.0f);

  if (decPart < 0)
    decPart = -decPart;

  char intStr[7];
  char decStr[4];

  intToStr(intPart, intStr);

  /* Format decimal part with leading zero if needed */
  if (decPart < 10)
  {
    decStr[0] = '0';
    intToStr(decPart, &decStr[1]);
  }
  else
  {
    intToStr(decPart, decStr);
  }

  /* Combine integer and decimal parts */
  int i = 0, j = 0;
  while (intStr[i])
  {
    buffer[j++] = intStr[i++];
  }
  buffer[j++] = '.';
  i = 0;
  while (decStr[i])
  {
    buffer[j++] = decStr[i++];
  }
  buffer[j] = '\0';
}

/**
 * @brief Log PID control data via UART in CSV format
 * @param Kp Proportional gain constant
 * @param Ki Integral gain constant
 * @param Kd Derivative gain constant
 * @param error Line position error in pixels
 * @param error_derivative Rate of change of error
 * @param adjustment Calculated motor speed adjustment
 * @param left_speed Final left motor speed
 * @param right_speed Final right motor speed
 *
 * Output format: LOG,Kp,Ki,Kd,error,error_derivative,adjustment,left_speed,right_speed\r\n
 */
void LogPidData(float Kp, float Ki, float Kd, int16_t error,
                int16_t error_derivative, float adjustment,
                int16_t left_speed, int16_t right_speed)
{
  char buffer[10];

  Uart_Puts("LOG,");
  floatToStr(Kp, buffer);
  Uart_Puts(buffer);
  Uart_Putc(',');
  floatToStr(Ki, buffer);
  Uart_Puts(buffer);
  Uart_Putc(',');
  floatToStr(Kd, buffer);
  Uart_Puts(buffer);
  Uart_Putc(',');
  intToStr(error, buffer);
  Uart_Puts(buffer);
  Uart_Putc(',');
  intToStr(error_derivative, buffer);
  Uart_Puts(buffer);
  Uart_Putc(',');
  floatToStr(adjustment, buffer);
  Uart_Puts(buffer);
  Uart_Putc(',');
  intToStr(left_speed, buffer);
  Uart_Puts(buffer);
  Uart_Putc(',');
  intToStr(right_speed, buffer);
  Uart_Puts(buffer);
  Uart_Puts("\r\n");
}
