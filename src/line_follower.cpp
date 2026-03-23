/**
 * @file line_follower.cpp
 * @brief Implementation of Line Following Controller
 *
 * State-aware PID controller that adjusts motor behavior based on
 * both line position error and robot navigation state (forward, searching, etc.)
 */

#include "line_follower.h"
#include "motor_driver.h"
#include "serial_logger.h"

/* Default motor speed settings */
#define MOTOR_SPEED 70
#define MIN_MOTOR_SPEED 50

/**
 * @brief Process line following command with state-aware PID control
 * @param error Line position error in pixels
 *              - Negative: line is left, turn left
 *              - Positive: line is right, turn right
 *              - Zero: line centered, go straight
 * @param direction Command state from Raspberry Pi
 *                  - DIR_FORWARD: Normal line following
 *                  - DIR_WAITING: Stop motors (no line detected)
 *                  - DIR_REVERSE_LEFT/RIGHT: Lost line, turning back
 *                  - DIR_SEARCH_LEFT/RIGHT: Scanning for line
 *
 * Adjusts PID parameters and base speed based on direction state.
 * Uses differential drive for smooth turning.
 */
void ProcessLineCommand(int16_t error, Direction direction)
{
  static int16_t previous_error = 0;
  static int16_t integral = 0;
  int16_t baseSpeed = MOTOR_SPEED; /* Base speed for motors, adjusted by state */
  float Kp = 0.0f;                 /* Proportional control constant - main driver of correction */
  const float Ki = 0.0f;           /* Integral control constant - helps reduce steady-state error */
  const float Kd = 0.0f;           /* Derivative control constant - helps reduce overshoot */

  /* Handle WAITING state - stop completely */
  if (direction == DIR_WAITING)
  {
    StopAllMotors();
    /* Reset PID state when stopped */
    previous_error = 0;
    integral = 0;
  }
  else
  {
    /* Adjust control parameters based on navigation state */
    switch (direction)
    {
    case DIR_FORWARD: /* Normal line following */
      Kp = 0.1f;
      baseSpeed = MOTOR_SPEED;
      break;

    case DIR_REVERSE_LEFT: /* Recovering - more aggressive turns */
    case DIR_REVERSE_RIGHT:
      Kp = 0.15f;
      baseSpeed = (int16_t)(MOTOR_SPEED * 0.7f); /* 70% speed when recovering */
      break;

    case DIR_SEARCH_LEFT: /* Searching - slow and gentle */
    case DIR_SEARCH_RIGHT:
      Kp = 0.05f;
      baseSpeed = MIN_MOTOR_SPEED; /* Minimum speed while searching */
      break;

    default: /* Unknown state - use safe defaults */
      Kp = 0.1f;
      baseSpeed = MOTOR_SPEED;
      break;
    }

    /* Update integral term */
    integral += error;
    /* Clamp integral to prevent windup */
    if (integral > 500)
      integral = 500;
    if (integral < -500)
      integral = -500;

    /* Calculate derivative term */
    int16_t error_derivative = error - previous_error;

    /* Calculate PID adjustment */
    float adjustment = (Kp * error) + (Ki * integral) + (Kd * error_derivative);

    /* Clamp adjustment to prevent excessive speed difference */
    if (adjustment > 80.0f)
      adjustment = 80.0f;
    if (adjustment < -80.0f)
      adjustment = -80.0f;

    /* Apply differential drive: subtract from left, add to right
     * Positive error (line right) → reduce left speed, increase right speed → turn right
     * Negative error (line left) → increase left speed, reduce right speed → turn left
     * NOTE: If robot turns opposite direction, swap MotorA/MotorB calls below */
    int16_t left_speed = baseSpeed - (int16_t)adjustment;
    int16_t right_speed = baseSpeed + (int16_t)adjustment;

    /* Clamp motor speeds to valid range */
    if (left_speed > 255)
      left_speed = 255;
    if (left_speed < MIN_MOTOR_SPEED)
      left_speed = MIN_MOTOR_SPEED;
    if (right_speed > 255)
      right_speed = 255;
    if (right_speed < MIN_MOTOR_SPEED)
      right_speed = MIN_MOTOR_SPEED;

    /* Drive motors forward */
    if (direction == DIR_FORWARD || direction == DIR_SEARCH_LEFT || direction == DIR_SEARCH_RIGHT)
    {
      MotorA_Drive(left_speed);
      MotorB_Drive(right_speed);
    }
    else
    {
      /* Drive motors in reverse */
      MotorB_Drive(-left_speed);
      MotorA_Drive(-right_speed);
    }

    /* Log PID parameters and control values (CSV format) */
    LogPidData(Kp, Ki, Kd, error, error_derivative, adjustment, left_speed, right_speed);

    /* Update previous error for derivative calculation */
    previous_error = error;
  }
}
