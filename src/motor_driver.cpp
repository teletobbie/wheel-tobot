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
    // Forward direction
    digitalWrite(motor.direction1, true);
    digitalWrite(motor.direction2, false);
    analogWrite(motor.pwm, speed);
  }
  else if (speed < 0)
  {
    // Reverse direction
    digitalWrite(motor.direction1, false);
    digitalWrite(motor.direction2, true);
    analogWrite(motor.pwm, -speed);
  }
  else
  {
    // Stop (brake by setting both direction pins HIGH)
    digitalWrite(motor.direction1, true);
    digitalWrite(motor.direction2, true);
    analogWrite(motor.pwm, 0);
  }
}

void setupMotors()
{
  // Configure all motor control pins as outputs
  pinMode(motorA.direction1, true);
  pinMode(motorA.direction2, true);
  pinMode(motorA.pwm, true);
  pinMode(motorB.direction1, true);
  pinMode(motorB.direction2, true);
  pinMode(motorB.pwm, true);
  pinMode(standbyPin, true);

  // Initialize PWM hardware
  initPWM();

  // Disable PWM output on pin 10 (we use it as digital output for STBY)
  // Clear COM1B1 and COM1B0 bits to disconnect OC1B from pin 10
  TCCR1A &= ~((1 << COM1B1) | (1 << COM1B0));

  // Enable motor driver (TB6612FNG standby pin must be HIGH)
  digitalWrite(standbyPin, true);
}

void motorA_drive(int16_t speed)
{
  setMotor(motorA, speed);
}

void motorB_drive(int16_t speed)
{
  setMotor(motorB, speed);
}

void stopAllMotors()
{
  motorA_drive(0);
  motorB_drive(0);
}
