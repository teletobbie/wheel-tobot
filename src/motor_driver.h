/**
 * @file motor_driver.h
 * @brief TB6612FNG Dual H-Bridge Motor Driver Interface
 *
 * Provides high-level control for two DC motors using the TB6612FNG driver.
 * Supports bidirectional control with PWM speed regulation.
 */

#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <stdint.h>

/**
 * @brief Initialize the TB6612FNG motor driver
 *
 * Configures GPIO pins and PWM for motor control.
 * Must be called once before using motor functions.
 *
 * Pin configuration:
 * - Motor A: AI1=pin 9, AI2=pin 8, PWMA=pin 3
 * - Motor B: BI1=pin 11, BI2=pin 12, PWMB=pin 5
 * - Standby: pin 10
 */
void setupMotors();

/**
 * @brief Control Motor A speed and direction
 * @param speed Motor speed and direction
 *              - Positive values (1-255): Forward motion
 *              - Negative values (-255 to -1): Reverse motion
 *              - Zero (0): Brake (both direction pins HIGH)
 */
void motorA_drive(int16_t speed);

/**
 * @brief Control Motor B speed and direction
 * @param speed Motor speed and direction
 *              - Positive values (1-255): Forward motion
 *              - Negative values (-255 to -1): Reverse motion
 *              - Zero (0): Brake (both direction pins HIGH)
 */
void motorB_drive(int16_t speed);

/**
 * @brief Stop both motors immediately
 *
 * Applies brake to both motors by setting direction pins HIGH.
 */
void stopAllMotors();

#endif // MOTOR_DRIVER_H
