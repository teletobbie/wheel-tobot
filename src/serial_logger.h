/**
 * @file serial_logger.h
 * @brief Serial logging utilities for PID control data
 */

#ifndef SERIAL_LOGGER_H
#define SERIAL_LOGGER_H

#include <stdint.h>

void LogPidData(float Kp, float Ki, float Kd, int16_t error,
                int16_t error_derivative, float adjustment,
                int16_t left_speed, int16_t right_speed);

#endif /* SERIAL_LOGGER_H */
