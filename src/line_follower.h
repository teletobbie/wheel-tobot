/**
 * @file line_follower.h
 * @brief Line Following Controller with State-Aware PID Control
 */

#ifndef LINE_FOLLOWER_H
#define LINE_FOLLOWER_H

#include <stdint.h>

/* Direction command constants (must match Python side) */
typedef enum
{
  DIR_FORWARD = 0,       /* Normal line following */
  DIR_WAITING = 1,       /* Stopped, no line detected yet */
  DIR_REVERSE_LEFT = 2,  /* Just lost line, reversing left */
  DIR_REVERSE_RIGHT = 3, /* Just lost line, reversing right */
  DIR_SEARCH_LEFT = 4,   /* Gentle sweep searching left */
  DIR_SEARCH_RIGHT = 5   /* Gentle sweep searching right */
} Direction;

void ProcessLineCommand(int16_t error, Direction direction);

#endif /* LINE_FOLLOWER_H */
