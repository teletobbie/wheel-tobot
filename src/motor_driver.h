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

void SetupMotors();
void MotorA_Drive(int16_t speed);
void MotorB_Drive(int16_t speed);
void StopAllMotors();

#endif // MOTOR_DRIVER_H
