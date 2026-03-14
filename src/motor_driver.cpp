/**
 * @file motor_driver.cpp
 * @brief Implementation of TB6612FNG motor driver control
 *
 * Uses Arduino HAL to provide clean motor control interface.
 * Handles direction and PWM speed control for two independent motors.
 */

#include "motor_driver.h"
#include "arduino_hal.h"

/**
 * @brief Motor pin configuration structure
 *
 * Holds the pin assignments for a single motor channel.
 */
struct MotorPins
{
  Pin direction1; ///< First direction control pin (IN1 or IN3)
  Pin direction2; ///< Second direction control pin (IN2 or IN4)
  Pin pwm;        ///< PWM speed control pin (PWMA or PWMB)
};

static MotorPins motorA = {PIN_9, PIN_8, PIN_3};   ///< Motor A pin configuration (AI1=9, AI2=8, PWMA=3)
static MotorPins motorB = {PIN_11, PIN_12, PIN_5}; ///< Motor B pin configuration (BI1=11, BI2=12, PWMB=5)
static Pin standbyPin = PIN_10;                    ///< TB6612FNG standby pin

/**
 * @brief Internal helper to control a single motor
 * @param motor Motor pin configuration
 * @param speed Speed and direction (-255 to 255)
 */
static void setMotor(MotorPins motor, int16_t speed)
{
  if (speed > 0)
  {
    /* Forward direction */
    DigitalWrite(motor.direction1, true);
    DigitalWrite(motor.direction2, false);
    AnalogWrite(motor.pwm, speed);
  }
  else if (speed < 0)
  {
    /* Reverse direction */
    DigitalWrite(motor.direction1, false);
    DigitalWrite(motor.direction2, true);
    AnalogWrite(motor.pwm, -speed);
  }
  else
  {
    /* Stop (brake by setting both direction pins HIGH) */
    DigitalWrite(motor.direction1, true);
    DigitalWrite(motor.direction2, true);
    AnalogWrite(motor.pwm, 0);
  }
}

void SetupMotors()
{
  /* Configure all motor control pins as outputs */
  PinMode(motorA.direction1, true);
  PinMode(motorA.direction2, true);
  PinMode(motorA.pwm, true);
  PinMode(motorB.direction1, true);
  PinMode(motorB.direction2, true);
  PinMode(motorB.pwm, true);
  PinMode(standbyPin, true);

  /* Disable SPI to free pin 12 (MISO) for use as digital output
   * Pin 12 (PB4/MISO) is used for Motor B direction control */
  SPCR = 0;

  /* Initialize PWM hardware */
  InitPWM();

  /* Disable PWM output on pins used for direction control
   * These pins must be pure digital outputs, not PWM */

  /* Disconnect pin 9 (OC1A) from Timer1 - used for Motor A direction */
  TCCR1A &= ~((1 << COM1A1) | (1 << COM1A0));

  /* Disconnect pin 10 (OC1B) from Timer1 - used for STBY */
  TCCR1A &= ~((1 << COM1B1) | (1 << COM1B0));

  /* Disconnect pin 11 (OC2A) from Timer2 - used for Motor B direction */
  TCCR2A &= ~((1 << COM2A1) | (1 << COM2A0));

  /* Enable motor driver (TB6612FNG standby pin must be HIGH) */
  DigitalWrite(standbyPin, true);
}

void MotorA_Drive(int16_t speed)
{
  setMotor(motorA, speed);
}

void MotorB_Drive(int16_t speed)
{
  setMotor(motorB, speed);
}

void StopAllMotors()
{
  MotorA_Drive(0);
  MotorB_Drive(0);
}
